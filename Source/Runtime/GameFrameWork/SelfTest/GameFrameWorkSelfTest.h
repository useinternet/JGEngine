#pragma once
#include "Core.h"
#include "Core/GameFrameWorkDefines.h"

// GameFrameWork 자체 시험 하나의 결과. 시험 파일은 진입 함수만 내놓고, 명령 등록 · 묶음 실행은 SelfTest/GameFrameWorkSelfTest.cpp(gfw.selftest)가 맡는다.
struct HSelfTestResult
{
	int32 Passed  = 0;
	int32 Failed  = 0;
	int32 Skipped = 0;   // 스스로 건너뛴 절 · 그룹 수(실패가 아니다)
};

class HConsoleCommandArgs;

// 파일 하나로 된 시험의 진입 함수. inArgs 는 옛 명령(별칭)의 인자 그대로, 묶음에서는 빈 인자다.
HSelfTestResult RunGameUISelfTest(const HConsoleCommandArgs& inArgs);
HSelfTestResult RunDataTableSelfTest(const HConsoleCommandArgs& inArgs);
HSelfTestResult RunCookCheckSelfTest(const HConsoleCommandArgs& inArgs);
HSelfTestResult RunAudioSelfTest(const HConsoleCommandArgs& inArgs);
