#pragma once

#include "CoreDefines.h"
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

	static const PString& EngineDirectory();
	static const PString& EngineBuildDirectory();
	static const PString& EngineContentsDirectory();
	static const PString& EngineConfigDirectory();
	static const PString& EngineSourceDirectory();
	static const PString& EngineEditorSourceDirectory();
	static const PString& EngineProgramsSourceDirectory();
	static const PString& EngineRuntimeSourceDirectory();
	static const PString& EngineThirdPartyDirectory();
	static const PString& EngineTempDirectory();
	static const PString& EngineCodeGenDirectory();
	static const PString& EngineShaderDirectory();
};