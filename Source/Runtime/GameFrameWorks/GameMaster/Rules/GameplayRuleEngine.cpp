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
	_phaseMachine.Start(ctx, OrderPolicy.GetRawPointer());

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
		_phaseMachine.EndTurn(ctx, OrderPolicy.GetRawPointer());
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
	HGameplayEffectRequest copy = request;
	copy.CauseSequence = ctx.CauseSequence;
	// 핸들러(깊이 0)가 넣은 것은 깊이 0, 효과 · 트리거가 넣은 것은 한 단계 깊어진다.
	copy.Depth = (ctx.Depth == 0 && _resolvedCount == 0) ? 0 : ctx.Depth + 1;

	if (bFront == true)
	{
		_queue.PushFront(copy);
	}
	else
	{
		_queue.PushBack(copy);
	}
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

	_dispatcher.Dispatch(ctx.State, *this, Triggers, copy);
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

void PGameplayRuleEngine::ContextRequestChoice(const HGameplayContext& ctx, const HGameplayChoice& choice)
{
	_bChoiceRequested = true;
	_requestedChoice  = choice;
	_requestedChoice.bPending = true;
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
	while (_queue.IsEmpty() == false)
	{
		if (_resolvedCount >= HGameplayLimits::MaxEffectsPerCommand)
		{
			HGameplayContext ctx(state, *this, causeSequence, 0);
			HGameplayEvent exceeded{ PName(HGameplayBuiltin::EventEffectDepthExceeded) };
			exceeded.Amount = _resolvedCount;
			ContextEmit(ctx, exceeded);
			_queue.Clear();
			break;
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

		_bChoiceRequested = false;
		resolveOne(state, request, causeSequence);

		if (_bChoiceRequested == true)
		{
			// 남은 큐를 상태에 보존하고 중단. ResolveChoice 명령이 같은 효과를 재진입시킨다.
			state.Choice = _requestedChoice;
			state.Choice.bPending = true;
			state.Choice.Waiting  = request;
			state.Choice.Waiting.bHasChoice = false;
			state.Choice.Waiting.Choice.clear();
			state.Choice.SavedQueue.clear();
			_queue.ToList(state.Choice.SavedQueue);
			_queue.Clear();

			HGameplayContext ctx(state, *this, causeSequence, request.Depth);
			HGameplayEvent requested(PName(HGameplayBuiltin::EventChoiceRequested), state.Choice.Chooser, HGameplayEntityId::None());
			requested.Tag    = state.Choice.Kind;
			requested.Amount = (int32)state.Choice.Candidates.size();
			ContextEmit(ctx, requested);

			_bChoiceRequested = false;
			return EGameplaySubmitResult::PendingChoice;
		}
	}

	return EGameplaySubmitResult::Executed;
}

void PGameplayRuleEngine::resolveOne(HGameplayState& state, const HGameplayEffectRequest& request, uint32 causeSequence)
{
	HGameplayContext ctx(state, *this, causeSequence, request.Depth);

	if (resolveBuiltin(ctx, request) == true)
	{
		return;
	}

	PSharedPtr<JGGameplayEffect> effect = Effects.Find(request.Kind);
	if (effect == nullptr)
	{
		HGameplayEvent unknown(PName(HGameplayBuiltin::EventEffectUnknown), request.Subject, request.Target);
		unknown.Tag = request.Kind;
		ContextEmit(ctx, unknown);
		JG_LOG(GameMaster, ELogLevel::Warning, "PGameplayRuleEngine: unknown effect %s", request.Kind.ToString());
		return;
	}

	effect->Resolve(ctx, request);
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

	if (request.Kind == PName(HGameplayBuiltin::EffectMoveToZone))
	{
		PName from;
		ctx.State.Zones.FindZoneOf(request.Target, &from);
		if (ctx.State.Zones.MoveTo(request.Target, request.Tag) == true)
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
		if (ctx.State.Board.SetPosition(request.Target, request.Coord) == true)
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
		_phaseMachine.EndTurn(ctx, OrderPolicy.GetRawPointer());
		return true;
	}

	return false;
}

bool PGameplayRuleEngine::isBuiltinCommand(const PName& kind) const
{
	return kind == PName(HGameplayBuiltin::CommandEndTurn) || kind == PName(HGameplayBuiltin::CommandResolveChoice);
}

void PGameplayRuleEngine::beginExecution(HList<HGameplayEvent>& outEvents)
{
	_outEvents        = &outEvents;
	_bChoiceRequested = false;
	_resolvedCount    = 0;
	_queue.Clear();
}

void PGameplayRuleEngine::endExecution()
{
	_outEvents        = nullptr;
	_bChoiceRequested = false;
	_resolvedCount    = 0;
	_queue.Clear();
}
