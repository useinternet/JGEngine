#include "PCH/PCH.h"
#include "Module.h"
#include "Log.h"
#include "Platform/Platform.h"
#include "FileIO/Json.h"


void HModuleSystemInfo::WriteJson(PJsonData& json) const
{
	json.AddMember("CodeGenableModuleSet", CodeGenableModuleSet);
}

void HModuleSystemInfo::ReadJson(const PJsonData& json)
{
	json.GetData("CodeGenableModuleSet", &CodeGenableModuleSet);
}

HModuleSystemInfo HModuleSystemInfo::Get()
{
	HModuleSystemInfo sysInfo;
	PString codeGenPath;
	if (codeGenPath.Empty())
	{
		HFileHelper::CombinePath(HFileHelper::EngineCodeGenDirectory(), "module_system_info.json", &codeGenPath);
		if (HFileHelper::Exists(codeGenPath))
		{
			PJson Json;
			PString jsonString;
			if (HFileHelper::ReadAllText(codeGenPath, &jsonString) == true)
			{
				if (PJson::ToObject(jsonString, &Json) == true)
				{
					Json.GetData("module_sys", &sysInfo);
					return sysInfo;
				}
			}
		}
	}

	return sysInfo;
}

const bool HModuleSystemInfo::Set(const HModuleSystemInfo& inSysInfo)
{
	static PString moduleSysInfoJsonPath;
	HFileHelper::CombinePath(HFileHelper::EngineCodeGenDirectory(), "module_system_info.json", &moduleSysInfoJsonPath);

	PJson json;
	json.AddMember("module_sys", inSysInfo);

	PString jsonString;
	if (PJson::ToString(json, &jsonString) == true)
	{
		if (HFileHelper::WriteAllText(moduleSysInfoJsonPath, jsonString) == true)
		{
			return true;
		}
	}

	return false;
}


IModuleInterface* GModuleGlobalSystem::FindModule(const JGType& type) const
{
	HLockGuard<HMutex> lock(_mutex);

	if (_modulesByType.find(type) == _modulesByType.end())
	{
		return nullptr;
	}
	return _modulesByType.at(type);
}

IModuleInterface* GModuleGlobalSystem::FindModule(const PName& moduleName) const
{
	HLockGuard<HMutex> lock(_mutex);

	if (_modulesByName.find(moduleName) == _modulesByName.end())
	{
		return nullptr;
	}

	return _modulesByName.at(moduleName);
}

bool GModuleGlobalSystem::ConnectModule(const PString& moduleName)
{
	if (FindModule(moduleName) != nullptr)
	{
		return true;
	}

	PString dllName = PString::Format("%s.dll", moduleName);;
	PString getTypeFuncName = PString::Format("_Get_%s_Type_", moduleName);
	PString createModuleFuncName = "_Create_Module_Interface_";
	
	HJInstance dllIns = HPlatform::LoadDll(dllName);
	if (dllIns == 0)
	{
		JG_LOG(Core, ELogLevel::Error, "Fail Connect Module:%s", moduleName);
		return false;
	}

	HPlatformFunction<void, GCoreSystem*> linkModuleFunc = HPlatform::LoadFuncInDll<void, GCoreSystem*>(dllIns, "Link_Module");
	if (linkModuleFunc.IsVaild() == false)
	{
		// Error Log
		HPlatform::UnLoadDll(dllIns);
		return false;
	}

	linkModuleFunc(&GCoreSystem::GetInstance());

	HPlatformFunction<IModuleInterface*> createModuleFunc = HPlatform::LoadFuncInDll<IModuleInterface*>(dllIns, createModuleFuncName);
	if (createModuleFunc.IsVaild() == false)
	{
		JG_LOG(Core, ELogLevel::Error, "Fail Connect Module:%s", moduleName);

		HPlatform::UnLoadDll(dllIns);
		return false;
	}

	IModuleInterface* moduleIf = createModuleFunc();
	if (moduleIf == nullptr)
	{
		JG_LOG(Core, ELogLevel::Error, "Fail Connect Module:%s", moduleName);
		HPlatform::UnLoadDll(dllIns);
		return false;
	}

	if (FindModule(moduleIf->GetModuleType()) != nullptr)
	{
		JG_LOG(Core, ELogLevel::Error, "Fail Connect Module:%s", moduleName);

		HPlatform::UnLoadDll(dllIns);
		HPlatform::Deallocate(moduleIf);
		return false;
	}

	{
		HLockGuard<HMutex> lock(_mutex);
		_modulesByType.emplace(moduleIf->GetModuleType(), moduleIf);
		_modulesByName.emplace(PName(moduleName), moduleIf);
	}

	moduleIf->StartupModule();

	{
		HLockGuard<HMutex> lock(_mutex);
		_moduleOrder.push_back(moduleIf);
	}

	GCoreSystem::RegisterDll(dllIns);
	return true;
}

void GModuleGlobalSystem::unregisterModule(IModuleInterface* moduleIf)
{
	if (moduleIf == nullptr)
	{
		return;
	}

	_modulesByType.erase(moduleIf->GetModuleType());

	for (HHashMap<PName, IModuleInterface*>::iterator iter = _modulesByName.begin();
		iter != _modulesByName.end(); ++iter)
	{
		if (iter->second == moduleIf)
		{
			_modulesByName.erase(iter);
			break;
		}
	}

	for (HList<IModuleInterface*>::iterator iter = _moduleOrder.begin();
		iter != _moduleOrder.end(); ++iter)
	{
		if (*iter == moduleIf)
		{
			_moduleOrder.erase(iter);
			break;
		}
	}
}

bool GModuleGlobalSystem::DisconnectModule(const PString& moduleName)
{
	IModuleInterface* moduleIf = nullptr;

	{
		HLockGuard<HMutex> lock(_mutex);

		HHashMap<PName, IModuleInterface*>::iterator iter = _modulesByName.find(PName(moduleName));
		if (iter == _modulesByName.end())
		{
			return true;
		}

		moduleIf = iter->second;
		if (moduleIf == nullptr)
		{
			return false;
		}
	}

	moduleIf->ShutdownModule();

	{
		HLockGuard<HMutex> lock(_mutex);
		unregisterModule(moduleIf);
	}

	GMemoryGlobalSystem::GetInstance().Flush();

	HPlatform::Deallocate(moduleIf);

	return true;
}

bool GModuleGlobalSystem::ReconnectModule(const PString& moduleName)
{
	JG_ASSERT("not impl reconnectModule");
	return true;
}

void GModuleGlobalSystem::Destroy()
{
	HList<IModuleInterface*> shutdownedModules;

	while (true)
	{
		IModuleInterface* moduleIf = nullptr;
		{
			HLockGuard<HMutex> lock(_mutex);
			if (_moduleOrder.empty())
			{
				break;
			}

			moduleIf = _moduleOrder.back();
			if (moduleIf == nullptr)
			{
				_moduleOrder.pop_back();
				continue;
			}
		}

		moduleIf->ShutdownModule();

		{
			HLockGuard<HMutex> lock(_mutex);
			unregisterModule(moduleIf);
		}

		shutdownedModules.push_back(moduleIf);
	}

	GMemoryGlobalSystem::GetInstance().Flush();

	for (IModuleInterface* moduleIf : shutdownedModules)
	{
		HPlatform::Deallocate(moduleIf);
	}

	_modulesByType.clear();
	_modulesByName.clear();
	_moduleOrder.clear();
}