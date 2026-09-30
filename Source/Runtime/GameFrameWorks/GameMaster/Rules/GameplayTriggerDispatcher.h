#pragma once
#include "GameMaster/Rules/GameplayTrigger.h"
#include "GameMaster/Rules/GameplayRegistry.h"

class PGameplayRuleEngine;

// 이벤트 → 트리거 매칭 → 결정론 정렬 → (이벤트를 낸 단계가 끝난 뒤) React.
// 정렬 키: 이벤트 발행 순서 → 우선순위(낮을수록 먼저) → 레지스트리 순서.
class GAMEFRAMEWORKS_API PGameplayTriggerDispatcher
{
	struct HPendingReaction
	{
		PSharedPtr<JGGameplayTrigger> Trigger;
		HGameplayEvent                Event;
	};

	HList<HPendingReaction> _pending;

public:
	// 발행 순간의 상태로 Matches 를 평가해 반응 대기열에 넣는다. React 는 부르지 않는다.
	void Match(const HGameplayState& state, const PGameplayRegistry<JGGameplayTrigger>& triggers, const HGameplayEvent& event);

	// 대기 중인 반응을 순서대로 부른다. React 는 효과 적재만 할 수 있어 도는 동안 새 반응이 생기지 않는다.
	void React(const HGameplayState& state, PGameplayRuleEngine& engine);

	void Clear();
};
