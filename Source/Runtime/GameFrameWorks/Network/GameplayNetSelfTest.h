#pragma once
#include "GameMaster/GameMasterDefines.h"

// 네트워크(전송 · 세션) 자체 검증. 콘텐츠가 아니라 복제 골격을 검사하는 최소 규칙을 내부에 둔다.
//   JGConsole.exe net.test [transport|session|recovery|all]   — 한 프로세스 (루프백 · 같은 프로세스 TCP)
//   JGConsole.exe net.host / net.join                          — 2 프로세스 결정론 (TCP)
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
};
