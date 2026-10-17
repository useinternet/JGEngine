#include "PCH/PCH.h"
#include "FileHelper.h"
#include "CoreSystem.h"
#include "Misc/Log.h"
#include <fstream>

static const char EXTENSION_TOKEN = '.';
static const char PATH_SEP_TOKEN1 = '/';
static const char PATH_SEP_TOKEN2 = '\\';

#if JG_WITH_EDITOR == 0
namespace
{
	// 패키지 읽기 가드(패키지 전용 빌드만. DevOps GP-D1, 사용자 결정 D-23 ③ "Content 만 패키징"). 패키지에는 Content · Saved · Bin 밖의 데이터가 없다.
	// 그 밖을 읽으면 폴더마다 한 번 경고해서, 개발 중 Content 밖에 둔 실행 데이터를 스모크 실행 로그에서 경로째 찾게 한다.
	// HFileHelper 를 거치지 않는 읽기(직접 ifstream)는 보지 않는다. 게임 UI 글꼴 · 이미지는 ReadAllBytes 로 거친다(ER-016 — OS 글꼴 대체도 여기 잡힌다).
	void warnPackageReadOutsideMounts(const PString& path)
	{
		if (GCoreSystem::HasInstance() == false || GCoreSystem::GetGlobalValues().bProjectMode == false)
		{
			return;
		}

		PString absolutePath = fs::absolute(path.GetRawString()).lexically_normal().string().c_str();
		HFileHelper::NormalizePath(&absolutePath);
		const HRawString lowerPath = PString::ToLower(absolutePath).GetRawString();

		const HRawString& packageDirectory = GCoreSystem::GetGlobalValues().ProjectDirectory;   // 패키지 루트('/' 로 끝남)
		static const char* const allowedFolders[] = { "Content", "Saved", "Bin" };
		for (const char* folder : allowedFolders)
		{
			const HRawString allowedRoot = PString::ToLower(PString((packageDirectory + folder).c_str())).GetRawString();
			if (lowerPath == allowedRoot || lowerPath.rfind(allowedRoot + "/", 0) == 0)
			{
				return;
			}
		}

		PString directory;
		HFileHelper::FilePathOnly(absolutePath, &directory);

		static std::mutex warnedMutex;
		static std::unordered_set<HRawString> warnedDirectories;
		{
			std::lock_guard<std::mutex> lock(warnedMutex);
			if (warnedDirectories.insert(PString::ToLower(directory).GetRawString()).second == false)
			{
				return;
			}
		}

		JG_LOG(Core, ELogLevel::Warning, "Package reads outside Content/Saved/Bin: %s - runtime data must be inside Content", absolutePath);
	}
}
#endif

bool HFileHelper::WriteAllText(const PString& path, const PString& str)
{
	std::ofstream fout;
	fout.open(path.GetRawString());

	if (fout.is_open() == true)
	{
		fout << str.GetRawString();
		fout.close();
		return true;
	}
	else
	{
		fout.close();
		return false;
	}
}

bool HFileHelper::WriteAllTextAtomic(const PString& path, const PString& str)
{
	const PString tempPath = path + ".tmp";
	if (WriteAllText(tempPath, str) == false)
	{
		return false;
	}

	// std::filesystem::rename 은 Windows 에서 MoveFileEx(MOVEFILE_REPLACE_EXISTING) 라 있는 파일을 바꿔 넣는다
	std::error_code errCode;
	fs::rename(tempPath.GetRawString(), path.GetRawString(), errCode);
	if (errCode)
	{
		std::error_code removeErrCode;
		fs::remove(tempPath.GetRawString(), removeErrCode);
		return false;
	}

	return true;
}

bool HFileHelper::ReadAllText(const PString& path, PString* out_str)
{
	if (out_str == nullptr)
	{
		return false;
	}
#if JG_WITH_EDITOR == 0
	warnPackageReadOutsideMounts(path);
#endif

	std::ifstream fin;
	fin.open(path.GetRawString());
	if (fin.is_open() == true)
	{
		std::stringstream ss;
		ss << fin.rdbuf();
		
		*out_str = ss.str().c_str();
		
		fin.close();
		return true;
	}
	else
	{
		fin.close();
		return false;
	}
}

