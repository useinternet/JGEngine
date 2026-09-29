#pragma once
#include "Core.h"
#include "DirectX12Helper.h"


// 프레임 제출 순서. End()는 우선순위가 낮은 리스트부터 실행한다.
// 같은 우선순위 안의 리스트 순서는 정해져 있지 않으므로, 서로 의존하는 작업은 다른 우선순위에 둔다.
namespace ECommandListPriority
{
	constexpr uint64 Upload   = 0;                  // 스테이징 관리자: 스테이징 -> 리소스 업로드. 드로우보다 먼저.
	constexpr uint64 Default  = 1000;               // 일반 드로우 / 컴퓨트
	constexpr uint64 Readback = JG_UINT64_MAX - 1;  // 스테이징 관리자: 리소스 -> 스테이징 리드백. 드로우 뒤.
	constexpr uint64 Present  = JG_UINT64_MAX;      // 프레임버퍼 갱신(최종 텍스처 -> 백버퍼)
}

class PCommandList;
class PFence;
class PCommandQueue : public IMemoryObject
{
public:
	// 동시에 GPU에 올라가 있을 수 있는 프레임 수 (5-5 프레임 파이프라이닝).
	// 프레임 인덱스마다 커맨드 리스트 묶음(할당자·업로드 페이지·디스크립터 힙 포함)을 따로 두고,
	// Begin()은 같은 인덱스를 마지막으로 쓴 프레임(FramesInFlight 프레임 전)의 완료만 기다린다.
	// 스왑체인 버퍼 수와 ImGui의 프레임 버퍼 수(둘 다 HJGGraphicsArguments::BufferCount)보다 크면 안 된다.
	static constexpr uint32 FramesInFlight = 2;

private:
	enum ECommandListState
	{
		Open,
		Close,
	};

	// 프레임 인덱스 하나의 몫. 이 인덱스의 차례가 다시 오면(Begin의 펜스 대기 뒤) 리스트를 Reset해 다시 쓴다.
	struct HFrameContext
	{
		HMap<uint64, HHashMap<uint64, PSharedPtr<PCommandList>>> ExecuteCmdLists;
		HMap<uint64, HHashMap<uint64, PSharedPtr<PCommandList>>> ExecutePendingCmdLists;
		uint64 FenceValue = 0;   // 이 인덱스의 마지막 End()가 신호한 값
	};

	// 해제를 미룬 리소스. FrameSerial 번 프레임의 제출이 GPU에서 끝나야 놓는다.
	// 번호는 해제 시점의 _submittedFrameCount(= 지금 기록 중이거나 다음에 제출될 프레임)라서, End()와 다음 Begin() 사이(GC 등)에 해제돼도
	// 방금 제출한 프레임이 끝나기 전에는 놓지 않는다. 프레임 인덱스 버킷으로 나누면 그 구간의 해제가 한 프레임 일찍 풀린다.
	struct HDeferredRelease
	{
		uint64 FrameSerial = 0;
		HDX12ComPtr<HDX12Resource> Resource;
	};

	HDX12ComPtr<HDX12CommandQueue> _dx12CommandQueue;
	D3D12_COMMAND_LIST_TYPE    _dx12CommandListType;
	PSharedPtr<PFence> _fence;
	uint64 _fenceValue;
	HMutex _mutex;
	bool _bLock = false;

	HAtomicBool _bCommandListExcute = false;
	HFrameContext _frames[FramesInFlight];
	uint64 _submittedFrameCount = 0;   // End() 횟수 = 지금 기록 중인 프레임의 번호
	uint32 _frameIndex = 0;            // _submittedFrameCount % FramesInFlight
	HMutex _deferredReleaseMutex;      // DeferRelease는 로드 스레드에서도 불린다. _submittedFrameCount 변경도 이 락 안에서 한다.
	HList<HDeferredRelease> _deferredReleases;   // FrameSerial 오름차순
	HHashMap<PCommandList*, ECommandListState> _cmdListStates;
public:
	PCommandQueue(D3D12_COMMAND_LIST_TYPE type);
	~PCommandQueue();
public:
	PSharedPtr<PCommandList> RequestCommandList(ECommandListType commandListType, uint64 priority = ECommandListPriority::Default);

	void Begin();
	void End();
	void SubmitAndFlush();
	void Flush();

	// 프레임 리스트와 무관하게 주어진 리스트 쌍(본 리스트 + 보류 배리어용)만 지금 제출하고 GPU 완료까지 기다린 뒤 다시 열어 둔다.
	// 같은 큐이므로 이 시점까지 큐에 들어가 있던 작업이 모두 끝난 뒤에 돌아온다. (ReadbackTextureImmediate 전용)
	void ExecuteImmediate(PSharedPtr<PCommandList> cmdList, PSharedPtr<PCommandList> pendCmdList);

	// 앞선 프레임이 GPU에서 아직 쓰고 있을 수 있는 리소스의 참조를 이번 프레임의 제출이 끝날 때까지 들고 있는다. (어느 스레드에서 불러도 된다)
	void DeferRelease(HDX12ComPtr<HDX12Resource> resource);
	// 미룬 해제를 전부 지금 놓는다. Flush() 뒤, 더 제출하지 않을 때(종료)만 쓴다.
	void ReleaseAllDeferred();

	// 마지막 End()가 신호한 펜스 값. 그 제출에 포함된 작업의 완료 판정에 쓴다.
	uint64 GetSubmittedFenceValue() const { return _fenceValue; }
	bool IsFenceComplete(uint64 fenceValue) const;

	// 지금 기록 중인 프레임의 인덱스(0 ~ FramesInFlight-1). End()에서 다음 인덱스로 넘어간다.
	// 프레임마다 CPU가 새로 쓰는 GPU 가시 데이터(예: GUI의 SRV 슬롯)는 이 인덱스로 나눠 써야 한다.
	uint32 GetFrameIndex() const { return _frameIndex; }

	HDX12CommandQueue* Get() const {
		return _dx12CommandQueue.Get();
	}

private:
	PSharedPtr<PCommandList> RequestCommandList(ECommandListType commandListType, uint64 commandID, uint64 priority);
	PSharedPtr<PCommandList> CreateCommandList(ECommandListType commandListType);
	void releaseDeferred(uint64 inCompletedFrameSerial);
	ECommandListState GetCommandListState(PSharedPtr<PCommandList> cmdList);
	void SetCommandListState(PSharedPtr<PCommandList> cmdList, ECommandListState state);
};
