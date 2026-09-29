#include "PCH/PCH.h"
#include "GameMaster/Rules/GameplayEffectQueue.h"

void PGameplayEffectQueue::PushBack(const HGameplayEffectRequest& request)
{
	_requests.push_back(request);
}

void PGameplayEffectQueue::PushFront(const HGameplayEffectRequest& request)
{
	_requests.push_front(request);
}

HGameplayEffectRequest PGameplayEffectQueue::PopFront()
{
	if (_requests.empty() == true)
	{
		return HGameplayEffectRequest();
	}
	HGameplayEffectRequest request = _requests.front();
	_requests.pop_front();
	return request;
}

bool PGameplayEffectQueue::IsEmpty() const
{
	return _requests.empty();
}

int32 PGameplayEffectQueue::Count() const
{
	return (int32)_requests.size();
}

void PGameplayEffectQueue::Clear()
{
	_requests.clear();
}

void PGameplayEffectQueue::ToList(HList<HGameplayEffectRequest>& outRequests) const
{
	for (const HGameplayEffectRequest& request : _requests)
	{
		outRequests.push_back(request);
	}
}

void PGameplayEffectQueue::FromList(const HList<HGameplayEffectRequest>& requests)
{
	_requests.clear();
	for (const HGameplayEffectRequest& request : requests)
	{
		_requests.push_back(request);
	}
}
