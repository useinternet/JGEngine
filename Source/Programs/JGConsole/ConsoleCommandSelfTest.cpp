#include "PCH/PCH.h"

#include <iostream>
#include "Core.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

// JGConsole.exe console.selftest — 콘솔 명령 시스템(파서·레지스트리·전역 변수 등록) 자체 검증.
// 음성 케이스(없는 명령, 따옴표 오류 등)가 [error] 로그를 남기는 것은 정상이다.

namespace
{
	int32 GCheckCount   = 0;
	int32 GFailureCount = 0;

	// 레지스트리 테스트용 핸들러가 남기는 흔적. 전역 변수 명령과 같은 조건(캡처 없음)으로 쓴다.
	int32   GEchoCallCount = 0;
	PString GEchoLastName;
	PString GEchoLastPositional;

	void check(bool bCondition, const char* InCaseName)
	{
		++GCheckCount;
		if (bCondition == false)
		{
			++GFailureCount;
			std::cout << "console.selftest FAILED: " << InCaseName << std::endl;
		}
	}

	bool tokenize(const char* InLine, HList<PString>& OutTokens)
	{
		PString error;
		return HConsoleCommandArgs::Tokenize(PString(InLine), OutTokens, error);
	}

	HConsoleCommandArgs makeArgs(const char* InLine)
	{
		HList<PString> tokens;
		tokenize(InLine, tokens);
		return HConsoleCommandArgs(tokens);
	}

	bool tokensEqual(const HList<PString>& InTokens, std::initializer_list<const char*> InExpected)
	{
		if (InTokens.size() != InExpected.size())
		{
			return false;
		}

		uint64 index = 0;
		for (const char* expected : InExpected)
		{
			if (InTokens[index].GetRawString() != expected)
			{
				return false;
			}
			++index;
		}

		return true;
	}

	void testTokenize()
	{
		HList<PString> tokens;

		check(tokenize("a b  c", tokens) && tokensEqual(tokens, { "a", "b", "c" }), "tokenize: spaces");
		check(tokenize("  lead\ttrail  ", tokens) && tokensEqual(tokens, { "lead", "trail" }), "tokenize: leading/trailing white space");
		check(tokenize("cmd \"a b\" -path=\"C:\\My Dir\\x.png\"", tokens) && tokensEqual(tokens, { "cmd", "a b", "-path=C:\\My Dir\\x.png" }), "tokenize: quotes and backslashes");
		check(tokenize("cmd \"\"", tokens) && tokensEqual(tokens, { "cmd", "" }), "tokenize: empty quoted token");
		check(tokenize("", tokens) && tokens.empty(), "tokenize: empty line");

		PString error;
		const bool bResult = HConsoleCommandArgs::Tokenize(PString("cmd \"open"), tokens, error);
		check(bResult == false && tokens.empty() && error.Empty() == false, "tokenize: unterminated quote");
	}

	void testArgs()
	{
		const HConsoleCommandArgs args = makeArgs("DevScene.ReadBack pos1 -Path=a=b -FORCE -n=1 -n=2 - -=x");
		check(args.GetName().GetRawString() == "devscene.readback", "args: name is lower-cased");
		check(args.GetPositionalCount() == 3, "args: positional count (pos1, -, -=x)");
		check(args.GetPositional(0).GetRawString() == "pos1", "args: positional 0");
		check(args.GetPositional(1).GetRawString() == "-", "args: lone dash is positional");
		check(args.GetPositional(9).Empty(), "args: positional out of range is empty");
		check(args.Has("force") && args.Has("FORCE"), "args: flag is case-insensitive");
		check(args.Has("missing") == false, "args: missing flag");

		PString path = "default";
		check(args.TryGetString("path", path) && path.GetRawString() == "a=b", "args: value keeps text after the first '='");

		PString missing = "default";
		check(args.TryGetString("missing", missing) == false && missing.GetRawString() == "default", "args: missing string leaves the default");

		int32 n = 0;
		check(args.TryGetInt("n", n) && n == 2, "args: the later value wins");

		const HConsoleCommandArgs numbers = makeArgs("cmd -a=abc -b=12x -c=99999999999 -d=-5 -e=");
		int32 value = 7;
		check(numbers.TryGetInt("a", value) == false && value == 7, "args: TryGetInt rejects 'abc'");
		check(numbers.TryGetInt("b", value) == false && value == 7, "args: TryGetInt rejects '12x'");
		check(numbers.TryGetInt("c", value) == false && value == 7, "args: TryGetInt rejects out of range");
		check(numbers.TryGetInt("e", value) == false && value == 7, "args: TryGetInt rejects empty");
		check(numbers.TryGetInt("d", value) && value == -5, "args: TryGetInt accepts negative");

		// 한글 등 비 ASCII 이름에서 크래시하지 않는다(PString::ToLower 를 쓰면 디버그 CRT assert).
		const HConsoleCommandArgs korean = makeArgs("\xED\x85\x8C\xEC\x8A\xA4\xED\x8A\xB8 -\xEA\xB0\x92=1");
		check(korean.GetName().Empty() == false, "args: non-ASCII name does not crash");
	}

