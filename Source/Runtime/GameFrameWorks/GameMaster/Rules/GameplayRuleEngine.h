#pragma once
#include "GameMaster/Rules/GameplayContext.h"
#include "GameMaster/Rules/GameplayCommandHandler.h"
#include "GameMaster/Rules/GameplayEffect.h"
#include "GameMaster/Rules/GameplayTrigger.h"
#include "GameMaster/Rules/GameplayModifier.h"
#include "GameMaster/Rules/GameplayOrderPolicy.h"
#include "GameMaster/Rules/GameplayRegistry.h"
#include "GameMaster/Rules/GameplayEffectQueue.h"
#include "GameMaster/Rules/GameplayTriggerDispatcher.h"
#include "GameMaster/Rules/GameplayValuePipeline.h"
#include "GameMaster/Rules/GameplayFlow.h"
#include "GameMaster/Rules/GameplayRoundTurnFlow.h"
#include "GameMaster/Boards/GameplayBoard.h"

// 검증 · 실행 오케스트레이션. 명령 → 핸들러 → 효과 큐 루프 → 트리거 → 선택 대기 중단 · 재개.
// 루프 한 바퀴: 방금 끝난 단계(핸들러 · 효과 · 흐름 단계)가 낸 이벤트의 트리거 반응 → 효과 하나 해결. 큐가 비면 흐름(IGameplayFlow)의 예약된 단계.
// 상태를 소유하지 않는다. 어떤 HGameplayState 에도 적용할 수 있어 AI 가 복사본 위에서 같은 규칙을 돌린다.
class GAMEFRAMEWORKS_API PGameplayRuleEngine : public IMemoryObject
{
public:
	PGameplayRegistry<JGGameplayCommandHandler> Handlers;
	PGameplayRegistry<JGGameplayEffect>         Effects;
	PGameplayRegistry<JGGameplayTrigger>        Triggers;
	PGameplayRegistry<JGGameplayModifier>       Modifiers;
	PGameplayValuePipeline                        ValuePipeline;
	PSharedPtr<IGameplayOrderPolicy>              OrderPolicy;   // 기본 흐름의 행동 순서. 게임 흐름도 쓸 수 있다
	PSharedPtr<IGameplayBoard>                    Board;
	PSharedPtr<IGameplayFlow>                     Flow;          // 게임 흐름. null 이면 기본 흐름(PGameplayRoundTurnFlow)

private:
	PGameplayEffectQueue       _queue;
	PGameplayTriggerDispatcher _dispatcher;
	PGameplayRoundTurnFlow     _defaultFlow;

	// 실행 중 임시 상태
	HList<HGameplayEvent>* _outEvents        = nullptr;
	bool                     _bResolvingEffect = false;   // 효과 Resolve 안인가. 선택 요청 허용 · 적재 깊이의 기준
	bool                     _bChoiceRequested = false;
	HGameplayChoice        _requestedChoice;
	int32                    _resolvedCount    = 0;
	bool                     _bHalted          = false;   // 효과 총량 한도를 넘어 이 명령에서는 효과 · 반응을 더 받지 않음

public:
	PGameplayRuleEngine() = default;
	virtual ~PGameplayRuleEngine() = default;

	// 흐름 시작. GameStarted 를 내고 흐름이 첫 대기 지점(명령 대기 · 선택 대기 · 종료)에 닿을 때까지 진행한 뒤 그 이벤트를 돌려준다.
	EGameplaySubmitResult Start(HGameplayState& state, HList<HGameplayEvent>& outEvents);

	// 지금 쓰는 흐름 (Flow 가 없으면 기본 흐름).
	const IGameplayFlow& GetActiveFlow() const;

	bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const;
	EGameplaySubmitResult Execute(HGameplayState& state, const HGameplayCommand& command, HList<HGameplayEvent>& outEvents, PString* outReason);
	void EnumerateLegal(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const;

	// 등록 순서 의존을 없앤다. 등록이 끝난 뒤 한 번 부른다 (Start · Load · Replay 가 부른다).
	void Finalize();

	// HGameplayContext · HGameplayTriggerContext 가 부르는 서비스. 게임 코드가 직접 부르지 않는다.
	void  ContextEnqueue(const HGameplayContext& ctx, const HGameplayEffectRequest& request, bool bFront);
	void  ContextEmit(HGameplayContext& ctx, const HGameplayEvent& event);
	int32 ContextCompute(const HGameplayContext& ctx, const PName& valueKind, const HGameplayEntityId& subject, const HGameplayEntityId& target, int32 base) const;
	bool  ContextRequestChoice(const HGameplayContext& ctx, const HGameplayChoice& choice);
	HGameplayEntityId ContextSpawnEntity(HGameplayContext& ctx, const PName& zoneName);
	bool  ContextDestroyEntity(HGameplayContext& ctx, const HGameplayEntityId& id);
	void  ContextFinishGame(HGameplayContext& ctx, int32 resultCode);
	void  ContextSetStep(HGameplayContext& ctx, const PName& step);
	void  ContextSetActors(HGameplayContext& ctx, const HList<HGameplayEntityId>& actors);
	void  TriggerEnqueue(const HGameplayTriggerContext& ctx, const HGameplayEffectRequest& request, bool bFront);

private:
	IGameplayFlow& activeFlow();
	EGameplaySubmitResult runQueue(HGameplayState& state, uint32 causeSequence);
	void resolveOne(HGameplayState& state, const HGameplayEffectRequest& request, uint32 causeSequence);
	bool resolveBuiltin(HGameplayContext& ctx, const HGameplayEffectRequest& request);
	void suspendForChoice(HGameplayState& state, const HGameplayEffectRequest& request, uint32 causeSequence);
	void halt(HGameplayState& state, uint32 causeSequence);
	void enqueue(const HGameplayEffectRequest& request, uint32 causeSequence, int32 depth, bool bFront);
	bool isBuiltinCommand(const PName& kind) const;
	void beginExecution(HList<HGameplayEvent>& outEvents);
	void endExecution();
};
