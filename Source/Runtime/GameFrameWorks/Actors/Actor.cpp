#include "PCH/PCH.h"
#include "Actors/Actor.h"
#include "Core/World.h"

JGActor::JGActor()
	: Guid(HGuid::New())
{
}

const HGuid& JGActor::GetGuid() const
{
	return Guid;
}

PSharedPtr<PWorld> JGActor::GetWorld() const
{
	return _world.Pin();
}

bool JGActor::HasBegunPlay() const
{
	return _bBegun;
}

bool JGActor::IsPendingDestroy() const
{
	return _bPendingDestroy;
}

void JGActor::Destroy()
{
	PSharedPtr<PWorld> world = _world.Pin();
	if (world == nullptr)
	{
		_bPendingDestroy = true;
		return;
	}
	world->DestroyActor(SharedWrap(this));
}

bool JGActor::IsTickEnabled() const
{
	return _bTickEnabled;
}

void JGActor::SetTickEnabled(bool bEnabled)
{
	_bTickEnabled = bEnabled;
}

const HTransform& JGActor::GetLocalTransform() const
{
	return LocalTransform;
}

void JGActor::SetLocalTransform(const HTransform& transform)
{
	LocalTransform = transform;
}

const HVector3& JGActor::GetLocalPosition() const
{
	return LocalTransform.Position;
}

void JGActor::SetLocalPosition(const HVector3& position)
{
	LocalTransform.Position = position;
}

HMatrix JGActor::GetWorldMatrix() const
{
	PSharedPtr<JGActor> parent = _parent.Pin();
	if (parent == nullptr)
	{
		return LocalTransform.ToMatrix();
	}
	return LocalTransform.ToWorldMatrix(parent->GetWorldMatrix());
}

HVector3 JGActor::GetWorldPosition() const
{
	PSharedPtr<JGActor> parent = _parent.Pin();
	if (parent == nullptr)
	{
		return LocalTransform.Position;
	}
	HMatrix world = GetWorldMatrix();
	return HVector3(world.Get_C(3, 0), world.Get_C(3, 1), world.Get_C(3, 2));
}

bool JGActor::AttachTo(PSharedPtr<JGActor> parent)
{
	if (parent == nullptr || parent.GetRawPointer() == this)
	{
		return false;
	}

	// 순환 방지: 새 부모가 내 자손이면 거부
	PSharedPtr<JGActor> cursor = parent;
	while (cursor != nullptr)
	{
		if (cursor.GetRawPointer() == this)
		{
			return false;
		}
		cursor = cursor->GetParent();
	}

	Detach();

	PSharedPtr<JGActor> self = SharedWrap(this);
	parent->Children.push_back(self);
	_parent = parent;
	return true;
}

void JGActor::Detach()
{
	PSharedPtr<JGActor> parent = _parent.Pin();
	if (parent == nullptr)
	{
		return;
	}
	parent->removeChild(SharedWrap(this));
	_parent.Reset();
}

PSharedPtr<JGActor> JGActor::GetParent() const
{
	return _parent.Pin();
}

const HList<PSharedPtr<JGActor>>& JGActor::GetChildren() const
{
	return Children;
}

PSharedPtr<JGActor> JGActor::GetRoot() const
{
	PSharedPtr<JGActor> parent = _parent.Pin();
	if (parent == nullptr)
	{
		return SharedWrap(this);
	}
	return parent->GetRoot();
}

PSharedPtr<JGActorComponent> JGActor::AddComponentByClass(PSharedPtr<JGClass> componentClass)
{
	if (componentClass == nullptr)
	{
		return nullptr;
	}
	PSharedPtr<JGActorComponent> component = RawDynamicCast<JGActorComponent>(AllocateByClass(componentClass));
	if (component == nullptr)
	{
		return nullptr;
	}
	attachComponent(component);
	return component;
}

bool JGActor::RemoveComponent(PSharedPtr<JGActorComponent> component)
{
	if (component == nullptr)
	{
		return false;
	}
	for (auto iter = Components.begin(); iter != Components.end(); ++iter)
	{
		if (*iter == component)
		{
			component->detached();
			Components.erase(iter);
			return true;
		}
	}
	return false;
}

const HList<PSharedPtr<JGActorComponent>>& JGActor::GetComponents() const
{
	return Components;
}

void JGActor::attachComponent(PSharedPtr<JGActorComponent> component)
{
	if (component == nullptr)
	{
		return;
	}
	Components.push_back(component);
	component->attached(SharedWrap(this));
	if (_bBegun == true)
	{
		component->beginPlay();
	}
}

void JGActor::beginPlay()
{
	if (_bBegun == true)
	{
		return;
	}
	_bBegun = true;

	OnBeginPlay();

	HList<PSharedPtr<JGActorComponent>> components = Components;
	for (PSharedPtr<JGActorComponent>& component : components)
	{
		component->beginPlay();
	}

	HList<PSharedPtr<JGActor>> children = Children;
	for (PSharedPtr<JGActor>& child : children)
	{
		child->beginPlay();
	}
}

void JGActor::tick(float32 deltaSeconds)
{
	if (_bPendingDestroy == true)
	{
		return;
	}

	if (_bTickEnabled == true)
	{
		OnTick(deltaSeconds);

		HList<PSharedPtr<JGActorComponent>> components = Components;
		for (PSharedPtr<JGActorComponent>& component : components)
		{
			component->tick(deltaSeconds);
		}
	}

	HList<PSharedPtr<JGActor>> children = Children;
	for (PSharedPtr<JGActor>& child : children)
	{
		child->tick(deltaSeconds);
	}
}

void JGActor::endPlay()
{
	if (_bBegun == false)
	{
		return;
	}
	_bBegun = false;

	HList<PSharedPtr<JGActor>> children = Children;
	for (PSharedPtr<JGActor>& child : children)
	{
		child->endPlay();
	}

	HList<PSharedPtr<JGActorComponent>> components = Components;
	for (PSharedPtr<JGActorComponent>& component : components)
	{
		component->endPlay();
	}

	OnEndPlay();
}

void JGActor::removeChild(PSharedPtr<JGActor> child)
{
	for (auto iter = Children.begin(); iter != Children.end(); ++iter)
	{
		if (*iter == child)
		{
			Children.erase(iter);
			return;
		}
	}
}
