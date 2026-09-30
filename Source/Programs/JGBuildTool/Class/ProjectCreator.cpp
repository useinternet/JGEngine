#include "PCH/PCH.h"
#include "ProjectCreator.h"
#include "FileIO/ProjectDescriptor.h"

namespace
{
	struct HProjectTokens
	{
		PString ProjectName;
		PString ProjectNameUpper;
		PString EngineRoot;
		PString EngineRootWindows;

		PString Apply(const PString& text) const
		{
			PString result = text;
			result.ReplaceAll("{PROJECT_NAME_UPPER}", ProjectNameUpper);
			result.ReplaceAll("{PROJECT_NAME}", ProjectName);
			result.ReplaceAll("{ENGINE_ROOT_WIN}", EngineRootWindows);
			result.ReplaceAll("{ENGINE_ROOT}", EngineRoot);
			return result;
		}
	};

	// 이름이 폴더 · 모듈 · DLL · 클래스(H<Name>Module) 이름이 되므로 C++ 식별자여야 한다.
	bool isValidProjectName(const PString& projectName)
	{
		const HRawString& name = projectName.GetRawString();
		if (name.empty() == true)
		{
			return false;
		}

		for (size_t i = 0; i < name.size(); ++i)
		{
			const unsigned char c = (unsigned char)name[i];
			const bool bLetter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
			const bool bDigit  = c >= '0' && c <= '9';
			if (bLetter == false && (i == 0 || bDigit == false))
			{
				return false;
			}
		}

		return true;
	}

	// 엔진 모듈과 이름이 같으면 DLL 이 같은 Bin 에서 부딪히고 premake 가 두 프로젝트를 하나로 합친다.
	// JG 로 시작하는 이름은 엔진 몫이다. <Name>Editor 가 JGEditor 가 되는 경우("JG")도 이것으로 막힌다.
	bool isReservedProjectName(const PString& projectName)
	{
		if (PString::ToUpper(projectName).StartWidth("JG") == true)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Project names starting with JG are reserved for engine modules: %s", projectName);
			return true;
		}

		static const char* const categories[] = { "Runtime", "Editor", "Programs" };
		for (const char* category : categories)
		{
			PString categoryDirectory;
			HFileHelper::CombinePath(HFileHelper::EngineSourceDirectory(), category, &categoryDirectory);

			PString moduleDirectory;
			HFileHelper::CombinePath(categoryDirectory, projectName, &moduleDirectory);
			if (HFileHelper::IsDirectory(moduleDirectory) == true)
			{
				JG_LOG(BuildTool, ELogLevel::Critical, "%s is an engine module name (Source/%s/%s). Choose another project name", projectName, category, projectName);
				return true;
			}
		}

		return false;
	}

	bool createDirectories(const PString& directory)
	{
		std::error_code errorCode;
		fs::create_directories(directory.GetRawString(), errorCode);
		if (errorCode.value() != 0)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Fail create directory %s: %s", directory, errorCode.message().c_str());
			return false;
		}

		return true;
	}
}

bool PProjectCreator::Create(const PString& inProjectDirectory, const PString& inProjectName)
{
	PString projectDirectory;
	HFileHelper::AbsoluteDirectory(inProjectDirectory, &projectDirectory);

	PString projectName = inProjectName;
	if (projectName.Empty() == true)
	{
		PString directoryWithoutSlash = projectDirectory;
		directoryWithoutSlash.PopBack();
		HFileHelper::FileName(directoryWithoutSlash, &projectName);
	}

	if (isValidProjectName(projectName) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Critical, "Project name must be a C++ identifier (ASCII letters, digits, _ and not starting with a digit): %s", projectName);
		return false;
	}

	if (isReservedProjectName(projectName) == true)
	{
		return false;
	}

	if (HFileHelper::IsDirectory(projectDirectory) == true)
	{
		HList<PString> descriptorFiles;
		HFileHelper::FileListInDirectory(projectDirectory, &descriptorFiles, false, { HProjectDescriptor::FileExtension });
		if (descriptorFiles.empty() == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Already a game project: %s", descriptorFiles[0]);
			return false;
		}
	}

	PString templateDirectory;
	HFileHelper::CombinePath(HFileHelper::EngineBuildDirectory(), "Templates/GameProject", &templateDirectory);
	HFileHelper::AbsoluteDirectory(templateDirectory, &templateDirectory);
	if (HFileHelper::IsDirectory(templateDirectory) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Critical, "Template not found: %s", templateDirectory);
		return false;
	}

	PString engineRoot;
	HFileHelper::AbsoluteDirectory(HFileHelper::EngineDirectory(), &engineRoot);
	engineRoot.PopBack();

	HProjectTokens tokens;
	tokens.ProjectName       = projectName;
	tokens.ProjectNameUpper  = PString::ToUpper(projectName);
	tokens.EngineRoot        = engineRoot;
	tokens.EngineRootWindows = PString::ReplaceAll(engineRoot, "/", "\\");

	if (createDirectories(projectDirectory) == false)
	{
		return false;
	}

	HList<PString> templateEntries;
	HFileHelper::FileListInDirectory(templateDirectory, &templateEntries, true);

	for (const PString& templateEntry : templateEntries)
	{
		PString relativePath = templateEntry;
		relativePath.Remove(0, templateDirectory.Length());

		const PString targetPath = projectDirectory + tokens.Apply(relativePath);

		if (HFileHelper::IsDirectory(templateEntry) == true)
		{
			if (createDirectories(targetPath) == false)
			{
				return false;
			}
			continue;
		}

		PString targetDirectory;
		HFileHelper::FilePathOnly(targetPath, &targetDirectory);
		if (createDirectories(targetDirectory) == false)
		{
			return false;
		}

		PString text;
		if (HFileHelper::ReadAllText(templateEntry, &text) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Fail read template file %s", templateEntry);
			return false;
		}

		if (HFileHelper::WriteAllText(targetPath, tokens.Apply(text)) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Fail write %s", targetPath);
			return false;
		}
	}

	static const char* const emptyDirectories[] = { "Bin", "Content", "Temp" };
	for (const char* directory : emptyDirectories)
	{
		if (createDirectories(projectDirectory + directory) == false)
		{
			return false;
		}
	}

	JG_LOG(BuildTool, ELogLevel::Info, "Created game project %s: %s", projectName, projectDirectory);
	JG_LOG(BuildTool, ELogLevel::Info, "Next: run %sGenerateProjectFiles.bat", projectDirectory);
	return true;
}
