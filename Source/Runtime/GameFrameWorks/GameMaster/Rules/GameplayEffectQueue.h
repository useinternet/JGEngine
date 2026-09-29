#pragma once
#include "GameMaster/Messages/GameplayEffectRequest.h"

// 효과 큐. FIFO 가 기본이고 끼어들기는 PushFront. 선택 대기 시 내용을 상태(HGameplayChoice::SavedQueue)에 옮겨 보존한다.
class GAMEFRAMEWORKS_API PGameplayEffectQueue
{
	HDeque<HGameplayEffectRequest> _requests;

public:
	void PushBack(const HGameplayEffectRequest& request);
	void PushFront(const HGameplayEffectRequest& request);
	HGameplayEffectRequest PopFront();

	bool  IsEmpty() const;
	int32 Count() const;
	void  Clear();

	void ToList(HList<HGameplayEffectRequest>& outRequests) const;
	void FromList(const HList<HGameplayEffectRequest>& requests);
};