bool HFileHelper::ReadAllBytes(const PString& path, std::vector<uint8>& outData)
{
	outData.clear();
#if JG_WITH_EDITOR == 0
	warnPackageReadOutsideMounts(path);
#endif

	std::ifstream file(path.GetRawWString(), std::ios::binary | std::ios::ate);
	if (file.is_open() == false)
	{
		return false;
	}

	const std::streamsize size = file.tellg();
	if (size <= 0)
	{
		return false;
	}

	outData.resize((size_t)size);
	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(outData.data()), size);
	return file.good() || file.eof();
}

bool HFileHelper::CreateDirectory(const PString& path)
{
	if (Exists(path) == false)
	{
		return fs::create_directory(path.GetRawString());
	}

	return true;
}

bool HFileHelper::CopyFileOrDirectory(const PString& dest, const PString& src)
{
	std::error_code errCode;

	if (IsDirectory(dest) == true)
	{
		fs::copy(dest.GetRawString(), src.GetRawString(), fs::copy_options::recursive, errCode);
	}
	else
	{
		fs::copy_file(dest.GetRawString(), src.GetRawString(), fs::copy_options::overwrite_existing, errCode);
	}

	if (errCode.value() != 0)
	{
		JG_LOG(Core, ELogLevel::Error, "Fail Copy[%s -> %s]: %s", dest, src, errCode.message().c_str());
		return false;
	}

	return true;
}

bool HFileHelper::RemoveFileOrDirectory(const PString& path)
{
	if (IsDirectory(path) == true)
	{
		return fs::remove_all(path.GetRawString());
	}
	else
	{
		return fs::remove(path.GetRawString());
	}
}

bool HFileHelper::Exists(const PString& inPath)
{
	return fs::exists(inPath.GetRawString());
}

void HFileHelper::FileName(const PString& inPath, PString* outStr)
{
	if (outStr == nullptr)
	{
		return;
	}

	uint64 pos = inPath.FindLastOf(PATH_SEP_TOKEN1);
	if (pos == PString::NPOS)
	{
		pos = inPath.FindLastOf(PATH_SEP_TOKEN2);
	}

	inPath.SubString(outStr, pos + 1);
}

void HFileHelper::FileNameOnly(const PString& inPath, PString* outStr)
{
	if (outStr == nullptr)
	{
		return;
	}

	PString tempPath = inPath;
	uint64 pos = inPath.FindLastOf(EXTENSION_TOKEN);
	if (pos != PString::NPOS)
	{
		inPath.SubString(&tempPath, 0, pos);
	}

	FileName(tempPath, outStr);
}

void HFileHelper::FilePathOnly(const PString& inPath, PString* outStr)
{
	if (outStr == nullptr)
	{
		return;
	}

	uint64 pos = inPath.FindLastOf(PATH_SEP_TOKEN1);
	if (pos == PString::NPOS)
	{
		pos = inPath.FindLastOf(PATH_SEP_TOKEN2);
	}

	inPath.SubString(outStr, 0, pos);
}

bool HFileHelper::FileExtension(const PString& inPath, PString* outStr)
{
	if (outStr == nullptr)
	{
		return false;
	}

	uint64 pos = inPath.FindLastOf(EXTENSION_TOKEN);
	if (pos == PString::NPOS)
	{
		return false;
	}

	inPath.SubString(outStr, pos);

	return true;
}

void AddFileList(const PString& inFilePath, HList<PString>* outFileList, const HList<PString>& filterFileFormats)
{
	PString filePath = inFilePath;
	PString fileExtension;
	HFileHelper::FileExtension(filePath, &fileExtension);

	bool bPass = true;
	if (filterFileFormats.empty() == false)
	{
		for (const PString& extension : filterFileFormats)
		{
			if (fileExtension == extension)
			{
				bPass = false;
				break;
			}
		}
	}
	else
	{
		bPass = false;
	}

	if (bPass == true)
	{
		return;
	}

	HFileHelper::NormalizePath(&filePath);
	outFileList->push_back(filePath);
}

