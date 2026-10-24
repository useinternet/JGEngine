#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayTriggerDispatcher.h"
#include "GameMaster/Rules/GameplayRuleEngine.h"
#include <algorithm>

void PGameplayTriggerDispatcher::Match(const HGameplayState& state, const PGameplayRegistry<JGGameplayTrigger>& triggers, const HGameplayEvent& event)
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
		HPendingReaction reaction;
		reaction.Trigger = entry.second;
		reaction.Event   = event;
		_pending.push_back(reaction);
	}
}

void PGameplayTriggerDispatcher::React(const HGameplayState& state, PGameplayRuleEngine& engine)
{
	if (_pending.empty() == true)
	{
		return;
	}

	HList<HPendingReaction> pending;
	pending.swap(_pending);

	for (HPendingReaction& reaction : pending)
	{
		HGameplayTriggerContext ctx(state, engine, reaction.Event.CauseSequence, reaction.Event.Depth + 1);
		reaction.Trigger->React(ctx, reaction.Event);
	}
}

void PGameplayTriggerDispatcher::Clear()
{
	_pending.clear();
}
