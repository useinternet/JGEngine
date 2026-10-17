#pragma once

#include "CoreDefines.h"
#include "Platform/PlatformDefines.h"

JG_ENUM_FLAG(ECoreSystemFlags)
enum class ECoreSystemFlags
{
	None = 0x000,
	No_CodeGen = 0x001,
	// 개발 실행에서 게임만(JGLauncher -game): codeGen 이 runtime_module_info.json 의 런타임 모듈만 올린다 — 에디터 DLL 을 로드하지 않는다(GP-D5 · R5).
	// 파일이 없으면(배치 2 를 다시 안 돌린 프로젝트) 경고하고 전체 목록(module_system_info.json)으로 돈다.
	RuntimeModulesOnly = 0x002
};

class GGlobalSystemInstanceBase;
class GCoreSystem;
class PJGGraphicsAPI;
class PJWindow;
class IGUIBuild;
struct HWindowCallBacks;

namespace HCoreSystemPrivate
{
	void SetInstance(GCoreSystem* instance);
}

struct HCoreSystemArguments
{
	ECoreSystemFlags Flags;
	HRawString LaunchModule;

	// 게임 프로젝트 폴더. 툴이 -project= 로 받은 값을 넘긴다(JG_WITH_EDITOR 1 빌드).
	// 비어 있으면 실행 파일 위치 기준 "../../" 에 *.jgproject 가 있는지 보고, 없으면 엔진 단독으로 돈다.
	HRawString ProjectDirectory;

	// 게임 패키지 루트(<Name>.exe 가 있는 폴더). JG_WITH_EDITOR 0 빌드(패키지 전용)에서만 쓴다 — <루트>/Bin/JGPackage.json 을 읽고
	// 루트 · Content 마운트 · 모듈 목록을 패키지 구조로 잡는다. 비어 있으면 시작하지 않는다. JGGameLaunch 가 넘긴다(DevOps GP-D1, 2026-10-02).
	HRawString PackageDirectory;

	// 로그 파일 경로. 비어 있으면 작업 폴더의 jg_log.txt(툴 · 에디터 — 지금 동작). 패키지는 <루트>/Saved/Logs/<Name>.log.
	HRawString LogFilePath;

	HCoreSystemArguments() : Flags(ECoreSystemFlags::None) {}
};

struct HCoreSystemGlobalValues
{
	PJWindow*		MainWindow;
	HSTLSharedPtr<HWindowCallBacks> WindowCallBacks;

	// Create 가 한 번 정하는 루트. 모든 모듈(DLL)이 같은 값을 보도록 여기에 둔다. 읽는 쪽은 HFileHelper.
	HRawString EngineDirectory;
	HRawString ProjectDirectory;
	HRawString ProjectName;
	bool       bProjectMode = false;

	// JG_WITH_EDITOR 0 빌드(패키지 전용)에서 JGPackage.json 이 채운다. 엔진 루트 = 프로젝트 루트 = 패키지 루트(위 두 값), bProjectMode 는
	// 서술자를 읽었을 때만 true. 모듈 목록은 엔진 컨테이너(HList)가 아닌 STL 이다: GCoreSystem 이 메모리 시스템보다 오래 산다.
	HRawString              PackageLaunchModule;    // JGPackage.json LaunchModule. HCoreSystemArguments::LaunchModule 이 비면 이것을 연결한다
	std::vector<HRawString> PackageRuntimeModules;  // JGPackage.json RuntimeModules. 시작할 때 codeGen 이 올린다

	// ECoreSystemFlags::RuntimeModulesOnly (에디터 빌드의 게임만 실행)
	bool bRuntimeModulesOnly = false;

	HCoreSystemGlobalValues();
};

class GCoreSystem
{
	friend void HCoreSystemPrivate::SetInstance(GCoreSystem* instance);
private:
	static GCoreSystem* Instance;
	std::unordered_map<uint64, GGlobalSystemInstanceBase*> SystemInstancePool;
	std::vector<GGlobalSystemInstanceBase*> SystemInstanceList;
	std::unordered_set<HJInstance> DllInstances;
	HCoreSystemGlobalValues GlobalValues;

