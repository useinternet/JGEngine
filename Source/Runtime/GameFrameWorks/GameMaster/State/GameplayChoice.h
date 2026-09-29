#pragma once
#include "GameMaster/State/GameplayEntityId.h"
#include "GameMaster/Messages/GameplayEffectRequest.h"

// 선택 대기. 효과 해결 도중 플레이어 입력이 필요할 때 엔진이 채우고 Submit 을 PendingChoice 로 반환한다.
// 미해결 효과 큐(SavedQueue)를 함께 보존하므로 저장 · 되돌리기 · 리플레이가 대기 중에도 동작한다.
struct GAMEFRAMEWORKS_API HGameplayChoice : public IJsonable
{
	bool                       bPending = false;
	HGameplayEntityId        Chooser;
	PName                      Kind;
	HList<HGameplayEntityId> Candidates;
	int32                      Min = 1;
	int32                      Max = 1;

	HGameplayEffectRequest        Waiting;      // 선택을 요청한 효과. 재진입 시 Choice 가 채워진다
	HList<HGameplayEffectRequest> SavedQueue;   // 요청 시점에 남아 있던 큐

	void Clear();
	bool IsCandidate(const HGameplayEntityId& id) const;
	bool IsValidSelection(const HList<HGameplayEntityId>& selection, PString* outReason) const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