void HFileHelper::FileListInDirectory(const PString& inDir, HList<PString>* outFileList, bool bIsRecursive, const HList<PString>& filterFileFormats)
{
	if (outFileList == nullptr)
	{
		return;
	}

	outFileList->clear();
#if JG_WITH_EDITOR == 0
	warnPackageReadOutsideMounts(inDir);
#endif

	if (bIsRecursive == false)
	{
		fs::directory_iterator dirIter = fs::directory_iterator(inDir.GetRawString());
		for (auto& fileIter : dirIter)
		{
			PString filePath  = fs::path(fileIter).string().c_str();

			AddFileList(filePath, outFileList, filterFileFormats);
		}
	}
	else
	{
		fs::recursive_directory_iterator dirIter = fs::recursive_directory_iterator(inDir.GetRawString());
		for (auto& fileIter : dirIter)
		{
			PString filePath = fs::path(fileIter).string().c_str();

			AddFileList(filePath, outFileList, filterFileFormats);
		}
	}
}

bool HFileHelper::IsDirectory(const PString& directoryName)
{
	return fs::is_directory(directoryName.GetRawString());
}

void HFileHelper::CombinePath(const PString& p1, const PString& p2, PString* outStr)
{
	if (outStr == nullptr)
	{
		return;
	}

	if (&p1 == outStr && &p2 == outStr)
	{
		PString temp1 = p1;
		PString temp2 = p2;

		CombinePath(temp1, temp2, outStr);
	}
	else if (&p1 == outStr)
	{
		PString temp1 = p1;
		CombinePath(temp1, p2, outStr);
	}
	else if (&p2 == outStr)
	{
		PString temp2 = p2;
		CombinePath(p1, temp2, outStr);
	}
	else
	{
		*outStr = PString();
		outStr->Append(p1);

		uint64 p1LastPos = p1.Length() - 1;

		if (p1[p1LastPos] != PATH_SEP_TOKEN1 && p1[p1LastPos] != PATH_SEP_TOKEN2)
		{
			outStr->Append("/");
		}

		outStr->Append(p2);
	}
}

void HFileHelper::NormalizePath(PString* outPath)
{
	if (outPath == nullptr)
	{
		return;
	}

	outPath->ReplaceAll(PATH_SEP_TOKEN2, PATH_SEP_TOKEN1);
}

void HFileHelper::AbsolutePath(const PString& inPath, PString* outPath)
{
	if (outPath == nullptr)
	{
		return;
	}

	(*outPath) = fs::absolute(inPath.GetRawString()).string().c_str();
	NormalizePath(outPath);
}

void HFileHelper::AbsoluteDirectory(const PString& inPath, PString* outPath)
{
	if (outPath == nullptr)
	{
		return;
	}

	(*outPath) = fs::absolute(inPath.GetRawString()).lexically_normal().string().c_str();
	NormalizePath(outPath);

	if (outPath->Empty() == false && outPath->EndWidth("/") == false)
	{
		outPath->Append("/");
	}
}

namespace
{
	const char* const DEFAULT_ROOT_DIRECTORY = "../../";

	// GCoreSystem 이 정한 값을 읽는다. Core 가 StaticLib 이라 이 파일의 static 은 모듈(DLL)마다 따로 있으므로
	// 루트 자체는 모든 모듈이 같은 GCoreSystem 인스턴스(Link_Module 로 전달)에서 가져온다.
	const HRawString& resolvedRoot(bool bEngineRoot)
	{
		static const HRawString fallback = DEFAULT_ROOT_DIRECTORY;
		if (GCoreSystem::HasInstance() == false)
		{
			return fallback;
		}

		const HCoreSystemGlobalValues& values = GCoreSystem::GetGlobalValues();
		const HRawString& root = bEngineRoot ? values.EngineDirectory : values.ProjectDirectory;
		if (root.empty() == true)
		{
			return fallback;
		}

		return root;
	}

