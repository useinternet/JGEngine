#pragma once
#pragma warning(disable : 4251 4275)

#include "CoreDefines.h"
#include "CoreSystem.h"
#include "Object/ObjectGlobals.h"

#define JG_MODULE_IMPL(ModuleName, APIDefine) \
APIDefine IModuleInterface* _Create_Module_Interface_()\
{\
	return HPlatform::Allocate<##ModuleName>();\
}\
APIDefine void Link_Module(GCoreSystem* ins)\
{ \
HCoreSystemPrivate::SetInstance(ins); \
} \


class IModuleInterface
{
	friend class GModuleGlobalSystem;
public:
	virtual ~IModuleInterface() = default;
protected:
// 시작/끝 함수
	virtual JGType GetModuleType() const = 0;
	virtual void StartupModule()  = 0;
	virtual void ShutdownModule() = 0;
};

class PJsonData;
struct HModuleSystemInfo : public IJsonable
{
	HHashSet<PString> CodeGenableModuleSet;

	virtual void WriteJson(PJsonData& json) const;
	virtual void ReadJson(const PJsonData& json);

	static const HModuleSystemInfo& Get();
	static const bool Set(const HModuleSystemInfo& inSysInfo);
};

class GModuleGlobalSystem : public GGlobalSystemInstance<GModuleGlobalSystem>
{
	HHashMap<PName, IModuleInterface*>  _modulesByName;
	HHashMap<JGType, IModuleInterface*> _modulesByType;

	mutable HMutex _mutex;
public:
	virtual ~GModuleGlobalSystem() = default;

public:
	template<class T>
	T* FindModule() const
	{
		return static_cast<T*>(FindModule(JGTYPE(T)));
	}

	IModuleInterface* FindModule(const JGType& type) const;
	IModuleInterface* FindModule(const PName& moduleName) const;

	bool ConnectModule(const PString& moduleName);
	bool DisconnectModule(const PString& moduleName);
	bool ReconnectModule(const PString& moduleName);

protected:
	virtual void Destroy() override;
};