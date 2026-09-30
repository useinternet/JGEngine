#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayPhaseMachine.h"

void PGameplayPhaseMachine::Start(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	if (turn.Phase != EGameplayPhase::NotStarted)
	{
		return;
	}

	turn.Round     = 0;
	turn.TurnCount = 0;

	HGameplayEvent started{ PName(HGameplayBuiltin::EventGameStarted) };
	ctx.Emit(started);

	turn.PendingStep = EGameplayPhaseStep::BeginRound;
}

void PGameplayPhaseMachine::EndTurn(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	if (turn.Phase != EGameplayPhase::TurnMain && turn.Phase != EGameplayPhase::TurnStart)
	{
		return;
	}
	if (turn.CurrentActor.IsValid() == false)
	{
		return;
	}

	setPhase(ctx, EGameplayPhase::TurnEnd);

	HGameplayEvent ended(PName(HGameplayBuiltin::EventTurnEnded), turn.CurrentActor, HGameplayEntityId::None());
	ended.Amount = turn.Round;
	ctx.Emit(ended);

	// TurnStart 에서 왔으면 예약돼 있던 EnterTurnMain 을 덮는다.
	turn.PendingStep = EGameplayPhaseStep::NextTurn;
}

void PGameplayPhaseMachine::Finish(HGameplayContext& ctx, int32 resultCode)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	if (turn.Phase == EGameplayPhase::Finished)
	{
		return;
	}

	turn.CurrentActor = HGameplayEntityId::None();
	turn.OrderIndex   = INDEX_NONE;
	turn.PendingStep  = EGameplayPhaseStep::None;
	setPhase(ctx, EGameplayPhase::Finished);

	HGameplayEvent finished{ PName(HGameplayBuiltin::EventGameFinished) };
	finished.Amount = resultCode;
	ctx.Emit(finished);
}

bool PGameplayPhaseMachine::RunPendingStep(HGameplayContext& ctx, const IGameplayOrderPolicy* policy)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	EGameplayPhaseStep step = turn.PendingStep;
	turn.PendingStep = EGameplayPhaseStep::None;

	if (step == EGameplayPhaseStep::None || turn.Phase == EGameplayPhase::Finished)
	{
		return false;
	}

	switch (step)
	{
	case EGameplayPhaseStep::BeginRound:
		beginRound(ctx);
		break;

	case EGameplayPhaseStep::ResolveOrder:
		setPhase(ctx, EGameplayPhase::OrderResolve);
		turn.PendingStep = EGameplayPhaseStep::BuildOrder;
		break;

	case EGameplayPhaseStep::BuildOrder:
		buildOrder(ctx, policy);
		nextTurn(ctx);
		break;

	case EGameplayPhaseStep::NextTurn:
		nextTurn(ctx);
		break;

	case EGameplayPhaseStep::EnterTurnMain:
		setPhase(ctx, EGameplayPhase::TurnMain);
		break;

	default:
		break;
	}
	return true;
}

void PGameplayPhaseMachine::setPhase(HGameplayContext& ctx, EGameplayPhase phase)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	if (turn.Phase == phase)
	{
		return;
	}

	EGameplayPhase before = turn.Phase;
	turn.Phase = phase;

	HGameplayEvent changed(PName(HGameplayBuiltin::EventPhaseChanged), turn.CurrentActor, HGameplayEntityId::None());
	changed.Before = (int32)before;
	changed.After  = (int32)phase;
	changed.Amount = turn.Round;
	ctx.Emit(changed);
}

void PGameplayPhaseMachine::beginRound(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;

	++turn.Round;
	turn.CurrentActor = HGameplayEntityId::None();
	turn.OrderIndex   = INDEX_NONE;

	setPhase(ctx, EGameplayPhase::RoundStart);

	HGameplayEvent started{ PName(HGameplayBuiltin::EventRoundStarted) };
	started.Amount = turn.Round;
	ctx.Emit(started);

	turn.PendingStep = EGameplayPhaseStep::ResolveOrder;
}

void PGameplayPhaseMachine::nextTurn(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;

	int32 count = (int32)turn.Order.size();
	int32 index = turn.OrderIndex + 1;
	while (index < count)
	{
		// 라운드 도중 죽은 행동자는 건너뛴다.
		if (ctx.State.IsAlive(turn.Order[index]) == true)
		{
			break;
		}
		++index;
	}

	if (index >= count)
	{
		// 새로 만든 순서에 행동자가 없으면 게임을 끝낸다. 이번 라운드에 누군가 행동했으면 라운드를 닫는다.
		if (turn.OrderIndex == INDEX_NONE)
		{
			Finish(ctx, 0);
			return;
		}
		endRound(ctx);
		return;
	}

	turn.OrderIndex   = index;
	turn.CurrentActor = turn.Order[index];
	++turn.TurnCount;

	setPhase(ctx, EGameplayPhase::TurnStart);

	HGameplayEvent started(PName(HGameplayBuiltin::EventTurnStarted), turn.CurrentActor, HGameplayEntityId::None());
	started.Amount = turn.Round;
	ctx.Emit(started);

	turn.PendingStep = EGameplayPhaseStep::EnterTurnMain;
}

void PGameplayPhaseMachine::endRound(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;

	turn.CurrentActor = HGameplayEntityId::None();
	setPhase(ctx, EGameplayPhase::RoundEnd);

	HGameplayEvent ended{ PName(HGameplayBuiltin::EventRoundEnded) };
	ended.Amount = turn.Round;
	ctx.Emit(ended);

	turn.PendingStep = EGameplayPhaseStep::BeginRound;
}

void PGameplayPhaseMachine::buildOrder(HGameplayContext& ctx, const IGameplayOrderPolicy* policy)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	turn.Order.clear();

	HList<HGameplayEntityId> order;
	if (policy != nullptr)
	{
		policy->BuildOrder(ctx.State, order);
	}
	else
	{
		ctx.State.Entities.CollectAlive(order);
	}

	for (const HGameplayEntityId& id : order)
	{
		if (ctx.State.IsAlive(id) == true)
		{
			turn.Order.push_back(id);
		}
	}
}
