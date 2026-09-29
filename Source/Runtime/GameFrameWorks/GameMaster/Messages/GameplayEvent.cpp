#include "PCH/PCH.h"
#include "GameMaster/Messages/GameplayEvent.h"

HGameplayEvent::HGameplayEvent(const PName& inKind)
	: Kind(inKind)
{
}

HGameplayEvent::HGameplayEvent(const PName& inKind, const HGameplayEntityId& inSubject, const HGameplayEntityId& inTarget)
	: Kind(inKind)
	, Subject(inSubject)
	, Target(inTarget)
{
}

void HGameplayEvent::SetExtra(const PName& key, int32 value)
{
	uint64 count = ExtraKeys.size();
	for (uint64 i = 0; i < count; ++i)
	{
		if (ExtraKeys[i] == key)
		{
			ExtraValues[i] = value;
			return;
		}
	}
	ExtraKeys.push_back(key);
	ExtraValues.push_back(value);
}

bool HGameplayEvent::GetExtra(const PName& key, int32* outValue) const
{
	uint64 count = ExtraKeys.size();
	for (uint64 i = 0; i < count; ++i)
	{
		if (ExtraKeys[i] == key)
		{
			if (outValue != nullptr)
			{
				*outValue = ExtraValues[i];
			}
			return true;
		}
	}
	return false;
}

int32 HGameplayEvent::GetExtra(const PName& key, int32 defaultValue) const
{
	int32 value = defaultValue;
	GetExtra(key, &value);
	return value;
}

PString HGameplayEvent::ToString() const
{
	return PString::Format("%s subject=%s target=%s before=%d after=%d amount=%d cause=%u depth=%d",
		Kind.ToString(), Subject.ToString(), Target.ToString(), Before, After, Amount, CauseSequence, Depth);
}

void HGameplayEvent::WriteJson(PJsonData& json) const
{
	json.AddMember("Kind", Kind);
	json.AddMember("Subject", Subject);
	json.AddMember("Target", Target);
	json.AddMember("Before", Before);
	json.AddMember("After", After);
	json.AddMember("Amount", Amount);
	json.AddMember("From", From);
	json.AddMember("To", To);
	json.AddMember("Tag", Tag);
	json.AddMember("Tag2", Tag2);
	json.AddMember("CauseSequence", CauseSequence);
	json.AddMember("Depth", Depth);
	json.AddMember("ExtraKeys", ExtraKeys);
	json.AddMember("ExtraValues", ExtraValues);
}

void HGameplayEvent::ReadJson(const PJsonData& json)
{
	ExtraKeys.clear();
	ExtraValues.clear();

	json.GetData("Kind", &Kind);
	json.GetData("Subject", &Subject);
	json.GetData("Target", &Target);
	json.GetData("Before", &Before);
	json.GetData("After", &After);
	json.GetData("Amount", &Amount);
	json.GetData("From", &From);
	json.GetData("To", &To);
	json.GetData("Tag", &Tag);
	json.GetData("Tag2", &Tag2);
	json.GetData("CauseSequence", &CauseSequence);
	json.GetData("Depth", &Depth);
	json.GetData("ExtraKeys", &ExtraKeys);
	json.GetData("ExtraValues", &ExtraValues);
}
