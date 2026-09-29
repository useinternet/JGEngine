#include "PCH/PCH.h"
#include "GameMaster/State/GameplayEntityRegistry.h"

HGameplayEntityId HGameplayEntityRegistry::Create()
{
	uint32 index = 0;
	if (FreeIndices.empty() == false)
	{
		index = FreeIndices.back();
		FreeIndices.pop_back();
	}
	else
	{
		index = (uint32)Generations.size();
		Generations.push_back(0);
		Alive.push_back(0);
	}

	// 세대는 1 부터. 재사용 인덱스는 파괴 시점에 이미 증가되어 있다.
	if (Generations[index] == 0)
	{
		Generations[index] = 1;
	}

	Alive[index] = 1;
	++AliveCount;

	return HGameplayEntityId(index, Generations[index]);
}

bool HGameplayEntityRegistry::Destroy(const HGameplayEntityId& id)
{
	if (IsAlive(id) == false)
	{
		return false;
	}

	Alive[id.Index] = 0;
	++Generations[id.Index];
	FreeIndices.push_back(id.Index);
	--AliveCount;
	return true;
}

bool HGameplayEntityRegistry::IsAlive(const HGameplayEntityId& id) const
{
	if (id.IsValid() == false)
	{
		return false;
	}
	if (id.Index >= (uint32)Alive.size())
	{
		return false;
	}
	if (Alive[id.Index] == 0)
	{
		return false;
	}
	return Generations[id.Index] == id.Generation;
}

uint32 HGameplayEntityRegistry::Capacity() const
{
	return (uint32)Alive.size();
}

uint32 HGameplayEntityRegistry::Count() const
{
	return AliveCount;
}

void HGameplayEntityRegistry::CollectAlive(HList<HGameplayEntityId>& outIds) const
{
	Each([&](const HGameplayEntityId& id)
	{
		outIds.push_back(id);
	});
}

void HGameplayEntityRegistry::WriteJson(PJsonData& json) const
{
	json.AddMember("Generations", Generations);
	json.AddMember("Alive", Alive);
	json.AddMember("FreeIndices", FreeIndices);
	json.AddMember("AliveCount", AliveCount);
}

void HGameplayEntityRegistry::ReadJson(const PJsonData& json)
{
	Generations.clear();
	Alive.clear();
	FreeIndices.clear();
	AliveCount = 0;

	json.GetData("Generations", &Generations);
	json.GetData("Alive", &Alive);
	json.GetData("FreeIndices", &FreeIndices);
	json.GetData("AliveCount", &AliveCount);
}
