#include "PCH/PCH.h"
#include "SelfTest/GameFrameWorkSelfTest.h"
#include "SelfTest/GameMasterSelfTest.h"
#include "SelfTest/WorldSelfTest.h"
#include "SelfTest/GameViewportSelfTest.h"
#include "SelfTest/GameplayNetSelfTest.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include <chrono>
#include <iostream>

// GameFrameWork 자체 시험의 명령 등록은 모두 여기 있다(사용자 결정 H-084 · H-094).
//   gfw.selftest [group ...] [net=<sub>] [-list] — 고른 그룹(없으면 모두)을 표 차례로 돌려 그룹마다 한 줄, 끝에 요약 한 줄.
//   실패가 하나라도 있거나 모르는 이름이면 false(JGConsole 종료 1).
//   옛 명령 6개(gmtest · gameui.selftest · datatable.selftest · cookcheck.selftest · net.test · audio.selftest)는 같은 시험을 부르는 별칭이고, 출력 · 인자는 예전 그대로다.
// 여러 시험이 잘못된 입력을 일부러 넣어 Error 로그를 남기므로, 판정은 Error 로그 수가 아니라 실패 수로 한다.
namespace
{
	// inArgs 는 별칭의 인자(묶음에서는 빈 인자), inNetSub 는 네트워크 하위 그룹
	using HSelfTestFunction = HSelfTestResult (*)(const HConsoleCommandArgs& inArgs, const PString& inNetSub);

	struct HSelfTestGroup
	{
		const char*       Name;
		const char*       Description;
		HSelfTestFunction Run;
	};

	const HSelfTestGroup GROUPS[] =
	{
		{ "gamemaster", "GameMaster kernel (determinism, undo, save/load, replay, triggers, board, agents)",
			[](const HConsoleCommandArgs&, const PString&) { return PGameMasterSelfTest::Run(); } },
		{ "world",      "world, game instance and GameMasterActor binding (headless)",
			[](const HConsoleCommandArgs&, const PString&) { return PWorldSelfTest::Run(); } },
		{ "viewport",   "game viewport input routing",
			[](const HConsoleCommandArgs&, const PString&) { return PGameViewportSelfTest::Run(); } },
		{ "gameui",     "game UI (widget stacks, input routing, styles, layout, font, text, scroll boxes). No GPU needed",
			[](const HConsoleCommandArgs& inArgs, const PString&) { return RunGameUISelfTest(inArgs); } },
		{ "datatable",  "data tables (values, JSON, binding, edits, undo, TSV paste, files)",
			[](const HConsoleCommandArgs& inArgs, const PString&) { return RunDataTableSelfTest(inArgs); } },
		{ "cookcheck",  "cook content check (parse, GUID, references, data tables)",
			[](const HConsoleCommandArgs& inArgs, const PString&) { return RunCookCheckSelfTest(inArgs); } },
		{ "net",        "listen server (transport, session, recovery, lobby, rewind, world)",
			[](const HConsoleCommandArgs&, const PString& inNetSub) { return PGameplayNetSelfTest::Run(inNetSub); } },
		{ "audio",      "audio mixing without a device, null device, lazy device open",
			[](const HConsoleCommandArgs& inArgs, const PString&) { return RunAudioSelfTest(inArgs); } },
		{ "asset",      "asset folder listing and import for the content browser (ListAssetFolder, ImportAssetFile)",
			[](const HConsoleCommandArgs& inArgs, const PString&) { return RunAssetFolderSelfTest(inArgs); } },
	};
	constexpr int32 GROUP_COUNT = (int32)(sizeof(GROUPS) / sizeof(GROUPS[0]));

	const char* NET_SUBS[] = { "transport", "session", "recovery", "lobby", "rewind", "world", "all" };

	const HSelfTestGroup* findGroup(const PString& inName)
	{
		for (const HSelfTestGroup& group : GROUPS)
		{
			if (inName == PString(group.Name))
			{
				return &group;
			}
		}
		return nullptr;
	}

	bool isNetSub(const PString& inSub)
	{
		for (const char* sub : NET_SUBS)
		{
			if (inSub == PString(sub))
			{
				return true;
			}
		}
		return false;
	}

