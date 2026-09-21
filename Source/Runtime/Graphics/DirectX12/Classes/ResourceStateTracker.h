#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "DirectX12Helper.h"


class HResourceState
{
public:
	HMap<uint32, D3D12_RESOURCE_STATES> StateMap;
	D3D12_RESOURCE_STATES State;

public:
	D3D12_RESOURCE_STATES Get(uint32_t subresource) const {
		if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) return State;
		else
		{
			auto iter = StateMap.find(subresource);
			if (iter == StateMap.end())
			{
				return StateMap.at(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
			}
			return iter->second;
		}

	}
	void Set(uint32_t subresource, D3D12_RESOURCE_STATES state)
	{
		if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
		{
			State = state;
			StateMap.clear();
		}
		else
		{
			StateMap[subresource] = state;

		}
	}
};

class HResourceInfo
{
public:
	HResourceState State;
	PName Name;
	uint64 RefCount = 0;
public:
	HResourceInfo() = default;
	HResourceInfo(const PName& name, D3D12_RESOURCE_STATES initState) : Name(name), RefCount(1)
	{
		State.Set(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, initState);
	}
};

class PResourceStateTracker : public IMemoryObject
{
private:
	HList<D3D12_RESOURCE_BARRIER> _resourceBarriers;
	HList<D3D12_RESOURCE_BARRIER> _pendingResourceBarriers;
	HHashMap<HDX12Resource*, HResourceState> _resourceStates;
public:
	virtual ~PResourceStateTracker() = default;

	void TransitionBarrier(HDX12ComPtr<HDX12Resource> d3dResource, D3D12_RESOURCE_STATES state, uint32_t subResource);

	void UAVBarrier(HDX12ComPtr<HDX12Resource> d3dResource);
	void AliasBarrier(HDX12ComPtr<HDX12Resource> beforeD3DResource, HDX12ComPtr<HDX12Resource> afterD3DResource);

	void FlushResourceBarrier(HDX12CommandList* cmdList);
	bool FlushPendingResourceBarrier(HDX12CommandList* cmdList);
	void CommitResourceState();
	void Reset();

public:
	static void Lock();
	static void UnLock();
	static void RegisterResource(const PName& name, HDX12Resource* d3dResource, D3D12_RESOURCE_STATES initState);
	static void SetResourceName(HDX12Resource* d3dResource, const PName& name);
	static PName GetResourceName(HDX12Resource* d3dResource);
	static void UnRegisterResource(HDX12Resource* d3dResource);
	// 전역 상태 맵에 기록된 리소스의 현재 상태(전체 서브리소스 기준). 등록되지 않은 리소스면 false.
	// End() 안(Lock ~ UnLock 사이)에서는 부르지 않는다. 같은 뮤텍스를 다시 잡는다.
	static bool GetResourceState(HDX12Resource* d3dResource, D3D12_RESOURCE_STATES& outState);
	// 등록되어 있는가(= 소유 객체가 아직 살아 있는가). 스테이징 관리자가 기록 직전에 대상이 파괴됐는지 확인하는 데 쓴다.
	static bool IsRegistered(HDX12Resource* d3dResource);
};