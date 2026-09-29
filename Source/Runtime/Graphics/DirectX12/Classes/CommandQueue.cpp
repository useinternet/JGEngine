#include "PCH/PCH.h"
#include "CommandQueue.h"
#include "DirectX12/DirectX12API.h"
#include "DirectX12/Classes/Fence.h"
#include "DirectX12/Classes/CommandList.h"
#include "DirectX12/Classes/ResourceStateTracker.h"

PCommandQueue::PCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	_dx12CommandListType = type;
	_fence = Allocate<PFence>();
	_dx12CommandQueue = HDirectX12Helper::CreateD3DCommandQueue(HDirectXAPI::GetDevice(), _dx12CommandListType);
	_fenceValue = 0;
}

PCommandQueue::~PCommandQueue() = default;

PSharedPtr<PCommandList> PCommandQueue::RequestCommandList(ECommandListType commandListType, uint64 priority)
{
	HLockGuard<HMutex> lock(_mutex);

	_bLock = true;

	std::thread::id curr_thread_id = std::this_thread::get_id();
	uint64 hash = std::hash<std::thread::id>()(curr_thread_id);

	return RequestCommandList(commandListType, hash, priority);
}

void PCommandQueue::Begin()
{
	// 이 프레임 인덱스를 마지막으로 쓴 프레임(FramesInFlight 프레임 전)의 GPU 작업만 기다린다. 바로 앞 프레임은 GPU에서 계속 돈다.
	// (5-5 이전에는 바로 앞 프레임의 완료까지 기다려 CPU와 GPU가 번갈아 쉬었다)
	// 이 인덱스의 리스트는 이 대기 뒤 첫 RequestCommandList에서 Reset된다.
	_fence->WaitForFenceValue(_frames[_frameIndex].FenceValue);

	// 방금 기다린 프레임(지금 번호 - FramesInFlight)까지는 GPU가 끝냈으므로 그 번호 이하로 미룬 해제를 놓는다.
	if (_submittedFrameCount >= FramesInFlight)
	{
		releaseDeferred(_submittedFrameCount - FramesInFlight);
	}
}

void PCommandQueue::End()
{
	_bCommandListExcute = true;

	HList<ID3D12CommandList*>   d3dCmdLists;
	HFrameContext& frame = _frames[_frameIndex];

	PResourceStateTracker::Lock();

	for (HPair < const uint64, HHashMap<uint64, PSharedPtr<PCommandList>>>& pair : frame.ExecuteCmdLists)
	{
		for (HPair<const uint64, PSharedPtr<PCommandList>>& cmdList : pair.second)
		{
			PSharedPtr<PCommandList> pendCmdList = frame.ExecutePendingCmdLists[pair.first][cmdList.first];

			if (GetCommandListState(cmdList.second) == ECommandListState::Close)
			{
				continue;
			}
			if (GetCommandListState(pendCmdList) == ECommandListState::Close)
			{
				continue;
			}

			bool bHasPendingBarrier = cmdList.second->Close(pendCmdList.GetRawPointer());

			pendCmdList->Close();
			if (bHasPendingBarrier)
			{
				d3dCmdLists.push_back(pendCmdList->Get());
			}
			d3dCmdLists.push_back(cmdList.second->Get());

			SetCommandListState(cmdList.second, ECommandListState::Close);
			SetCommandListState(pendCmdList, ECommandListState::Close);
		}
	}

	PResourceStateTracker::UnLock();
	_dx12CommandQueue->ExecuteCommandLists((uint32_t)d3dCmdLists.size(), d3dCmdLists.data());

	_fence->IncreaseValue();
	_dx12CommandQueue->Signal(_fence->Get(), _fence->GetValue());
	_fenceValue = _fence->GetValue();

	// 이 인덱스의 리스트는 이 펜스 값이 지나야 다시 쓸 수 있다. 다음 프레임은 다른 인덱스의 리스트에 기록한다.
	{
		HLockGuard<HMutex> lock(_deferredReleaseMutex);
		frame.FenceValue = _fenceValue;
		++_submittedFrameCount;
		_frameIndex = (uint32)(_submittedFrameCount % FramesInFlight);
	}

	_bCommandListExcute = false;
}

void PCommandQueue::SubmitAndFlush()
{
	End();
	Flush();
	Begin();
}

void PCommandQueue::Flush()
{
	_fence->IncreaseValue();
	_dx12CommandQueue->Signal(_fence->Get(), _fence->GetValue());
	_fence->WaitForFenceValue(_fence->GetValue());
}

void PCommandQueue::ExecuteImmediate(PSharedPtr<PCommandList> cmdList, PSharedPtr<PCommandList> pendCmdList)
{
	if (cmdList == nullptr || pendCmdList == nullptr)
	{
		return;
	}

	// End()와 같은 절차를 이 두 리스트에만 적용한다. 프레임 리스트(_frames)는 건드리지 않는다.
	HLockGuard<HMutex> lock(_mutex);
	_bCommandListExcute = true;

	HList<ID3D12CommandList*> d3dCmdLists;

	PResourceStateTracker::Lock();
	const bool bHasPendingBarrier = cmdList->Close(pendCmdList.GetRawPointer());
	pendCmdList->Close();
	PResourceStateTracker::UnLock();

	if (bHasPendingBarrier)
	{
		d3dCmdLists.push_back(pendCmdList->Get());
	}
	d3dCmdLists.push_back(cmdList->Get());

	_dx12CommandQueue->ExecuteCommandLists((uint32_t)d3dCmdLists.size(), d3dCmdLists.data());

	// 같은 큐라서 이 신호는 앞서 제출된 모든 작업이 끝난 뒤에 도달한다. 프레임 펜스 값(_fenceValue)은 바꾸지 않는다.
	_fence->IncreaseValue();
	_dx12CommandQueue->Signal(_fence->Get(), _fence->GetValue());
	_fence->WaitForFenceValue(_fence->GetValue());

	// 완료를 기다렸으므로 할당자를 바로 되감아 다음 즉시 호출에 쓸 수 있게 열어 둔다.
	cmdList->Reset();
	pendCmdList->Reset();

	_bCommandListExcute = false;
}

