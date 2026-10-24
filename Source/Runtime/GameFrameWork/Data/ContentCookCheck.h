#pragma once
#include "Core/GameFrameWorkDefines.h"

// 마운트 토큰 — Asset AssetDefines.h 와 같은 값. JGConsole 은 Asset 헤더를 include 하지 못해서 여기서도 정의한다.
// 값이 달라지면 두 헤더를 함께 include 하는 ContentCookCheck.cpp 가 C4005(매크로 재정의) 경고를 낸다
#ifndef JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN
#define JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN "/JGEngine/"
#endif
#ifndef JG_ASSET_GAME_PATH_RECOGNITION_TOEKN
#define JG_ASSET_GAME_PATH_RECOGNITION_TOEKN "/JGGame/"
#endif

// 쿠킹 검사 입구(게임 빌드 파이프라인 GP-A1). 쿠킹 명령(JGConsole cook.run 7단계)이 부른다.
// Asset 의 ValidateAssetsForCook(1 형식 · 2 GUID · 3 참조, Cook/AssetCookValidator.h)에
// 4 데이터 테이블 검사(datatable.validate 와 같은 규칙: 오류면 실패, 경고는 경고)를 끼워 부른다.
// 헤더는 Core 형식만 쓴다 — JGConsole 은 Core · GameFrameWork 헤더만 include 한다.
//   inFiles  : 패키지에 넣을 모든 파일(원본 절대 경로, 고르기 · 제외를 끝낸 목록). .jgasset 은 검사하고 다른 파일은 참조 대상으로만 쓴다
//   inMounts : (토큰, 원본 Content 폴더) — { "/JGEngine/", <Engine>/Content/ }, { "/JGGame/", <Project>/Content/ }
//   outErrors · outWarnings : "<토큰 경로> : <이유>" 한 줄씩 덧붙인다
//   반환 : 오류가 없으면 true(경고만 있어도 true)
GAMEFRAMEWORK_API bool ValidateContentForCook(const HList<PString>& inFiles, const HList<HPair<PString, PString>>& inMounts,
	HList<PString>& outErrors, HList<PString>& outWarnings);
