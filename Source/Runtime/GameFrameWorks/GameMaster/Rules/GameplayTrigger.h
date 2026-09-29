#pragma once
#include "GameMaster/Rules/GameplayContext.h"
#include "GameplayTrigger.generation.h"

// 이벤트에 반응해 효과를 큐에 넣는다. 게임이 파생해 리플렉션으로 자동 등록된다.
// Matches 는 순수 조건이고 React 는 효과 적재만 한다. 반응 순서는 우선순위(낮을수록 먼저) → 등록 순서.
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayTrigger : public JGObject
{
	JG_GENERATED_CLASS_BODY

public:
	JGGameplayTrigger() = default;
	virtual ~JGGameplayTrigger() = default;

	virtual PName GetKind() const;
	virtual int32 GetPriority() const;
	virtual bool  Matches(const HGameplayState& state, const HGameplayEvent& event) const;
	virtual void  React(HGameplayContext& ctx, const HGameplayEvent& event);
};
