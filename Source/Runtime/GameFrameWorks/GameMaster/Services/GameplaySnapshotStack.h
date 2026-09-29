#pragma once
#include "GameMaster/State/GameplayState.h"

// 되돌리기용 상태 스택. 명령마다 실행 전 상태를 push 하고 Undo 가 pop 한다. 상태가 작으므로(KB 단위) 복사로 충분하다.
class GAMEFRAMEWORKS_API PGameplaySnapshotStack
{
	HList<HGameplayState> _states;
	int32                   _limit = 256;

public:
	void  SetLimit(int32 limit);
	int32 GetLimit() const;

	void  Push(const HGameplayState& state);
	bool  Pop(HGameplayState& outState);
	bool  Peek(HGameplayState& outState) const;
	int32 Count() const;
	void  Clear();
};