	bool executeEcho(const HConsoleCommandArgs& InArgs)
	{
		++GEchoCallCount;
		GEchoLastName       = InArgs.GetName();
		GEchoLastPositional = InArgs.GetPositional(0);
		return true;
	}

	bool executeFail(const HConsoleCommandArgs&)
	{
		return false;
	}

	bool executeUnregisterSelf(const HConsoleCommandArgs&)
	{
		GConsoleCommandGlobalSystem::GetInstance().Unregister("test.selftest.once");
		return true;
	}

	bool registerTestCommand(const char* InName, HAutoConsoleCommand::HandlerFunction InHandler)
	{
		HConsoleCommandDesc desc;
		desc.Name        = InName;
		desc.Usage       = InName;
		desc.Description = "console.selftest temporary command";
		desc.Handler     = HConsoleCommandDelegate::CreateStatic(InHandler);
		return GConsoleCommandGlobalSystem::GetInstance().Register(desc);
	}

	void testRegistry()
	{
		GConsoleCommandGlobalSystem& commands = GConsoleCommandGlobalSystem::GetInstance();

		check(registerTestCommand("Test.SelfTest.Echo", &executeEcho), "registry: register");
		check(commands.IsRegistered("test.selftest.echo"), "registry: name is lower-cased on register");
		check(registerTestCommand("test.selftest.echo", &executeEcho) == false, "registry: duplicate name is rejected");
		check(registerTestCommand("", &executeEcho) == false, "registry: empty name is rejected");
		check(registerTestCommand("bad name", &executeEcho) == false, "registry: name with a space is rejected");

		HConsoleCommandDesc unbound;
		unbound.Name = "test.selftest.unbound";
		check(commands.Register(unbound) == false, "registry: missing handler is rejected");

		GEchoCallCount = 0;
		check(commands.Execute(PString("TEST.SELFTEST.ECHO first -x=1")) && GEchoCallCount == 1, "registry: execute");
		check(GEchoLastName.GetRawString() == "test.selftest.echo" && GEchoLastPositional.GetRawString() == "first", "registry: handler receives parsed args");

		HList<PString> tokens;
		tokens.push_back(PString("test.selftest.echo"));
		tokens.push_back(PString("with space"));
		check(commands.Execute(tokens) && GEchoLastPositional.GetRawString() == "with space", "registry: execute tokens (argv) keeps spaces");

		check(commands.Execute(PString("")) && GEchoCallCount == 2, "registry: empty line is ignored");
		check(commands.Execute(PString("test.selftest.echo \"open")) == false && GEchoCallCount == 2, "registry: parse error does not run the handler");
		check(commands.Execute(PString("test.selftest.nosuch")) == false, "registry: unknown command fails");

		check(registerTestCommand("test.selftest.fail", &executeFail), "registry: register failing command");
		check(commands.Execute(PString("test.selftest.fail")) == false, "registry: handler failure is reported");

		check(registerTestCommand("test.selftest.once", &executeUnregisterSelf), "registry: register self-unregistering command");
		check(commands.Execute(PString("test.selftest.once")), "registry: handler can unregister itself");
		check(commands.IsRegistered("test.selftest.once") == false, "registry: self-unregistered command is gone");
		check(commands.Execute(PString("test.selftest.once")) == false, "registry: executing it again fails without crashing");

		commands.Submit(PString("test.selftest.echo"));
		check(GEchoCallCount == 2, "registry: submit does not run immediately");
		commands.ExecutePending();
		check(GEchoCallCount == 3, "registry: submit runs on ExecutePending");

		// 사용자가 친 서식 문자가 로그 서식으로 해석되지 않는다(해석되면 가변 인자를 읽다 크래시한다).
		check(commands.Execute(PString("help %s %n %d")) == false, "registry: format characters in user input");
		check(commands.Execute(PString("%s%s%s%n")) == false, "registry: format characters as the command name");

		check(commands.Unregister("test.selftest.echo") && commands.Unregister("TEST.SELFTEST.FAIL"), "registry: unregister (case-insensitive)");
		check(commands.Unregister("test.selftest.echo") == false, "registry: unregister twice fails");
	}