	void syncString(PString* outCached, const HRawString& value)
	{
		if (outCached->GetRawString() != value)
		{
			(*outCached) = value.c_str();
		}
	}

	// 개발 전용 폴더(Source · Build · ThirdParty · Config · Temp · CodeGen)는 패키지에 없다(DevOps GP-D1).
	// 패키지 전용 빌드(JG_WITH_EDITOR 0)에서는 경로 모양은 그대로 주되(빈 값이면 CombinePath 가 깨진다) 폴더를 만들지 않고,
	// 함수마다 한 번 오류를 남긴다 — 런타임 코드가 개발 경로에 기대는 곳을 스모크 실행 로그에서 찾게 한다. 에디터 빌드는 늘 false.
	bool isDevelopmentOnlyInPackage(const char* functionName)
	{
#if JG_WITH_EDITOR
		return false;
#else
		static std::mutex reportedMutex;
		static std::unordered_set<HRawString> reportedFunctions;
		bool bFirst = false;
		{
			std::lock_guard<std::mutex> lock(reportedMutex);
			bFirst = reportedFunctions.insert(functionName).second;
		}

		if (bFirst == true)
		{
			JG_LOG(Core, ELogLevel::Error, "HFileHelper::%s is a development path and does not exist in a package", functionName);
		}

		return true;
#endif
	}

#if JG_WITH_EDITOR == 0
	// 패키지 Content 마운트. 폴더 이름은 에셋 경로 토큰(/JGEngine/ · /JGGame/)과 같다.
	void packageContentDirectory(const char* mountName, PString* outPath)
	{
		PString contentDirectory;
		HFileHelper::CombinePath(HFileHelper::ProjectDirectory(), "Content", &contentDirectory);
		HFileHelper::CombinePath(contentDirectory, mountName, outPath);
	}
#endif
}

const PString& HFileHelper::SavedDirectory()
{
	// 저장 · 로그처럼 실행 중에 쓰는 파일. 엔진 단독 <Engine>/Saved, 게임 프로젝트 <Project>/Saved, 패키지 <패키지>/Saved
	// (사용자 결정 D-23 ② — 패키지 폴더 안). 없으면 만든다.
	static PString savedDirectory;
	if (savedDirectory.Empty())
	{
		CombinePath(ProjectDirectory(), "Saved", &savedDirectory);
		if (Exists(savedDirectory) == false)
		{
			CreateDirectory(savedDirectory);
		}
	}

	return savedDirectory;
}

const PString& HFileHelper::EngineDirectory()
{
	static PString enginePath;
	syncString(&enginePath, resolvedRoot(true));

	return enginePath;
}

const PString& HFileHelper::ProjectDirectory()
{
	static PString projectPath;
	syncString(&projectPath, resolvedRoot(false));

	return projectPath;
}

const PString& HFileHelper::ProjectName()
{
	static PString projectName;
	if (GCoreSystem::HasInstance() == true)
	{
		syncString(&projectName, GCoreSystem::GetGlobalValues().ProjectName);
	}

	return projectName;
}

bool HFileHelper::IsProjectMode()
{
	if (GCoreSystem::HasInstance() == false)
	{
		return false;
	}

	return GCoreSystem::GetGlobalValues().bProjectMode;
}

const PString& HFileHelper::ProjectSourceDirectory()
{
	static PString projectSourcePath;
	if (projectSourcePath.Empty())
	{
		CombinePath(ProjectDirectory(), "Source", &projectSourcePath);
		isDevelopmentOnlyInPackage("ProjectSourceDirectory");
	}

	return projectSourcePath;
}

const PString& HFileHelper::ProjectBinDirectory()
{
	static PString projectBinPath;
	if (projectBinPath.Empty())
	{
		CombinePath(ProjectDirectory(), "Bin", &projectBinPath);
		if (Exists(projectBinPath) == false)
		{
			CreateDirectory(projectBinPath);
		}
	}

	return projectBinPath;
}

