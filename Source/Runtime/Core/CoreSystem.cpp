#include "PCH/PCH.h"
#include "CoreSystem.h"
#include "Misc/Log.h"
#include "Memory/Memory.h"
#include "Object/ObjectGlobalSystem.h"
#include "Thread/Scheduler.h"
#include "String/StringTable.h"
#include "Misc/Timer.h"
#include "Misc/Module.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "Platform/JWindow.h"
#include "FileIO/FileHelper.h"
#include "FileIO/ProjectDescriptor.h"
#include <crtdbg.h>

namespace
{
	const char* const DEFAULT_ROOT_DIRECTORY = "../../";

	// 엔진 루트 · 프로젝트 루트를 정한다.
	// 1) 툴: -project= 로 받은 폴더가 프로젝트, 툴이 도는 엔진(실행 위치 기준 ../../)이 엔진
	// 2) 런타임: 실행 위치 기준 ../../ 에 *.jgproject 가 있으면 그곳이 프로젝트, 엔진은 EngineRoot → JGENGINE_ROOT
	// 3) 그 밖: 엔진 단독. 두 루트 모두 "../../" (이전 동작 그대로)
	void resolveRootDirectories(const HCoreSystemArguments& args, HCoreSystemGlobalValues* outValues)
	{
		outValues->EngineDirectory  = DEFAULT_ROOT_DIRECTORY;
		outValues->ProjectDirectory = DEFAULT_ROOT_DIRECTORY;
		outValues->ProjectName.clear();
		outValues->bProjectMode = false;

		HProjectDescriptor descriptor;
		PString projectDirectory;
		PString engineDirectory;

		if (args.ProjectDirectory.empty() == false)
		{
			HFileHelper::AbsoluteDirectory(args.ProjectDirectory.c_str(), &projectDirectory);
			if (HProjectDescriptor::Load(projectDirectory, &descriptor) == false)
			{
				JG_LOG(Core, ELogLevel::Critical, "No %s in project directory: %s", HProjectDescriptor::FileExtension, projectDirectory);
				return;
			}

			HFileHelper::AbsoluteDirectory(DEFAULT_ROOT_DIRECTORY, &engineDirectory);

			// 프로젝트가 가리키는 엔진과 지금 도는 툴의 엔진이 다르면 다른 엔진 소스로 솔루션을 만들게 된다.
			PString boundEngineDirectory;
			if (descriptor.ResolveEngineRoot(projectDirectory, &boundEngineDirectory) == true)
			{
				if (PString::ToLower(boundEngineDirectory) != PString::ToLower(engineDirectory))
				{
					JG_LOG(Core, ELogLevel::Critical, "Project %s belongs to engine %s, but this tool is in %s. Use that engine's batch files or fix EngineRoot", descriptor.Name, boundEngineDirectory, engineDirectory);
					return;
				}
			}
			else
			{
				JG_LOG(Core, ELogLevel::Warning, "Project %s has no EngineRoot and %s is not set. The game will not find the engine at run time", descriptor.Name, HProjectDescriptor::EngineRootEnvironmentVariable);
			}
		}
		else if (HProjectDescriptor::Load(DEFAULT_ROOT_DIRECTORY, &descriptor) == true)
		{
			HFileHelper::AbsoluteDirectory(DEFAULT_ROOT_DIRECTORY, &projectDirectory);
			if (descriptor.ResolveEngineRoot(projectDirectory, &engineDirectory) == false)
			{
				JG_LOG(Core, ELogLevel::Critical, "Engine root is unknown. Set EngineRoot in the %s file or the %s environment variable", HProjectDescriptor::FileExtension, HProjectDescriptor::EngineRootEnvironmentVariable);
				return;
			}
		}
		else
		{
			return;
		}

		outValues->EngineDirectory  = engineDirectory.GetRawString();
		outValues->ProjectDirectory = projectDirectory.GetRawString();
		outValues->ProjectName      = descriptor.Name.GetRawString();
		outValues->bProjectMode     = true;

		JG_LOG(Core, ELogLevel::Info, "Project %s: project %s, engine %s", descriptor.Name, projectDirectory, engineDirectory);
	}
}

void HCoreSystemPrivate::SetInstance(GCoreSystem* instance)
{
	GCoreSystem::Instance = instance;
}

HCoreSystemGlobalValues::HCoreSystemGlobalValues()
{

}

GCoreSystem* GCoreSystem::Instance = nullptr;

