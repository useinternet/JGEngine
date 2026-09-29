#pragma once
#include "Core.h"
#include "Core/GameFrameWorksDefines.h"

// GameMaster 공통 열거형과 상수.
// GameMaster/ 아래 파일은 엔진 Core 헤더만 포함한다. (Graphics · GUI 금지 — 헤드리스 실행 조건)

// 페이즈 기계의 상태. 전이는 PGameplayPhaseMachine 이 담당한다.
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
}

namespace HGameplayLimits
{
	// 한 명령에서 효과 연쇄가 이 깊이를 넘으면 중단하고 EffectDepthExceeded 이벤트를 낸다.
	constexpr int32 MaxEffectDepth = 64;
	// 한 명령에서 해결하는 효과 총 개수 상한 (무한 루프 방어).
	constexpr int32 MaxEffectsPerCommand = 4096;
}
