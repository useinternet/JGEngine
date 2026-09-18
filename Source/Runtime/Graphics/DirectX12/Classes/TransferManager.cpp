#include "PCH/PCH.h"
#include "TransferManager.h"
#include "DirectX12/DirectX12API.h"
#include "DirectX12/DX12Texture.h"
#include "DirectX12/Classes/CommandQueue.h"
#include "DirectX12/Classes/CommandList.h"
#include "DirectX12/Classes/ResourceStateTracker.h"
#include "JGGraphicsHelper.h"
#include "Thread/Thread.h"

namespace
{
	bool isMainThread()
	{
		return CurrentThreadID() == GCoreSystem::GetMainThreadID();
	}

	HDX12ComPtr<HDX12Resource> createStagingBuffer(D3D12_HEAP_TYPE inHeapType, uint64 inByteSize, const PName& inOwnerName, const char* inSuffix)
	{
		// UPLOAD 힙은 GENERIC_READ, READBACK 힙은 COPY_DEST로만 만들 수 있다. 둘 다 이후 상태 전이는 하지 않는다.
		const D3D12_RESOURCE_STATES initialState = (inHeapType == D3D12_HEAP_TYPE_UPLOAD) ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;

		CD3DX12_HEAP_PROPERTIES heapProperties(inHeapType);
		CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(inByteSize);

		return HDirectXAPI::CreateCommittedResource(
			PString::Format("%s%s", inOwnerName, PString(inSuffix)),
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			initialState,
			nullptr);
	}
}

PTransferManager::~PTransferManager()
{
	Destroy();
}

void PTransferManager::Destroy()
{
	{
		HLockGuard<HMutex> lock(_uploadMutex);
		if (_pendingUploads.empty() == false)
		{
			JG_LOG(Graphics, ELogLevel::Warning, "TransferManager : %d pending upload(s) dropped at destroy", (int32)_pendingUploads.size());
		}
		for (HUploadRequest& request : _pendingUploads)
		{
			releaseStaging(request.Staging);
		}
		_pendingUploads.clear();
	}
	{
		HLockGuard<HMutex> lock(_readbackMutex);
		const int32 droppedCount = (int32)(_pendingReadbacks.size() + _submittedReadbacks.size());
		if (droppedCount > 0)
		{
			JG_LOG(Graphics, ELogLevel::Warning, "TransferManager : %d readback(s) dropped at destroy (callbacks are not called)", droppedCount);
		}
		for (HReadbackRequest& request : _pendingReadbacks)
		{
			releaseStaging(request.Staging);
		}
		for (HReadbackRequest& request : _submittedReadbacks)
		{
			releaseStaging(request.Staging);
		}
		_pendingReadbacks.clear();
		_submittedReadbacks.clear();
	}

	_immediateCmdList     = nullptr;
	_immediatePendCmdList = nullptr;
}

// ---------------------------------------------------------------- Upload

bool PTransferManager::RequestUploadBuffer(HDX12ComPtr<HDX12Resource> inDest, const void* inData, uint64 inByteSize, D3D12_RESOURCE_STATES inFinalState, const PName& inName)
{
	if (inDest == nullptr || inData == nullptr || inByteSize == 0)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : RequestUploadBuffer invalid arguments (dest %s, data %s, %d bytes)",
			inName, PString(inDest != nullptr ? "ok" : "null"), PString(inData != nullptr ? "ok" : "null"), (int32)inByteSize);
		return false;
	}

	HLockGuard<HMutex> lock(_uploadMutex);

	// 같은 대상의 대기 요청은 하나만 둔다. 크기가 같으면 스테이징을 재사용하고, 다르면 버리고 새로 만든다.
	for (int32 i = (int32)_pendingUploads.size() - 1; i >= 0; --i)
	{
		HUploadRequest& request = _pendingUploads[i];
		if (request.Dest.Get() != inDest.Get())
		{
			continue;
		}

		if (request.Footprints.empty() && request.ByteSize == inByteSize && request.Staging != nullptr)
		{
			void* mapped = nullptr;
			CD3DX12_RANGE readRange(0, 0);
			if (SUCCEEDED(request.Staging->Map(0, &readRange, &mapped)) && mapped != nullptr)
			{
				memcpy(mapped, inData, inByteSize);
				request.Staging->Unmap(0, nullptr);
				request.FinalState = inFinalState;
				return true;
			}
		}

		releaseStaging(request.Staging);
		_pendingUploads.erase(_pendingUploads.begin() + i);
	}

	HUploadRequest request;
	request.Dest       = inDest;
	request.ByteSize   = inByteSize;
	request.FinalState = inFinalState;
	request.Name       = inName;
	request.Staging    = createStagingBuffer(D3D12_HEAP_TYPE_UPLOAD, inByteSize, inName, "_UploadStaging");
	if (request.Staging == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Create Upload Staging (%d bytes)", inName, (int32)inByteSize);
		return false;
	}

	void* mapped = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(request.Staging->Map(0, &readRange, &mapped)) || mapped == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Map Upload Staging", inName);
		releaseStaging(request.Staging);
		return false;
	}
	memcpy(mapped, inData, inByteSize);
	request.Staging->Unmap(0, nullptr);

	_pendingUploads.push_back(request);
	return true;
}

