#pragma once
#include "GameMaster/Rules/GameplayContext.h"
#include "GameMaster/Rules/GameplayOrderPolicy.h"

// 페이즈 기계. 전이 골격은 엔진, 행동 순서는 IGameplayOrderPolicy 가 정한다.
//   RoundStart → OrderResolve → TurnStart → TurnMain → TurnEnd → (다음 행동자) … → RoundEnd → RoundStart …
// 모든 전이는 이벤트로 발행되어 트리거와 프레젠테이션이 반응한다.
class GAMEFRAMEWORKS_API PGameplayPhaseMachine
{
public:
	// NotStarted 에서 첫 라운드 · 첫 턴까지 진행한다. 행동자가 없으면 Finished.
	void Start(HGameplayContext& ctx, const IGameplayOrderPolicy* policy);

	// TurnMain 에서 턴을 끝내고 다음 행동자 또는 다음 라운드로 진행한다.
	void EndTurn(HGameplayContext& ctx, const IGameplayOrderPolicy* policy);

	// 게임 종료.
	void Finish(HGameplayContext& ctx, int32 resultCode);

private:
	void setPhase(HGameplayContext& ctx, EGameplayPhase phase);
	void beginRound(HGameplayContext& ctx, const IGameplayOrderPolicy* policy);
	bool nextTurn(HGameplayContext& ctx);
	void endRound(HGameplayContext& ctx);
	void buildOrder(HGameplayContext& ctx, const IGameplayOrderPolicy* policy);
};