const PString& HFileHelper::ProjectTempDirectory()
{
	static PString projectTempPath;
	if (projectTempPath.Empty())
	{
		CombinePath(ProjectDirectory(), "Temp", &projectTempPath);
		if (isDevelopmentOnlyInPackage("ProjectTempDirectory") == false && Exists(projectTempPath) == false)
		{
			CreateDirectory(projectTempPath);
		}
	}

	return projectTempPath;
}

const PString& HFileHelper::ProjectCodeGenDirectory()
{
	static PString projectCodeGenPath;
	if (projectCodeGenPath.Empty())
	{
		CombinePath(ProjectTempDirectory(), "CodeGen", &projectCodeGenPath);
		if (isDevelopmentOnlyInPackage("ProjectCodeGenDirectory") == false && Exists(projectCodeGenPath) == false)
		{
			CreateDirectory(projectCodeGenPath);
		}
	}

	return projectCodeGenPath;
}

const PString& HFileHelper::EngineBinDirectory()
{
	static PString engineBinPath;
	if (engineBinPath.Empty())
	{
		CombinePath(EngineDirectory(), "Bin", &engineBinPath);
		if (Exists(engineBinPath) == false)
		{
			CreateDirectory(engineBinPath);
		}
	}

	return engineBinPath;
}

const PString& HFileHelper::EngineBuildDirectory()
{
	static PString engineBuildPath;
	if (engineBuildPath.Empty())
	{
		CombinePath(EngineDirectory(), "Build", &engineBuildPath);
		if (isDevelopmentOnlyInPackage("EngineBuildDirectory") == false && Exists(engineBuildPath) == false)
		{
			CreateDirectory(engineBuildPath);
		}
	}

	return engineBuildPath;
}

const PString& HFileHelper::EngineContentDirectory()
{
	static PString engineContentPath;
	if (engineContentPath.Empty())
	{
#if JG_WITH_EDITOR
		CombinePath(EngineDirectory(), "Content", &engineContentPath);
		if (Exists(engineContentPath) == false)
		{
			CreateDirectory(engineContentPath);
		}
#else
		// 패키지는 <패키지>/Content/JGEngine (= /JGEngine/). 쿠킹이 만든 폴더라 여기서 만들지 않는다.
		packageContentDirectory("JGEngine", &engineContentPath);
#endif
	}

	return engineContentPath;
}

const PString& HFileHelper::EngineConfigDirectory()
{
	static PString engineConfigPath;
	if (engineConfigPath.Empty())
	{
		CombinePath(EngineDirectory(), "Config", &engineConfigPath);
		if (isDevelopmentOnlyInPackage("EngineConfigDirectory") == false && Exists(engineConfigPath) == false)
		{
			CreateDirectory(engineConfigPath);
		}
	}

	return engineConfigPath;
}

const PString& HFileHelper::EngineSourceDirectory()
{
	static PString engineSourcePath;
	if (engineSourcePath.Empty())
	{
		CombinePath(EngineDirectory(), "Source", &engineSourcePath);
		if (isDevelopmentOnlyInPackage("EngineSourceDirectory") == false && Exists(engineSourcePath) == false)
		{
			CreateDirectory(engineSourcePath);
		}
	}

	return engineSourcePath;
}

const PString& HFileHelper::EngineEditorSourceDirectory()
{
	static PString engineEditorSourcePath;
	if (engineEditorSourcePath.Empty())
	{
		CombinePath(EngineSourceDirectory(), "Editor", &engineEditorSourcePath);
		if (isDevelopmentOnlyInPackage("EngineEditorSourceDirectory") == false && Exists(engineEditorSourcePath) == false)
		{
			CreateDirectory(engineEditorSourcePath);
		}
	}

	return engineEditorSourcePath;
}

const PString& HFileHelper::EngineProgramsSourceDirectory()
{
	static PString engineProgramsSourcePath;
	if (engineProgramsSourcePath.Empty())
	{
		CombinePath(EngineSourceDirectory(), "Programs", &engineProgramsSourcePath);
		if (isDevelopmentOnlyInPackage("EngineProgramsSourceDirectory") == false && Exists(engineProgramsSourcePath) == false)
		{
			CreateDirectory(engineProgramsSourcePath);
		}
	}

	return engineProgramsSourcePath;
}

