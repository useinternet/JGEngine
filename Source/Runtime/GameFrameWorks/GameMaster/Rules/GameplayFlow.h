#pragma once
#include "GameMaster/Rules/GameplayContext.h"

// 게임 흐름. 판이 어떤 단계로 어떻게 진행되는지(턴 · 페이즈 구조)를 정한다. 엔진은 언제 부를지만 정하고 무엇을 할지는 흐름이 정한다.
//   Start          PGameMaster::Start 에서 한 번 (엔진이 GameStarted 를 낸 뒤)
//   RunPendingStep 효과 큐가 빌 때마다. 할 일을 했으면 true(엔진이 그 단계의 이벤트 · 효과를 처리한 뒤 다시 부른다), 할 일이 없으면 false = 명령 대기
//   CanEndTurn · EndTurn  내장 EndTurn 명령 · 효과
// 흐름 객체는 진행 상태를 갖지 않는다. 진행 상태는 HGameplayState 에 둔다(Turn 의 공용 칸, 필요하면 게임 컴포넌트).
//   → 되돌리기 · 저장 · 리플레이 · 체크섬 · 네트워크 동기화가 흐름과 상관없이 되고, AI 가 상태 복사본 위에서 같은 흐름을 돌린다.
// 흐름이 채우는 공용 칸 (엔진 기능이 읽는다 — 컨트롤러 입력 · 네트워크 세션 · AI 러너 · DevView):
//   Turn.Actors    지금 명령을 낼 수 있는 행동자. ctx.SetActors. 0명 = 자동 단계, 1명 = 차례, 여러 명 = 동시 · 아무나
//   Turn.Step      지금 단계 이름. ctx.SetStep 이 StepChanged 이벤트를 함께 낸다
//   Turn.NextStep  다음 RunPendingStep 에서 할 일 (흐름의 예약 칸, 선택)
//   Turn.Round · Turn.TurnCount  공용 카운터 (원하는 흐름만)
// 판 끝은 흐름이 정한다: ctx.FinishGame(결과 코드). 단계 안에서 선택을 받으려면 선택을 요청하는 효과를 큐에 넣는다(RequestChoice 는 효과 Resolve 안에서만).
// 대기 없이 단계를 계속 이으면 명령당 HGameplayLimits::MaxFlowStepsPerCommand 에서 멈춘다(FlowStepLimitExceeded).
// 게임이 흐름을 주지 않으면 기본 흐름 PGameplayRoundTurnFlow(라운드 → 순서 → 행동자마다 차례)를 쓴다.
class GAMEFRAMEWORKS_API IGameplayFlow : public IMemoryObject
{
public:
	virtual ~IGameplayFlow() = default;

	// 규칙 지문(PGameMaster::RulesFingerprint)에 들어간다. 기계마다 같은 흐름인지 확인하는 데 쓴다.
	virtual PName GetName() const = 0;

	virtual void Start(HGameplayContext& ctx) = 0;
	virtual bool RunPendingStep(HGameplayContext& ctx) = 0;

	// EndTurn 을 받을 수 있는가. 거절 사유는 outReason 에(nullptr 이면 쓰지 않는다). EnumerateLegal 도 이것으로 EndTurn 후보를 정한다.
	virtual bool CanEndTurn(const HGameplayState& state, const HGameplayEntityId& actor, PString* outReason) const = 0;
	// 내장 EndTurn 명령(검증을 통과한 행동자)과 EndTurn 효과(요청의 Subject)가 부른다.
	virtual void EndTurn(HGameplayContext& ctx, const HGameplayEntityId& actor) = 0;
};
