#include "PCH/PCH.h"
#include "Components/ActorComponent.h"
#include "Actors/Actor.h"
#include "Core/World.h"

PSharedPtr<JGActor> JGActorComponent::GetOwner() const
{
	return _owner.Pin();
}

PSharedPtr<PWorld> JGActorComponent::GetWorld() const
{
	PSharedPtr<JGActor> owner = _owner.Pin();
	if (owner == nullptr)
	{
		return nullptr;
	}
	return owner->GetWorld();
}

bool JGActorComponent::IsTickEnabled() const
{
	return _bTickEnabled;
}

void JGActorComponent::SetTickEnabled(bool bEnabled)
{
	_bTickEnabled = bEnabled;
}

bool JGActorComponent::HasBegunPlay() const
{
	return _bBegun;
}

void JGActorComponent::attached(PSharedPtr<JGActor> owner)
{
	_owner = owner;
	OnAttached();
}

void JGActorComponent::detached()
{
	if (_bBegun == true)
	{
		endPlay();
	}
	OnDetached();
	_owner.Reset();
}

void JGActorComponent::beginPlay()
{
	if (_bBegun == true)
	{
		return;
	}
	_bBegun = true;
	OnBeginPlay();
}

void JGActorComponent::tick(float32 deltaSeconds)
{
	if (_bTickEnabled == false)
	{
		return;
	}
	OnTick(deltaSeconds);
}

void JGActorComponent::endPlay()
{
	if (_bBegun == false)
	{
		return;
	}
	_bBegun = false;
	OnEndPlay();
}