	void testAutoCommands()
	{
		GConsoleCommandGlobalSystem& commands = GConsoleCommandGlobalSystem::GetInstance();

		check(commands.IsRegistered("help"), "auto: help is built in");
		check(commands.IsRegistered("console.selftest"), "auto: EXE global command (this one) is registered");
		check(commands.IsRegistered("gmtest"), "auto: module command is registered (JGConsole connects GameFrameWorks at startup)");
		check(registerTestCommand("gmtest", &executeEcho) == false, "auto: Register cannot take a declared name");
		check(commands.Execute(PString("help")) && commands.Execute(PString("help gmtest")), "auto: help list and help <command>");
		check(commands.Execute(PString("help test.selftest.nosuch")) == false, "auto: help <unknown> fails");
	}

	bool hasRecentLog(const HList<HLogLine>& InLines, const char* InText, ELogLevel InLevel)
	{
		for (const HLogLine& line : InLines)
		{
			if (line.Level == InLevel && line.Text.GetRawString() == InText)
			{
				return true;
			}
		}

		return false;
	}

	// DevConsole 로그 뷰가 읽는 최근 줄 버퍼(DevConsole_TODO 2-2)
	void testRecentLogs()
	{
		GLogGlobalSystem& logs = GLogGlobalSystem::GetInstance();
		HList<HLogLine> lines;

		const uint64 serialBefore = logs.GetLogSerial();
		JG_LOG(ConsoleCommand, ELogLevel::Info, "%s", "console.selftest log probe");
		check(logs.GetLogSerial() > serialBefore, "logs: serial increases when a line is added");

		logs.GetRecentLogs(lines);
		check(lines.empty() == false && lines.back().Text.GetRawString() == "[ConsoleCommand]: console.selftest log probe"
			&& lines.back().Level == ELogLevel::Info, "logs: the last recent line is the probe");

		const uint64 serialBeforeTrace = logs.GetLogSerial();
		JG_LOG(ConsoleCommand, ELogLevel::Trace, "%s", "console.selftest trace probe");
		check(logs.GetLogSerial() == serialBeforeTrace, "logs: trace lines are not kept");

		GConsoleCommandGlobalSystem::GetInstance().Execute(PString("help %s %n"));
		logs.GetRecentLogs(lines);
		check(hasRecentLog(lines, "[ConsoleCommand]: > help %s %n", ELogLevel::Info), "logs: format characters are echoed verbatim");
		check(hasRecentLog(lines, "[ConsoleCommand]: Unknown command '%s'", ELogLevel::Error), "logs: error line keeps the level");
	}

	bool executeSelfTest(const HConsoleCommandArgs&)
	{
		GCheckCount   = 0;
		GFailureCount = 0;

		testTokenize();
		testArgs();
		testRegistry();
		testAutoCommands();
		testRecentLogs();

		std::cout << "console.selftest: " << (GFailureCount == 0 ? "OK" : "FAILED")
			<< " (" << (GCheckCount - GFailureCount) << "/" << GCheckCount << ")" << std::endl;
		return GFailureCount == 0;
	}

	HAutoConsoleCommand SelfTestCommand(
		"console.selftest",
		"console.selftest",
		"Run the console command system self tests",
		&executeSelfTest);
}
