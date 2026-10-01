#pragma once
#include "GameMaster/Rules/GameplayFlow.h"
#include "GameMaster/Rules/GameplayOrderPolicy.h"

// 기본 흐름. 게임이 흐름을 주지 않으면(PGameMaster::SetFlow) 이것을 쓴다. 행동 순서는 순서 정책(PGameplayRuleEngine::OrderPolicy)이 정한다.
//   RoundStart → OrderResolve → TurnStart → TurnMain → TurnEnd → (다음 행동자) … → RoundEnd → RoundStart …
// 모든 전이는 이벤트로 발행되어 트리거와 프레젠테이션이 반응한다.
// 전이는 단계(EGameplayPhaseStep)로 나뉜다. 한 단계가 이벤트를 내고 다음 단계를 예약하면, 룰 엔진이 그 이벤트로 발동한
// 효과를 다 해결한 뒤에 다음 단계를 부른다. 그래서 턴 종료 효과는 다음 행동자가 정해지기 전에, 라운드 시작 효과는 순서가 정해지기 전에 해결된다.
// 공용 칸: Step = 페이즈 이름, Actors = TurnMain 동안 [CurrentActor]. 새로 만든 순서에 행동자가 없으면 판을 끝낸다(결과 0).
class GAMEFRAMEWORKS_API PGameplayRoundTurnFlow : public IGameplayFlow
{
public:
	PGameplayRoundTurnFlow() = default;
	virtual ~PGameplayRoundTurnFlow() = default;

	virtual PName GetName() const override;

	// 첫 라운드를 예약한다.
	virtual void Start(HGameplayContext& ctx) override;
	virtual bool RunPendingStep(HGameplayContext& ctx) override;

	// TurnMain 의 현재 행동자만.
	virtual bool CanEndTurn(const HGameplayState& state, const HGameplayEntityId& actor, PString* outReason) const override;
	// 현재 행동자의 턴을 끝낸다(actor 는 보지 않는다). 턴 시작 효과가 턴을 넘기는 경우를 위해 TurnStart 에서도 받는다.
	// TurnEnded 로 발동한 효과가 해결된 뒤 다음 행동자 또는 다음 라운드로 간다.
	virtual void EndTurn(HGameplayContext& ctx, const HGameplayEntityId& actor) override;

private:
	void setPhase(HGameplayContext& ctx, EGameplayPhase phase);
	void beginRound(HGameplayContext& ctx);
	void nextTurn(HGameplayContext& ctx);
	void endRound(HGameplayContext& ctx);
	void buildOrder(HGameplayContext& ctx);
};