bool PTransferManager::RequestUploadTexture(HDX12ComPtr<HDX12Resource> inDest, const uint8* inPixels, uint32 inWidth, uint32 inHeight, uint32 inPixelSize, D3D12_RESOURCE_STATES inFinalState, const PName& inName)
{
	HDX12Device* device = HDirectXAPI::GetDevice();
	if (device == nullptr || inDest == nullptr || inPixels == nullptr || inWidth == 0 || inHeight == 0 || inPixelSize == 0)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : RequestUploadTexture invalid arguments (%dx%d, pixel %d bytes)", inName, (int32)inWidth, (int32)inHeight, (int32)inPixelSize);
		return false;
	}

	const D3D12_RESOURCE_DESC desc = inDest->GetDesc();
	const uint32 arraySize = desc.DepthOrArraySize;
	const uint32 mipLevels = desc.MipLevels;
	const uint64 srcRowPitch   = (uint64)inWidth * inPixelSize;
	const uint64 srcSlicePitch = srcRowPitch * inHeight;

	HUploadRequest request;
	request.Dest       = inDest;
	request.FinalState = inFinalState;
	request.Name       = inName;

	// 슬라이스마다 밉 0 풋프린트를 구해 스테이징 안에 512바이트 정렬로 이어 놓는다.
	HList<uint32> numRowsList;
	HList<uint64> rowSizeList;
	uint64 stagingSize = 0;
	for (uint32 slice = 0; slice < arraySize; ++slice)
	{
		const uint32 subresource = D3D12CalcSubresource(0, slice, 0, mipLevels, arraySize);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		uint32 numRows = 0;
		uint64 rowSizeInBytes = 0;
		uint64 totalBytes = 0;
		device->GetCopyableFootprints(&desc, subresource, 1, stagingSize, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

		if (rowSizeInBytes != srcRowPitch || numRows != inHeight)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : RequestUploadTexture pixel layout mismatch (row %d != %d bytes, rows %d != %d)",
				inName, (int32)rowSizeInBytes, (int32)srcRowPitch, (int32)numRows, (int32)inHeight);
			return false;
		}

		request.Footprints.push_back(footprint);
		request.Subresources.push_back(subresource);
		numRowsList.push_back(numRows);
		rowSizeList.push_back(rowSizeInBytes);

		stagingSize = HMath::AlignUp(footprint.Offset + (uint64)footprint.Footprint.RowPitch * numRows, (uint64)D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
	}

	request.ByteSize = stagingSize;
	request.Staging  = createStagingBuffer(D3D12_HEAP_TYPE_UPLOAD, stagingSize, inName, "_UploadStaging");
	if (request.Staging == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Create Upload Staging (%d bytes)", inName, (int32)stagingSize);
		return false;
	}

	uint8* mapped = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(request.Staging->Map(0, &readRange, (void**)&mapped)) || mapped == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Map Upload Staging", inName);
		releaseStaging(request.Staging);
		return false;
	}

	for (uint32 slice = 0; slice < arraySize; ++slice)
	{
		const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = request.Footprints[slice];
		const uint8* src = inPixels + slice * srcSlicePitch;
		uint8* dst = mapped + footprint.Offset;
		for (uint32 row = 0; row < numRowsList[slice]; ++row)
		{
			memcpy(dst + (uint64)row * footprint.Footprint.RowPitch, src + (uint64)row * srcRowPitch, rowSizeList[slice]);
		}
	}
	request.Staging->Unmap(0, nullptr);

	HLockGuard<HMutex> lock(_uploadMutex);
	for (int32 i = (int32)_pendingUploads.size() - 1; i >= 0; --i)
	{
		if (_pendingUploads[i].Dest.Get() == inDest.Get())
		{
			releaseStaging(_pendingUploads[i].Staging);
			_pendingUploads.erase(_pendingUploads.begin() + i);
		}
	}
	_pendingUploads.push_back(request);
	return true;
}

