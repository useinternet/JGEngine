#include "PCH/PCH.h"
#include "GameMaster/Services/GameplaySerializer.h"

bool PGameplaySerializer::ToJsonText(const HGameplayState& initial, const HGameplayState& current, const PGameplayCommandLog& log, PString* outText)
{
	if (outText == nullptr)
	{
		return false;
	}

	PJson json;
	json.AddMember("SchemaVersion", HGameplayState::SchemaVersion);
	json.AddMember("Initial", initial);
	json.AddMember("Current", current);
	json.AddMember("Log", log);

	return PJson::ToString(json, outText);
}

bool PGameplaySerializer::FromJsonText(const PString& text, HGameplayState& outInitial, HGameplayState& outCurrent, PGameplayCommandLog& outLog, const IGameplayMigrator* migrator)
{
	PJson json;
	if (PJson::ToObject(text, &json) == false)
	{
		JG_LOG(GameMaster, ELogLevel::Error, "PGameplaySerializer: invalid json");
		return false;
	}

	uint32 version = 0;
	json.GetData("SchemaVersion", &version);
	if (version != HGameplayState::SchemaVersion)
	{
		if (migrator == nullptr)
		{
			JG_LOG(GameMaster, ELogLevel::Error, "PGameplaySerializer: schema %u != %u and no migrator", version, HGameplayState::SchemaVersion);
			return false;
		}
		if (migrator->Migrate(version, HGameplayState::SchemaVersion, json) == false)
		{
			JG_LOG(GameMaster, ELogLevel::Error, "PGameplaySerializer: migration %u -> %u failed", version, HGameplayState::SchemaVersion);
			return false;
		}
	}

	if (json.GetData("Initial", &outInitial) == false)
	{
		return false;
	}
	if (json.GetData("Current", &outCurrent) == false)
	{
		return false;
	}
	json.GetData("Log", &outLog);
	return true;
}

bool PGameplaySerializer::SaveToFile(const PString& path, const HGameplayState& initial, const HGameplayState& current, const PGameplayCommandLog& log)
{
	PString text;
	if (ToJsonText(initial, current, log, &text) == false)
	{
		return false;
	}
	return HFileHelper::WriteAllText(path, text);
}

bool PGameplaySerializer::LoadFromFile(const PString& path, HGameplayState& outInitial, HGameplayState& outCurrent, PGameplayCommandLog& outLog, const IGameplayMigrator* migrator)
{
	PString text;
	if (HFileHelper::ReadAllText(path, &text) == false)
	{
		JG_LOG(GameMaster, ELogLevel::Error, "PGameplaySerializer: cannot read %s", path);
		return false;
	}
	return FromJsonText(text, outInitial, outCurrent, outLog, migrator);
}
