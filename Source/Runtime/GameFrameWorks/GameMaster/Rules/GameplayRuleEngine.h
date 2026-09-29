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
#include "GameMaster/Rules/GameplayPhaseMachine.h"
#include "GameMaster/Boards/GameplayBoard.h"

// 검증 · 실행 오케스트레이션. 명령 → 핸들러 → 효과 큐 루프 → 트리거 → 선택 대기 중단 · 재개.
// 상태를 소유하지 않는다. 어떤 HGameplayState 에도 적용할 수 있어 AI 가 복사본 위에서 같은 규칙을 돌린다.
class GAMEFRAMEWORKS_API PGameplayRuleEngine : public IMemoryObject
{
public:
	PGameplayRegistry<JGGameplayCommandHandler> Handlers;
	PGameplayRegistry<JGGameplayEffect>         Effects;
	PGameplayRegistry<JGGameplayTrigger>        Triggers;
	PGameplayRegistry<JGGameplayModifier>       Modifiers;
	PGameplayValuePipeline                        ValuePipeline;
	PSharedPtr<IGameplayOrderPolicy>              OrderPolicy;
	PSharedPtr<IGameplayBoard>                    Board;

private:
	PGameplayEffectQueue       _queue;
	PGameplayTriggerDispatcher _dispatcher;
	PGameplayPhaseMachine      _phaseMachine;

	// 실행 중 임시 상태
	HList<HGameplayEvent>* _outEvents        = nullptr;
	bool                     _bChoiceRequested = false;
	HGameplayChoice        _requestedChoice;
	int32                    _resolvedCount    = 0;

public:
	PGameplayRuleEngine() = default;
	virtual ~PGameplayRuleEngine() = default;

	// 페이즈 기계 시작. 첫 라운드 · 첫 턴까지 진행하고 그 이벤트를 돌려준다.
	EGameplaySubmitResult Start(HGameplayState& state, HList<HGameplayEvent>& outEvents);

	bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const;
	EGameplaySubmitResult Execute(HGameplayState& state, const HGameplayCommand& command, HList<HGameplayEvent>& outEvents, PString* outReason);
	void EnumerateLegal(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const;

	// 등록 순서 의존을 없앤다. 등록이 끝난 뒤 한 번 부른다.
	void Finalize();

	// HGameplayContext 가 부르는 서비스. 게임 코드가 직접 부르지 않는다.
	void  ContextEnqueue(const HGameplayContext& ctx, const HGameplayEffectRequest& request, bool bFront);
	void  ContextEmit(HGameplayContext& ctx, const HGameplayEvent& event);
	int32 ContextCompute(const HGameplayContext& ctx, const PName& valueKind, const HGameplayEntityId& subject, const HGameplayEntityId& target, int32 base) const;
	void  ContextRequestChoice(const HGameplayContext& ctx, const HGameplayChoice& choice);
	HGameplayEntityId ContextSpawnEntity(HGameplayContext& ctx, const PName& zoneName);
	bool  ContextDestroyEntity(HGameplayContext& ctx, const HGameplayEntityId& id);
	void  ContextFinishGame(HGameplayContext& ctx, int32 resultCode);

private:
	EGameplaySubmitResult runQueue(HGameplayState& state, uint32 causeSequence);
	void resolveOne(HGameplayState& state, const HGameplayEffectRequest& request, uint32 causeSequence);
	bool resolveBuiltin(HGameplayContext& ctx, const HGameplayEffectRequest& request);
	bool isBuiltinCommand(const PName& kind) const;
	void beginExecution(HList<HGameplayEvent>& outEvents);
	void endExecution();
};