bool PTransferManager::HasPendingUpload(HDX12Resource* inResource) const
{
	if (inResource == nullptr)
	{
		return false;
	}

	HLockGuard<HMutex> lock(_uploadMutex);
	for (const HUploadRequest& request : _pendingUploads)
	{
		if (request.Dest.Get() == inResource)
		{
			return true;
		}
	}
	return false;
}

void PTransferManager::RecordUploads()
{
	HList<HUploadRequest> requests;
	{
		HLockGuard<HMutex> lock(_uploadMutex);
		// HAllocator에 operator==가 없어 swap/move 대입이 컴파일되지 않는다. 복사 뒤 비운다.
		requests = _pendingUploads;
		_pendingUploads.clear();
	}
	if (requests.empty())
	{
		return;
	}

	PSharedPtr<PCommandQueue> commandQueue = HDirectXAPI::GetCommandQueue();
	if (commandQueue == nullptr)
	{
		for (HUploadRequest& request : requests)
		{
			releaseStaging(request.Staging);
		}
		return;
	}

	// Upload 우선순위 리스트는 이번 제출에서 드로우 리스트보다 먼저 실행된다.
	PSharedPtr<PCommandList> cmdList = commandQueue->RequestCommandList(ECommandListType::Base, ECommandListPriority::Upload);
	for (HUploadRequest& request : requests)
	{
		recordUpload(cmdList.GetRawPointer(), request);
	}
}

bool PTransferManager::recordUpload(PCommandList* inCmdList, HUploadRequest& inRequest)
{
	if (inCmdList == nullptr || inRequest.Dest == nullptr || inRequest.Staging == nullptr)
	{
		releaseStaging(inRequest.Staging);
		return false;
	}

	// 대상 리소스의 소유자가 기록 전에 파괴됐으면(추적기 등록 해제) 버린다. 요청이 ComPtr을 들고 있어 리소스 자체는 아직 살아 있다.
	if (PResourceStateTracker::IsRegistered(inRequest.Dest.Get()) == false)
	{
		JG_LOG(Graphics, ELogLevel::Trace, "%s : upload target was destroyed before recording. dropped", inRequest.Name);
		releaseStaging(inRequest.Staging);
		return false;
	}

	if (inRequest.Footprints.empty())
	{
		inCmdList->UploadBuffer(inRequest.Dest.Get(), inRequest.Staging.Get(), inRequest.ByteSize, inRequest.FinalState);
	}
	else
	{
		inCmdList->UploadTexture(inRequest.Dest.Get(), inRequest.Staging.Get(), inRequest.Footprints, inRequest.Subresources, inRequest.FinalState);
	}

	// 스테이징의 GPU 수명은 커맨드 리스트가 BackupResource로 잡는다(리스트 Reset까지). 여기서는 추적기 등록만 푼다.
	releaseStaging(inRequest.Staging);
	return true;
}

// ---------------------------------------------------------------- Readback

