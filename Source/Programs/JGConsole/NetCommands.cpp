#include "PCH/PCH.h"

#include <iostream>
#include <charconv>
#include "Core.h"
#include "Misc/Module.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "Network/GameplayNetSelfTest.h"

// 리슨 서버 검증 명령 (Network_TODO Phase 1–4). 문법은 DevConsole_TODO 0-3 (`-이름=값`).
//   JGConsole.exe net.test [transport|session|recovery|all]
//   JGConsole.exe net.host -bind=127.0.0.1 -port=47771 -clients=2 -commands=1000 -seed=12345
//   JGConsole.exe net.join -address=127.0.0.1 -port=47771 -players=3 -agent-seed=501 -name=ClientA
// 잘못된 숫자는 예외 없이 거부한다 (핸들러가 false → 레지스트리가 사용법을 출력하고 종료 코드 1).

namespace
{
	bool connectGameFrameWorks()
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
		{
			std::cout << "net: fail to connect GameFrameWorks module" << std::endl;
			return false;
		}
		return true;
	}

	// 없으면 기본값 그대로 true, 주어졌는데 숫자가 아니면 false.
	bool readInt(const HConsoleCommandArgs& args, const char* name, int32& inOutValue)
	{
		if (args.Has(name) == true && args.TryGetInt(name, inOutValue) == false)
		{
			std::cout << "-" << name << " is not a number" << std::endl;
			return false;
		}
		return true;
	}

	bool readPort(const HConsoleCommandArgs& args, uint16& inOutPort)
	{
		int32 port = (int32)inOutPort;
		if (readInt(args, "port", port) == false)
		{
			return false;
		}
		if (port <= 0 || port > 65535)
		{
			std::cout << "-port must be 1..65535" << std::endl;
			return false;
		}
		inOutPort = (uint16)port;
		return true;
	}

	// 64 비트 시드. TryGetInt 는 32 비트라 문자열로 받아 std::from_chars 로 읽는다 (예외 없음).
	bool readUint64(const HConsoleCommandArgs& args, const char* name, uint64& inOutValue)
	{
		PString text;
		if (args.TryGetString(name, text) == false)
		{
			return true;
		}

		const HRawString& raw = text.GetRawString();
		const char* first = raw.data();
		const char* last  = raw.data() + raw.size();
		uint64 value = 0;
		std::from_chars_result result = std::from_chars(first, last, value);
		if (raw.empty() == true || result.ec != std::errc() || result.ptr != last)
		{
			std::cout << "-" << name << " is not a number" << std::endl;
			return false;
		}
		inOutValue = value;
		return true;
	}

	bool executeNetTest(const HConsoleCommandArgs& args)
	{
		if (connectGameFrameWorks() == false)
		{
			return false;
		}

		PString which = "all";
		if (args.GetPositionalCount() > 0)
		{
			which = args.GetPositional(0);
		}
		int32 failures = PGameplayNetSelfTest::Run(which);
		std::cout << "net.test: " << (failures == 0 ? "OK" : "FAILED") << " (" << failures << " failures)" << std::endl;
		return failures == 0;
	}

	bool executeNetHost(const HConsoleCommandArgs& args)
	{
		// -bind 기본은 127.0.0.1 (한 기계 안 검사). 다른 기계에서 접속받으려면 -bind= (빈 값 = 모든 인터페이스).
		PString bind     = "127.0.0.1";
		uint16  port     = 47771;
		int32   clients  = 2;
		int32   commands = 1000;
		uint64  seed     = 12345;
		args.TryGetString("bind", bind);
		if (readPort(args, port) == false || readInt(args, "clients", clients) == false || readInt(args, "commands", commands) == false || readUint64(args, "seed", seed) == false)
		{
			return false;
		}
		if (connectGameFrameWorks() == false)
		{
			return false;
		}
		return PGameplayNetSelfTest::RunHostProcess(bind, port, clients, commands, seed) == 0;
	}

	bool executeNetJoin(const HConsoleCommandArgs& args)
	{
		PString address   = "127.0.0.1";
		PString name      = "Client";
		uint16  port      = 47771;
		int32   players   = 3;
		uint64  agentSeed = 501;
		args.TryGetString("address", address);
		args.TryGetString("name", name);
		if (readPort(args, port) == false || readInt(args, "players", players) == false || readUint64(args, "agent-seed", agentSeed) == false)
		{
			return false;
		}
		if (connectGameFrameWorks() == false)
		{
			return false;
		}
		return PGameplayNetSelfTest::RunJoinProcess(address, port, players, agentSeed, name) == 0;
	}

	HAutoConsoleCommand NetTestCommand(
		"net.test",
		"net.test [transport|session|recovery|all]",
		"Run the listen-server self tests (loopback and same-process TCP)",
		&executeNetTest);

	HAutoConsoleCommand NetHostCommand(
		"net.host",
		"net.host [-bind=127.0.0.1] [-port=47771] [-clients=2] [-commands=1000] [-seed=12345]",
		"Headless listen-server host for the two-process determinism test",
		&executeNetHost);

	HAutoConsoleCommand NetJoinCommand(
		"net.join",
		"net.join [-address=127.0.0.1] [-port=47771] [-players=3] [-agent-seed=501] [-name=Client]",
		"Headless client for the two-process determinism test",
		&executeNetJoin);
}
