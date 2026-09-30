#pragma once
#include "GameMaster/State/GameplayState.h"
#include "GameMaster/Messages/GameplayCommand.h"
#include "GameMaster/Messages/GameplayEffectRequest.h"
#include "GameMaster/Messages/GameplayEvent.h"

class PGameplayRuleEngine;
class IGameplayBoard;

// 명령 핸들러 · 효과 · 수정자에 넘기는 작업 창구. 게임 코드가 상태를 바꾸는 길은 이것뿐이다.
// 명령 하나를 실행하는 동안만 유효하다. 보관하지 말 것. 트리거는 HGameplayTriggerContext 를 받는다.
struct GAMEFRAMEWORKS_API HGameplayContext
{
	HGameplayState&      State;
	PGameplayRuleEngine& Engine;
	const uint32           CauseSequence;   // 이 실행을 일으킨 명령 번호
	const int32            Depth;           // 현재 효과 연쇄 깊이. 핸들러는 0

	HGameplayContext(HGameplayState& inState, PGameplayRuleEngine& inEngine, uint32 inCauseSequence, int32 inDepth);

	// 효과 큐. 핸들러가 넣으면 깊이 0, 효과가 넣으면 현재 깊이 + 1.
	void Enqueue(const HGameplayEffectRequest& request);
	void EnqueueFront(const HGameplayEffectRequest& request);

	// 이벤트 발행. 원인 · 깊이는 여기서 채운다. 트리거는 발행 순간의 상태로 매칭되고, 반응은 지금 단계(효과 · 핸들러)가 끝난 뒤에 한다.
	void Emit(const HGameplayEvent& event);

	// 상태 안의 난수 스트림. 외부 난수는 쓰지 않는다.
	HGameplayRandomStream& Rng(EGameplayRandomStream stream) const;

	// 수치 파이프라인. 게임이 정의한 단계 순서로 수정자를 적용한 값.
	int32 Compute(const PName& valueKind, const HGameplayEntityId& subject, const HGameplayEntityId& target, int32 base) const;

	// 해결 중단 → 선택 대기. 효과의 Resolve 안에서만 받는다. 부른 뒤에는 바로 반환해야 한다.
	// 효과 밖(핸들러 Execute)에서 부르면 오류 로그와 함께 거부하고 false. 핸들러는 선택을 요청하는 효과를 넣는다.
	bool RequestChoice(const HGameplayChoice& choice);

	// 엔티티 생성 · 파괴. 이벤트(EntitySpawned / EntityDestroyed)를 함께 낸다.
	HGameplayEntityId SpawnEntity(const PName& zoneName = PName());
	bool DestroyEntity(const HGameplayEntityId& id);

	// 게임 종료. 페이즈를 Finished 로 두고 GameFinished 이벤트를 낸다. Amount 에 결과 코드.
	void FinishGame(int32 resultCode);

	// 보드 질의 객체. 보드 상태는 State.Board, 규칙(거리 · 이웃 · 경로)은 이것.
	const IGameplayBoard* Board() const;

	HGameplayContext(const HGameplayContext&) = delete;
	HGameplayContext& operator=(const HGameplayContext&) = delete;
};

// 트리거 React 에 넘기는 창구. 효과를 큐에 넣는 것만 된다. 상태는 읽기 전용이다.
// 반응은 이벤트를 낸 단계(효과 · 핸들러 · 페이즈 전이)가 끝난 뒤에 불린다. 넣은 효과의 깊이는 이벤트 깊이 + 1.
// 선택 요청 · 엔티티 생성/파괴 · 이벤트 발행 · 난수가 필요하면 그것을 하는 효과를 넣는다.
class GAMEFRAMEWORKS_API HGameplayTriggerContext
{
	PGameplayRuleEngine& _engine;

public:
	const HGameplayState& State;
	const uint32            CauseSequence;
	const int32             Depth;   // 여기서 넣는 효과의 깊이

	HGameplayTriggerContext(const HGameplayState& inState, PGameplayRuleEngine& inEngine, uint32 inCauseSequence, int32 inDepth);

	void Enqueue(const HGameplayEffectRequest& request);
	void EnqueueFront(const HGameplayEffectRequest& request);

	const IGameplayBoard* Board() const;

	HGameplayTriggerContext(const HGameplayTriggerContext&) = delete;
	HGameplayTriggerContext& operator=(const HGameplayTriggerContext&) = delete;
};
