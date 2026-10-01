#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayRoundTurnFlow.h"
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

PName PGameplayRoundTurnFlow::GetName() const
{
	return PName("RoundTurn");
}

void PGameplayRoundTurnFlow::Start(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	turn.Round     = 0;
	turn.TurnCount = 0;
	turn.PendingStep = EGameplayPhaseStep::BeginRound;
}

bool PGameplayRoundTurnFlow::CanEndTurn(const HGameplayState& state, const HGameplayEntityId& actor, PString* outReason) const
{
	if (state.Turn.CanAct() == false)
	{
		setReason(outReason, "cannot end turn now");
		return false;
	}
	if (state.Turn.CurrentActor != actor)
	{
		setReason(outReason, "not your turn");
		return false;
	}
	return true;
}

void PGameplayRoundTurnFlow::EndTurn(HGameplayContext& ctx, const HGameplayEntityId& actor)
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

bool PGameplayRoundTurnFlow::RunPendingStep(HGameplayContext& ctx)
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
		buildOrder(ctx);
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

void PGameplayRoundTurnFlow::setPhase(HGameplayContext& ctx, EGameplayPhase phase)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	if (turn.Phase == phase)
	{
		return;
	}

	EGameplayPhase before = turn.Phase;
	turn.Phase = phase;

	// 공용 칸. 이벤트를 내기 전에 채워 PhaseChanged 를 매칭하는 트리거가 같은 상태를 본다.
	turn.Step = PName(GetGameplayPhaseName(phase));
	turn.Actors.clear();
	if (phase == EGameplayPhase::TurnMain && turn.CurrentActor.IsValid() == true)
	{
		turn.Actors.push_back(turn.CurrentActor);
	}

	HGameplayEvent changed(PName(HGameplayBuiltin::EventPhaseChanged), turn.CurrentActor, HGameplayEntityId::None());
	changed.Before = (int32)before;
	changed.After  = (int32)phase;
	changed.Amount = turn.Round;
	ctx.Emit(changed);
}

void PGameplayRoundTurnFlow::beginRound(HGameplayContext& ctx)
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

void PGameplayRoundTurnFlow::nextTurn(HGameplayContext& ctx)
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
			ctx.FinishGame(0);
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

void PGameplayRoundTurnFlow::endRound(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;

	turn.CurrentActor = HGameplayEntityId::None();
	setPhase(ctx, EGameplayPhase::RoundEnd);

	HGameplayEvent ended{ PName(HGameplayBuiltin::EventRoundEnded) };
	ended.Amount = turn.Round;
	ctx.Emit(ended);

	turn.PendingStep = EGameplayPhaseStep::BeginRound;
}

void PGameplayRoundTurnFlow::buildOrder(HGameplayContext& ctx)
{
	HGameplayTurnState& turn = ctx.State.Turn;
	turn.Order.clear();

	HList<HGameplayEntityId> order;
	const IGameplayOrderPolicy* policy = ctx.Engine.OrderPolicy.GetRawPointer();
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
