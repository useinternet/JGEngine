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
#include "FileIO/PackageDescriptor.h"
#include "Platform/Platform.h"
#include <crtdbg.h>

namespace
{
	const char* const DEFAULT_ROOT_DIRECTORY = "../../";

#if JG_WITH_EDITOR
	// 기본 루트 = 실행 파일(exe) 위치 기준 ../../ — 작업 폴더와 무관하다(다른 폴더에서 JGConsole 을 실행해도 같은 엔진, DevOps GP-D1).
	// 작업 폴더 기준 ../../ 와 같은 곳이면 지금처럼 상대경로를 그대로 쓴다: 런처는 Bin/<Config> 에서, 배치는 Build/BatchFiles 에서
	// Bin 의 exe 를 부르므로 둘이 같고, 툴이 만드는 생성물(jgengine.lua 등)이 지금과 바이트 같게 남는다.
	HRawString defaultRootDirectory()
	{
		const PString executableDirectory = HPlatform::GetExecutableDirectory();
		if (executableDirectory.Empty() == true)
		{
			return DEFAULT_ROOT_DIRECTORY;
		}

		PString executableRoot;
		HFileHelper::AbsoluteDirectory(executableDirectory + DEFAULT_ROOT_DIRECTORY, &executableRoot);

		PString workingRoot;
		HFileHelper::AbsoluteDirectory(DEFAULT_ROOT_DIRECTORY, &workingRoot);

		if (PString::ToLower(executableRoot) == PString::ToLower(workingRoot))
		{
			return DEFAULT_ROOT_DIRECTORY;
		}

		return executableRoot.GetRawString();
	}

