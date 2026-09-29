#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayTrigger.h"

PName JGGameplayTrigger::GetKind() const
{
	return PName();
}

int32 JGGameplayTrigger::GetPriority() const
{
	return 0;
}

bool JGGameplayTrigger::Matches(const HGameplayState& state, const HGameplayEvent& event) const
{
	return false;
}

void JGGameplayTrigger::React(HGameplayContext& ctx, const HGameplayEvent& event)
{
}
