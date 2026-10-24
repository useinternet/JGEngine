#include "PCH/PCH.h"

#include <iostream>
#include <charconv>
#include "Core.h"
#include "Misc/Module.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "SelfTest/GameplayNetSelfTest.h"

// 리슨 서버 2 프로세스 검증 명령 (Server TODO Phase 4 · 5-6). 문법은 DevConsole_TODO 0-3 (`-이름=값`).
//   JGConsole.exe net.host [-world] -bind=127.0.0.1 -port=47771 -clients=2 -commands=1000 -seed=12345 [-rejoin-grace=30]
//   JGConsole.exe net.join [-world] -address=127.0.0.1 -port=47771 -players=3 -agent-seed=501 -name=ClientA [-leave-after-welcome] [-token=…] [-leave-at=300]
//   -world: 세션을 게임 인스턴스가 갖고 월드 · GameMasterActor · 컨트롤러를 거친다 (호스트가 월드 둘을 차례로 로드).
//   호스트는 세션 공유 데이터를 정하고 클라 모두가 준비를 켜면 시작한다. -leave-after-welcome 클라는 Welcome 만 받고 나간다(시작 전 나간 자리 비우기 확인).
//   창을 다시 띄워 같은 자리로(ER-026, kernel 모드만): -leave-at=N 클라는 순번 N 을 적용한 뒤 창을 닫듯 나가고, -token= 으로 새 프로세스가 같은 토큰을 쓴다.
//   호스트 -rejoin-grace=초 는 판 도중 나간 클라가 돌아오기를 그만큼 기다린다(없으면 나가는 즉시 실패).
// 호스트 · 클라 루프가 명령이 끝날 때까지 돌아 에디터 콘솔에서는 쓰지 않는다. 한 프로세스 검사 net.test 는 GameFrameWork 모듈이 선언한다.
// 잘못된 숫자는 예외 없이 거부한다 (핸들러가 false → 레지스트리가 사용법을 출력하고 종료 코드 1).

namespace
{
	bool connectGameFrameWork()
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWork") == false)
		{
			std::cout << "net: fail to connect GameFrameWork module" << std::endl;
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

	bool executeNetHost(const HConsoleCommandArgs& args)
	{
		// -bind 기본은 127.0.0.1 (한 기계 안 검사). 다른 기계에서 접속받으려면 -bind= (빈 값 = 모든 인터페이스).
		PString bind     = "127.0.0.1";
		uint16  port     = 47771;
		int32   clients  = 2;
		int32   commands = 1000;
		uint64  seed     = 12345;
		int32   grace    = 0;
		args.TryGetString("bind", bind);
		if (readPort(args, port) == false || readInt(args, "clients", clients) == false || readInt(args, "commands", commands) == false || readUint64(args, "seed", seed) == false
			|| readInt(args, "rejoin-grace", grace) == false)
		{
			return false;
		}
		if (args.Has("world") == true && grace > 0)
		{
			std::cout << "-rejoin-grace is for the kernel mode (no -world)" << std::endl;
			return false;
		}
		if (connectGameFrameWork() == false)
		{
			return false;
		}
		if (args.Has("world") == true)
		{
			return PGameplayNetSelfTest::RunWorldHostProcess(bind, port, clients, commands, seed) == 0;
		}
		return PGameplayNetSelfTest::RunHostProcess(bind, port, clients, commands, seed, grace) == 0;
	}

	bool executeNetJoin(const HConsoleCommandArgs& args)
	{
		PString address   = "127.0.0.1";
		PString name      = "Client";
		uint16  port      = 47771;
		int32   players   = 3;
		uint64  agentSeed = 501;
		int32   leaveAt   = 0;
		HNetJoinProcessOptions options;
		args.TryGetString("address", address);
		args.TryGetString("name", name);
		args.TryGetString("token", options.ReconnectToken);
		if (readPort(args, port) == false || readInt(args, "players", players) == false || readUint64(args, "agent-seed", agentSeed) == false
			|| readInt(args, "leave-at", leaveAt) == false)
		{
			return false;
		}
		if (leaveAt < 0)
		{
			std::cout << "-leave-at must be 0 or more" << std::endl;
			return false;
		}
		options.LeaveAtSeq         = (uint32)leaveAt;
		options.bLeaveAfterWelcome = args.Has("leave-after-welcome");
		if (args.Has("world") == true && options.bLeaveAfterWelcome == false && (options.ReconnectToken.Empty() == false || leaveAt > 0))
		{
			std::cout << "-token and -leave-at are for the kernel mode (no -world)" << std::endl;
			return false;
		}
		if (connectGameFrameWork() == false)
		{
			return false;
		}
		// 대기방 손님은 월드 모드 호스트에도 같은 길로 들어왔다 나간다(Welcome 까지만).
		if (options.bLeaveAfterWelcome == true)
		{
			return PGameplayNetSelfTest::RunJoinProcess(address, port, players, agentSeed, name, options) == 0;
		}
		if (args.Has("world") == true)
		{
			return PGameplayNetSelfTest::RunWorldJoinProcess(address, port, players, agentSeed, name) == 0;
		}
		return PGameplayNetSelfTest::RunJoinProcess(address, port, players, agentSeed, name, options) == 0;
	}

	HAutoConsoleCommand NetHostCommand(
		"net.host",
		"net.host [-world] [-bind=127.0.0.1] [-port=47771] [-clients=2] [-commands=1000] [-seed=12345] [-rejoin-grace=0]",
		"Headless listen-server host for the two-process determinism test (-world: through the game instance and world)",
		&executeNetHost);

	HAutoConsoleCommand NetJoinCommand(
		"net.join",
		"net.join [-world] [-address=127.0.0.1] [-port=47771] [-players=3] [-agent-seed=501] [-name=Client] [-leave-after-welcome] [-token=] [-leave-at=0]",
		"Headless client for the two-process determinism test (-world: through the game instance and world)",
		&executeNetJoin);
}
