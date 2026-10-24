#include "PCH/PCH.h"
#include "Actors/GameEntryActor.h"
#include "Core/GameInstance.h"

JGGameInstance& JGGameEntryActor::GetGameInstance() const
{
	return JGGameInstance::Get();
}
