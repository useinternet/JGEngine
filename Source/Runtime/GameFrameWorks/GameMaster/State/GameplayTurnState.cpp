#include "PCH/PCH.h"
#include "GameMaster/State/GameplayTurnState.h"

bool HGameplayTurnState::IsStarted() const
{
	return Phase != EGameplayPhase::NotStarted;
}

bool HGameplayTurnState::IsFinished() const
{
	return Phase == EGameplayPhase::Finished;
}

bool HGameplayTurnState::IsActorTurn(const HGameplayEntityId& id) const
{
	return CurrentActor.IsValid() == true && CurrentActor == id;
}

bool HGameplayTurnState::CanAct() const
{
	return Phase == EGameplayPhase::TurnMain && CurrentActor.IsValid() == true;
}

void HGameplayTurnState::WriteJson(PJsonData& json) const
{
	json.AddMember("Round", Round);
	json.AddMember("Phase", (int32)Phase);
	json.AddMember("Order", Order);
	json.AddMember("OrderIndex", OrderIndex);
	json.AddMember("CurrentActor", CurrentActor);
	json.AddMember("TurnCount", TurnCount);
}

void HGameplayTurnState::ReadJson(const PJsonData& json)
{
	Order.clear();

	int32 phase = (int32)EGameplayPhase::NotStarted;
	json.GetData("Round", &Round);
	json.GetData("Phase", &phase);
	json.GetData("Order", &Order);
	json.GetData("OrderIndex", &OrderIndex);
	json.GetData("CurrentActor", &CurrentActor);
	json.GetData("TurnCount", &TurnCount);

	Phase = (EGameplayPhase)phase;
}
