#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayEffect.h"

PName JGGameplayEffect::GetKind() const
{
	return PName();
}

void JGGameplayEffect::Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request)
{
}
