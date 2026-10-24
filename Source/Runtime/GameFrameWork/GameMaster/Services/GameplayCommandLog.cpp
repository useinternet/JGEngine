#include "PCH/PCH.h"
#include "GameMaster/Services/GameplayCommandLog.h"

void PGameplayCommandLog::Append(const HGameplayCommand& command)
{
	Commands.push_back(command);
}

bool PGameplayCommandLog::PopLast()
{
	if (Commands.empty() == true)
	{
		return false;
	}
	Commands.pop_back();
	return true;
}

int32 PGameplayCommandLog::Count() const
{
	return (int32)Commands.size();
}

void PGameplayCommandLog::Clear()
{
	Commands.clear();
}

void PGameplayCommandLog::WriteJson(PJsonData& json) const
{
	json.AddMember("Commands", Commands);
}

void PGameplayCommandLog::ReadJson(const PJsonData& json)
{
	Commands.clear();
	json.GetData("Commands", &Commands);
}
