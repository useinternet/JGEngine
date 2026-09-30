#pragma once

#include "CoreDefines.h"
#include "Platform/PlatformDefines.h"

JG_ENUM_FLAG(ECoreSystemFlags)
enum class ECoreSystemFlags
{
	None = 0x000,
	No_CodeGen = 0x001
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

	// 게임 프로젝트 폴더. 툴이 -project= 로 받은 값을 넘긴다.
	// 비어 있으면 실행 위치 기준 "../../" 에 *.jgproject 가 있는지 보고, 없으면 엔진 단독으로 돈다.
	HRawString ProjectDirectory;

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
public:
	bool bIsRunning;

private:
	GCoreSystem() = default;
	~GCoreSystem() = default;

public:
	static bool Create(const HCoreSystemArguments& args = HCoreSystemArguments());
	static bool Update();
	static void Destroy();
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


