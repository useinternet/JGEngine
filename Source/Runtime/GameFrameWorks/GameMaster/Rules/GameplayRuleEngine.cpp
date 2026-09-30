#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayRuleEngine.h"

namespace
{
	void setReason(PString* outReason, const PString& reason)
	{
		if (outReason != nullptr)
		{
			*outReason = reason;
		}
	}
}

EGameplaySubmitResult PGameplayRuleEngine::Start(HGameplayState& state, HList<HGameplayEvent>& outEvents)
{
	beginExecution(outEvents);

	uint32 cause = state.Sequence;
	HGameplayContext ctx(state, *this, cause, 0);
	_phaseMachine.Start(ctx);

	EGameplaySubmitResult result = runQueue(state, cause);
	endExecution();
	return result;
}

bool PGameplayRuleEngine::Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const
{
	if (state.Turn.IsStarted() == false)
	{
		setReason(outReason, "gameMaster not started");
		return false;
	}
	if (state.Turn.IsFinished() == true)
	{
		setReason(outReason, "game finished");
		return false;
	}

	PName resolveChoice(HGameplayBuiltin::CommandResolveChoice);
	PName endTurn(HGameplayBuiltin::CommandEndTurn);

	if (command.Kind == resolveChoice)
	{
		if (state.Choice.bPending == false)
		{
			setReason(outReason, "no choice pending");
			return false;
		}
		if (state.Choice.Chooser != command.Actor)
		{
			setReason(outReason, "not the chooser");
			return false;
		}
		return state.Choice.IsValidSelection(command.Targets, outReason);
	}

	if (state.Choice.bPending == true)
	{
		setReason(outReason, "choice pending");
		return false;
	}

	if (command.Kind == endTurn)
	{
		if (state.Turn.CanAct() == false)
		{
			setReason(outReason, "cannot end turn now");
			return false;
		}
		if (state.Turn.CurrentActor != command.Actor)
		{
			setReason(outReason, "not your turn");
			return false;
		}
		return true;
	}

	PSharedPtr<JGGameplayCommandHandler> handler = Handlers.Find(command.Kind);
	if (handler == nullptr)
	{
		setReason(outReason, PString::Format("unknown command %s", command.Kind.ToString()));
		return false;
	}

	return handler->Validate(state, command, outReason);
}

EGameplaySubmitResult PGameplayRuleEngine::Execute(HGameplayState& state, const HGameplayCommand& command, HList<HGameplayEvent>& outEvents, PString* outReason)
{
	PString reason;
	if (Validate(state, command, &reason) == false)
	{
		setReason(outReason, reason);
		return EGameplaySubmitResult::Rejected;
	}

	beginExecution(outEvents);

	uint32 cause = state.Sequence;
	HGameplayContext ctx(state, *this, cause, 0);

	PName resolveChoice(HGameplayBuiltin::CommandResolveChoice);
	PName endTurn(HGameplayBuiltin::CommandEndTurn);

	if (command.Kind == resolveChoice)
	{
		// 보존해 둔 큐를 복원하고, 기다리던 효과에 선택 결과를 채워 맨 앞에 넣는다.
		HGameplayEffectRequest waiting = state.Choice.Waiting;
		waiting.bHasChoice = true;
		waiting.Choice     = command.Targets;

		_queue.FromList(state.Choice.SavedQueue);
		_queue.PushFront(waiting);

		HGameplayEvent resolved(PName(HGameplayBuiltin::EventChoiceResolved), state.Choice.Chooser, HGameplayEntityId::None());
		resolved.Tag    = state.Choice.Kind;
		resolved.Amount = (int32)command.Targets.size();

		state.Choice.Clear();
		ctx.Emit(resolved);
	}
	else if (command.Kind == endTurn)
	{
		_phaseMachine.EndTurn(ctx);
	}
	else
	{
		PSharedPtr<JGGameplayCommandHandler> handler = Handlers.Find(command.Kind);
		handler->Execute(ctx, command);
	}

	EGameplaySubmitResult result = runQueue(state, cause);

	++state.Sequence;
	endExecution();
	return result;
}

