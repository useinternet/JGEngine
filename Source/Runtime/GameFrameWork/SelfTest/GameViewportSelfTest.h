#pragma once
#include "Core/GameFrameWorkDefines.h"
#include "SelfTest/GameFrameWorkSelfTest.h"

// 게임 뷰포트(GP-F1) 검증. gfw.selftest(그룹 viewport) · 별칭 gmtest 가 월드 자체 테스트 뒤에 실행한다. 통과 · 실패 · 건너뛴 절 수를 돌려준다.
// 자기 전용 PWorld · PGameUIManager 를 SetSource 로 준다 — 에디터 안에서 돌려도 게임 인스턴스(게임 월드 · 게임 UI)를 건드리지 않는다.
// 입력 라우팅은 렌더러 없는 뷰포트(Resize 로 크기만)로 본다(헤드리스에서도 돈다).
// 그래픽이 있으면(에디터 안 gmtest) 마지막에 자기 렌더러(64x36 → Resize 32x18)로 그리기 · 크기 변경을 본다. 콘솔 명령은 프레임 밖(스케줄러 프레임
// 제출 뒤 GConsoleCommandGlobalSystem::Update)에서 실행되지만, 그때의 GPU 기록은 Graphics 5-33(사용자 결정 D-31, 2026-10-03)으로 안전해졌다
// — 버린 렌더러의 상태 기록은 그 프레임의 GPU 작업이 끝날 때까지 남고, 프레임 밖 리스트 재사용은 앞 GPU 작업을 기다린다. 이 검사가 그 회귀를 잡는다.
class GAMEFRAMEWORK_API PGameViewportSelfTest
{
public:
	static HSelfTestResult Run();
};
