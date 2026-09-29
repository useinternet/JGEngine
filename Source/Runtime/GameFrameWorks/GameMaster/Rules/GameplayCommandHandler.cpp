#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayCommandHandler.h"

PName JGGameplayCommandHandler::GetKind() const
{
	return PName();
}

bool JGGameplayCommandHandler::Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const
{
	if (outReason != nullptr)
	{
		*outReason = "not implemented";
	}
	return false;
}

void JGGameplayCommandHandler::Execute(HGameplayContext& ctx, const HGameplayCommand& command)
{
}

void JGGameplayCommandHandler::Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const
{
}
