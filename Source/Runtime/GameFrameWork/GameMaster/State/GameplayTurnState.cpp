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

bool HGameplayTurnState::IsActor(const HGameplayEntityId& id) const
{
	if (id.IsValid() == false)
	{
		return false;
	}
	for (const HGameplayEntityId& actor : Actors)
	{
		if (actor == id)
		{
			return true;
		}
	}
	return false;
}

void HGameplayTurnState::WriteJson(PJsonData& json) const
{
	json.AddMember("Round", Round);
	json.AddMember("Phase", (int32)Phase);
	json.AddMember("Order", Order);
	json.AddMember("OrderIndex", OrderIndex);
	json.AddMember("CurrentActor", CurrentActor);
	json.AddMember("TurnCount", TurnCount);
	json.AddMember("PendingStep", (int32)PendingStep);
	json.AddMember("Actors", Actors);
	json.AddMember("Step", Step);
	json.AddMember("NextStep", NextStep);
}

void HGameplayTurnState::ReadJson(const PJsonData& json)
{
	Order.clear();
	Actors.clear();
	Step     = PName();
	NextStep = PName();

	int32 phase = (int32)EGameplayPhase::NotStarted;
	int32 step  = (int32)EGameplayPhaseStep::None;
	json.GetData("Round", &Round);
	json.GetData("Phase", &phase);
	json.GetData("Order", &Order);
	json.GetData("OrderIndex", &OrderIndex);
	json.GetData("CurrentActor", &CurrentActor);
	json.GetData("TurnCount", &TurnCount);
	json.GetData("PendingStep", &step);   // 이 필드가 없는 저장(2026-09-29 이전)은 전이 도중일 수 없으므로 None
	// 아래 세 칸이 없는 저장(2026-10-01 이전)은 빈 값으로 읽는다 — 다음 전이 전까지 입력 행동자가 비어 보인다. 그때 저장은 자체 테스트 산출물뿐이다.
	json.GetData("Actors", &Actors);
	json.GetData("Step", &Step);
	json.GetData("NextStep", &NextStep);

	Phase       = (EGameplayPhase)phase;
	PendingStep = (EGameplayPhaseStep)step;
}
