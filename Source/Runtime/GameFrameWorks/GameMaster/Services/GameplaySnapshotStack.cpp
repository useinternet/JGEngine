#include "PCH/PCH.h"
#include "GameMaster/Services/GameplaySnapshotStack.h"

void PGameplaySnapshotStack::SetLimit(int32 limit)
{
	_limit = limit < 1 ? 1 : limit;
	while ((int32)_states.size() > _limit)
	{
		_states.pop_front();
	}
}

int32 PGameplaySnapshotStack::GetLimit() const
{
	return _limit;
}

void PGameplaySnapshotStack::Push(const HGameplayState& state)
{
	_states.push_back(state);
	while ((int32)_states.size() > _limit)
	{
		_states.pop_front();
	}
}

bool PGameplaySnapshotStack::Pop(HGameplayState& outState)
{
	if (_states.empty() == true)
	{
		return false;
	}
	outState = std::move(_states.back());
	_states.pop_back();
	return true;
}

bool PGameplaySnapshotStack::Peek(HGameplayState& outState) const
{
	if (_states.empty() == true)
	{
		return false;
	}
	outState = _states.back();
	return true;
}

int32 PGameplaySnapshotStack::Count() const
{
	return (int32)_states.size();
}

void PGameplaySnapshotStack::Clear()
{
	_states.clear();
}