void PGameplayRuleEngine::EnumerateLegal(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const
{
	if (state.Turn.IsStarted() == false || state.Turn.IsFinished() == true)
	{
		return;
	}
	if (state.Choice.bPending == true)
	{
		// 선택 후보는 조합이 많다. UI 는 GetPendingChoice 를 직접 읽는다.
		return;
	}

	HList<HGameplayCommand> candidates;

	if (state.Turn.CanAct() == true && state.Turn.CurrentActor == actor)
	{
		candidates.push_back(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), actor));
	}

	for (const PSharedPtr<JGGameplayCommandHandler>& handler : Handlers.All())
	{
		handler->Enumerate(state, actor, candidates);
	}

	for (const HGameplayCommand& candidate : candidates)
	{
		if (Validate(state, candidate, nullptr) == true)
		{
			outCommands.push_back(candidate);
		}
	}
}

void PGameplayRuleEngine::Finalize()
{
	Handlers.SortByKind();
	Effects.SortByKind();
	Triggers.SortByKind();
	Modifiers.SortByKind();
}

void PGameplayRuleEngine::ContextEnqueue(const HGameplayContext& ctx, const HGameplayEffectRequest& request, bool bFront)
{
	// 핸들러가 넣은 것은 깊이 0, 효과가 넣은 것은 한 단계 깊어진다.
	int32 depth = _bResolvingEffect == true ? ctx.Depth + 1 : ctx.Depth;
	enqueue(request, ctx.CauseSequence, depth, bFront);
}

void PGameplayRuleEngine::TriggerEnqueue(const HGameplayTriggerContext& ctx, const HGameplayEffectRequest& request, bool bFront)
{
	// 트리거 컨텍스트의 깊이가 이미 이벤트 깊이 + 1 이다.
	enqueue(request, ctx.CauseSequence, ctx.Depth, bFront);
}

void PGameplayRuleEngine::ContextEmit(HGameplayContext& ctx, const HGameplayEvent& event)
{
	HGameplayEvent copy = event;
	copy.CauseSequence = ctx.CauseSequence;
	copy.Depth         = ctx.Depth;

	if (_outEvents != nullptr)
	{
		_outEvents->push_back(copy);
	}

	// 여기서는 매칭만 한다. React 는 이 이벤트를 낸 단계가 끝난 뒤 runQueue 가 부른다.
	// 그래서 Emit → React → Emit 재귀가 생기지 않고, 반응이 낳는 연쇄는 효과 큐의 깊이 한도를 거친다.
	if (_bHalted == false)
	{
		_dispatcher.Match(ctx.State, Triggers, copy);
	}
}

int32 PGameplayRuleEngine::ContextCompute(const HGameplayContext& ctx, const PName& valueKind, const HGameplayEntityId& subject, const HGameplayEntityId& target, int32 base) const
{
	HGameplayValueQuery query;
	query.Kind    = valueKind;
	query.Subject = subject;
	query.Target  = target;
	query.Base    = base;
	return ValuePipeline.Compute(ctx, Modifiers, query);
}

bool PGameplayRuleEngine::ContextRequestChoice(const HGameplayContext& ctx, const HGameplayChoice& choice)
{
	// 선택은 그것을 요청한 효과에 붙는다 (ResolveChoice 때 그 효과가 재진입한다). 효과 밖에서는 붙일 곳이 없다.
	if (_bResolvingEffect == false)
	{
		JG_LOG(GameMaster, ELogLevel::Error, "PGameplayRuleEngine: RequestChoice(%s) outside an effect Resolve is rejected. Enqueue an effect that requests it", choice.Kind.ToString());
		return false;
	}

	_bChoiceRequested = true;
	_requestedChoice  = choice;
	_requestedChoice.bPending = true;
	return true;
}

HGameplayEntityId PGameplayRuleEngine::ContextSpawnEntity(HGameplayContext& ctx, const PName& zoneName)
{
	HGameplayEntityId id = ctx.State.CreateEntity();
	if (zoneName != NAME_NONE)
	{
		ctx.State.Zone(zoneName).PushBack(id);
	}

	HGameplayEvent spawned(PName(HGameplayBuiltin::EventEntitySpawned), id, HGameplayEntityId::None());
	spawned.Tag = zoneName;
	ContextEmit(ctx, spawned);
	return id;
}

