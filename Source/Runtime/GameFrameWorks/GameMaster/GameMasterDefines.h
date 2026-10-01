#pragma once
#include "Core.h"
#include "Core/GameFrameWorksDefines.h"

// GameMaster 공통 열거형과 상수.
// GameMaster/ 아래 파일은 엔진 Core 헤더만 포함한다. (Graphics · GUI 금지 — 헤드리스 실행 조건)

// 게임 진행 상태. NotStarted · Finished 는 모든 흐름 공통이다.
// RoundStart ~ RoundEnd 는 기본 흐름(PGameplayRoundTurnFlow) 전용이고, 게임 흐름(IGameplayFlow)은 그 사이를 Flow 하나로 두고 단계는 Turn.Step 으로 나타낸다.
enum class EGameplayPhase : int32
{
	NotStarted = 0,
	RoundStart,
	OrderResolve,
	TurnStart,
	TurnMain,
	TurnEnd,
	RoundEnd,
	Finished,
	Flow,              // 게임 흐름 진행 중 (세부 단계는 Turn.Step)
};

inline const char* GetGameplayPhaseName(EGameplayPhase phase)
{
	switch (phase)
	{
	case EGameplayPhase::NotStarted:   return "NotStarted";
	case EGameplayPhase::RoundStart:   return "RoundStart";
	case EGameplayPhase::OrderResolve: return "OrderResolve";
	case EGameplayPhase::TurnStart:    return "TurnStart";
	case EGameplayPhase::TurnMain:     return "TurnMain";
	case EGameplayPhase::TurnEnd:      return "TurnEnd";
	case EGameplayPhase::RoundEnd:     return "RoundEnd";
	case EGameplayPhase::Finished:     return "Finished";
	case EGameplayPhase::Flow:         return "Flow";
	default:                           return "?";
	}
}

// 기본 흐름이 효과 큐가 빈 뒤에 이어 갈 전이 단계. 상태(HGameplayTurnState)에 들어가므로 선택 대기 · 저장 · 되돌리기에도 보존된다.
// 한 단계가 낸 이벤트로 발동한 효과가 모두 해결된 다음에 다음 단계가 진행된다. 게임 흐름은 Turn.NextStep 을 쓴다.
enum class EGameplayPhaseStep : int32
{
	None = 0,
	BeginRound,        // RoundStart 진입, RoundStarted 발행
	ResolveOrder,      // OrderResolve 진입
	BuildOrder,        // 행동 순서를 만들고 첫 행동자로
	NextTurn,          // 다음 행동자의 TurnStart · TurnStarted. 남은 행동자가 없으면 RoundEnd · RoundEnded
	EnterTurnMain,     // TurnMain 진입. 명령을 기다린다
};

// PGameMaster::Submit 의 결과.
enum class EGameplaySubmitResult : int32
{
	Rejected = 0,      // 검증 실패. outReason 에 사유
	Executed,          // 실행 완료
	PendingChoice,     // 해결 도중 선택 대기로 중단. ResolveChoice 명령으로 재개
};

// 보드 종류.
enum class EGameplayBoardKind : int32
{
	None = 0,          // 보드 없음 (카드 전투류)
	Square,            // 사각 격자
	Hex,               // 육각 격자 (축 좌표)
	Free,              // 자유 공간 (정수 좌표, 거리 기반)
};

// 용도별 난수 스트림. 같은 시드라도 용도를 나누면 한 용도의 소비가 다른 용도의 결과를 바꾸지 않는다.
enum class EGameplayRandomStream : int32
{
	Shuffle = 0,
	Action,
	Reward,
	Map,
	Misc0,
	Misc1,
	Misc2,
	Misc3,
	Count,
};

// 엔진 내장 명령 · 효과 · 이벤트 이름. 게임은 같은 이름을 쓰지 않는다.
namespace HGameplayBuiltin
{
	// 명령
	constexpr const char* CommandEndTurn      = "EndTurn";
	constexpr const char* CommandResolveChoice = "ResolveChoice";

	// 효과
	constexpr const char* EffectSpawnEntity      = "SpawnEntity";
	constexpr const char* EffectDestroyEntity    = "DestroyEntity";
	constexpr const char* EffectMoveToZone       = "MoveToZone";
	constexpr const char* EffectSetBoardPosition = "SetBoardPosition";
	constexpr const char* EffectEndTurn          = "EndTurn";

	// 이벤트
	constexpr const char* EventGameStarted       = "GameStarted";
	constexpr const char* EventGameFinished      = "GameFinished";
	constexpr const char* EventRoundStarted      = "RoundStarted";
	constexpr const char* EventRoundEnded        = "RoundEnded";
	constexpr const char* EventTurnStarted       = "TurnStarted";
	constexpr const char* EventTurnEnded         = "TurnEnded";
	constexpr const char* EventPhaseChanged      = "PhaseChanged";
	constexpr const char* EventEntitySpawned     = "EntitySpawned";
	constexpr const char* EventEntityDestroyed   = "EntityDestroyed";
	constexpr const char* EventZoneChanged       = "ZoneChanged";
	constexpr const char* EventBoardMoved        = "BoardMoved";
	constexpr const char* EventChoiceRequested   = "ChoiceRequested";
	constexpr const char* EventChoiceResolved    = "ChoiceResolved";
	constexpr const char* EventEffectDepthExceeded = "EffectDepthExceeded";
	constexpr const char* EventEffectUnknown     = "EffectUnknown";
	constexpr const char* EventStepChanged       = "StepChanged";             // 게임 흐름의 단계 전환. Tag = 새 단계, Tag2 = 이전 단계, Amount = Round
	constexpr const char* EventFlowStepLimitExceeded = "FlowStepLimitExceeded";  // 흐름이 대기 없이 단계를 계속 이어 한도에서 멈춤. Amount = 단계 수
}

namespace HGameplayLimits
{
	// 한 명령에서 효과 연쇄가 이 깊이를 넘으면 중단하고 EffectDepthExceeded 이벤트를 낸다.
	// 깊이는 명령 → 효과 → 이벤트 → 트리거 → 효과 … 인과 한 단계마다 1 씩 늘어난다.
	constexpr int32 MaxEffectDepth = 64;
	// 한 명령에서 해결하는 효과 총 개수 상한 (무한 루프 방어). 넘으면 남은 효과 · 트리거 반응을 버리고 페이즈 전이만 마친다.
	constexpr int32 MaxEffectsPerCommand = 4096;
	// 한 명령에서 흐름(IGameplayFlow::RunPendingStep)이 진행하는 단계 수 상한. 입력을 기다리지 않고 단계를 계속 잇는 흐름의 무한 루프 방어.
	// 넘으면 FlowStepLimitExceeded 를 내고 그 명령에서는 단계를 더 진행하지 않는다.
	constexpr int32 MaxFlowStepsPerCommand = 1024;
}
