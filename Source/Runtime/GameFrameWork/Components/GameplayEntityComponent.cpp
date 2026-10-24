#include "PCH/PCH.h"
#include "Components/GameplayEntityComponent.h"

const HGameplayEntityId& JGGameplayEntityComponent::GetEntityId() const
{
	return EntityId;
}

void JGGameplayEntityComponent::SetEntityId(const HGameplayEntityId& id)
{
	EntityId = id;
}

bool JGGameplayEntityComponent::IsBound() const
{
	return EntityId.IsValid();
}
