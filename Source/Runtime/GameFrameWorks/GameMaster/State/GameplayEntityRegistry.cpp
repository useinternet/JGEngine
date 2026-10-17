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

	// 세대는 1 부터. 재사용 인덱스는 파괴 시점에 이미 증가되어 있다. 되감기 뒤에는 그 전에 낸 세대보다 위에서 낸다.
	if (index < (uint32)GenerationFloors.size() && Generations[index] < GenerationFloors[index])
	{
		Generations[index] = GenerationFloors[index];
	}
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

void HGameplayEntityRegistry::KeepIssuedGenerations(const HGameplayEntityRegistry& before)
{
	const uint32 capacity       = (uint32)Generations.size();
	const uint32 beforeCapacity = (uint32)before.Generations.size();
	const uint32 maxCapacity    = capacity > beforeCapacity ? capacity : beforeCapacity;

	if ((uint32)GenerationFloors.size() < maxCapacity)
	{
		GenerationFloors.resize(maxCapacity, 0);
	}

	// 바닥은 내려가지 않는다 (되감기를 여러 번 해도 그 전 모든 시점이 낸 세대 위에서 낸다).
	const uint32 beforeFloorCount = (uint32)before.GenerationFloors.size();
	for (uint32 i = 0; i < beforeFloorCount; ++i)
	{
		if (GenerationFloors[i] < before.GenerationFloors[i])
		{
			GenerationFloors[i] = before.GenerationFloors[i];
		}
	}

	// 되감기 직전에 다음으로 낼 세대: 살아 있으면 지금 세대 + 1, 죽었으면 이미 올라가 있는 지금 세대.
	for (uint32 i = 0; i < beforeCapacity; ++i)
	{
		uint32 next = before.Alive[i] != 0 ? before.Generations[i] + 1 : before.Generations[i];
		if (GenerationFloors[i] < next)
		{
			GenerationFloors[i] = next;
		}
	}

	// 지점 뒤에 생긴 인덱스: 빈 칸으로 두고 재사용 대기에 넣는다 (인덱스 오름차순 — 결정론).
	for (uint32 i = capacity; i < beforeCapacity; ++i)
	{
		Generations.push_back(GenerationFloors[i]);
		Alive.push_back(0);
		FreeIndices.push_back(i);
	}
}

void HGameplayEntityRegistry::WriteJson(PJsonData& json) const
{
	json.AddMember("Generations", Generations);
	json.AddMember("Alive", Alive);
	json.AddMember("FreeIndices", FreeIndices);
	json.AddMember("AliveCount", AliveCount);
	if (GenerationFloors.empty() == false)
	{
		json.AddMember("GenerationFloors", GenerationFloors);
	}
}

void HGameplayEntityRegistry::ReadJson(const PJsonData& json)
{
	Generations.clear();
	Alive.clear();
	FreeIndices.clear();
	AliveCount = 0;
	GenerationFloors.clear();

	json.GetData("Generations", &Generations);
	json.GetData("Alive", &Alive);
	json.GetData("FreeIndices", &FreeIndices);
	json.GetData("AliveCount", &AliveCount);
	json.GetData("GenerationFloors", &GenerationFloors);
}