bool PGameplayRuleEngine::ContextDestroyEntity(HGameplayContext& ctx, const HGameplayEntityId& id)
{
	if (ctx.State.IsAlive(id) == false)
	{
		return false;
	}

	HGameplayEvent destroyed(PName(HGameplayBuiltin::EventEntityDestroyed), id, HGameplayEntityId::None());
	ctx.State.Zones.FindZoneOf(id, &destroyed.Tag);
	ContextEmit(ctx, destroyed);

	return ctx.State.DestroyEntity(id);
}

void PGameplayRuleEngine::ContextFinishGame(HGameplayContext& ctx, int32 resultCode)
{
	_phaseMachine.Finish(ctx, resultCode);
}

EGameplaySubmitResult PGameplayRuleEngine::runQueue(HGameplayState& state, uint32 causeSequence)
{
	while (true)
	{
		// 방금 끝난 단계(핸들러 · 효과 · 페이즈 전이)가 낸 이벤트에 트리거가 반응한다. 반응은 효과를 큐에 넣을 뿐이다.
		_dispatcher.React(state, *this);

		if (_queue.IsEmpty() == true)
		{
			// 효과가 다 해결된 뒤에 페이즈 기계가 예약된 다음 전이 단계로 간다.
			HGameplayContext ctx(state, *this, causeSequence, 0);
			if (_phaseMachine.RunPendingStep(ctx, OrderPolicy.GetRawPointer()) == true)
			{
				continue;
			}
			break;
		}

		if (_resolvedCount >= HGameplayLimits::MaxEffectsPerCommand)
		{
			halt(state, causeSequence);
			continue;
		}

		HGameplayEffectRequest request = _queue.PopFront();
		++_resolvedCount;

		if (request.Depth > HGameplayLimits::MaxEffectDepth)
		{
			HGameplayContext ctx(state, *this, causeSequence, request.Depth);
			HGameplayEvent exceeded(PName(HGameplayBuiltin::EventEffectDepthExceeded), request.Subject, request.Target);
			exceeded.Tag    = request.Kind;
			exceeded.Amount = request.Depth;
			ContextEmit(ctx, exceeded);
			continue;
		}

		resolveOne(state, request, causeSequence);

		if (_bChoiceRequested == true)
		{
			suspendForChoice(state, request, causeSequence);
			return EGameplaySubmitResult::PendingChoice;
		}
	}

	return EGameplaySubmitResult::Executed;
}

void PGameplayRuleEngine::resolveOne(HGameplayState& state, const HGameplayEffectRequest& request, uint32 causeSequence)
{
	HGameplayContext ctx(state, *this, causeSequence, request.Depth);
	_bResolvingEffect = true;

	if (resolveBuiltin(ctx, request) == false)
	{
		PSharedPtr<JGGameplayEffect> effect = Effects.Find(request.Kind);
		if (effect != nullptr)
		{
			effect->Resolve(ctx, request);
		}
		else
		{
			HGameplayEvent unknown(PName(HGameplayBuiltin::EventEffectUnknown), request.Subject, request.Target);
			unknown.Tag = request.Kind;
			ContextEmit(ctx, unknown);
			JG_LOG(GameMaster, ELogLevel::Warning, "PGameplayRuleEngine: unknown effect %s", request.Kind.ToString());
		}
	}

	_bResolvingEffect = false;
}

bool PGameplayRuleEngine::resolveBuiltin(HGameplayContext& ctx, const HGameplayEffectRequest& request)
{
	if (request.Kind == PName(HGameplayBuiltin::EffectSpawnEntity))
	{
		ContextSpawnEntity(ctx, request.Tag);
		return true;
	}

	if (request.Kind == PName(HGameplayBuiltin::EffectDestroyEntity))
	{
		ContextDestroyEntity(ctx, request.Target);
		return true;
	}

	// 이동 · 배치 대상이 이미 죽었으면(파괴 뒤 늦게 온 요청) 상태가 거부하고, 이벤트 없이 지나간다.
	if (request.Kind == PName(HGameplayBuiltin::EffectMoveToZone))
	{
		PName from;
		ctx.State.Zones.FindZoneOf(request.Target, &from);
		if (ctx.State.MoveToZone(request.Target, request.Tag) == true)
		{
			HGameplayEvent moved(PName(HGameplayBuiltin::EventZoneChanged), request.Subject, request.Target);
			moved.Tag  = request.Tag;
			moved.Tag2 = from;
			ContextEmit(ctx, moved);
		}
		return true;
	}

	if (request.Kind == PName(HGameplayBuiltin::EffectSetBoardPosition))
	{
		HGameplayCoord from;
		const HGameplayCoord* current = ctx.State.Board.FindPosition(request.Target);
		if (current != nullptr)
		{
			from = *current;
		}
		if (ctx.State.SetBoardPosition(request.Target, request.Coord) == true)
		{
			HGameplayEvent moved(PName(HGameplayBuiltin::EventBoardMoved), request.Subject, request.Target);
			moved.From = from;
			moved.To   = request.Coord;
			ContextEmit(ctx, moved);
		}
		return true;
	}

	if (request.Kind == PName(HGameplayBuiltin::EffectEndTurn))
	{
		_phaseMachine.EndTurn(ctx);
		return true;
	}

	return false;
}