bool PTransferManager::isReadbackSupported(const PSharedPtr<PDX12Texture>& inTexture, const char* inCaller)
{
	if (inTexture.IsValid() == false || inTexture->IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : texture is invalid", PString(inCaller));
		return false;
	}

	const HTextureInfo& texInfo = inTexture->GetTextureInfo();
	if (HJGGraphicsHelper::IsDepthStencilFormat(texInfo.Format) || HJGGraphicsHelper::GetTextureFormatPixelSize(texInfo.Format) == 0)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s format(%s) is not supported for readback",
			PString(inCaller), inTexture->GetName(), StaticEnum<ETextureFormat>()->GetEnumNameByValue((int32)texInfo.Format));
		return false;
	}

	return true;
}

bool PTransferManager::createReadbackRequest(PSharedPtr<PDX12Texture> inTexture, HReadbackRequest& outRequest) const
{
	HDX12Device* device = HDirectXAPI::GetDevice();
	if (device == nullptr)
	{
		return false;
	}

	// 서브리소스 0(밉 0, 슬라이스 0)만 읽는다.
	const D3D12_RESOURCE_DESC desc = inTexture->Get()->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	uint32 numRows = 0;
	uint64 rowSizeInBytes = 0;
	uint64 totalBytes = 0;
	device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

	outRequest.Staging = createStagingBuffer(D3D12_HEAP_TYPE_READBACK, totalBytes, inTexture->GetName(), "_ReadbackStaging");
	if (outRequest.Staging == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Create Readback Staging (%d bytes)", inTexture->GetName(), (int32)totalBytes);
		return false;
	}

	outRequest.Texture        = inTexture;
	outRequest.Footprint      = footprint;
	outRequest.NumRows        = numRows;
	outRequest.RowSizeInBytes = rowSizeInBytes;
	outRequest.TotalBytes     = totalBytes;
	outRequest.FenceValue     = 0;
	return true;
}

bool PTransferManager::RequestReadPixels(PSharedPtr<PDX12Texture> inTexture, const HOnReadPixelsComplete& inOnComplete)
{
	if (isReadbackSupported(inTexture, "RequestReadPixels") == false)
	{
		return false;
	}
	if (inOnComplete.IsBound() == false)
	{
		JG_LOG(Graphics, ELogLevel::Warning, "%s : RequestReadPixels without a bound callback is ignored", inTexture->GetName());
		return false;
	}

	HReadbackRequest request;
	if (createReadbackRequest(inTexture, request) == false)
	{
		return false;
	}
	request.OnComplete = inOnComplete;

	HLockGuard<HMutex> lock(_readbackMutex);
	_pendingReadbacks.push_back(request);
	return true;
}

bool PTransferManager::ReadPixelsImmediate(PSharedPtr<PDX12Texture> inTexture, HTexturePixels& outPixels)
{
	outPixels = HTexturePixels();

	if (isMainThread() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "ReadPixelsImmediate must be called on the main thread. Use RequestReadPixels instead");
		return false;
	}
	if (isReadbackSupported(inTexture, "ReadPixelsImmediate") == false)
	{
		return false;
	}

	PSharedPtr<PCommandQueue> commandQueue = HDirectXAPI::GetCommandQueue();
	if (commandQueue == nullptr || ensureImmediateCommandLists() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : ReadPixelsImmediate is not available (no command queue)", inTexture->GetName());
		return false;
	}

	if (_bInsideFrame)
	{
		// 동작은 하지만 GPU 완료를 기다리는 스톨이 프레임 시간에 더해진다. 도구 코드가 아닌 곳의 습관적 사용을 눈에 띄게 한다.
		JG_LOG(Graphics, ELogLevel::Warning, "%s : ReadPixelsImmediate called inside a frame. It stalls the GPU; prefer RequestReadPixels for per-frame reads", inTexture->GetName());
	}

	HReadbackRequest request;
	if (createReadbackRequest(inTexture, request) == false)
	{
		return false;
	}

	HDX12Resource* resource = inTexture->Get();

	// 복원할 상태 = 전역 상태 맵의 현재 값. 열려 있는 프레임 리스트는 아직 커밋 전이라 이 값이 GPU의 실제 상태다.
	D3D12_RESOURCE_STATES restoreState = D3D12_RESOURCE_STATE_COMMON;
	bool bHasRestoreState = PResourceStateTracker::GetResourceState(resource, restoreState);

	// 같은 텍스처의 대기 업로드를 먼저 같은 리스트에 기록한다. 리스트 안에서 순서가 보장되므로 방금 올린 내용이 그대로 읽힌다.
	{
		HLockGuard<HMutex> lock(_uploadMutex);
		for (int32 i = 0; i < (int32)_pendingUploads.size(); ++i)
		{
			if (_pendingUploads[i].Dest.Get() != resource)
			{
				continue;
			}
			if (recordUpload(_immediateCmdList.GetRawPointer(), _pendingUploads[i]))
			{
				restoreState     = _pendingUploads[i].FinalState;
				bHasRestoreState = true;
			}
			_pendingUploads.erase(_pendingUploads.begin() + i);
			--i;
		}
	}

	recordReadback(_immediateCmdList.GetRawPointer(), request, bHasRestoreState ? &restoreState : nullptr);

	// 이 두 리스트만 제출하고 완료를 기다린다. 프레임 리스트는 열린 채 그대로다.
	commandQueue->ExecuteImmediate(_immediateCmdList, _immediatePendCmdList);

	return resolveReadback(request, outPixels);
}

