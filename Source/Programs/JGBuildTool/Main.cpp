#include "PCH/PCH.h"
#include "Core.h"
#include "Class/BuildTool.h"
#include "Class/ProjectCreator.h"
#include <crtdbg.h>

// 사용법 (작업 폴더는 엔진의 Build/BatchFiles)
//   JGBuildTool.exe                                  엔진 솔루션 생성 (PreBuild.bat)
//   JGBuildTool.exe -project=<ProjectDir>            게임 프로젝트 솔루션 생성 (GenerateGameProjectFiles.bat)
//   JGBuildTool.exe -newproject=<ProjectDir> [-name=<Name>]   게임 프로젝트 만들기 (CreateGameProject.bat)

bool ReadArguments(PArguments* outArguments);
void CreateModuleInfoTemplate();

namespace
{
	// "-key=value" 형식 인자에서 value 를 찾는다. 따옴표는 셸이 벗겨서 넘긴다.
	bool findArgument(int32 argc, char* argv[], const HRawString& key, HRawString* outValue)
	{
		for (int32 i = 1; i < argc; ++i)
		{
			HRawString argument = argv[i];
			if (argument.rfind(key, 0) == 0)
			{
				*outValue = argument.substr(key.length());
				return true;
			}
		}

		return false;
	}

	// 배치에서 도는 툴이라 오류 창을 띄우지 않고 종료 코드로 알린다.
	// Debug CRT 는 처리되지 않은 예외에서 abort 대화상자를 띄워 배치를 무한 대기시킨다.
	void disableErrorDialogs()
	{
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	}

	// 스택 객체(PArguments · PBuildTool)가 GCoreSystem::Destroy 전에 소멸하도록 함수로 분리한다.
	// (main 스코프에 두면 메모리 시스템이 내려간 뒤 소멸해 종료 시 세그폴트가 난다)
	int32 runBuildTool()
	{
		PArguments args;
		if (ReadArguments(&args) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Fail read arguments");
			return 1;
		}

		if (HFileHelper::IsProjectMode() == true)
		{
			// 게임 프로젝트는 Source 바로 아래 폴더가 모듈이다. Source 를 카테고리로 보고 훑는다.
			args.UserWorkDirectory = HFileHelper::ProjectDirectory();
			args.UserWorkCategories.clear();
			args.UserWorkCategories.insert("Source");
		}
		else
		{
			CreateModuleInfoTemplate();
		}

		PBuildTool buildTool(args);
		if (buildTool.Run() == false)
		{
			return 1;
		}

		return 0;
	}
}

int32 main(int32 argc, char* argv[])
{
	disableErrorDialogs();

	HRawString projectDirectory;
	HRawString newProjectDirectory;
	HRawString newProjectName;
	findArgument(argc, argv, "-project=", &projectDirectory);
	findArgument(argc, argv, "-newproject=", &newProjectDirectory);
	findArgument(argc, argv, "-name=", &newProjectName);

	HCoreSystemArguments coreSystemArgs;
	coreSystemArgs.Flags = ECoreSystemFlags::No_CodeGen;
	coreSystemArgs.ProjectDirectory = projectDirectory;

	GCoreSystem::Create(coreSystemArgs);

	int32 result = 1;
	try
	{
		if (newProjectDirectory.empty() == false)
		{
			if (PProjectCreator::Create(newProjectDirectory.c_str(), newProjectName.c_str()) == true)
			{
				result = 0;
			}
		}
		else if (projectDirectory.empty() == false && HFileHelper::IsProjectMode() == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Not a usable game project: %s", projectDirectory.c_str());
		}
		else
		{
			result = runBuildTool();
		}
	}
	catch (const std::exception& exception)
	{
		JG_LOG(BuildTool, ELogLevel::Critical, "Unhandled exception: %s", exception.what());
		result = 1;
	}

	GCoreSystem::Destroy();

	return result;
}

bool ReadArguments(PArguments* outArguments)
{
	if (outArguments == nullptr)
	{
		JG_LOG(BuildTool, ELogLevel::Critical, "Arguments is nullptr");
		return false;
	}

	PString buildToolSourcePath = PBuildTool::BuildToolDirectory();

	PString argumentsJsonPath;
	HFileHelper::CombinePath(buildToolSourcePath, PBuildTool::ARGUMENTS_JSON_FILE_NAME, &argumentsJsonPath);

	PString argumentsJsonText;
	if (HFileHelper::ReadAllText(argumentsJsonPath, &argumentsJsonText) == false)
	{
		// Create Default Json File
		PJson json;
		json.AddMember("Arguments", PArguments());

		PString jsonText;
		if (PJson::ToString(json, &jsonText) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Fail read json file");
			return false;
		}

		argumentsJsonText = jsonText;

		if (HFileHelper::WriteAllText(argumentsJsonPath, argumentsJsonText) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "Fail write default arguments json file");
			return false;
		}
	}

	PJson json;
	if (PJson::ToObject(argumentsJsonText, &json) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Critical, "Fail text to json object");
		return false;
	}

	if (json.GetData<PArguments>("Arguments", outArguments) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Critical, "Fail getData in json");
		return false;
	}

	return true;
}

void CreateModuleInfoTemplate()
{
	PString buildToolSourcePath;
	HFileHelper::CombinePath(HFileHelper::EngineProgramsSourceDirectory(), "JGBuildTool", &buildToolSourcePath);

	PString moduleInfoTemplatePath;
	HFileHelper::CombinePath(buildToolSourcePath, "ModuleInfoTemplate.json", &moduleInfoTemplatePath);

	PModuleInfo info;
	info.ModuleFormat = "SharedLib/StaticLib/ConsoleApp";
	info.ModuleDependencies.push_back("Module_A");
	info.ModuleDependencies.push_back("Module_B");

	info.ModuleName = "Name";
	info.ModulePath = "Source/[Category]/[ModuleName]";

	info.Defines.push_back("DEFINE_A");
	info.Defines.push_back("DEFINE_B");

	info.ModuleFilters[(int32)EModuleFilter::DevelopEngine].Config = "DebugConfig/ConfirmConfig/ReleaseConfig";
	info.ModuleFilters[(int32)EModuleFilter::DevelopEngine].Defines.push_back("DEFINE_A");

	info.ModuleFilters[(int32)EModuleFilter::DevelopGame].Config = "DebugConfig/ConfirmConfig/ReleaseConfig";
	info.ModuleFilters[(int32)EModuleFilter::DevelopGame].Defines.push_back("DEFINE_A");

	info.ModuleFilters[(int32)EModuleFilter::ConfirmGame].Config = "DebugConfig/ConfirmConfig/ReleaseConfig";
	info.ModuleFilters[(int32)EModuleFilter::ConfirmGame].Defines.push_back("DEFINE_A");

	info.ModuleFilters[(int32)EModuleFilter::ReleaseGame].Config = "DebugConfig/ConfirmConfig/ReleaseConfig";
	info.ModuleFilters[(int32)EModuleFilter::ReleaseGame].Defines.push_back("DEFINE_A");

	PJson json;
	json.AddMember("ModuleInfo", info);

	PString jsonText;
	PJson::ToString(json, &jsonText);

	HFileHelper::WriteAllText(moduleInfoTemplatePath, jsonText);
}