bool GCoreSystem::Create(const HCoreSystemArguments& args)
{
	if (Instance != nullptr)
	{
		return false;
	}

	Instance = new GCoreSystem;
	Instance->MainThreadID = std::hash<std::thread::id>()(std::this_thread::get_id());

	GCoreSystem::RegisterSystemInstance<GLogGlobalSystem>();
	GCoreSystem::RegisterSystemInstance<GMemoryGlobalSystem>();
	GCoreSystem::RegisterSystemInstance<GTimerGlobalSystem>();
	GCoreSystem::RegisterSystemInstance<GScheduleGlobalSystem>();
	GCoreSystem::RegisterSystemInstance<GStringTable>();
	// 콘솔 명령 레지스트리는 모듈 시스템 앞에 둔다: Update 는 스케줄러(프레임 버킷) 뒤에, Destroy 는 모듈이 모두 해제된 뒤에 온다.
	GCoreSystem::RegisterSystemInstance<GConsoleCommandGlobalSystem>();
	GCoreSystem::RegisterSystemInstance<GModuleGlobalSystem>();
	GCoreSystem::RegisterSystemInstance<GObjectGlobalSystem>();

	Instance->GlobalValues.MainWindow = nullptr;
	Instance->GlobalValues.WindowCallBacks = std::make_unique<HWindowCallBacks>();

	// 경로를 묻는 코드(코드젠 DLL 로드, 모듈 연결)보다 먼저 정한다.
	resolveRootDirectories(args, &Instance->GlobalValues);

	if (EnumHasAnyFlags(args.Flags, ECoreSystemFlags::No_CodeGen) == false)
	{
		if (GObjectGlobalSystem::GetInstance().codeGen() == false)
		{
			JG_LOG(Core, ELogLevel::Critical, "Fail ObjectGlobalSystem Code Generation");
		}
	}
	
	if (args.LaunchModule.length() > 0)
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule(args.LaunchModule.c_str()) == false)
		{
			JG_LOG(Core, ELogLevel::Error, "Fail Launch Module:%s", args.LaunchModule.c_str());
		}
	}

	for (GGlobalSystemInstanceBase* SystemInstance : Instance->SystemInstanceList)
	{
		SystemInstance->Start();
	}
	
	Instance->bIsRunning = true;

	return true;
}
bool GCoreSystem::Update()
{
	for (GGlobalSystemInstanceBase* SystemInstance : Instance->SystemInstanceList)
	{
		SystemInstance->Update();
	}

	return Instance->bIsRunning;
}
void GCoreSystem::Destroy()
{
	if (Instance == nullptr)
	{
		return;
	}

	Instance->GlobalValues.MainWindow = nullptr;
	Instance->GlobalValues.WindowCallBacks = nullptr;
	
	int32 NumSystem = (int32)Instance->SystemInstanceList.size();
	for (int32 i = NumSystem - 1; i >= 0; --i)
	{
		Instance->SystemInstanceList[i]->Destroy();
	}

	GMemoryGlobalSystem::GetInstance().Flush();

	GCoreSystem::UnRegisterSystemInstance<GStringTable>();
	GCoreSystem::UnRegisterSystemInstance<GConsoleCommandGlobalSystem>();
	GCoreSystem::UnRegisterSystemInstance<GObjectGlobalSystem>();
	GCoreSystem::UnRegisterSystemInstance<GModuleGlobalSystem>();
	GCoreSystem::UnRegisterSystemInstance<GScheduleGlobalSystem>();
	GCoreSystem::UnRegisterSystemInstance<GTimerGlobalSystem>();
	GCoreSystem::UnRegisterSystemInstance<GMemoryGlobalSystem>();
	GCoreSystem::UnRegisterSystemInstance<GLogGlobalSystem>();

	

	for (HJInstance dllIns : Instance->DllInstances)
	{
		HPlatform::UnLoadDll(dllIns);
	}

	Instance->DllInstances.clear();
	Instance->SystemInstancePool.clear();

	delete Instance;
	Instance = nullptr;

#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // _DEBUG
}

HCoreSystemGlobalValues& GCoreSystem::GetGlobalValues()
{
	return Instance->GlobalValues;
}

GCoreSystem& GCoreSystem::GetInstance()
{
	return *Instance;
}

void GCoreSystem::RegisterDll(HJInstance InInstance)
{
	Instance->DllInstances.insert(InInstance);
}

void GCoreSystem::UnregisterDll(HJInstance InInstance)
{
	Instance->DllInstances.erase(InInstance);
}

uint32 GCoreSystem::GetThreadCount()
{
	static uint32  threadCount = std::thread::hardware_concurrency();
	return threadCount;
}

ThreadID GCoreSystem::GetMainThreadID()
{
	return Instance->MainThreadID;
}
