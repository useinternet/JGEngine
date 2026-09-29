#pragma once
#include "GameMaster/Rules/GameplayContext.h"
#include "GameplayEffect.generation.h"

// 효과 종류 하나. 큐에서 하나씩 해결된다. 게임이 파생해 리플렉션으로 자동 등록된다.
// 효과 하나는 "한 가지 상태 변화 + 그 이벤트" 로 작게 만들고, 연쇄는 ctx.Enqueue 로 넣는다.
// 선택 입력이 필요하면 request.HasChoice() 가 false 일 때 ctx.RequestChoice 후 반환하고,
// true 일 때 request.Choice 를 써서 진행한다 (재진입 패턴).
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayEffect : public JGObject
{
	JG_GENERATED_CLASS_BODY

public:
	JGGameplayEffect() = default;
	virtual ~JGGameplayEffect() = default;

	virtual PName GetKind() const;
	virtual void  Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request);
};
