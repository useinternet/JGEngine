#pragma once
#include "CoreDefines.h"
#include "FileIO/Jsonable.h"
#include "Memory/Allocator.h"
#include "String/String.h"
#include "FileIO/PackageDescriptor.h"

// .jgproject 의 "Package" 섹션. 없으면 기본값이라 기존 프로젝트는 고치지 않아도 된다.
// 쿠킹 명령(JGConsole -runCook)과 개발 모드 게임 호스트(JGLauncher -game)가 읽는다(DevOps GP-D1, 2026-10-02).
// 패키지에는 Content 만 들어간다(사용자 결정 D-23 ③) — 실행 중 쓰는 데이터는 <Project>/Content 안에 둔다.
class HProjectPackageSettings : public IJsonable
{
public:
	PString             Version = "0.1.0";
	HGameWindowSettings Window;
	HList<PString>      ExcludeContent;   // <Project>/Content 기준 상대 경로. 폴더는 '/' 로 끝낸다. 패키지에서 뺀다

public:
	virtual ~HProjectPackageSettings() = default;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

// 게임 프로젝트 파일 <Project>/<Name>.jgproject. 툴(-project=)과 런타임(실행 위치 기준 ../../)이 같은 코드로 읽는다.
//
// {
//   "Project": {
//     "Name": "MyGame",
//     "EngineRoot": "C:/JG/JGEngine",
//     "GameModules": [ "MyGame" ],
//     "EditorModules": [ "MyGameEditor" ],
//     "Package": { "Version": "0.1.0", "Window": { "Width": 1920, "Height": 1080 }, "ExcludeContent": [ "Dev/" ] }
//   }
// }
class HProjectDescriptor : public IJsonable
{
public:
	static constexpr const char* FileExtension = ".jgproject";
	static constexpr const char* EngineRootEnvironmentVariable = "JGENGINE_ROOT";

	PString        Name;
	PString        EngineRoot;      // 절대경로 또는 프로젝트 폴더 기준 상대경로. 비어 있으면 환경변수 JGENGINE_ROOT
	HList<PString> GameModules;     // 게임 실행 · 에디터 환경 모두에서 로드
	HList<PString> EditorModules;   // 에디터 환경에서만 로드
	HProjectPackageSettings Package; // 선택. 패키지 버전 · 게임 창 · Content 제외 목록

public:
	virtual ~HProjectDescriptor() = default;

	// projectDirectory 에서 *.jgproject 하나를 찾아 읽는다. 없으면 false, 둘 이상이면 오류 로그 후 false.
	static bool Load(const PString& projectDirectory, HProjectDescriptor* outDescriptor);

	// EngineRoot → JGENGINE_ROOT 순으로 엔진 루트를 구한다. 결과는 '/' 로 끝나는 절대경로.
	bool ResolveEngineRoot(const PString& projectDirectory, PString* outEngineRoot) const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
