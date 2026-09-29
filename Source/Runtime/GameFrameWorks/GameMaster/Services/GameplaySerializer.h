#pragma once
#include "GameMaster/State/GameplayState.h"
#include "GameMaster/Services/GameplayCommandLog.h"
#include "GameMaster/Services/GameplayObserver.h"

// 저장 파일 = 초기 상태 + 현재 상태 + 명령 로그. 초기 상태가 있어야 리플레이가 된다.
class GAMEFRAMEWORKS_API PGameplaySerializer
{
public:
	static bool ToJsonText(const HGameplayState& initial, const HGameplayState& current, const PGameplayCommandLog& log, PString* outText);
	static bool FromJsonText(const PString& text, HGameplayState& outInitial, HGameplayState& outCurrent, PGameplayCommandLog& outLog, const IGameplayMigrator* migrator);

	static bool SaveToFile(const PString& path, const HGameplayState& initial, const HGameplayState& current, const PGameplayCommandLog& log);
	static bool LoadFromFile(const PString& path, HGameplayState& outInitial, HGameplayState& outCurrent, PGameplayCommandLog& outLog, const IGameplayMigrator* migrator);
};
