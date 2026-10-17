#include "PCH/PCH.h"

#include "Core.h"
#include "Misc/Module.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "FileIO/ProjectDescriptor.h"

// JGConsole.exe [-game] <명령> [인자...] — 명령 하나를 실행하고 끝나는 헤드리스 실행기.
// 명령은 HAutoConsoleCommand 전역 변수로 선언한다. 모듈의 테스트 명령은 그 모듈에(예: GameFrameWorks 의 gmtest),
// JGConsole 전용 명령은 이 프로그램에 명령마다 .cpp 하나로 둔다. 인자 없이 실행하면 help 를 출력한다.
// -game: 게임 프로젝트 Bin(<Project>/Bin/<구성>)에서 .jgproject 의 GameModules 를 연결한 뒤 실행한다. 게임 모듈이 선언한 명령은
//        이 옵션이 있어야 보인다(예: JGConsole.exe -game ah.selftest). 에디터 모듈은 연결하지 않는다. 옵션은 명령 이름 앞에만 쓴다.
// 종료 코드: 성공 0, 실패·알 수 없는 명령 1. -game 인데 게임 모듈을 연결하지 못하면 명령을 돌리지 않고 1.

namespace
{
	// 시작할 때 연결하는 엔진 모듈. 연결돼야 그 모듈이 선언한 명령(GameFrameWorks 의 gmtest · gameui.selftest, Audio 의 audio.selftest 등)이 등록된다.
	// 종료 때는 GModuleGlobalSystem::Destroy 가 역순으로 내린다. Audio 는 창이 없으면 실제 장치를 열지 않고 무음 장치로 돈다(오디오 재생, D-37).
	const char* const CONSOLE_ENGINE_MODULES[] = { "GameFrameWorks", "Audio" };

	// 명령 앞에 붙이는 JGConsole 옵션: 게임 모듈 연결(ProjectAH ER-005, 사용자 결정 D-30). 명령 뒤에 오면 그 명령의 인자로 넘어간다.
	const char* const GAME_MODULES_OPTION = "-game";

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

	// argv 맨 앞의 JGConsole 옵션을 읽고, 명령이 시작하는 argv 위치를 돌려준다.
	int consumeConsoleOptions(int argc, char** argv, bool* outConnectGameModules)
	{
		int index = 1;
		while (index < argc && _stricmp(argv[index], GAME_MODULES_OPTION) == 0)
		{
			*outConnectGameModules = true;
			++index;
		}

		return index;
	}

	// 프로젝트의 게임 모듈(.jgproject GameModules)을 적힌 순서대로 연결한다. GameFrameWorks 다음이라 게임 모듈 안의
	// ConnectModule("GameFrameWorks") 는 true 만 받는다. 에디터 모듈(EditorModules)은 GUI 가 없어 연결하지 않는다(module.test 와 같은 이유).
	// 해제는 엔진 모듈과 함께 GModuleGlobalSystem::Destroy 가 역순으로 한다. 하나라도 못 올리면 false — 명령을 돌리지 않는다.
	bool connectGameModules()
	{
		if (HFileHelper::IsProjectMode() == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "-game needs a game project: run the JGConsole.exe in <Project>/Bin/<Configuration>");
			return false;
		}

		HProjectDescriptor descriptor;
		if (HProjectDescriptor::Load(HFileHelper::ProjectDirectory(), &descriptor) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "-game: fail read the project descriptor in %s", HFileHelper::ProjectDirectory());
			return false;
		}

		if (descriptor.GameModules.empty() == true)
		{
			JG_LOG(JGConsole, ELogLevel::Warning, "-game: project %s has no GameModules", descriptor.Name);
			return true;
		}

		for (const PString& moduleName : descriptor.GameModules)
		{
			if (GModuleGlobalSystem::GetInstance().ConnectModule(moduleName) == false)
			{
				JG_LOG(JGConsole, ELogLevel::Error, "-game: fail connect game module %s", moduleName);
				return false;
			}
		}

		return true;
	}

	// argv 의 명령 토큰(옵션 뒤)을 그대로 실행한다. CRT 가 이미 따옴표를 풀었으므로 줄로 다시 합치지 않는다.
	// 엔진 컨테이너(HList)가 GCoreSystem::Destroy 전에 해제되도록 main 과 나눈다.
	bool runCommandLine(int argc, char** argv)
	{
		GConsoleCommandGlobalSystem& commands = GConsoleCommandGlobalSystem::GetInstance();

		bool bConnectGameModules = false;
		const int firstCommandIndex = consumeConsoleOptions(argc, argv, &bConnectGameModules);
		if (bConnectGameModules == true && connectGameModules() == false)
		{
			return false;
		}

		if (firstCommandIndex >= argc)
		{
			return commands.Execute(PString("help"));
		}

		HList<PString> tokens;
		for (int i = firstCommandIndex; i < argc; ++i)
		{
			tokens.push_back(PString(argv[i]));
		}

		// 언리얼식 별칭: JGConsole.exe -runCook -project=… = cook.run -project=… (DevOps GP-D4, Cook/CookCommand.cpp)
		if (_stricmp(tokens[0].GetCStr(), "-runCook") == 0)
		{
			tokens[0] = PString("cook.run");
		}

		const bool bRegistered = commands.IsRegistered(tokens[0]);
		const bool bResult     = commands.Execute(tokens);

		// 게임 프로젝트 Bin 에서 -game 없이 모르는 명령이면 게임 모듈의 명령일 수 있다 — 쓰는 법을 알린다(결과 · 종료 코드는 그대로).
		if (bRegistered == false && bConnectGameModules == false && HFileHelper::IsProjectMode() == true)
		{
			JG_LOG(JGConsole, ELogLevel::Info, "Commands of game modules need -game: JGConsole.exe -game %s", tokens[0]);
		}

		return bResult;
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
