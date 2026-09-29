#include "PCH/PCH.h"
#include "GameMaster/Messages/GameplayEffectRequest.h"

HGameplayEffectRequest::HGameplayEffectRequest(const PName& inKind, const HGameplayEntityId& inSubject, const HGameplayEntityId& inTarget)
	: Kind(inKind)
	, Subject(inSubject)
	, Target(inTarget)
{
}

bool HGameplayEffectRequest::HasChoice() const
{
	return bHasChoice;
}

int32 HGameplayEffectRequest::Param(int32 index, int32 defaultValue) const
{
	if (index < 0 || index >= (int32)Params.size())
	{
		return defaultValue;
	}
	return Params[index];
}

PString HGameplayEffectRequest::ToString() const
{
	return PString::Format("%s subject=%s target=%s depth=%d", Kind.ToString(), Subject.ToString(), Target.ToString(), Depth);
}

void HGameplayEffectRequest::WriteJson(PJsonData& json) const
{
	json.AddMember("Kind", Kind);
	json.AddMember("Subject", Subject);
	json.AddMember("Target", Target);
	json.AddMember("Targets", Targets);
	json.AddMember("Params", Params);
	json.AddMember("Coord", Coord);
	json.AddMember("Tag", Tag);
	json.AddMember("CauseSequence", CauseSequence);
	json.AddMember("Depth", Depth);
	json.AddMember("bHasChoice", bHasChoice);
	json.AddMember("Choice", Choice);
}

void HGameplayEffectRequest::ReadJson(const PJsonData& json)
{
	Targets.clear();
	Params.clear();
	Choice.clear();

	json.GetData("Kind", &Kind);
	json.GetData("Subject", &Subject);
	json.GetData("Target", &Target);
	json.GetData("Targets", &Targets);
	json.GetData("Params", &Params);
	json.GetData("Coord", &Coord);
	json.GetData("Tag", &Tag);
	json.GetData("CauseSequence", &CauseSequence);
	json.GetData("Depth", &Depth);
	json.GetData("bHasChoice", &bHasChoice);
	json.GetData("Choice", &Choice);
}