	// 엔진 루트 · 프로젝트 루트를 정한다(에디터 빌드 — JG_WITH_EDITOR 1).
	// 1) 툴: -project= 로 받은 폴더가 프로젝트, 툴이 도는 엔진(실행 파일 위치 기준 ../../)이 엔진
	// 2) 런타임: 실행 파일 위치 기준 ../../ 에 *.jgproject 가 있으면 그곳이 프로젝트, 엔진은 EngineRoot → JGENGINE_ROOT
	// 3) 그 밖: 엔진 단독. 두 루트 모두 실행 파일 위치 기준 "../../" (작업 폴더와 같으면 이전 동작 그대로 상대경로)
	void resolveRootDirectories(const HCoreSystemArguments& args, HCoreSystemGlobalValues* outValues)
	{
		const HRawString defaultRoot = defaultRootDirectory();

		outValues->EngineDirectory  = defaultRoot;
		outValues->ProjectDirectory = defaultRoot;
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

			HFileHelper::AbsoluteDirectory(defaultRoot.c_str(), &engineDirectory);

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
		else if (HProjectDescriptor::Load(defaultRoot.c_str(), &descriptor) == true)
		{
			HFileHelper::AbsoluteDirectory(defaultRoot.c_str(), &projectDirectory);
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
#else
	// 엔진 루트 · 프로젝트 루트를 정한다(패키지 전용 빌드 — JG_WITH_EDITOR 0, ConfirmGame · ReleaseGame).
	// 엔진 루트 = 프로젝트 루트 = 패키지 루트(<Name>.exe 폴더, JGGameLaunch 가 넘김). Content 마운트(Content/JGEngine · Content/JGGame)는 HFileHelper 가 정한다.
	// 패키지 루트가 없거나 Bin/JGPackage.json 을 못 읽으면 bProjectMode 가 false 로 남고 Create 가 게임을 시작하지 않는다.
	void resolveRootDirectories(const HCoreSystemArguments& args, HCoreSystemGlobalValues* outValues)
	{
		outValues->EngineDirectory  = DEFAULT_ROOT_DIRECTORY;
		outValues->ProjectDirectory = DEFAULT_ROOT_DIRECTORY;
		outValues->ProjectName.clear();
		outValues->bProjectMode = false;
		outValues->PackageLaunchModule.clear();
		outValues->PackageRuntimeModules.clear();

		if (args.PackageDirectory.empty() == true)
		{
			JG_LOG(Core, ELogLevel::Critical, "%s is a game package build: start it from the package's <Name>.exe", JG_CONFIGURATIONS_NAME);
			return;
		}

		PString packageDirectory;
		HFileHelper::AbsoluteDirectory(args.PackageDirectory.c_str(), &packageDirectory);

		HPackageDescriptor package;
		if (HPackageDescriptor::Load(packageDirectory, &package) == false)
		{
			PString descriptorPath;
			HPackageDescriptor::FilePath(packageDirectory, &descriptorPath);
			JG_LOG(Core, ELogLevel::Critical, "No valid package descriptor: %s", descriptorPath);
			return;
		}

		outValues->EngineDirectory     = packageDirectory.GetRawString();
		outValues->ProjectDirectory    = packageDirectory.GetRawString();
		outValues->ProjectName         = package.Name.GetRawString();
		outValues->bProjectMode        = true;
		outValues->PackageLaunchModule = package.LaunchModule.GetRawString();
		for (const PString& moduleName : package.RuntimeModules)
		{
			outValues->PackageRuntimeModules.push_back(moduleName.GetRawString());
		}

		JG_LOG(Core, ELogLevel::Info, "Package %s %s (%s, engine %s): %s", package.Name, package.Version, package.BuildConfig, package.EngineCommit, packageDirectory);
	}
#endif
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
	Instance->ExitCode = 0;
	Instance->bExitRequested = false;

	// 로그는 가장 먼저 만든다. 경로가 비면 작업 폴더의 jg_log.txt(지금 동작), 패키지는 Saved/Logs.
	GCoreSystem::RegisterSystemInstance<GLogGlobalSystem>(args.LogFilePath);
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
	Instance->GlobalValues.bRuntimeModulesOnly = EnumHasAnyFlags(args.Flags, ECoreSystemFlags::RuntimeModulesOnly);

	// 패키지 전용 빌드는 패키지를 읽었을 때만 게임을 시작한다. 시스템은 그대로 올리고 내려야 하므로(Destroy 순서)
	// 코드젠 · 실행 모듈만 건너뛰고 Create 가 false(종료 코드 2)를 돌려준다. 에디터 빌드는 늘 true(지금 동작).
	bool bLaunchable = true;
#if JG_WITH_EDITOR == 0
	bLaunchable = Instance->GlobalValues.bProjectMode;
#endif

	if (bLaunchable == true && EnumHasAnyFlags(args.Flags, ECoreSystemFlags::No_CodeGen) == false)
	{
		if (GObjectGlobalSystem::GetInstance().codeGen() == false)
		{
			JG_LOG(Core, ELogLevel::Critical, "Fail ObjectGlobalSystem Code Generation");
		}
	}
	
	HRawString launchModule = args.LaunchModule;
#if JG_WITH_EDITOR == 0
	// 패키지는 실행 진입(JGGameLaunch)이 모듈 이름을 몰라도 되게 JGPackage.json 의 LaunchModule(기본 GameHost)을 쓴다.
	if (launchModule.empty() == true)
	{
		launchModule = Instance->GlobalValues.PackageLaunchModule;
	}
#endif

	if (bLaunchable == true && launchModule.length() > 0)
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule(launchModule.c_str()) == false)
		{
			JG_LOG(Core, ELogLevel::Error, "Fail Launch Module:%s", launchModule.c_str());
		}
	}

	for (GGlobalSystemInstanceBase* SystemInstance : Instance->SystemInstanceList)
	{
		SystemInstance->Start();
	}
	
	// 모듈이 시작하다 RequestExit 했으면(예: GameHost 가 창을 못 만듦) 루프를 돌리지 않는다.
	Instance->bIsRunning = (bLaunchable == true && Instance->bExitRequested == false);
	if (bLaunchable == false)
	{
		Instance->ExitCode = 2;
	}

	return bLaunchable;
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

void GCoreSystem::RequestExit(int32 exitCode)
{
	if (Instance == nullptr)
	{
		return;
	}

	Instance->ExitCode       = exitCode;
	Instance->bExitRequested = true;
	Instance->bIsRunning     = false;
}

int32 GCoreSystem::GetExitCode()
{
	if (Instance == nullptr)
	{
		return 0;
	}

	return Instance->ExitCode;
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