void PCommandQueue::DeferRelease(HDX12ComPtr<HDX12Resource> resource)
{
	if (resource == nullptr)
	{
		return;
	}

	HLockGuard<HMutex> lock(_deferredReleaseMutex);

	HDeferredRelease deferred;
	deferred.FrameSerial = _submittedFrameCount;
	deferred.Resource    = resource;
	_deferredReleases.push_back(deferred);
}

void PCommandQueue::ReleaseAllDeferred()
{
	HLockGuard<HMutex> lock(_deferredReleaseMutex);
	_deferredReleases.clear();
}

void PCommandQueue::releaseDeferred(uint64 inCompletedFrameSerial)
{
	HLockGuard<HMutex> lock(_deferredReleaseMutex);

	// 번호 오름차순이므로 앞에서부터 끝난 프레임의 것만 놓는다.
	uint64 releaseCount = 0;
	while (releaseCount < _deferredReleases.size() && _deferredReleases[releaseCount].FrameSerial <= inCompletedFrameSerial)
	{
		++releaseCount;
	}
	if (releaseCount > 0)
	{
		_deferredReleases.erase(_deferredReleases.begin(), _deferredReleases.begin() + releaseCount);
	}
}

bool PCommandQueue::IsFenceComplete(uint64 fenceValue) const
{
	if (_fence == nullptr || _fence->Get() == nullptr)
	{
		return true;
	}
	return _fence->Get()->GetCompletedValue() >= fenceValue;
}

PSharedPtr<PCommandList> PCommandQueue::RequestCommandList(ECommandListType commandListType, uint64 commandID, uint64 priority)
{
	while (_bCommandListExcute == true) {}

	if (_bLock == false)
	{
		_mutex.lock();
	}

	commandID = 16777619U * (uint64)commandListType ^ commandID;

	// 지금 기록 중인 프레임 인덱스의 리스트. Close 상태면 이 인덱스를 마지막으로 제출한 프레임이 끝난 뒤(Begin에서 기다림)라 Reset해도 된다.
	HFrameContext& frame = _frames[_frameIndex];

	PSharedPtr<PCommandList> result = nullptr;
	if (frame.ExecuteCmdLists[priority].find(commandID) == frame.ExecuteCmdLists[priority].end())
	{
		frame.ExecuteCmdLists[priority][commandID] = CreateCommandList(commandListType);
		frame.ExecutePendingCmdLists[priority][commandID] = CreateCommandList(commandListType);

		result = frame.ExecuteCmdLists[priority][commandID];
	}
	else
	{
		PSharedPtr<PCommandList> pCmdList = frame.ExecuteCmdLists[priority][commandID];
		PSharedPtr<PCommandList> pPendingCmdList = frame.ExecutePendingCmdLists[priority][commandID];

		if (GetCommandListState(pCmdList) == ECommandListState::Close)
		{
			pCmdList->Reset();
			SetCommandListState(pCmdList, ECommandListState::Open);
		}
		if (GetCommandListState(pPendingCmdList) == ECommandListState::Close)
		{
			pPendingCmdList->Reset();
			SetCommandListState(pPendingCmdList, ECommandListState::Open);
		}
		result = pCmdList;
	}

	if (_bLock == false)
	{
		_mutex.unlock();
	}

	_bLock = false;

	return result;
}

PSharedPtr<PCommandList> PCommandQueue::CreateCommandList(ECommandListType commandListType)
{
	PSharedPtr<PCommandList> cmdList = nullptr;

	switch (_dx12CommandListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		switch (commandListType)
		{
		case ECommandListType::Base:
			cmdList = Allocate<PCommandList>(_dx12CommandListType);
			break;
		case ECommandListType::Graphics:
			cmdList = Allocate<PGraphicsCommandList>(_dx12CommandListType);
			break;
		case ECommandListType::Compute:
			cmdList = Allocate<PComputeCommandList>(_dx12CommandListType);
			break;
		}
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		switch (commandListType)
		{
		case ECommandListType::Base:
			cmdList = Allocate<PCommandList>(_dx12CommandListType);
			break;

		case ECommandListType::Compute:
			cmdList = Allocate<PComputeCommandList>(_dx12CommandListType);
			break;
		}
		break;
	default:
		break;
	}

	SetCommandListState(cmdList, ECommandListState::Open);

	return cmdList;
}
PCommandQueue::ECommandListState PCommandQueue::GetCommandListState(PSharedPtr<PCommandList> cmdList)
{
	return _cmdListStates[cmdList.GetRawPointer()];
}

void PCommandQueue::SetCommandListState(PSharedPtr<PCommandList> cmdList, ECommandListState state)
{
	_cmdListStates[cmdList.GetRawPointer()] = state;
}
