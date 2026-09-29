#pragma once
#include "GameMaster/Rules/GameplayContext.h"
#include "GameplayCommandHandler.generation.h"

// 명령 종류 하나의 규칙. 게임이 파생해 리플렉션으로 자동 등록된다.
// 세 메서드 모두 기본 구현이 있다 (리플렉션 생성을 위해 추상 클래스일 수 없다).
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayCommandHandler : public JGObject
{
	JG_GENERATED_CLASS_BODY

public:
	JGGameplayCommandHandler() = default;
	virtual ~JGGameplayCommandHandler() = default;

	// 이 핸들러가 처리하는 명령 종류. HGameplayCommand::Kind 와 같아야 한다.
	virtual PName GetKind() const;

	// 상태를 바꾸지 않는다. 실패 사유를 outReason 에 적는다 (UI 가 그대로 보여 준다).
	virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const;

	// 효과를 큐에 넣거나 상태를 바꾸고 이벤트를 낸다. 반드시 ctx 로만 접근한다.
	virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command);

	// actor 가 지금 낼 수 있는 이 종류의 명령을 전부 out 에 넣는다. UI 하이라이트 · AI · 테스트의 공통 근거.
	virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const;
};
