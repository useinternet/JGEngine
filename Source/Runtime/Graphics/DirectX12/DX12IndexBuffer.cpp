#include "PCH/PCH.h"
#include "DX12IndexBuffer.h"
#include "DirectX12API.h"
#include "Classes/ResourceStagingManager.h"

PDX12IndexBuffer::~PDX12IndexBuffer()
{
	Reset();
}

const PName& PDX12IndexBuffer::GetName() const
{
	return _name;
}

void PDX12IndexBuffer::SetName(const PName& inName)
{
	_name = inName;
	if (IsValid())
	{
		_dx12Resource->SetName(inName.ToString().GetRawWString().c_str());
	}
}

void PDX12IndexBuffer::SetDatas(const uint32* inDatas, uint64 inCount)
{
	const uint64 btSize = sizeof(uint32) * inCount;
	if (btSize == 0)
	{
		// 0바이트 리소스는 만들 수 없다. 빈 버퍼로 둔다.
		Reset();
		return;
	}

	const uint64 originBtSize = sizeof(uint32) * _indexCount;
	const bool   bGPULoad     = (_loadMethod == EBufferLoadMethod::GPULoad);
	if (IsValid() && (originBtSize != btSize || (bGPULoad == false && _cpuData == nullptr)))
	{
		Reset();
	}

	// Reset()이 개수를 0으로 지우므로 그 뒤에 넣는다. (이전에는 크기가 바뀌면 GetIndexCount()가 0을 돌려줬다. 5-12)
	_indexCount = inCount;

	if (IsValid() == false)
	{
		// GPULoad: DEFAULT 힙, COMMON. 스테이징 관리자가 스테이징으로 올리고 INDEX_BUFFER로 전이한다.
		// CPULoad: UPLOAD 힙, GENERIC_READ, 상시 매핑.
		CD3DX12_HEAP_PROPERTIES heapProperties(bGPULoad ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(btSize);

		_dx12Resource = HDirectXAPI::CreateCommittedResource(
			GetName().ToString(),
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			bGPULoad ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr
		);

		if (_dx12Resource == nullptr)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Create IndexBuffer (%d bytes)", GetName(), (int32)btSize);
			_indexCount = 0;
			return;
		}

		{
			std::lock_guard<std::mutex> lock(_mutex);
			_srv.Reset();
			_uav.Reset();
		}

		if (bGPULoad == false)
		{
			_dx12Resource->Map(0, nullptr, (void**)&_cpuData);
		}
	}

	if (inDatas == nullptr)
	{
		return;
	}

	if (bGPULoad)
	{
		requestUpload(inDatas, inCount);
	}
	else if (_cpuData != nullptr)
	{
		memcpy(_cpuData, inDatas, btSize);
	}
}

uint64 PDX12IndexBuffer::GetIndexCount() const
{
	return _indexCount;
}

EBufferLoadMethod PDX12IndexBuffer::GetLoadMethod() const
{
	return _loadMethod;
}

void PDX12IndexBuffer::SetLoadMethod(EBufferLoadMethod inLoadMethod)
{
	if (_loadMethod == inLoadMethod)
	{
		return;
	}

	_loadMethod = inLoadMethod;
	if (IsValid())
	{
		Reset();
	}
}

void PDX12IndexBuffer::Reset()
{
	if (_dx12Resource == nullptr)
	{
		return;
	}

	// 대기 중인 업로드 요청이 있어도 된다. 등록이 풀린 대상은 기록 시점에 버려진다. (PResourceStagingManager::recordUpload)
	HDirectXAPI::DestroyCommittedResource(_dx12Resource);

	_dx12Resource.Reset();
	_dx12Resource = nullptr;
	_cpuData = nullptr;
	_indexCount = 0;
}

bool PDX12IndexBuffer::IsValid() const
{
	return _dx12Resource != nullptr;
}

HDX12Resource* PDX12IndexBuffer::Get() const
{
	return _dx12Resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12IndexBuffer::GetSRV() const
{
	if (IsValid() == false)
	{
		return { 0 };
	}
	if (_srv.IsValid())
	{
		return { _srv.CPU().ptr };
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Buffer.NumElements = (uint32)_indexCount;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	desc.Buffer.StructureByteStride = 0;

	std::lock_guard<std::mutex> lock(_mutex);
	HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
	HDirectXAPI::GetDevice()->CreateShaderResourceView(Get(), &desc, alloc.CPU());

	_srv = std::move(alloc);
	return _srv.CPU();
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12IndexBuffer::GetUAV() const
{
	if (IsValid() == false)
	{
		return { 0 };
	}
	if (_uav.IsValid())
	{
		return { _uav.CPU() };
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	desc.Buffer.NumElements = (uint32)_indexCount;
	desc.Buffer.StructureByteStride = 0;

	std::lock_guard<std::mutex> lock(_mutex);
	HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
	HDirectXAPI::GetDevice()->CreateUnorderedAccessView(Get(), nullptr, &desc, alloc.CPU());

	_uav = std::move(alloc);
	return _uav.CPU();
}

void PDX12IndexBuffer::requestUpload(const uint32* inDatas, uint64 inCount)
{
	if (IsValid() == false || inDatas == nullptr || inCount == 0)
	{
		return;
	}

	PSharedPtr<PResourceStagingManager> stagingManager = HDirectXAPI::GetResourceStagingManager();
	if (stagingManager == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : ResourceStagingManager is not available. index data is not uploaded", GetName());
		return;
	}

	stagingManager->RequestBufferUpload(_dx12Resource, inDatas, inCount * sizeof(uint32), D3D12_RESOURCE_STATE_INDEX_BUFFER, _name);
}
