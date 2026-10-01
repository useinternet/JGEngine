#include "PCH/PCH.h"

#include "Core.h"
#include "Misc/Module.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

// JGConsole.exe <명령> [인자...] — 명령 하나를 실행하고 끝나는 헤드리스 실행기.
// 명령은 HAutoConsoleCommand 전역 변수로 선언한다. 모듈의 테스트 명령은 그 모듈에(예: GameFrameWorks 의 gmtest),
// JGConsole 전용 명령은 이 프로그램에 명령마다 .cpp 하나로 둔다. 인자 없이 실행하면 help 를 출력한다.
// 종료 코드: 성공 0, 실패·알 수 없는 명령 1.

namespace
{
	// 시작할 때 연결하는 엔진 모듈. 연결돼야 그 모듈이 선언한 명령(GameFrameWorks 의 gmtest · gameui.selftest 등)이 등록된다.
	// 종료 때는 GModuleGlobalSystem::Destroy 가 역순으로 내린다.
	const char* const CONSOLE_ENGINE_MODULES[] = { "GameFrameWorks" };

	void connectEngineModules()
	{
		for (const char* moduleName : CONSOLE_ENGINE_MODULES)
		{
			if (GModuleGlobalSystem::GetInstance().ConnectModule(moduleName) == false)
			{
				JG_LOG(JGConsole, ELogLevel::Error, "Fail Connect %s Module", moduleName);
			}
		}
	}

	// argv[1..] 을 토큰 그대로 실행한다. CRT 가 이미 따옴표를 풀었으므로 줄로 다시 합치지 않는다.
	// 엔진 컨테이너(HList)가 GCoreSystem::Destroy 전에 해제되도록 main 과 나눈다.
	bool runCommandLine(int argc, char** argv)
	{
		GConsoleCommandGlobalSystem& commands = GConsoleCommandGlobalSystem::GetInstance();
		if (argc <= 1)
		{
			return commands.Execute(PString("help"));
		}

		HList<PString> tokens;
		for (int i = 1; i < argc; ++i)
		{
			tokens.push_back(PString(argv[i]));
		}

		return commands.Execute(tokens);
	}
}

int main(int argc, char** argv)
{
	GCoreSystem::Create();
	connectEngineModules();

	const int exitCode = runCommandLine(argc, argv) ? 0 : 1;

	GCoreSystem::Destroy();
	return exitCode;
}
