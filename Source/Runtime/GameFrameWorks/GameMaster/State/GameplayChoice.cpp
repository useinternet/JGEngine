#include "PCH/PCH.h"
#include "GameMaster/State/GameplayChoice.h"

void HGameplayChoice::Clear()
{
	bPending = false;
	Chooser  = HGameplayEntityId::None();
	Kind     = PName();
	Candidates.clear();
	Min = 1;
	Max = 1;
	Waiting = HGameplayEffectRequest();
	SavedQueue.clear();
}

bool HGameplayChoice::IsCandidate(const HGameplayEntityId& id) const
{
	for (const HGameplayEntityId& candidate : Candidates)
	{
		if (candidate == id)
		{
			return true;
		}
	}
	return false;
}

bool HGameplayChoice::IsValidSelection(const HList<HGameplayEntityId>& selection, PString* outReason) const
{
	int32 count = (int32)selection.size();
	if (count < Min || count > Max)
	{
		if (outReason != nullptr)
		{
			*outReason = PString::Format("selection count %d out of [%d, %d]", count, Min, Max);
		}
		return false;
	}

	for (int32 i = 0; i < count; ++i)
	{
		if (IsCandidate(selection[i]) == false)
		{
			if (outReason != nullptr)
			{
				*outReason = PString::Format("%s is not a candidate", selection[i].ToString());
			}
			return false;
		}
		for (int32 j = i + 1; j < count; ++j)
		{
			if (selection[i] == selection[j])
			{
				if (outReason != nullptr)
				{
					*outReason = PString::Format("%s selected twice", selection[i].ToString());
				}
				return false;
			}
		}
	}
	return true;
}

void HGameplayChoice::WriteJson(PJsonData& json) const
{
	json.AddMember("bPending", bPending);
	json.AddMember("Chooser", Chooser);
	json.AddMember("Kind", Kind);
	json.AddMember("Candidates", Candidates);
	json.AddMember("Min", Min);
	json.AddMember("Max", Max);
	json.AddMember("Waiting", Waiting);
	json.AddMember("SavedQueue", SavedQueue);
}

void HGameplayChoice::ReadJson(const PJsonData& json)
{
	Clear();

	json.GetData("bPending", &bPending);
	json.GetData("Chooser", &Chooser);
	json.GetData("Kind", &Kind);
	json.GetData("Candidates", &Candidates);
	json.GetData("Min", &Min);
	json.GetData("Max", &Max);
	json.GetData("Waiting", &Waiting);
	json.GetData("SavedQueue", &SavedQueue);
}
