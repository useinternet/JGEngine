#pragma once
#include "GameMaster/GameMasterDefines.h"

// GameMaster 커널 자체 검증. 콘텐츠가 아니라 규칙 골격을 검사하는 최소 규칙(TestAct · TestChange …)을 내부에 둔다.
// JGConsole.exe gmtest 로 실행한다. 실패 개수를 돌려준다 (0 = 합격).
//   결정론(같은 시드 · 같은 명령열 → 같은 체크섬) · 되돌리기 · 저장/로드 · 리플레이 · 선택 대기 재진입 · 트리거 연쇄 · 수치 파이프라인 · 보드 · 에이전트
class GAMEFRAMEWORKS_API PGameMasterSelfTest
{
public:
	static int32 Run();
};
