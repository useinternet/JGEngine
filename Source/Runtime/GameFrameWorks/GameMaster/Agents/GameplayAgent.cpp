#include "PCH/PCH.h"
#include "GameMaster/Agents/GameplayAgent.h"
#include "GameMaster/GameMaster.h"

// Random ----------------------------------------------------------------------

PGameplayRandomAgent::PGameplayRandomAgent(uint64 seed)
{
	_rng.Seed(seed, 0x5eed);
}

bool PGameplayRandomAgent::ChooseCommand(PGameMaster& gameMaster, const HGameplayEntityId& actor, HGameplayCommand& outCommand)
{
	HList<HGameplayCommand> legal;
	gameMaster.EnumerateLegal(actor, legal);
	if (legal.empty() == true)
	{
		return false;
	}

	outCommand = legal[_rng.Below((uint32)legal.size())];
	return true;
}

bool PGameplayRandomAgent::ChooseOption(PGameMaster& gameMaster, const HGameplayChoice& choice, HList<HGameplayEntityId>& outSelection)
{
	outSelection.clear();

	HList<HGameplayEntityId> pool = choice.Candidates;
	int32 count = choice.Min;
	if (count > (int32)pool.size())
	{
		count = (int32)pool.size();
	}

	for (int32 i = 0; i < count; ++i)
	{
		uint32 index = _rng.Below((uint32)pool.size());
		outSelection.push_back(pool[index]);
		pool.erase(pool.begin() + index);
	}
	return outSelection.empty() == false || choice.Min == 0;
}

// Greedy ----------------------------------------------------------------------

PGameplayGreedyAgent::PGameplayGreedyAgent(PSharedPtr<IGameplayEvaluator> evaluator)
	: _evaluator(evaluator)
{
}

void PGameplayGreedyAgent::SetEvaluator(PSharedPtr<IGameplayEvaluator> evaluator)
{
	_evaluator = evaluator;
}

bool PGameplayGreedyAgent::ChooseCommand(PGameMaster& gameMaster, const HGameplayEntityId& actor, HGameplayCommand& outCommand)
{
	HList<HGameplayCommand> legal;
	gameMaster.EnumerateLegal(actor, legal);
	if (legal.empty() == true)
	{
		return false;
	}
	if (_evaluator == nullptr)
	{
		outCommand = legal[0];
		return true;
	}

	int32 bestScore = 0;
	int32 bestIndex = INDEX_NONE;
	int32 count = (int32)legal.size();
	for (int32 i = 0; i < count; ++i)
	{
		HGameplayState after;
		HList<HGameplayEvent> events;
		gameMaster.Simulate(gameMaster.GetState(), legal[i], after, events);

		int32 score = _evaluator->Evaluate(after, actor);
		if (bestIndex == INDEX_NONE || score > bestScore)
		{
			bestScore = score;
			bestIndex = i;
		}
	}

	outCommand = legal[bestIndex];
	return true;
}

bool PGameplayGreedyAgent::ChooseOption(PGameMaster& gameMaster, const HGameplayChoice& choice, HList<HGameplayEntityId>& outSelection)
{
	outSelection.clear();
	if (choice.Candidates.empty() == true)
	{
		return choice.Min == 0;
	}

	// 하나만 고르는 선택은 후보마다 시뮬레이션해 평가한다. 그 밖에는 앞에서부터 Min 개.
	if (choice.Min == 1 && choice.Max == 1 && _evaluator != nullptr)
	{
		int32 bestScore = 0;
		int32 bestIndex = INDEX_NONE;
		int32 count = (int32)choice.Candidates.size();
		for (int32 i = 0; i < count; ++i)
		{
			HGameplayCommand resolve(PName(HGameplayBuiltin::CommandResolveChoice), choice.Chooser);
			resolve.Targets.push_back(choice.Candidates[i]);

			HGameplayState after;
			HList<HGameplayEvent> events;
			gameMaster.Simulate(gameMaster.GetState(), resolve, after, events);

			int32 score = _evaluator->Evaluate(after, choice.Chooser);
			if (bestIndex == INDEX_NONE || score > bestScore)
			{
				bestScore = score;
				bestIndex = i;
			}
		}
		outSelection.push_back(choice.Candidates[bestIndex]);
		return true;
	}

	int32 count = choice.Min;
	if (count > (int32)choice.Candidates.size())
	{
		count = (int32)choice.Candidates.size();
	}
	for (int32 i = 0; i < count; ++i)
	{
		outSelection.push_back(choice.Candidates[i]);
	}
	return true;
}

// Runner ----------------------------------------------------------------------

bool PGameplayAgentRunner::Step(PGameMaster& gameMaster, HList<HGameplayEvent>& outEvents, PString* outReason)
{
	const HGameplayState& state = gameMaster.GetState();
	if (state.Turn.IsStarted() == false || state.Turn.IsFinished() == true)
	{
		return false;
	}

	if (state.Choice.bPending == true)
	{
		PSharedPtr<IGameplayAgent> agent = gameMaster.FindAgentForActor(state.Choice.Chooser);
		if (agent == nullptr)
		{
			return false;
		}

		HList<HGameplayEntityId> selection;
		if (agent->ChooseOption(gameMaster, state.Choice, selection) == false)
		{
			return false;
		}

		HGameplayCommand resolve(PName(HGameplayBuiltin::CommandResolveChoice), state.Choice.Chooser);
		resolve.Targets = selection;
		return gameMaster.Submit(resolve, outEvents, outReason) != EGameplaySubmitResult::Rejected;
	}

	if (state.Turn.CanAct() == false)
	{
		return false;
	}

	PSharedPtr<IGameplayAgent> agent = gameMaster.FindAgentForActor(state.Turn.CurrentActor);
	if (agent == nullptr)
	{
		return false;
	}

	HGameplayCommand command;
	if (agent->ChooseCommand(gameMaster, state.Turn.CurrentActor, command) == false)
	{
		return false;
	}
	return gameMaster.Submit(command, outEvents, outReason) != EGameplaySubmitResult::Rejected;
}

int32 PGameplayAgentRunner::Run(PGameMaster& gameMaster, int32 maxSteps, HList<HGameplayEvent>* outEvents)
{
	int32 steps = 0;
	while (steps < maxSteps)
	{
		HList<HGameplayEvent> events;
		if (Step(gameMaster, events, nullptr) == false)
		{
			break;
		}
		if (outEvents != nullptr)
		{
			outEvents->insert(outEvents->end(), events.begin(), events.end());
		}
		++steps;
	}
	return steps;
}
