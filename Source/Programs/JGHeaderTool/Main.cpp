#include "PCH/PCH.h"


//
#include "Core.h"
#include "Class/Arguments.h"
#include "Class/HeaderTool.h"
#include <iostream>
#include <crtdbg.h>

// 사용법 (작업 폴더는 엔진의 Build/BatchFiles)
//   JGHeaderTool.exe                        엔진 모듈 코드젠 → 엔진 Temp/CodeGen (PreBuild.bat)
//   JGHeaderTool.exe -project=<ProjectDir>  게임 모듈 코드젠 → 프로젝트 Temp/CodeGen (GenerateGameProjectFiles.bat)

bool ReadArguments(PArguments* outArguments);

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

	// 스택 객체(PArguments · PHeaderTool)가 GCoreSystem::Destroy 전에 소멸하도록 함수로 분리한다.
	// (main 스코프에 두면 메모리 시스템이 내려간 뒤 소멸해 종료 시 세그폴트가 난다)
	int32 runHeaderTool()
	{
		PArguments args;
		if (ReadArguments(&args) == false)
		{
			JG_LOG(HeaderTool, ELogLevel::Critical, "Fail read arguments");
			return 1;
		}

		if (HFileHelper::IsProjectMode() == true)
		{
			// 게임 프로젝트는 Source 바로 아래 폴더가 모듈이다. Source 를 카테고리로 보고 훑는다.
			args.UserWorkDirectory = HFileHelper::ProjectDirectory();
			args.UserWorkCategories.clear();
			args.UserWorkCategories.insert("Source");
		}

		PHeaderTool headerTool(args);
		if (headerTool.Run() == false)
		{
			return 1;
		}

		return 0;
	}
}

int main(int argc, char* argv[])
{
	disableErrorDialogs();

	HRawString projectDirectory;
	findArgument(argc, argv, "-project=", &projectDirectory);

	HCoreSystemArguments coreSystemArgs;
	coreSystemArgs.Flags = ECoreSystemFlags::No_CodeGen;
	coreSystemArgs.ProjectDirectory = projectDirectory;

	GCoreSystem::Create(coreSystemArgs);

	int32 result = 1;
	try
	{
		if (projectDirectory.empty() == false && HFileHelper::IsProjectMode() == false)
		{
			JG_LOG(HeaderTool, ELogLevel::Critical, "Not a usable game project: %s", projectDirectory.c_str());
		}
		else
		{
			result = runHeaderTool();
		}
	}
	catch (const std::exception& exception)
	{
		JG_LOG(HeaderTool, ELogLevel::Critical, "Unhandled exception: %s", exception.what());
		result = 1;
	}

	GCoreSystem::Destroy();
	return result;
}

bool ReadArguments(PArguments* outArguments)
{
	if (outArguments == nullptr)
	{
		JG_LOG(HeaderTool, ELogLevel::Critical, "Arguments is nullptr");
		return false;
	}

	PString headerToolSourcePath = PHeaderTool::HeaderToolDirectory();

	PString argumentsJsonPath;
	HFileHelper::CombinePath(headerToolSourcePath, PHeaderTool::ARGUMENTS_JSON_FILE_NAME, &argumentsJsonPath);

	PString argumentsJsonText;
	if (HFileHelper::ReadAllText(argumentsJsonPath, &argumentsJsonText) == false)
	{
		// Create Default Json File
		PJson json;
		json.AddMember("Arguments", PArguments());

		PString jsonText;
		if (PJson::ToString(json, &jsonText) == false)
		{
			JG_LOG(HeaderTool, ELogLevel::Critical, "Fail read json file");
			return false;
		}

		argumentsJsonText = jsonText;

		if (HFileHelper::WriteAllText(argumentsJsonPath, argumentsJsonText) == false)
		{
			JG_LOG(HeaderTool, ELogLevel::Critical, "Fail write default arguments json file");
			return false;
		}
	}

	PJson json;
	if (PJson::ToObject(argumentsJsonText, &json) == false)
	{
		JG_LOG(HeaderTool, ELogLevel::Critical, "Fail text to json object");
		return false;
	}

	if (json.GetData("Arguments", outArguments) == false)
	{
		JG_LOG(HeaderTool, ELogLevel::Critical, "Fail getData in json");
		return false;
	}

	return true;
}
