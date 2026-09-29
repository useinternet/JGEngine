#pragma once
#include "Core/GameFrameWorksDefines.h"

// 월드 · 게임 인스턴스 · GameMasterActor 바인딩의 헤드리스 검증. JGConsole.exe gmtest 가 GameMaster 자체 테스트 뒤에 실행한다.
// 그래픽 없이 월드를 로드하고 액터를 스폰하고 프레임을 수동으로 틱한다. 실패 수를 돌려준다.
class GAMEFRAMEWORKS_API PWorldSelfTest
{
public:
	static int32 Run();
};