	// 사용법 줄은 명령이 false 를 돌려주면 콘솔이 찍는다
	void logGroups()
	{
		JG_LOG(GameFrameWork, ELogLevel::Error, "gfw.selftest groups: gamemaster world viewport gameui datatable cookcheck net audio asset");
	}

	HSelfTestResult runGroup(const HSelfTestGroup& inGroup, const HConsoleCommandArgs& inArgs, const PString& inNetSub, float64& outSeconds)
	{
		const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		const HSelfTestResult result = inGroup.Run(inArgs, inNetSub);
		outSeconds = std::chrono::duration<float64>(std::chrono::steady_clock::now() - start).count();
		return result;
	}

	bool executeAll(const HConsoleCommandArgs& inArgs)
	{
		if (inArgs.Has("list") == true)
		{
			for (const HSelfTestGroup& group : GROUPS)
			{
				JG_LOG(GameFrameWork, ELogLevel::Info, "gfw.selftest: %-10s %s", group.Name, group.Description);
			}
			return true;
		}

		bool    bSelected[GROUP_COUNT] = {};
		bool    bAny   = false;
		PString netSub = "all";
		for (uint32 i = 0; i < inArgs.GetPositionalCount(); ++i)
		{
			const PString token = HConsoleCommandArgs::NormalizeName(inArgs.GetPositional(i));
			PString name = token;
			const uint64 equal = token.Find("=");
			if (equal != PString::NPOS)
			{
				PString sub;
				token.SubString(&name, 0, equal);
				token.SubString(&sub, equal + 1);
				if (name != PString("net") || isNetSub(sub) == false)
				{
					JG_LOG(GameFrameWork, ELogLevel::Error, "gfw.selftest: unknown argument %s", token);
					logGroups();
					return false;
				}
				netSub = sub;
			}

			const HSelfTestGroup* group = findGroup(name);
			if (group == nullptr)
			{
				JG_LOG(GameFrameWork, ELogLevel::Error, "gfw.selftest: unknown group %s", token);
				logGroups();
				return false;
			}
			bSelected[group - GROUPS] = true;
			bAny = true;
		}

		HConsoleCommandArgs defaultArgs;
		HSelfTestResult total;
		int32   groupCount  = 0;
		int32   passedCount = 0;
		float64 totalSeconds = 0.0;
		for (int32 i = 0; i < GROUP_COUNT; ++i)
		{
			if (bAny == true && bSelected[i] == false)
			{
				continue;
			}

			float64 seconds = 0.0;
			const HSelfTestResult result = runGroup(GROUPS[i], defaultArgs, netSub, seconds);
			total.Passed  += result.Passed;
			total.Failed  += result.Failed;
			total.Skipped += result.Skipped;
			totalSeconds  += seconds;
			++groupCount;

			const int32 checks = result.Passed + result.Failed;
			if (result.Failed == 0)
			{
				++passedCount;
				JG_LOG(GameFrameWork, ELogLevel::Info, "gfw.selftest: %-10s PASS %3d/%-3d (%.1f s)%s", GROUPS[i].Name, result.Passed, checks, seconds,
					result.Skipped > 0 ? PString::Format(", skipped %d", result.Skipped) : PString());
			}
			else
			{
				JG_LOG(GameFrameWork, ELogLevel::Error, "gfw.selftest: %-10s FAIL %3d/%-3d (%.1f s), %d failed", GROUPS[i].Name, result.Passed, checks, seconds, result.Failed);
			}
		}

		const bool bOk = total.Failed == 0;
		JG_LOG(GameFrameWork, bOk == true ? ELogLevel::Info : ELogLevel::Error, "gfw.selftest: %s — %d/%d groups, %d checks, %d failed, %d skipped (%.1f s)",
			bOk == true ? "OK" : "FAILED", passedCount, groupCount, total.Passed + total.Failed, total.Failed, total.Skipped, totalSeconds);
		return bOk;
	}

	HAutoConsoleCommand GfwSelfTestCommand(
		"gfw.selftest",
		"gfw.selftest [group ...] [net=<transport|session|recovery|lobby|rewind|world|all>] [-list]",
		"Run the GameFrameWork self tests (all groups when none is given): gamemaster world viewport gameui datatable cookcheck net audio asset",
		&executeAll);

	// --- 옛 명령 이름(별칭). 출력 · 인자 · 판정은 예전 명령과 같다.

