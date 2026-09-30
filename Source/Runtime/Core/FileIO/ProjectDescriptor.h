#pragma once
#include "CoreDefines.h"
#include "FileIO/Jsonable.h"
#include "Memory/Allocator.h"
#include "String/String.h"

// 게임 프로젝트 파일 <Project>/<Name>.jgproject. 툴(-project=)과 런타임(실행 위치 기준 ../../)이 같은 코드로 읽는다.
//
// {
//   "Project": {
//     "Name": "MyGame",
//     "EngineRoot": "C:/JG/JGEngine",
//     "GameModules": [ "MyGame" ],
//     "EditorModules": [ "MyGameEditor" ]
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
