#pragma once
#include "GameMaster/State/GameplayEntityId.h"

// 턴 상태. 흐름(IGameplayFlow)이 진행 상태를 여기에 둔다 — 상태 안이라 되돌리기 · 저장 · 리플레이 · 체크섬에 그대로 들어간다.
//   공용 칸 (모든 흐름이 채우고 엔진 기능이 읽는다): Phase(NotStarted · Finished · 그 사이) · Step · Actors · NextStep · Round · TurnCount
//   기본 흐름(PGameplayRoundTurnFlow) 전용 칸: Phase 의 세부값(RoundStart ~ RoundEnd) · Order · OrderIndex · CurrentActor · PendingStep
//     (게임 흐름도 Order · OrderIndex 를 자기 순서 목록으로 써도 된다)
struct GAMEFRAMEWORKS_API HGameplayTurnState : public IJsonable
{
	int32                      Round      = 0;
	EGameplayPhase           Phase      = EGameplayPhase::NotStarted;
	HList<HGameplayEntityId> Order;
	int32                      OrderIndex = INDEX_NONE;
	HGameplayEntityId        CurrentActor;
	int32                      TurnCount  = 0;   // 시작 이후 진행한 턴 수
	EGameplayPhaseStep       PendingStep = EGameplayPhaseStep::None;   // 기본 흐름이 효과 큐가 비면 진행할 전이 단계

	HList<HGameplayEntityId> Actors;     // 지금 명령을 낼 수 있는 행동자. 비면 아무도 못 낸다 (선택 대기는 Choice.Chooser 가 따로 받는다)
	PName                      Step;       // 지금 단계 이름 (표시 · 로그 · 게임 규칙). 기본 흐름은 페이즈 이름
	PName                      NextStep;   // 게임 흐름이 다음 RunPendingStep 에서 할 일 (예약 칸. 기본 흐름은 PendingStep 을 쓴다)

	bool IsStarted() const;
	bool IsFinished() const;
	bool IsActorTurn(const HGameplayEntityId& id) const;   // 기본 흐름: 지금 차례인 행동자인가
	bool CanAct() const;                                     // 기본 흐름: TurnMain 이고 행동자가 있음
	bool IsActor(const HGameplayEntityId& id) const;       // 공용: Actors 에 있는가

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
