#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsAPI.h"
#include "DirectX12Helper.h"

class PCommandList;
class PDX12Texture;

// GPU <-> CPU 전송 관리자 (1단계: 그래픽 DIRECT 큐 하나 위에서 동작)
//
// 원칙
//  1. 요청과 실행을 분리한다. Request*는 어느 스레드, 프레임의 어느 시점에 불러도 스테이징만 만들고 대기 목록에 넣는다.
//  2. 실행은 정해진 지점에서만 한다. 업로드는 프레임 제출의 맨 앞(ECommandListPriority::Upload), 리드백은 맨 뒤(Readback).
//     같은 프레임에 요청한 업로드는 그 프레임의 드로우보다 먼저 GPU에서 실행되고, 리드백은 그 프레임의 드로우 결과를 담는다.
//  3. 완료는 펜스 값으로 판정한다. 리드백 콜백은 다음 BeginFrame에 메인 스레드에서 불린다.
//  4. ReadPixelsImmediate만 예외다. 전용 커맨드 리스트 쌍을 PCommandQueue::ExecuteImmediate로 따로 제출하고 GPU 완료를 기다린다.
//     프레임 리스트의 기록 내용과 리소스 상태 추적은 건드리지 않는다. (복사 뒤 전역 상태 맵의 원래 상태로 되돌린다)
//
// 큐를 COPY 전용으로 바꾸는 일은 이 클래스 안에서만 일어난다. 바깥(PDirectX12API, 버퍼/텍스처)은 Request* 호출만 안다.
class PTransferManager : public IMemoryObject
{
	struct HUploadRequest
	{
		HDX12ComPtr<HDX12Resource> Dest;
		HDX12ComPtr<HDX12Resource> Staging;                    // UPLOAD 힙. 요청 시점에 데이터를 복사해 둔다.
		uint64 ByteSize = 0;                                   // 버퍼 업로드 크기
		HList<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Footprints;  // 텍스처 업로드. 비어 있으면 버퍼 업로드.
		HList<uint32> Subresources;
		D3D12_RESOURCE_STATES FinalState = D3D12_RESOURCE_STATE_COMMON;
		PName Name;
	};

	struct HReadbackRequest
	{
		PSharedPtr<PDX12Texture>   Texture;                    // 복사가 끝날 때까지 살려 둔다
		HDX12ComPtr<HDX12Resource> Staging;                    // READBACK 힙
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint = {};
		uint32 NumRows = 0;
		uint64 RowSizeInBytes = 0;
		uint64 TotalBytes = 0;
		HOnReadPixelsComplete OnComplete;
		uint64 FenceValue = 0;                                 // 0이면 아직 제출 전
	};

	mutable HMutex _uploadMutex;                               // Request*Upload는 로드 스레드에서도 불린다
	HList<HUploadRequest> _pendingUploads;

	mutable HMutex _readbackMutex;
	HList<HReadbackRequest> _pendingReadbacks;                 // 기록 전
	HList<HReadbackRequest> _submittedReadbacks;               // 기록·제출됨. 펜스 대기

	// ReadPixelsImmediate 전용 리스트 쌍(본 리스트 + 보류 배리어용). 처음 쓸 때 만든다.
	PSharedPtr<PCommandList> _immediateCmdList;
	PSharedPtr<PCommandList> _immediatePendCmdList;

	// BeginFrame ~ EndFrame 사이인지. ReadPixelsImmediate가 프레임 안에서 불리면 경고를 남기기 위한 표시.
	bool _bInsideFrame = false;

public:
	PTransferManager() = default;
	virtual ~PTransferManager();

	// 대기 중인 요청과 스테이징을 모두 버린다. 큐를 Flush한 뒤(PDirectX12API::Destroy) 불러야 한다. 콜백은 부르지 않는다.
	void Destroy();

	// -- 업로드 요청 (어느 스레드에서든) --
	// 데이터는 이 호출 안에서 스테이징으로 복사되므로 호출이 끝나면 inData를 해제해도 된다.
	// 같은 대상에 이미 대기 중인 요청이 있으면 그 스테이징을 덮어쓴다. (한 프레임에 여러 번 SetData)
	bool RequestUploadBuffer(HDX12ComPtr<HDX12Resource> inDest, const void* inData, uint64 inByteSize, D3D12_RESOURCE_STATES inFinalState, const PName& inName);
	// inPixels는 슬라이스마다 Width * PixelSize 피치로 빈틈없이 채워진 밉 0 데이터. 밉 0만 올린다.
	bool RequestUploadTexture(HDX12ComPtr<HDX12Resource> inDest, const uint8* inPixels, uint32 inWidth, uint32 inHeight, uint32 inPixelSize, D3D12_RESOURCE_STATES inFinalState, const PName& inName);
	bool HasPendingUpload(HDX12Resource* inResource) const;

	// -- 리드백 --
	// 비동기. 이번 프레임 제출의 마지막에 복사가 기록되고, 콜백은 다음 BeginFrame에 메인 스레드에서 불린다.
	// 콜백은 실패해도 한 번은 불린다. (실패 시 HTexturePixels::IsValid() == false)
	bool RequestReadPixels(PSharedPtr<PDX12Texture> inTexture, const HOnReadPixelsComplete& inOnComplete);
	// 동기(도구 전용). 메인 스레드에서만. 결과는 GPU가 마지막으로 완료한 내용이며, 같은 텍스처의 대기 업로드는 먼저 반영된다.
	bool ReadPixelsImmediate(PSharedPtr<PDX12Texture> inTexture, HTexturePixels& outPixels);

	// -- 프레임 훅 (PDirectX12API가 부른다) --
	void BeginFrame();                          // 큐 Begin 직후. 펜스가 지난 리드백을 풀어 콜백을 부른다
	void RecordUploads();                       // EndFrame: 대기 업로드를 Upload 우선순위 리스트에 기록
	void RecordReadbacks();                     // EndFrame: 대기 리드백을 Readback 우선순위 리스트에 기록
	void OnFrameSubmitted(uint64 inFenceValue); // EndFrame: 큐 End 직후. 이번에 제출된 리드백에 펜스 값을 찍는다

private:
	void completeReadbacks();
	bool createReadbackRequest(PSharedPtr<PDX12Texture> inTexture, HReadbackRequest& outRequest) const;
	bool recordUpload(PCommandList* inCmdList, HUploadRequest& inRequest);
	void recordReadback(PCommandList* inCmdList, HReadbackRequest& inRequest, const D3D12_RESOURCE_STATES* inRestoreState);
	bool resolveReadback(HReadbackRequest& inRequest, HTexturePixels& outPixels);
	void releaseStaging(HDX12ComPtr<HDX12Resource>& inStaging);
	bool ensureImmediateCommandLists();
	static bool isReadbackSupported(const PSharedPtr<PDX12Texture>& inTexture, const char* inCaller);
};
