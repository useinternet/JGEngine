#pragma once
#include "GameMaster/Rules/GameplayContext.h"
#include "GameMaster/Rules/GameplayOrderPolicy.h"

// 페이즈 기계. 전이 골격은 엔진, 행동 순서는 IGameplayOrderPolicy 가 정한다.
//   RoundStart → OrderResolve → TurnStart → TurnMain → TurnEnd → (다음 행동자) … → RoundEnd → RoundStart …
// 모든 전이는 이벤트로 발행되어 트리거와 프레젠테이션이 반응한다.
// 전이는 단계(EGameplayPhaseStep)로 나뉜다. 한 단계가 이벤트를 내고 다음 단계를 예약하면, 룰 엔진이 그 이벤트로 발동한
// 효과를 다 해결한 뒤에 다음 단계를 부른다. 그래서 턴 종료 효과는 다음 행동자가 정해지기 전에, 라운드 시작 효과는 순서가 정해지기 전에 해결된다.
class GAMEFRAMEWORKS_API PGameplayPhaseMachine
{
public:
	// NotStarted 에서 GameStarted 를 내고 첫 라운드를 예약한다.
	void Start(HGameplayContext& ctx);

	// TurnMain 에서 턴을 끝낸다. 턴 시작 효과가 턴을 넘기는 경우를 위해 TurnStart 에서도 받는다.
	// TurnEnded 로 발동한 효과가 해결된 뒤 다음 행동자 또는 다음 라운드로 간다.
	void EndTurn(HGameplayContext& ctx);

	// 게임 종료. 예약된 단계도 버린다.
	void Finish(HGameplayContext& ctx, int32 resultCode);

	// 예약된 단계를 실행한다. 룰 엔진이 효과 큐가 비었을 때 부른다. 실행한 단계가 없으면 false.
	bool RunPendingStep(HGameplayContext& ctx, const IGameplayOrderPolicy* policy);

private:
	void setPhase(HGameplayContext& ctx, EGameplayPhase phase);
	void beginRound(HGameplayContext& ctx);
	void nextTurn(HGameplayContext& ctx);
	void endRound(HGameplayContext& ctx);
	void buildOrder(HGameplayContext& ctx, const IGameplayOrderPolicy* policy);
};
