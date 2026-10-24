#pragma once
#include "GameMaster/GameMasterDefines.h"
#include "SelfTest/GameFrameWorkSelfTest.h"

// 네트워크(전송 · 세션 · 월드 연결) 자체 검증. 콘텐츠가 아니라 복제 골격을 검사하는 최소 규칙을 내부에 둔다.
//   gfw.selftest net[=transport|session|recovery|lobby|rewind|world] · 별칭 net.test — 한 프로세스 (루프백 · 같은 프로세스 TCP · 대기방 · 규칙 효과 안 되감기 · 게임 인스턴스 월드). SelfTest/GameFrameWorkSelfTest.cpp 가 등록한다
//   JGConsole.exe net.host / net.join [-world]         — 2 프로세스 결정론 (TCP). -world 면 게임 인스턴스 · 월드 · 액터 경로
// 2 프로세스 테스트 클라의 선택 동작.
struct HNetJoinProcessOptions
{
	bool    bLeaveAfterWelcome = false;   // Welcome 을 받자마자 나간다(판 시작 전 나간 자리가 비는지 보는 대기방 손님, D-38)
	PString ReconnectToken;               // 비우지 않으면 이 토큰으로 들어간다(창을 다시 띄운 참가자, ER-026)
	uint32  LeaveAtSeq         = 0;       // 0 이 아니면 이 순번까지 적용한 뒤 세션을 닫고 끝낸다(판 도중 창 닫기, ER-026)
};

class GAMEFRAMEWORK_API PGameplayNetSelfTest
{
public:
	// 통과 · 실패 · 건너뛴 절 수를 돌려준다(실패 0 = 합격).
	static HSelfTestResult Run(const PString& which);

	// 2 프로세스 테스트의 호스트. 세션 공유 데이터를 정하고, 클라 clientCount 명이 들어와 모두 준비를 켜면 시작해 commandCount 명령까지 진행하고 닫는다
	// (절반에서 세션 공유 데이터를 하나 더 바꾼다). 종료 코드. bindAddress 가 비어 있으면 모든 인터페이스로 연다 (다른 기계에서 접속할 때).
	// rejoinGraceSeconds > 0 이면 판 도중 나간 클라가 같은 토큰으로 돌아오기를 그만큼 기다린다(0 이면 나가는 즉시 실패).
	static int32 RunHostProcess(const PString& bindAddress, uint16 port, int32 clientCount, int32 commandCount, uint64 seed, int32 rejoinGraceSeconds = 0);
	// 2 프로세스 테스트의 클라. 들어오면 준비를 켜고 호스트가 닫을 때까지 참가한다. 불일치가 없으면 0. 선택 동작은 options.
	static int32 RunJoinProcess(const PString& address, uint16 port, int32 playerCount, uint64 agentSeed, const PString& name, const HNetJoinProcessOptions& options = HNetJoinProcessOptions());

	// 같은 검사를 게임 인스턴스 경로로: 세션은 JGGameInstance 가 갖고, 월드 로드 · 이동 알림 · GameMasterActor · 컨트롤러 입력을 거친다.
	// 호스트는 월드 둘을 차례로 로드한다 (명령 수 절반씩). 클라는 두 월드를 모두 따라와야 0.
	static int32 RunWorldHostProcess(const PString& bindAddress, uint16 port, int32 clientCount, int32 commandCount, uint64 seed);
	static int32 RunWorldJoinProcess(const PString& address, uint16 port, int32 playerCount, uint64 agentSeed, const PString& name);
};
