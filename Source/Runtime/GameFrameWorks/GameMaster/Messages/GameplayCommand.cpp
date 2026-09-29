#include "PCH/PCH.h"
#include "GameMaster/Messages/GameplayCommand.h"

HGameplayCommand::HGameplayCommand(const PName& inKind, const HGameplayEntityId& inActor)
	: Kind(inKind)
	, Actor(inActor)
{
}

HGameplayEntityId HGameplayCommand::Target() const
{
	if (Targets.empty() == true)
	{
		return HGameplayEntityId::None();
	}
	return Targets[0];
}

int32 HGameplayCommand::Param(int32 index, int32 defaultValue) const
{
	if (index < 0 || index >= (int32)Params.size())
	{
		return defaultValue;
	}
	return Params[index];
}

void HGameplayCommand::SetExtra(const PName& key, int32 value)
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

bool HGameplayCommand::GetExtra(const PName& key, int32* outValue) const
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

int32 HGameplayCommand::GetExtra(const PName& key, int32 defaultValue) const
{
	int32 value = defaultValue;
	GetExtra(key, &value);
	return value;
}

PString HGameplayCommand::ToString() const
{
	PString result = PString::Format("%s actor=%s", Kind.ToString(), Actor.ToString());
	for (const HGameplayEntityId& target : Targets)
	{
		result.Append(PString::Format(" target=%s", target.ToString()));
	}
	for (int32 param : Params)
	{
		result.Append(PString::Format(" p=%d", param));
	}
	return result;
}

void HGameplayCommand::WriteJson(PJsonData& json) const
{
	json.AddMember("Kind", Kind);
	json.AddMember("Actor", Actor);
	json.AddMember("Targets", Targets);
	json.AddMember("Params", Params);
	json.AddMember("Path", Path);
	json.AddMember("ExtraKeys", ExtraKeys);
	json.AddMember("ExtraValues", ExtraValues);
}

void HGameplayCommand::ReadJson(const PJsonData& json)
{
	Targets.clear();
	Params.clear();
	Path.clear();
	ExtraKeys.clear();
	ExtraValues.clear();

	json.GetData("Kind", &Kind);
	json.GetData("Actor", &Actor);
	json.GetData("Targets", &Targets);
	json.GetData("Params", &Params);
	json.GetData("Path", &Path);
	json.GetData("ExtraKeys", &ExtraKeys);
	json.GetData("ExtraValues", &ExtraValues);
}
