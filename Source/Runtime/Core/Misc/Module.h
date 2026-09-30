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

	// 이 모듈 DLL 에 선언된 HAutoConsoleCommand 를 등록/해제한다. GModuleGlobalSystem 이 StartupModule 직후 / ShutdownModule 직전에 부른다.
	// 모듈 객체는 자기 DLL 에서 만들어지므로(_Create_Module_Interface_) 이 가상 호출은 그 DLL 의 Core 사본으로 가고, 그 DLL 의 목록을 본다.
	// 모듈이 재정의할 일은 없다.
	virtual void RegisterAutoConsoleCommands();
	virtual void UnregisterAutoConsoleCommands();
};

class PJsonData;
struct HModuleSystemInfo : public IJsonable
{
	HHashSet<PString> CodeGenableModuleSet;

	virtual void WriteJson(PJsonData& json) const;
	virtual void ReadJson(const PJsonData& json);

	static HModuleSystemInfo Get();
	static const bool Set(const HModuleSystemInfo& inSysInfo);
};

class GModuleGlobalSystem : public GGlobalSystemInstance<GModuleGlobalSystem>
{
	HHashMap<PName, IModuleInterface*>  _modulesByName;
	HHashMap<JGType, IModuleInterface*> _modulesByType;
	HList<IModuleInterface*> _moduleOrder;

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

private:
	// _modulesByType / _modulesByName / _moduleOrder 에서 모듈을 제거한다.
	// _mutex 를 이미 잡은 상태에서 호출할 것. (자체적으로 잠그지 않는다)
	void unregisterModule(IModuleInterface* moduleIf);
};