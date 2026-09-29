#include "PCH/PCH.h"

#include <iostream>
#include <string>
#include "Core.h"
#include "Memory/Allocator.h"
#include "Math/Math.h"
#include "Misc/Module.h"
#include "GameMaster/GameMasterSelfTest.h"
#include "Core/WorldSelfTest.h"

using namespace std;

namespace
{
	// JGConsole.exe gmtest  — GameFrameWorks 의 GameMaster 커널 + 월드/게임 인스턴스 자체 검증 (그래픽 없음)
	// 이전 이름 simtest 도 받는다 (리뷰 기록의 검증 절차 호환).
	int runGameMasterSelfTest()
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
		{
			cout << "gmtest: fail to connect GameFrameWorks module" << endl;
			return 2;
		}

		int32 failures = PGameMasterSelfTest::Run();
		failures += PWorldSelfTest::Run();
		cout << "gmtest: " << (failures == 0 ? "OK" : "FAILED") << " (" << failures << " failures)" << endl;
		return failures == 0 ? 0 : 1;
	}
}

int main(int argc, char** argv)
{
	string command;
	if (argc > 1)
	{
		command = argv[1];
	}

	GCoreSystem::Create();

	int exitCode = 0;
	if (command == "gmtest" || command == "simtest")
	{
		exitCode = runGameMasterSelfTest();
	}

	GCoreSystem::Destroy();
	return exitCode;
}