void PGameplayRuleEngine::suspendForChoice(HGameplayState& state, const HGameplayEffectRequest& request, uint32 causeSequence)
{
	// 선택을 요청한 효과가 재진입 대상이다. ResolveChoice 가 선택을 채워 같은 효과를 다시 부른다.
	state.Choice = _requestedChoice;
	state.Choice.bPending = true;
	state.Choice.Waiting  = request;
	state.Choice.Waiting.bHasChoice = false;
	state.Choice.Waiting.Choice.clear();
	_bChoiceRequested = false;

	HGameplayContext ctx(state, *this, causeSequence, request.Depth);
	HGameplayEvent requested(PName(HGameplayBuiltin::EventChoiceRequested), state.Choice.Chooser, HGameplayEntityId::None());
	requested.Tag    = state.Choice.Kind;
	requested.Amount = (int32)state.Choice.Candidates.size();
	ContextEmit(ctx, requested);

	// 그 효과와 ChoiceRequested 가 발동한 반응까지 큐에 넣은 뒤 남은 큐를 상태에 보존하고 중단한다.
	// 예약된 페이즈 전이 단계는 HGameplayTurnState 에 있어 함께 보존된다.
	_dispatcher.React(state, *this);
	state.Choice.SavedQueue.clear();
	_queue.ToList(state.Choice.SavedQueue);
	_queue.Clear();
}

void PGameplayRuleEngine::halt(HGameplayState& state, uint32 causeSequence)
{
	// 효과 총량 한도(무한 연쇄). 남은 효과와 반응을 버리고 이 명령에서는 더 받지 않는다. 페이즈 전이는 마저 진행한다.
	_bHalted = true;
	_queue.Clear();
	_dispatcher.Clear();

	HGameplayContext ctx(state, *this, causeSequence, 0);
	HGameplayEvent exceeded{ PName(HGameplayBuiltin::EventEffectDepthExceeded) };
	exceeded.Amount = _resolvedCount;
	ContextEmit(ctx, exceeded);

	JG_LOG(GameMaster, ELogLevel::Warning, "PGameplayRuleEngine: more than %d effects in one command, remaining effects dropped", HGameplayLimits::MaxEffectsPerCommand);
}

void PGameplayRuleEngine::enqueue(const HGameplayEffectRequest& request, uint32 causeSequence, int32 depth, bool bFront)
{
	if (_bHalted == true)
	{
		return;
	}

	HGameplayEffectRequest copy = request;
	copy.CauseSequence = causeSequence;
	copy.Depth         = depth;

	if (bFront == true)
	{
		_queue.PushFront(copy);
	}
	else
	{
		_queue.PushBack(copy);
	}
}

bool PGameplayRuleEngine::isBuiltinCommand(const PName& kind) const
{
	return kind == PName(HGameplayBuiltin::CommandEndTurn) || kind == PName(HGameplayBuiltin::CommandResolveChoice);
}

void PGameplayRuleEngine::beginExecution(HList<HGameplayEvent>& outEvents)
{
	_outEvents        = &outEvents;
	_bResolvingEffect = false;
	_bChoiceRequested = false;
	_resolvedCount    = 0;
	_bHalted          = false;
	_queue.Clear();
	_dispatcher.Clear();
}

void PGameplayRuleEngine::endExecution()
{
	_outEvents        = nullptr;
	_bResolvingEffect = false;
	_bChoiceRequested = false;
	_resolvedCount    = 0;
	_bHalted          = false;
	_queue.Clear();
	_dispatcher.Clear();
}
