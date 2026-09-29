#pragma once
#include "Core/GameFrameWorksDefines.h"
#include "Core/Transform.h"
#include "Components/ActorComponent.h"
#include "Actor.generation.h"

class PWorld;

// 월드에 놓이는 것. 트랜스폼과 부모-자식 트리를 직접 갖고, 기능은 컴포넌트로 붙인다.
// 소유: 월드 → 액터 → 컴포넌트 · 자식 액터 (PSharedPtr). 역참조(부모 · 월드 · 소유자)는 전부 PWeakPtr.
JGCLASS()
class GAMEFRAMEWORKS_API JGActor : public JGObject
{
	JG_GENERATED_CLASS_BODY

	friend class PWorld;
	friend class JGActorComponent;

private:
	JGPROPERTY()
	HGuid Guid;

	JGPROPERTY()
	HTransform LocalTransform;

	JGPROPERTY()
	HList<PSharedPtr<JGActorComponent>> Components;

	JGPROPERTY()
	HList<PSharedPtr<JGActor>> Children;

	PWeakPtr<JGActor> _parent;
	PWeakPtr<PWorld>  _world;
	bool _bBegun          = false;
	bool _bPendingDestroy = false;
	bool _bTickEnabled    = true;

public:
	JGActor();
	virtual ~JGActor() = default;

	// 식별 · 수명
	const HGuid&       GetGuid() const;
	PSharedPtr<PWorld> GetWorld() const;
	bool               HasBegunPlay() const;
	bool               IsPendingDestroy() const;
	void               Destroy();
	bool               IsTickEnabled() const;
	void               SetTickEnabled(bool bEnabled);

	// 트랜스폼
	const HTransform& GetLocalTransform() const;
	void              SetLocalTransform(const HTransform& transform);
	const HVector3&   GetLocalPosition() const;
	void              SetLocalPosition(const HVector3& position);
	HMatrix           GetWorldMatrix() const;
	HVector3          GetWorldPosition() const;

	// 계층
	bool                              AttachTo(PSharedPtr<JGActor> parent);
	void                              Detach();
	PSharedPtr<JGActor>               GetParent() const;
	const HList<PSharedPtr<JGActor>>& GetChildren() const;
	PSharedPtr<JGActor>               GetRoot() const;

	// 컴포넌트
	template<class T>
	PSharedPtr<T> AddComponent()
	{
		PSharedPtr<T> component = Allocate<T>();
		attachComponent(component);
		return component;
	}

	PSharedPtr<JGActorComponent> AddComponentByClass(PSharedPtr<JGClass> componentClass);
	bool                         RemoveComponent(PSharedPtr<JGActorComponent> component);
	const HList<PSharedPtr<JGActorComponent>>& GetComponents() const;

	template<class T>
	PSharedPtr<T> FindComponent() const
	{
		for (const PSharedPtr<JGActorComponent>& component : Components)
		{
			PSharedPtr<T> typed = RawDynamicCast<T>(component);
			if (typed != nullptr)
			{
				return typed;
			}
		}
		return nullptr;
	}

	template<class T>
	PSharedPtr<T> FindOrAddComponent()
	{
		PSharedPtr<T> found = FindComponent<T>();
		if (found != nullptr)
		{
			return found;
		}
		return AddComponent<T>();
	}

protected:
	// 스폰 직후 (월드에 등록되고 BeginPlay 전). 기본 컴포넌트를 붙이는 자리.
	virtual void OnSpawned() {}
	virtual void OnBeginPlay() {}
	virtual void OnTick(float32 deltaSeconds) {}
	virtual void OnEndPlay() {}

private:
	void attachComponent(PSharedPtr<JGActorComponent> component);
	void beginPlay();
	void tick(float32 deltaSeconds);
	void endPlay();
	void removeChild(PSharedPtr<JGActor> child);
};
