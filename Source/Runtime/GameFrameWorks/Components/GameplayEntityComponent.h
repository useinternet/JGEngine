#pragma once
#include "Components/ActorComponent.h"
#include "GameMaster/State/GameplayEntityId.h"
#include "GameplayEntityComponent.generation.h"

// 이 액터가 GameMaster 의 어느 엔티티인지. 바인딩의 액터 쪽 절반 (나머지 절반은 GameMasterActor 의 표).
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayEntityComponent : public JGActorComponent
{
	JG_GENERATED_CLASS_BODY

private:
	JGPROPERTY()
	HGameplayEntityId EntityId;

public:
	JGGameplayEntityComponent() = default;
	virtual ~JGGameplayEntityComponent() = default;

	const HGameplayEntityId& GetEntityId() const;
	void SetEntityId(const HGameplayEntityId& id);
	bool IsBound() const;
};
