#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayPhaseMachine.h"

void PGameplayPhaseMachine::Start(HGameplayContext& ctx, const IGameplayOrderPolicy* policy)
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

	beginRound(ctx, policy);
	if (nextTurn(ctx) == false)
	{
		Finish(ctx, 0);
	}
}

void PGameplayPhaseMachine::EndTurn(HGameplayContext& ctx, const IGameplayOrderPolicy* policy)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	if (turn.Phase != EGameplayPhase::TurnMain)
	{
		return;
	}

	setPhase(ctx, EGameplayPhase::TurnEnd);

	HGameplayEvent ended(PName(HGameplayBuiltin::EventTurnEnded), turn.CurrentActor, HGameplayEntityId::None());
	ended.Amount = turn.Round;
	ctx.Emit(ended);

	if (turn.Phase == EGameplayPhase::Finished)
	{
		return;
	}

	if (nextTurn(ctx) == true)
	{
		return;
	}

	endRound(ctx);
	if (turn.Phase == EGameplayPhase::Finished)
	{
		return;
	}

	beginRound(ctx, policy);
	if (nextTurn(ctx) == false)
	{
		Finish(ctx, 0);
	}
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
	setPhase(ctx, EGameplayPhase::Finished);

	HGameplayEvent finished{ PName(HGameplayBuiltin::EventGameFinished) };
	finished.Amount = resultCode;
	ctx.Emit(finished);
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

void PGameplayPhaseMachine::beginRound(HGameplayContext& ctx, const IGameplayOrderPolicy* policy)
{
	HGameplayTurnState& turn = ctx.State.Turn;

	++turn.Round;
	turn.CurrentActor = HGameplayEntityId::None();
	turn.OrderIndex   = INDEX_NONE;

	setPhase(ctx, EGameplayPhase::RoundStart);

	HGameplayEvent started{ PName(HGameplayBuiltin::EventRoundStarted) };
	started.Amount = turn.Round;
	ctx.Emit(started);

	setPhase(ctx, EGameplayPhase::OrderResolve);
	buildOrder(ctx, policy);
}

bool PGameplayPhaseMachine::nextTurn(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	if (turn.Phase == EGameplayPhase::Finished)
	{
		return false;
	}

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
		return false;
	}

	turn.OrderIndex   = index;
	turn.CurrentActor = turn.Order[index];
	++turn.TurnCount;

	setPhase(ctx, EGameplayPhase::TurnStart);

	HGameplayEvent started(PName(HGameplayBuiltin::EventTurnStarted), turn.CurrentActor, HGameplayEntityId::None());
	started.Amount = turn.Round;
	ctx.Emit(started);

	if (turn.Phase == EGameplayPhase::Finished)
	{
		return false;
	}

	setPhase(ctx, EGameplayPhase::TurnMain);
	return true;
}

void PGameplayPhaseMachine::endRound(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;

	turn.CurrentActor = HGameplayEntityId::None();
	setPhase(ctx, EGameplayPhase::RoundEnd);

	HGameplayEvent ended{ PName(HGameplayBuiltin::EventRoundEnded) };
	ended.Amount = turn.Round;
	ctx.Emit(ended);
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
