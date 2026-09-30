#pragma once
#include "GameMaster/State/GameplayEntityId.h"

// 턴 상태. 라운드 · 페이즈 · 이번 라운드 행동 순서 · 현재 행동자. 전이는 PGameplayPhaseMachine 이 한다.
struct GAMEFRAMEWORKS_API HGameplayTurnState : public IJsonable
{
	int32                      Round      = 0;
	EGameplayPhase           Phase      = EGameplayPhase::NotStarted;
	HList<HGameplayEntityId> Order;
	int32                      OrderIndex = INDEX_NONE;
	HGameplayEntityId        CurrentActor;
	int32                      TurnCount  = 0;   // 시작 이후 진행한 턴 수
	EGameplayPhaseStep       PendingStep = EGameplayPhaseStep::None;   // 효과 큐가 비면 진행할 전이 단계

	bool IsStarted() const;
	bool IsFinished() const;
	bool IsActorTurn(const HGameplayEntityId& id) const;
	bool CanAct() const;   // TurnMain 이고 행동자가 있음

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
