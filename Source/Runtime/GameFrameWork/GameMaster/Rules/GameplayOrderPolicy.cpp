#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayOrderPolicy.h"
#include <algorithm>

namespace
{
	void collectFromZoneOrAll(const HGameplayState& state, const PName& zoneName, HList<HGameplayEntityId>& outIds)
	{
		if (zoneName == NAME_NONE)
		{
			state.Entities.CollectAlive(outIds);
			return;
		}

		const HGameplayZone* zone = state.FindZone(zoneName);
		if (zone == nullptr)
		{
			return;
		}

		for (const HGameplayEntityId& id : zone->Entities)
		{
			if (state.IsAlive(id) == true)
			{
				outIds.push_back(id);
			}
		}
	}
}

PGameplayZoneOrderPolicy::PGameplayZoneOrderPolicy(const PName& zoneName)
	: _zoneName(zoneName)
{
}

void PGameplayZoneOrderPolicy::BuildOrder(const HGameplayState& state, HList<HGameplayEntityId>& outOrder) const
{
	collectFromZoneOrAll(state, _zoneName, outOrder);
}

PGameplayKeyedOrderPolicy::PGameplayKeyedOrderPolicy(const PName& zoneName, const HKeyFunction& keyFunction, bool bAscending)
	: _zoneName(zoneName)
	, _keyFunction(keyFunction)
	, _bAscending(bAscending)
{
}

void PGameplayKeyedOrderPolicy::BuildOrder(const HGameplayState& state, HList<HGameplayEntityId>& outOrder) const
{
	HList<HGameplayEntityId> ids;
	collectFromZoneOrAll(state, _zoneName, ids);

	if (_keyFunction == nullptr)
	{
		outOrder = ids;
		return;
	}

	// 키를 먼저 계산해 두고 안정 정렬한다 (정렬 중 키 함수 재호출 방지).
	HList<HPair<int32, HGameplayEntityId>> keyed;
	for (const HGameplayEntityId& id : ids)
	{
		keyed.push_back(HPair<int32, HGameplayEntityId>(_keyFunction(state, id), id));
	}

	bool bAscending = _bAscending;
	std::stable_sort(keyed.begin(), keyed.end(), [bAscending](const HPair<int32, HGameplayEntityId>& lhs, const HPair<int32, HGameplayEntityId>& rhs)
	{
		if (bAscending == true)
		{
			return lhs.first < rhs.first;
		}
		return lhs.first > rhs.first;
	});

	for (const HPair<int32, HGameplayEntityId>& entry : keyed)
	{
		outOrder.push_back(entry.second);
	}
}
