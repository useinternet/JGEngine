#pragma once
#include "Core.h"
#include <filesystem>

// 쿠킹 명령(cook.run · -runCook)이 쓰는 Windows 보조 함수(DevOps GP-D4, 2026-10-02).
// 프로세스 실행 · Visual Studio 찾기(vswhere) · DLL import 표 읽기 · 경로 변환. JGConsole 안에서만 쓴다 —
// 에디터 메뉴(GP-E2)가 같은 일을 하게 되면 Core HPlatform 으로 올린다.
namespace HCookPlatform
{
	// UTF-8 PString <-> std::filesystem::path(넓은 문자). 한글 경로도 그대로 다룬다.
	std::filesystem::path ToPath(const PString& path);
	PString FromPath(const std::filesystem::path& path);

	// '/' 구분자 · 끝 '/' 없는 정규화 절대경로(비교 · 표시용).
	PString NormalizedAbsolute(const PString& path);

	// a 가 b 와 같거나 b 의 조상 폴더면 true(대소문자 무시).
	bool IsSameOrAncestor(const PString& a, const PString& b);

	struct HProcessResult
	{
		bool   bStarted  = false;
		bool   bTimedOut = false;
		uint32 ExitCode  = 0;
	};

	// commandLine 을 workingDirectory 에서 실행하고 표준 출력 · 오류를 logPath 에 덮어쓴다. timeoutMs 0 = 무한.
	// 시간을 넘기면 프로세스를 끝내고 bTimedOut = true.
	HProcessResult RunProcess(const PString& commandLine, const PString& workingDirectory, const PString& logPath, uint32 timeoutMs);

	// Visual Studio 2022(v143 — premake vs2022 로 만든 솔루션)의 설치 경로. 없으면 가장 새 것, 그것도 없으면 빈 값.
	// 이 PC 처럼 VS 18 이 같이 있으면 -latest 는 18 을 주므로 버전 범위를 먼저 본다.
	PString FindVisualStudio2022();
	PString FindMSBuild(const PString& visualStudioPath);
	// <VS>/VC/Redist/MSVC/<가장 높은 버전>/x64/Microsoft.VC143.CRT (vcruntime140.dll 이 있는 곳). 없으면 빈 값.
	PString FindVCRuntimeRedist(const PString& visualStudioPath);

	// PE(exe · dll)가 import 하는 DLL 이름(지연 로드 포함, 소문자). 읽지 못하면 false.
	bool ReadImportedDlls(const PString& binaryPath, HList<PString>* outDllNames);

	// Windows 시스템 DLL 인가 — api-ms-win-* · ext-ms-* API 집합이거나 System32 에 있는 파일. VC 런타임은 false(앱 로컬로 넣는다).
	bool IsSystemDll(const PString& dllName);
	bool IsVCRuntimeDll(const PString& dllName);
}
