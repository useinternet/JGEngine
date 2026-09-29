#pragma once
#include "GameMaster/Rules/GameplayTrigger.h"
#include "GameMaster/Rules/GameplayRegistry.h"

class PGameplayRuleEngine;

// 이벤트 → 트리거 수집 → 결정론 정렬 → React. 정렬 키: 우선순위(낮을수록 먼저) → 레지스트리 순서.
class GAMEFRAMEWORKS_API PGameplayTriggerDispatcher
{
public:
	void Dispatch(HGameplayState& state, PGameplayRuleEngine& engine, const PGameplayRegistry<JGGameplayTrigger>& triggers, const HGameplayEvent& event);
};
