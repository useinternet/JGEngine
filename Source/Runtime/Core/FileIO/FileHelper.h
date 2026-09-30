#pragma once

#include "CoreDefines.h"
#include "Memory/Allocator.h"
#include "String/String.h"

class HFileHelper
{

public:
	static bool WriteAllText(const PString& path, const PString& str);
	static bool ReadAllText(const PString& path, PString* out_str);
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
};