	ThreadID MainThreadID;
	int32    ExitCode = 0;
	bool     bExitRequested = false;   // RequestExit 가 불렸다(Create 도중 모듈 시작 실패 포함 — Create 가 bIsRunning 을 다시 켜지 않는다)
public:
	bool bIsRunning;

private:
	GCoreSystem() = default;
	~GCoreSystem() = default;

public:
	static bool Create(const HCoreSystemArguments& args = HCoreSystemArguments());
	static bool Update();
	static void Destroy();

	// 메인 루프를 끝낸다(다음 Update 가 false). exitCode 는 프로세스 종료 코드 — JGGameLaunch 가 그대로 돌려준다.
	static void  RequestExit(int32 exitCode = 0);
	static int32 GetExitCode();
	static HCoreSystemGlobalValues& GetGlobalValues();
	static GCoreSystem& GetInstance();
	static bool HasInstance()
	{
		return Instance != nullptr;
	}

	template<class T, class ...Args>
	static void RegisterSystemInstance(Args... args)
	{
		if (IsValidSystemInstance<T>() == true)
		{
			return;
		}

		uint64 code = getTypeHashCode<T>();
		Instance->SystemInstancePool.emplace(code, new T(args...));
		Instance->SystemInstanceList.push_back(Instance->SystemInstancePool[code]);
	}
	template<class T>
	static void UnRegisterSystemInstance()
	{
		if (IsValidSystemInstance<T>() == false)
		{
			return;
		}

		uint64 code = getTypeHashCode<T>();
		GGlobalSystemInstanceBase*& instance = Instance->SystemInstancePool[code];

		int32 instanceIndex = INDEX_NONE;
		int32 numSystem = (int32)Instance->SystemInstanceList.size();
		for (int32 i = 0; i < numSystem; ++i)
		{
			if (instance == Instance->SystemInstanceList[i])
			{
				instanceIndex = i;
				break;
			}
		}

		delete instance;
		instance = nullptr;

		Instance->SystemInstancePool.erase(code);
		if (instanceIndex != INDEX_NONE)
		{
			Instance->SystemInstanceList.erase(Instance->SystemInstanceList.begin() + instanceIndex);
		}
	}
	template<class T>
	static T* GetSystemInstance()
	{
		if (IsValidSystemInstance<T>() == false)
		{
			return nullptr;
		}
		uint64 code = getTypeHashCode<T>();
		return static_cast<T*>(Instance->SystemInstancePool[code]);
	}

	template<class T>
	static bool IsValidSystemInstance()
	{
		if (Instance == nullptr)
		{
			return false;
		}

		uint64 code = getTypeHashCode<T>();
		return Instance->SystemInstancePool.find(code) != Instance->SystemInstancePool.end();
	}

	static void RegisterDll(HJInstance InInstance);
	static void UnregisterDll(HJInstance InInstance);

	static uint32 GetThreadCount();
	static ThreadID GetMainThreadID();

private:
	template<class T>
	static uint64 getTypeHashCode()
	{
		return typeid(T).hash_code();
	}
};

class GGlobalSystemInstanceBase
{
	friend GCoreSystem;
protected:
	GGlobalSystemInstanceBase() = default;
	virtual ~GGlobalSystemInstanceBase() = default;
	
protected:
	virtual void Start()   {}
	virtual void Update()  {}
	virtual void Destroy() {}
};

template<class T>
class GGlobalSystemInstance : public GGlobalSystemInstanceBase
{
	friend GCoreSystem;

protected:
	GGlobalSystemInstance() = default;
	virtual ~GGlobalSystemInstance() = default;

public:
	static T& GetInstance()
	{
		T* instance = GCoreSystem::GetSystemInstance<T>();
		return *instance;
	}
	static bool IsValid()
	{
		return GCoreSystem::IsValidSystemInstance<T>();
	}
};


