#pragma once
#include "Core.h"
#include "DirectX12Helper.h"


// 프레임 제출 순서. End()는 우선순위가 낮은 리스트부터 실행한다.
// 같은 우선순위 안의 리스트 순서는 정해져 있지 않으므로, 서로 의존하는 작업은 다른 우선순위에 둔다.
namespace ECommandListPriority
{
	constexpr uint64 Upload   = 0;                  // 전송 관리자: 스테이징 -> 리소스 업로드. 드로우보다 먼저.
	constexpr uint64 Default  = 1000;               // 일반 드로우 / 컴퓨트
	constexpr uint64 Readback = JG_UINT64_MAX - 1;  // 전송 관리자: 리소스 -> 스테이징 리드백. 드로우 뒤.
	constexpr uint64 Present  = JG_UINT64_MAX;      // 프레임버퍼 갱신(최종 텍스처 -> 백버퍼)
}

class PCommandList;
class PFence;
class PCommandQueue : public IMemoryObject
{
	enum ECommandListState
	{
		Open,
		Close,
	};
private:
	HDX12ComPtr<HDX12CommandQueue> _dx12CommandQueue;
	D3D12_COMMAND_LIST_TYPE    _dx12CommandListType;
	PSharedPtr<PFence> _fence;
	uint64 _fenceValue;
	HMutex _mutex;
	bool _bLock = false;

	HAtomicBool _bCommandListExcute = false;
	HMap<uint64, HHashMap<uint64, PSharedPtr<PCommandList>>> _excuteCmdLists;
	HMap<uint64, HHashMap<uint64, PSharedPtr<PCommandList>>> _excutePendingCmdLists;
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
	// 같은 큐이므로 이 시점까지 큐에 들어가 있던 작업이 모두 끝난 뒤에 돌아온다. (ReadPixelsImmediate 전용)
	void ExecuteImmediate(PSharedPtr<PCommandList> cmdList, PSharedPtr<PCommandList> pendCmdList);

	// 마지막 End()가 신호한 펜스 값. 그 제출에 포함된 작업의 완료 판정에 쓴다.
	uint64 GetSubmittedFenceValue() const { return _fenceValue; }
	bool IsFenceComplete(uint64 fenceValue) const;

	HDX12CommandQueue* Get() const {
		return _dx12CommandQueue.Get();
	}

private:
	PSharedPtr<PCommandList> RequestCommandList(ECommandListType commandListType, uint64 commandID, uint64 priority);
	PSharedPtr<PCommandList> CreateCommandList(ECommandListType commandListType);
	ECommandListState GetCommandListState(PSharedPtr<PCommandList> cmdList);
	void SetCommandListState(PSharedPtr<PCommandList> cmdList, ECommandListState state);
};
