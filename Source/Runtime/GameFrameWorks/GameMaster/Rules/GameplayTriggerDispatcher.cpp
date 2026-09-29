#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayTriggerDispatcher.h"
#include "GameMaster/Rules/GameplayRuleEngine.h"
#include <algorithm>

void PGameplayTriggerDispatcher::Dispatch(HGameplayState& state, PGameplayRuleEngine& engine, const PGameplayRegistry<JGGameplayTrigger>& triggers, const HGameplayEvent& event)
{
	HList<HPair<int32, PSharedPtr<JGGameplayTrigger>>> matched;

	for (const PSharedPtr<JGGameplayTrigger>& trigger : triggers.All())
	{
		if (trigger->Matches(state, event) == true)
		{
			matched.push_back(HPair<int32, PSharedPtr<JGGameplayTrigger>>(trigger->GetPriority(), trigger));
		}
	}

	if (matched.empty() == true)
	{
		return;
	}

	std::stable_sort(matched.begin(), matched.end(), [](const HPair<int32, PSharedPtr<JGGameplayTrigger>>& lhs, const HPair<int32, PSharedPtr<JGGameplayTrigger>>& rhs)
	{
		return lhs.first < rhs.first;
	});

	for (HPair<int32, PSharedPtr<JGGameplayTrigger>>& entry : matched)
	{
		HGameplayContext ctx(state, engine, event.CauseSequence, event.Depth + 1);
		entry.second->React(ctx, event);
	}
}
