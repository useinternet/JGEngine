#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayContext.h"
#include "GameMaster/Rules/GameplayRuleEngine.h"

HGameplayContext::HGameplayContext(HGameplayState& inState, PGameplayRuleEngine& inEngine, uint32 inCauseSequence, int32 inDepth)
	: State(inState)
	, Engine(inEngine)
	, CauseSequence(inCauseSequence)
	, Depth(inDepth)
{
}

void HGameplayContext::Enqueue(const HGameplayEffectRequest& request)
{
	Engine.ContextEnqueue(*this, request, /*bFront*/ false);
}

void HGameplayContext::EnqueueFront(const HGameplayEffectRequest& request)
{
	Engine.ContextEnqueue(*this, request, /*bFront*/ true);
}

void HGameplayContext::Emit(const HGameplayEvent& event)
{
	Engine.ContextEmit(*this, event);
}

HGameplayRandomStream& HGameplayContext::Rng(EGameplayRandomStream stream) const
{
	return State.Rng(stream);
}

int32 HGameplayContext::Compute(const PName& valueKind, const HGameplayEntityId& subject, const HGameplayEntityId& target, int32 base) const
{
	return Engine.ContextCompute(*this, valueKind, subject, target, base);
}

bool HGameplayContext::RequestChoice(const HGameplayChoice& choice)
{
	return Engine.ContextRequestChoice(*this, choice);
}

HGameplayEntityId HGameplayContext::SpawnEntity(const PName& zoneName)
{
	return Engine.ContextSpawnEntity(*this, zoneName);
}

bool HGameplayContext::DestroyEntity(const HGameplayEntityId& id)
{
	return Engine.ContextDestroyEntity(*this, id);
}

void HGameplayContext::FinishGame(int32 resultCode)
{
	Engine.ContextFinishGame(*this, resultCode);
}

void HGameplayContext::SetStep(const PName& step)
{
	Engine.ContextSetStep(*this, step);
}

void HGameplayContext::SetActors(const HList<HGameplayEntityId>& actors)
{
	Engine.ContextSetActors(*this, actors);
}

const IGameplayBoard* HGameplayContext::Board() const
{
	return Engine.Board.GetRawPointer();
}

HGameplayTriggerContext::HGameplayTriggerContext(const HGameplayState& inState, PGameplayRuleEngine& inEngine, uint32 inCauseSequence, int32 inDepth)
	: _engine(inEngine)
	, State(inState)
	, CauseSequence(inCauseSequence)
	, Depth(inDepth)
{
}

void HGameplayTriggerContext::Enqueue(const HGameplayEffectRequest& request)
{
	_engine.TriggerEnqueue(*this, request, /*bFront*/ false);
}

void HGameplayTriggerContext::EnqueueFront(const HGameplayEffectRequest& request)
{
	_engine.TriggerEnqueue(*this, request, /*bFront*/ true);
}

const IGameplayBoard* HGameplayTriggerContext::Board() const
{
	return _engine.Board.GetRawPointer();
}
