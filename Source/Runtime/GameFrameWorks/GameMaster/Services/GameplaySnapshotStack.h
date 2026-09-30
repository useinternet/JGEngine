#pragma once
#include "GameMaster/State/GameplayState.h"

// 되돌리기용 상태 스택. 명령마다 실행 전 상태를 push 하고 Undo 가 pop 한다. 상태가 작으므로(KB 단위) 복사로 충분하다.
// 한도를 넘으면 가장 오래된 것을 버린다. 덱이라 버리거나 늘어날 때 다른 상태를 옮기지 않는다 (명령당 상태 복사 한 번).
class GAMEFRAMEWORKS_API PGameplaySnapshotStack
{
	HDeque<HGameplayState> _states;
	int32                    _limit = 256;

public:
	void  SetLimit(int32 limit);
	int32 GetLimit() const;

	void  Push(const HGameplayState& state);
	bool  Pop(HGameplayState& outState);   // 맨 위 상태를 outState 로 옮긴다
	bool  Peek(HGameplayState& outState) const;
	int32 Count() const;
	void  Clear();
};
