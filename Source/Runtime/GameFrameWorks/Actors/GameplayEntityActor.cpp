#include "PCH/PCH.h"
#include "Actors/GameplayEntityActor.h"

HGameplayEntityId JGGameplayEntityActor::GetEntityId() const
{
	PSharedPtr<JGGameplayEntityComponent> component = FindComponent<JGGameplayEntityComponent>();
	if (component == nullptr)
	{
		return HGameplayEntityId::None();
	}
	return component->GetEntityId();
}

void JGGameplayEntityActor::SetEntityId(const HGameplayEntityId& id)
{
	PSharedPtr<JGGameplayEntityComponent> component = FindOrAddComponent<JGGameplayEntityComponent>();
	component->SetEntityId(id);
}

PSharedPtr<JGGameplayEntityComponent> JGGameplayEntityActor::GetEntityComponent() const
{
	return FindComponent<JGGameplayEntityComponent>();
}

void JGGameplayEntityActor::OnSpawned()
{
	FindOrAddComponent<JGGameplayEntityComponent>();
}