void PTransferManager::RecordReadbacks()
{
	HList<HReadbackRequest> requests;
	{
		HLockGuard<HMutex> lock(_readbackMutex);
		requests = _pendingReadbacks;
		_pendingReadbacks.clear();
	}
	if (requests.empty())
	{
		return;
	}

	PSharedPtr<PCommandQueue> commandQueue = HDirectXAPI::GetCommandQueue();
	if (commandQueue == nullptr)
	{
		for (HReadbackRequest& request : requests)
		{
			releaseStaging(request.Staging);
		}
		return;
	}

	// Readback 우선순위 리스트는 이번 제출에서 드로우 리스트 뒤에 실행된다. 상태 복원은 하지 않는다. 다음 사용자가 추적기로 전이한다.
	PSharedPtr<PCommandList> cmdList = commandQueue->RequestCommandList(ECommandListType::Base, ECommandListPriority::Readback);
	for (HReadbackRequest& request : requests)
	{
		recordReadback(cmdList.GetRawPointer(), request, nullptr);
	}

	HLockGuard<HMutex> lock(_readbackMutex);
	for (HReadbackRequest& request : requests)
	{
		_submittedReadbacks.push_back(request);
	}
}

void PTransferManager::recordReadback(PCommandList* inCmdList, HReadbackRequest& inRequest, const D3D12_RESOURCE_STATES* inRestoreState)
{
	if (inCmdList == nullptr || inRequest.Texture.IsValid() == false || inRequest.Staging == nullptr)
	{
		return;
	}

	inCmdList->ReadbackTexture(inRequest.Staging.Get(), inRequest.Footprint, inRequest.Texture->Get(), 0, inRestoreState);
}

void PTransferManager::OnFrameSubmitted(uint64 inFenceValue)
{
	_bInsideFrame = false;

	HLockGuard<HMutex> lock(_readbackMutex);
	for (HReadbackRequest& request : _submittedReadbacks)
	{
		if (request.FenceValue == 0)
		{
			request.FenceValue = inFenceValue;
		}
	}
}

void PTransferManager::BeginFrame()
{
	_bInsideFrame = true;
	completeReadbacks();
}

void PTransferManager::completeReadbacks()
{
	PSharedPtr<PCommandQueue> commandQueue = HDirectXAPI::GetCommandQueue();

	HList<HReadbackRequest> completed;
	{
		HLockGuard<HMutex> lock(_readbackMutex);
		for (int32 i = (int32)_submittedReadbacks.size() - 1; i >= 0; --i)
		{
			HReadbackRequest& request = _submittedReadbacks[i];
			if (request.FenceValue == 0)
			{
				continue;
			}
			if (commandQueue != nullptr && commandQueue->IsFenceComplete(request.FenceValue) == false)
			{
				continue;
			}
			completed.push_back(request);
			_submittedReadbacks.erase(_submittedReadbacks.begin() + i);
		}
	}

	// 콜백은 잠금 밖에서 부른다. 콜백 안에서 새 요청을 넣을 수 있다.
	for (HReadbackRequest& request : completed)
	{
		HTexturePixels pixels;
		if (resolveReadback(request, pixels) == false)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : Fail resolve readback. callback receives empty pixels", request.Texture.IsValid() ? request.Texture->GetName() : NAME_NONE);
			pixels = HTexturePixels();
		}
		request.OnComplete.ExecuteIfBound(pixels);
	}
}

