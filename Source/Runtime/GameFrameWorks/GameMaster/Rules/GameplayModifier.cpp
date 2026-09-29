#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayModifier.h"

PName JGGameplayModifier::GetKind() const
{
	return PName();
}

PName JGGameplayModifier::GetValueKind() const
{
	return PName();
}

PName JGGameplayModifier::GetStage() const
{
	return PName();
}

bool JGGameplayModifier::Applies(const HGameplayContext& ctx, const HGameplayValueQuery& query) const
{
	return false;
}

int32 JGGameplayModifier::Apply(const HGameplayContext& ctx, const HGameplayValueQuery& query, int32 value) const
{
	return value;
}
