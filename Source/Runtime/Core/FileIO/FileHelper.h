#pragma once

#include "CoreDefines.h"
#include "Memory/Allocator.h"
#include "String/String.h"
#include <vector>

class HFileHelper
{

public:
	static bool WriteAllText(const PString& path, const PString& str);
	// 같은 폴더의 임시 파일(<path>.tmp)에 다 쓴 뒤 path 로 바꿔 넣는다. 쓰는 도중 죽어도 원래 파일이 남는다.
	static bool WriteAllTextAtomic(const PString& path, const PString& str);
	static bool ReadAllText(const PString& path, PString* out_str);
	// 파일을 통째로 바이트로 읽는다(글꼴 · 이미지). 경로는 wide 로 연다(ASCII 가 아닌 경로도 열린다).
	// 수 MB 파일(한글 글꼴)도 읽도록 엔진 풀이 아닌 std 할당자(std::vector)를 쓴다.
	static bool ReadAllBytes(const PString& path, std::vector<uint8>& outData);
	static bool CreateDirectory(const PString& path);

	static bool CopyFileOrDirectory(const PString& dest, const PString& src);
	static bool RemoveFileOrDirectory(const PString& path);

	static bool Exists(const PString& inPath);
	static void FileName(const PString& inPath, PString* outStr);
	static void FileNameOnly(const PString& inPath, PString* outStr);
	static void FilePathOnly(const PString& inPath, PString* outStr);
	static bool FileExtension(const PString& inPath, PString* outStr);
	static void FileListInDirectory(const PString& inDir, HList<PString>* outFileList, bool bIsRecursive = false, const HList<PString>& filterFileFormats = HList<PString>());

	static bool IsDirectory(const PString& directoryName);
	static void CombinePath(const PString& p1, const PString& p2, PString* outStr);
	static void NormalizePath(PString* outPath);
	static void AbsolutePath(const PString& inPath, PString* outPath);
	// 절대경로로 바꾸고 '..' 을 접은 뒤 '/' 구분자와 끝의 '/' 를 붙인다. 두 폴더 경로를 문자열로 비교할 때 쓴다.
	static void AbsoluteDirectory(const PString& inPath, PString* outPath);

	// 루트는 GCoreSystem::Create 가 한 번 정한다(HCoreSystemGlobalValues). Create 전에는 "../../".
	// 엔진 단독 실행이면 엔진 루트 = 프로젝트 루트 = "../../"(실행 위치 기준). 게임 프로젝트면 둘 다 절대경로.
	static const PString& EngineDirectory();
	static const PString& ProjectDirectory();
	static const PString& ProjectName();
	static bool IsProjectMode();
	static const PString& ProjectSourceDirectory();
	static const PString& ProjectBinDirectory();
	static const PString& ProjectTempDirectory();
	static const PString& ProjectCodeGenDirectory();

	static const PString& EngineBinDirectory();
	static const PString& EngineBuildDirectory();
	static const PString& EngineContentDirectory();
	static const PString& EngineConfigDirectory();
	static const PString& EngineSourceDirectory();
	static const PString& EngineEditorSourceDirectory();
	static const PString& EngineProgramsSourceDirectory();
	static const PString& EngineRuntimeSourceDirectory();
	static const PString& EngineThirdPartyDirectory();
	static const PString& EngineTempDirectory();
	static const PString& EngineCodeGenDirectory();
	static const PString& EngineShaderDirectory();
	static const PString& GameContentDirectory();

	// 패키지 전용 빌드(JG_WITH_EDITOR 0 — ConfirmGame · ReleaseGame, DevOps GP-D1)에서는 루트가 패키지 루트(<Name>.exe 폴더)이고
	// EngineContentDirectory = <패키지>/Content/JGEngine, GameContentDirectory = <패키지>/Content/JGGame, 셰이더는 엔진 Content 의 Shaders/.
	// 개발 전용 폴더(Source · Build · ThirdParty · Config · Temp · CodeGen)는 만들지 않고 오류 로그를 한 번 남긴다.
	// Content · Saved · Bin 밖을 ReadAllText · ReadAllBytes · FileListInDirectory 로 읽으면 경고한다(Content 만 패키징 — 사용자 결정 D-23 ③).

	// 실행 중에 쓰는 파일(저장 · 설정 · 로그). <루트>/Saved — 엔진 단독은 엔진, 게임 프로젝트는 프로젝트, 패키지는 패키지 폴더 안. 없으면 만든다.
	static const PString& SavedDirectory();
};