bool PTransferManager::resolveReadback(HReadbackRequest& inRequest, HTexturePixels& outPixels)
{
	if (inRequest.Staging == nullptr || inRequest.Texture.IsValid() == false)
	{
		return false;
	}

	const HTextureInfo& texInfo = inRequest.Texture->GetTextureInfo();
	const uint32 width  = inRequest.Footprint.Footprint.Width;
	const uint32 height = inRequest.NumRows;

	uint32 pixelSize = HJGGraphicsHelper::GetTextureFormatPixelSize(texInfo.Format);
	if (width == 0 || pixelSize == 0 || inRequest.RowSizeInBytes != (uint64)width * pixelSize)
	{
		if (width > 0)
		{
			JG_LOG(Graphics, ELogLevel::Warning, "%s : readback row size(%d) != width(%d) * pixel size(%d). pixel size is derived from the row",
				inRequest.Texture->GetName(), (int32)inRequest.RowSizeInBytes, (int32)width, (int32)pixelSize);
			pixelSize = (uint32)(inRequest.RowSizeInBytes / width);
		}
	}

	uint8* mapped = nullptr;
	CD3DX12_RANGE readRange(0, inRequest.TotalBytes);
	HRESULT hResult = inRequest.Staging->Map(0, &readRange, (void**)&mapped);
	if (FAILED(hResult) || mapped == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Map Readback Staging (0x%08X)", inRequest.Texture->GetName(), (int32)hResult);
		releaseStaging(inRequest.Staging);
		return false;
	}

	// 행 피치(256 정렬)를 떼고 빈틈없이 다시 채운다.
	outPixels.Width     = width;
	outPixels.Height    = height;
	outPixels.PixelSize = pixelSize;
	outPixels.Format    = texInfo.Format;
	outPixels.Data.resize(inRequest.RowSizeInBytes * height);
	for (uint32 row = 0; row < height; ++row)
	{
		memcpy(outPixels.Data.data() + (uint64)row * inRequest.RowSizeInBytes,
			mapped + inRequest.Footprint.Offset + (uint64)row * inRequest.Footprint.Footprint.RowPitch,
			inRequest.RowSizeInBytes);
	}

	CD3DX12_RANGE writtenRange(0, 0);
	inRequest.Staging->Unmap(0, &writtenRange);
	releaseStaging(inRequest.Staging);

	return outPixels.IsValid();
}

// ---------------------------------------------------------------- Helpers

void PTransferManager::releaseStaging(HDX12ComPtr<HDX12Resource>& inStaging)
{
	if (inStaging == nullptr)
	{
		return;
	}

	// 추적기 등록만 푼다. GPU가 아직 쓰고 있을 수 있는 경우(기록된 업로드)는 커맨드 리스트의 BackupResource가 수명을 잡는다.
	HDirectXAPI::DestroyCommittedResource(inStaging);
	inStaging.Reset();
}

bool PTransferManager::ensureImmediateCommandLists()
{
	if (_immediateCmdList.IsValid() && _immediatePendCmdList.IsValid())
	{
		return true;
	}
	if (HDirectXAPI::GetDevice() == nullptr)
	{
		return false;
	}

	// 프레임 리스트와 분리된 전용 할당자/리스트. 생성 직후 열린 상태이고 ExecuteImmediate가 실행 뒤 다시 열어 둔다.
	_immediateCmdList     = Allocate<PCommandList>(D3D12_COMMAND_LIST_TYPE_DIRECT);
	_immediatePendCmdList = Allocate<PCommandList>(D3D12_COMMAND_LIST_TYPE_DIRECT);
	return _immediateCmdList.IsValid() && _immediatePendCmdList.IsValid();
}
