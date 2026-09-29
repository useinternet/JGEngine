#include "PCH/PCH.h"
#include "GameMaster/State/GameplayZone.h"

HGameplayZone::HGameplayZone(const PName& inName)
	: Name(inName)
{
}

int32 HGameplayZone::Count() const
{
	return (int32)Entities.size();
}

bool HGameplayZone::Contains(const HGameplayEntityId& id) const
{
	return IndexOf(id) != INDEX_NONE;
}

int32 HGameplayZone::IndexOf(const HGameplayEntityId& id) const
{
	int32 count = Count();
	for (int32 i = 0; i < count; ++i)
	{
		if (Entities[i] == id)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void HGameplayZone::PushBack(const HGameplayEntityId& id)
{
	Entities.push_back(id);
}

void HGameplayZone::PushFront(const HGameplayEntityId& id)
{
	Entities.insert(Entities.begin(), id);
}

void HGameplayZone::Insert(int32 index, const HGameplayEntityId& id)
{
	if (index < 0)
	{
		index = 0;
	}
	if (index > Count())
	{
		index = Count();
	}
	Entities.insert(Entities.begin() + index, id);
}

bool HGameplayZone::Remove(const HGameplayEntityId& id)
{
	int32 index = IndexOf(id);
	if (index == INDEX_NONE)
	{
		return false;
	}
	Entities.erase(Entities.begin() + index);
	return true;
}

void HGameplayZone::Clear()
{
	Entities.clear();
}

HGameplayEntityId HGameplayZone::Front() const
{
	if (Entities.empty() == true)
	{
		return HGameplayEntityId::None();
	}
	return Entities.front();
}

HGameplayEntityId HGameplayZone::Back() const
{
	if (Entities.empty() == true)
	{
		return HGameplayEntityId::None();
	}
	return Entities.back();
}

HGameplayEntityId HGameplayZone::PopFront()
{
	if (Entities.empty() == true)
	{
		return HGameplayEntityId::None();
	}
	HGameplayEntityId id = Entities.front();
	Entities.erase(Entities.begin());
	return id;
}

HGameplayEntityId HGameplayZone::PopBack()
{
	if (Entities.empty() == true)
	{
		return HGameplayEntityId::None();
	}
	HGameplayEntityId id = Entities.back();
	Entities.pop_back();
	return id;
}

void HGameplayZone::Shuffle(HGameplayRandomStream& rng)
{
	int32 count = Count();
	for (int32 i = count - 1; i > 0; --i)
	{
		int32 j = (int32)rng.Below((uint32)i + 1);
		HGameplayEntityId temp = Entities[i];
		Entities[i] = Entities[j];
		Entities[j] = temp;
	}
}

void HGameplayZone::CollectTo(HList<HGameplayEntityId>& outIds) const
{
	for (const HGameplayEntityId& id : Entities)
	{
		outIds.push_back(id);
	}
}

void HGameplayZone::WriteJson(PJsonData& json) const
{
	json.AddMember("Name", Name);
	json.AddMember("Entities", Entities);
}

void HGameplayZone::ReadJson(const PJsonData& json)
{
	Entities.clear();
	json.GetData("Name", &Name);
	json.GetData("Entities", &Entities);
}

HGameplayZone* HGameplayZoneSet::Find(const PName& name)
{
	for (HGameplayZone& zone : Zones)
	{
		if (zone.Name == name)
		{
			return &zone;
		}
	}
	return nullptr;
}

const HGameplayZone* HGameplayZoneSet::Find(const PName& name) const
{
	for (const HGameplayZone& zone : Zones)
	{
		if (zone.Name == name)
		{
			return &zone;
		}
	}
	return nullptr;
}

HGameplayZone& HGameplayZoneSet::FindOrAdd(const PName& name)
{
	HGameplayZone* found = Find(name);
	if (found != nullptr)
	{
		return *found;
	}
	Zones.push_back(HGameplayZone(name));
	return Zones.back();
}

bool HGameplayZoneSet::Has(const PName& name) const
{
	return Find(name) != nullptr;
}

bool HGameplayZoneSet::FindZoneOf(const HGameplayEntityId& id, PName* outZoneName) const
{
	for (const HGameplayZone& zone : Zones)
	{
		if (zone.Contains(id) == true)
		{
			if (outZoneName != nullptr)
			{
				*outZoneName = zone.Name;
			}
			return true;
		}
	}
	return false;
}

void HGameplayZoneSet::RemoveEverywhere(const HGameplayEntityId& id)
{
	for (HGameplayZone& zone : Zones)
	{
		zone.Remove(id);
	}
}

bool HGameplayZoneSet::MoveTo(const HGameplayEntityId& id, const PName& toZone)
{
	HGameplayZone* target = Find(toZone);
	if (target == nullptr)
	{
		return false;
	}
	RemoveEverywhere(id);
	target->PushBack(id);
	return true;
}

void HGameplayZoneSet::WriteJson(PJsonData& json) const
{
	json.AddMember("Zones", Zones);
}

void HGameplayZoneSet::ReadJson(const PJsonData& json)
{
	Zones.clear();
	json.GetData("Zones", &Zones);
}
