#pragma once
#include "Core/GameFrameWorksDefines.h"
#include "ActorComponent.generation.h"

class JGActor;
class PWorld;

// 액터에 붙는 기능 단위. 트랜스폼은 없다 (트랜스폼은 액터의 것).
JGCLASS()
class GAMEFRAMEWORKS_API JGActorComponent : public JGObject
{
	JG_GENERATED_CLASS_BODY

	friend class JGActor;

private:
	PWeakPtr<JGActor> _owner;
	bool _bBegun       = false;
	bool _bTickEnabled = true;

public:
	JGActorComponent() = default;
	virtual ~JGActorComponent() = default;

	PSharedPtr<JGActor> GetOwner() const;
	PSharedPtr<PWorld>  GetWorld() const;

	bool IsTickEnabled() const;
	void SetTickEnabled(bool bEnabled);
	bool HasBegunPlay() const;

protected:
	virtual void OnAttached() {}
	virtual void OnDetached() {}
	virtual void OnBeginPlay() {}
	virtual void OnTick(float32 deltaSeconds) {}
	virtual void OnEndPlay() {}

private:
	void attached(PSharedPtr<JGActor> owner);
	void detached();
	void beginPlay();
	void tick(float32 deltaSeconds);
	void endPlay();
};
