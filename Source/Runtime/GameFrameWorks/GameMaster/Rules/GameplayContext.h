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

	// 게임 종료. 페이즈를 Finished 로 두고 GameFinished 이벤트를 낸다. Amount 에 결과 코드. 흐름의 예약 단계 · 입력 행동자도 비운다.
	void FinishGame(int32 resultCode);

	// 흐름 공용 칸 (게임 흐름과 그 효과가 쓴다. IGameplayFlow 참고).
	// 단계 이름을 바꾸고 StepChanged 이벤트를 낸다(Tag = 새 단계, Tag2 = 이전 단계, Amount = Round). 같은 이름이면 아무 일도 없다.
	void SetStep(const PName& step);
	// 지금 명령을 낼 수 있는 행동자. 죽은 엔티티 · 중복은 넣지 않는다. 빈 목록 = 아무도 못 낸다.
	void SetActors(const HList<HGameplayEntityId>& actors);

	// 되감기 지점 (ER-027). 지점 = 그 순간의 상태 사본에 붙인 이름. 상태 안이라 저장 · 문서 · 체크섬 · Undo · Simulate · 같이 하기에 함께 간다.
	// 핸들러 · 효과에서 부른다 (트리거는 그것을 하는 효과를 넣는다). 같은 이름으로 다시 표시하면 새 것으로 바꾼다.
	// 다 쓴 지점은 지운다 — 남기면 저장 · 문서 · 명령마다 남기는 되돌리기 사본이 지점 수만큼 커진다.
	void MarkRewindPoint(const PName& name);
	bool ClearRewindPoint(const PName& name);
	bool HasRewindPoint(const PName& name) const;
	// 지점의 상태로 되돌린다. 지점이 없으면 false 이고 아무것도 바꾸지 않는다.
	// 되돌리지 않는 것: 명령 순번 · 시드 · 지점 목록 · 지금까지 낸 엔티티 세대(지점 뒤에 생긴 엔티티 번호는 죽은 번호가 된다).
	// random: 앞으로 뽑을 난수를 지점 것으로 되돌릴지(RestorePoint) 지금 것을 이어 갈지(KeepCurrent). 덱 순서처럼 상태에 든 것은 늘 지점 것.
	// 남은 효과 큐 · 대기 반응 · 이 효과의 선택 요청은 버리고, 흐름은 지점의 단계에서 이어 간다. Rewound 이벤트(Tag = 지점 이름)를 낸 뒤 돌아온다 —
	// 부른 쪽은 이어서 남길 변화(낸 카드 옮기기 · 비용 다시 빼기)를 평소처럼 적용한다. 되감기 전 값이 필요하면 부르기 전에 지역 변수로 복사하고,
	// 그 전에 받은 상태 포인터(Find 결과 등)는 쓰지 않는다.
	bool RewindTo(const PName& name, EGameplayRewindRandom random);

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