const PString& HFileHelper::EngineRuntimeSourceDirectory()
{
	static PString engineRuntimeSourcePath;
	if (engineRuntimeSourcePath.Empty())
	{
		CombinePath(EngineSourceDirectory(), "Runtime", &engineRuntimeSourcePath);
		if (isDevelopmentOnlyInPackage("EngineRuntimeSourceDirectory") == false && Exists(engineRuntimeSourcePath) == false)
		{
			CreateDirectory(engineRuntimeSourcePath);
		}
	}

	return engineRuntimeSourcePath;
}

const PString& HFileHelper::EngineThirdPartyDirectory()
{
	static PString engineThirdPartyPath;
	if (engineThirdPartyPath.Empty())
	{
		CombinePath(EngineSourceDirectory(), "ThirdParty", &engineThirdPartyPath);
		if (isDevelopmentOnlyInPackage("EngineThirdPartyDirectory") == false && Exists(engineThirdPartyPath) == false)
		{
			CreateDirectory(engineThirdPartyPath);
		}
	}

	return engineThirdPartyPath;
}

const PString& HFileHelper::EngineTempDirectory()
{
	static PString engineTempDirectory;
	if (engineTempDirectory.Empty())
	{
		CombinePath(EngineDirectory(), "Temp", &engineTempDirectory);
		if (isDevelopmentOnlyInPackage("EngineTempDirectory") == false && Exists(engineTempDirectory) == false)
		{
			CreateDirectory(engineTempDirectory);
		}
	}

	return engineTempDirectory;
}

const PString& HFileHelper::EngineCodeGenDirectory()
{
	static PString engineCodeGenDirectory;
	if (engineCodeGenDirectory.Empty())
	{
		CombinePath(EngineTempDirectory(), "CodeGen", &engineCodeGenDirectory);
		if (isDevelopmentOnlyInPackage("EngineCodeGenDirectory") == false && Exists(engineCodeGenDirectory) == false)
		{
			CreateDirectory(engineCodeGenDirectory);
		}
	}

	return engineCodeGenDirectory;
}

const PString& HFileHelper::EngineShaderDirectory()
{
	static PString engineShaderDirectory;
	if (engineShaderDirectory.Empty())
	{
		// 셰이더 템플릿(.hlsl)은 실행 중에 소스 텍스트로 읽는다. 엔진 Content 의 Shaders/ 에 두어 모든 모드가 같은 자리를 본다
		// (에디터 <Engine>/Content/Shaders, 패키지 <Pkg>/Content/JGEngine/Shaders — 쿠킹이 Content 를 통째로 옮긴다). 사용자 결정 D-23 ③, GP-G4.
		// 셰이더는 엔진과 함께 오는 파일이라 없으면 만들지 않는다(빈 폴더를 만들면 "셰이더 없음"이 가려진다).
		CombinePath(EngineContentDirectory(), "Shaders", &engineShaderDirectory);
	}

	return engineShaderDirectory;
}

const PString& HFileHelper::GameContentDirectory()
{
	// 게임 프로젝트의 <Project>/Content. 엔진 단독 실행이면 게임이 없으므로 빈 문자열이다.
	// (엔진 단독은 프로젝트 루트 = 엔진 루트라 같은 폴더를 두 번 올리게 된다)
	static PString gameContentDirectory;
	if (gameContentDirectory.Empty() && IsProjectMode())
	{
#if JG_WITH_EDITOR
		CombinePath(ProjectDirectory(), "Content", &gameContentDirectory);
		if (Exists(gameContentDirectory) == false)
		{
			CreateDirectory(gameContentDirectory);
		}
#else
		// 패키지는 <패키지>/Content/JGGame (= /JGGame/).
		packageContentDirectory("JGGame", &gameContentDirectory);
#endif
	}

	return gameContentDirectory;
}