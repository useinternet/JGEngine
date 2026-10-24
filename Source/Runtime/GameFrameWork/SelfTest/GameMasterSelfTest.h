#pragma once
#include "GameMaster/GameMasterDefines.h"
#include "SelfTest/GameFrameWorkSelfTest.h"

// GameMaster 커널 자체 검증. 콘텐츠가 아니라 규칙 골격을 검사하는 최소 규칙(TestAct · TestChange …)을 내부에 둔다.
// gfw.selftest(그룹 gamemaster) · 별칭 gmtest 로 실행한다. 통과 · 실패 수를 돌려준다(실패 0 = 합격).
//   결정론(같은 시드 · 같은 명령열 → 같은 체크섬) · 되돌리기 · 저장/로드 · 리플레이 · 선택 대기 재진입 · 트리거 연쇄 · 수치 파이프라인 · 보드 · 에이전트
class GAMEFRAMEWORK_API PGameMasterSelfTest
{
public:
	static HSelfTestResult Run();
};
