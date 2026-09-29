#pragma once
#include "Actors/Actor.h"
#include "Components/GameplayEntityComponent.h"
#include "GameplayEntityActor.generation.h"

// Gameplay 엔티티에 묶인 액터의 기본형. 엔티티의 몸이고, 엔티티의 실체(ID · 데이터)는 GameMaster 상태에 있다. 스폰 시 JGGameplayEntityComponent 를 장착한다.
// 상태는 읽기만 한다. 규칙은 GameMaster 가, 연출 지시는 GameMasterActor 가 한다.
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayEntityActor : public JGActor
{
	JG_GENERATED_CLASS_BODY

public:
	JGGameplayEntityActor() = default;
	virtual ~JGGameplayEntityActor() = default;

	HGameplayEntityId GetEntityId() const;
	void                SetEntityId(const HGameplayEntityId& id);
	PSharedPtr<JGGameplayEntityComponent> GetEntityComponent() const;

protected:
	virtual void OnSpawned() override;
};
