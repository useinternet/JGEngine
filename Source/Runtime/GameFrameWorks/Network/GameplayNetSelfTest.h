#pragma once
#include "GameMaster/GameMasterDefines.h"

// 네트워크(전송 · 세션 · 월드 연결) 자체 검증. 콘텐츠가 아니라 복제 골격을 검사하는 최소 규칙을 내부에 둔다.
//   net.test [transport|session|recovery|world|all]   — 한 프로세스 (루프백 · 같은 프로세스 TCP · 게임 인스턴스 월드). 이 모듈이 선언한다
//   JGConsole.exe net.host / net.join [-world]         — 2 프로세스 결정론 (TCP). -world 면 게임 인스턴스 · 월드 · 액터 경로
class GAMEFRAMEWORKS_API PGameplayNetSelfTest
{
public:
	// 실패 개수를 돌려준다 (0 = 합격).
	static int32 Run(const PString& which);

	// 2 프로세스 테스트의 호스트. 클라 clientCount 명을 기다렸다가 commandCount 명령까지 진행하고 닫는다. 종료 코드.
	// bindAddress 가 비어 있으면 모든 인터페이스로 연다 (다른 기계에서 접속할 때).
	static int32 RunHostProcess(const PString& bindAddress, uint16 port, int32 clientCount, int32 commandCount, uint64 seed);
	// 2 프로세스 테스트의 클라. 호스트가 닫을 때까지 참가한다. 불일치가 없으면 0.
	static int32 RunJoinProcess(const PString& address, uint16 port, int32 playerCount, uint64 agentSeed, const PString& name);

	// 같은 검사를 게임 인스턴스 경로로: 세션은 JGGameInstance 가 갖고, 월드 로드 · 이동 알림 · GameMasterActor · 컨트롤러 입력을 거친다.
	// 호스트는 월드 둘을 차례로 로드한다 (명령 수 절반씩). 클라는 두 월드를 모두 따라와야 0.
	static int32 RunWorldHostProcess(const PString& bindAddress, uint16 port, int32 clientCount, int32 commandCount, uint64 seed);
	static int32 RunWorldJoinProcess(const PString& address, uint16 port, int32 playerCount, uint64 agentSeed, const PString& name);
};
