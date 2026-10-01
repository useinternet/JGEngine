#include "PCH/PCH.h"

#include "Core.h"
#include "Misc/Module.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

// 모듈이 창 없이 연결·해제되는지 확인한다 (GameModule R8).
//   JGConsole.exe module.test GameFrameWorks    엔진 Bin
//   JGConsole.exe module.test MyGame            게임 프로젝트 Bin(<Project>/Bin/<구성>). 게임 모듈의 StartupModule(월드 로드)까지 돈다
// 이 명령이 연결한 모듈은 끝날 때 역순으로 해제해 ShutdownModule 도 확인한다.
// 연결·해제하는 동안 [error]/[critical] 로그가 한 줄이라도 나오면 실패다(종료 코드 1).
// 이미 연결된 모듈(JGConsole 이 시작할 때 연결하는 GameFrameWorks)은 연결 상태만 확인한다.
// 모듈 이름은 DLL 이름 그대로 쓴다(대소문자 포함). 에디터 모듈(<Name>Editor)은 GUI 가 없어 실패하는 것이 정상이다.

namespace
{
	// InSerial 이후에 찍힌 로그 중 Error · Critical 줄 수. 최근 줄 버퍼(Info 이상, 최대 1024줄)의 끝에서 serial 이 늘어난 만큼 센다.
	int32 countErrorLogsSince(uint64 InSerial)
	{
		const GLogGlobalSystem& log = GLogGlobalSystem::GetInstance();
		const uint64 newLineCount = log.GetLogSerial() - InSerial;

		HList<HLogLine> lines;
		log.GetRecentLogs(lines);

		const uint64 lineCount = lines.size();
		const uint64 firstNewLine = (newLineCount < lineCount) ? lineCount - newLineCount : 0;

		int32 errorCount = 0;
		for (uint64 i = firstNewLine; i < lineCount; ++i)
		{
			if (lines[i].Level == ELogLevel::Error || lines[i].Level == ELogLevel::Critical)
			{
				++errorCount;
			}
		}

		return errorCount;
	}

	bool executeModuleTest(const HConsoleCommandArgs& InArgs)
	{
		if (InArgs.GetPositionalCount() == 0)
		{
			return false;
		}

		GModuleGlobalSystem& modules = GModuleGlobalSystem::GetInstance();
		HList<PString> connectedModules;
		int32 failureCount = 0;

		for (uint32 i = 0; i < InArgs.GetPositionalCount(); ++i)
		{
			const PString moduleName = InArgs.GetPositional(i);
			if (modules.FindModule(PName(moduleName)) != nullptr)
			{
				JG_LOG(JGConsole, ELogLevel::Info, "module.test: %s already connected", moduleName);
				continue;
			}

			const uint64 serial = GLogGlobalSystem::GetInstance().GetLogSerial();
			const bool bConnected = modules.ConnectModule(moduleName);
			const int32 errorCount = countErrorLogsSince(serial);
			if (bConnected)
			{
				connectedModules.push_back(moduleName);
			}

			if (bConnected == false || errorCount > 0)
			{
				++failureCount;
				JG_LOG(JGConsole, ELogLevel::Error, "module.test: %s connect FAILED (connected %s, error logs %d)", moduleName, bConnected ? "yes" : "no", errorCount);
			}
			else
			{
				JG_LOG(JGConsole, ELogLevel::Info, "module.test: %s connected (error logs 0)", moduleName);
			}
		}

		// 연결한 역순으로 해제한다. DLL 은 GCoreSystem::Destroy 까지 남으므로 해제 뒤에도 그 모듈의 코드는 유효하다.
		for (HList<PString>::reverse_iterator iter = connectedModules.rbegin(); iter != connectedModules.rend(); ++iter)
		{
			const PString& moduleName = *iter;

			const uint64 serial = GLogGlobalSystem::GetInstance().GetLogSerial();
			const bool bDisconnected = modules.DisconnectModule(moduleName);
			const int32 errorCount = countErrorLogsSince(serial);
			if (bDisconnected == false || errorCount > 0)
			{
				++failureCount;
				JG_LOG(JGConsole, ELogLevel::Error, "module.test: %s disconnect FAILED (error logs %d)", moduleName, errorCount);
			}
			else
			{
				JG_LOG(JGConsole, ELogLevel::Info, "module.test: %s disconnected (error logs 0)", moduleName);
			}
		}

		if (failureCount == 0)
		{
			JG_LOG(JGConsole, ELogLevel::Info, "module.test: OK (%d module(s))", (int32)InArgs.GetPositionalCount());
		}
		else
		{
			JG_LOG(JGConsole, ELogLevel::Error, "module.test: FAILED (%d failure(s))", failureCount);
		}

		return failureCount == 0;
	}

	HAutoConsoleCommand ModuleTestCommand(
		"module.test",
		"module.test <Module> [<Module>...]",
		"Connect modules headless, then disconnect them; fails on any error log",
		&executeModuleTest);
}