	// gmtest — GameMaster 커널 + 월드/게임 인스턴스 + 게임 뷰포트(그래픽 없음).
	// GameFrameWork 가 연결된 프로세스에서 쓸 수 있다: JGConsole(시작할 때 연결), 에디터(엔진 모듈로 늘 연결 — 프로젝트 없이 띄워도, 2026-10-02 D-7).
	// 월드 테스트의 게임 인스턴스 절만 모듈의 게임 인스턴스로 월드를 로드 · 언로드한다. 이미 월드가 있으면(에디터에서 게임이 도는 중) 그 절은 건너뛰고,
	// 나머지는 자기 전용 PGameMaster / PWorld 를 쓰므로 에디터 안에서 돌려도 게임 월드를 건드리지 않는다.
	bool executeGmTest(const HConsoleCommandArgs&)
	{
		int32 failures = PGameMasterSelfTest::Run().Failed;
		failures += PWorldSelfTest::Run().Failed;
		failures += PGameViewportSelfTest::Run().Failed;

		if (failures == 0)
		{
			JG_LOG(GameFrameWork, ELogLevel::Info, "gmtest: OK (0 failures)");
		}
		else
		{
			JG_LOG(GameFrameWork, ELogLevel::Error, "gmtest: FAILED (%d failures)", failures);
		}

		return failures == 0;
	}

	bool executeGameUISelfTest(const HConsoleCommandArgs& inArgs)
	{
		return RunGameUISelfTest(inArgs).Failed == 0;
	}

	bool executeDataTableSelfTest(const HConsoleCommandArgs& inArgs)
	{
		return RunDataTableSelfTest(inArgs).Failed == 0;
	}

	bool executeCookCheckSelfTest(const HConsoleCommandArgs& inArgs)
	{
		return RunCookCheckSelfTest(inArgs).Failed == 0;
	}

	// net.host · net.join 은 JGConsole(NetCommands.cpp)에 있다.
	bool executeNetTest(const HConsoleCommandArgs& inArgs)
	{
		PString which = "all";
		if (inArgs.GetPositionalCount() > 0)
		{
			which = inArgs.GetPositional(0);
		}
		int32 failures = PGameplayNetSelfTest::Run(which).Failed;
		std::cout << "net.test: " << (failures == 0 ? "OK" : "FAILED") << " (" << failures << " failures)" << std::endl;
		return failures == 0;
	}

	bool executeAudioSelfTest(const HConsoleCommandArgs& inArgs)
	{
		return RunAudioSelfTest(inArgs).Failed == 0;
	}

	HAutoConsoleCommand GameMasterSelfTestCommand(
		"gmtest",
		"gmtest",
		"Run the GameFrameWork self tests (GameMaster kernel + world + game viewport)",
		&executeGmTest);

	HAutoConsoleCommand GameUISelfTestCommand(
		"gameui.selftest",
		"gameui.selftest",
		"Run the game UI self tests (widget stacks, input routing, back, styles, layout, font, text wrap, text input, drag and drop, tweens, transitions, scroll boxes, wheel input). No GPU needed",
		&executeGameUISelfTest);

	HAutoConsoleCommand DataTableSelfTestCommand(
		"datatable.selftest",
		"datatable.selftest",
		"Run the data table self tests (values, JSON, binding, edits, undo, TSV paste, files). No GPU or asset database needed",
		&executeDataTableSelfTest);

	HAutoConsoleCommand CookCheckSelfTestCommand(
		"cookcheck.selftest",
		"cookcheck.selftest",
		"Run the cook content check self tests (parse, GUID, references, data tables) on temporary fixtures and the engine Content",
		&executeCookCheckSelfTest);

	HAutoConsoleCommand NetTestCommand(
		"net.test",
		"net.test [transport|session|recovery|lobby|rewind|world|all]",
		"Run the listen-server self tests (loopback, same-process TCP, lobby, rule rewind, game instance world)",
		&executeNetTest);

	HAutoConsoleCommand AudioSelfTestCommand(
		"audio.selftest",
		"audio.selftest [-ogg=<file path>]",
		"Run the audio self tests (mixing without a device: overlap, volume, pitch, buses, loop, fades, pause, music cross-fade, streaming, null device, lazy device open)",
		&executeAudioSelfTest);
}
