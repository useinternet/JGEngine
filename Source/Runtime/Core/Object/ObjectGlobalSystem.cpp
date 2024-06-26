#include "PCH/PCH.h"
#include "ObjectGlobalSystem.h"
#include "ObjectGlobals.h"
#include "Misc/Log.h"
#include "Platform/Platform.h"
#include "Misc/Module.h"

void GObjectGlobalSystem::Start()
{
	JG_LOG(Core, ELogLevel::Info, "Recognized Class List...");
	for (const HPair<const JGType, PSharedPtr<JGClass>> pair : _classMap)
	{
		PSharedPtr<JGClass> Class = pair.second;
		if (Class == nullptr)
		{
			continue;
		}

		for (JGType ParentType : Class->ParentTypeSet)
		{
			PSharedPtr<JGClass> ParentClass = GetStaticClass(ParentType);
			if (ParentClass == nullptr)
			{
				continue;
			}

			ParentClass->ChildTypeSet.insert(pair.first);
		}

		JG_LOG(Core, ELogLevel::Debug, "- %s", pair.first.GetName().ToString());
	}
}

void GObjectGlobalSystem::Destroy()
{
	_enumMap.clear();
	_classMap.clear();
	_typeMap.clear();
}

PSharedPtr<JGEnum> GObjectGlobalSystem::GetStaticEnum(const JGType& type) const
{
	if (_enumMap.find(type) == _enumMap.end())
	{
		return nullptr;
	}

	return _enumMap.at(type);
}

PSharedPtr<JGClass> GObjectGlobalSystem::GetStaticClass(const JGType& type) const
{
	if (_classMap.find(type) == _classMap.end())
	{
		return nullptr;
	}

	return _classMap.at(type);
}

PSharedPtr<JGClass> GObjectGlobalSystem::GetClass(const JGType& type, const JGObject* object) const
{
	if (object == nullptr)
	{
		return nullptr;
	}

	if (type != object->GetType())
	{
		return nullptr;
	}

	if (_createClassFuncPool.find(type) == _createClassFuncPool.end())
	{
		return nullptr;
	}

	HCreateClassFunc Func = _createClassFuncPool.at(type);

	return Func(object);
}

const JGType& GObjectGlobalSystem::GetType(const PName& typeName) const
{
	if (_typeMap.contains(typeName) == false)
	{
		static JGType nullType;
		return nullType;
	}

	return _typeMap.at(typeName);
}

PSharedPtr<JGObject> GObjectGlobalSystem::NewObject(const JGType& type) const
{
	if (_createObjectFuncPool.contains(type) == false)
	{
		return nullptr;
	}

	if (_createObjectFuncPool.at(type).IsVaild() == false)
	{
		return nullptr;
	}

	HCreateObjectFunc func = _createObjectFuncPool.at(type);

	return func();
}

bool GObjectGlobalSystem::CanCast(const JGType& destType, const JGType& srcType) const
{
	if (canCastInternal(destType, srcType) == true)
	{
		return true;
	}

	if (canCastInternal(srcType, destType) == true)
	{
		return true;
	}

	return false;
}

bool GObjectGlobalSystem::RegisterJGClass(PSharedPtr<JGClass> classObject, const HCreateClassFunc& func, const HCreateObjectFunc& createObjectFunc)
{
	if (classObject.IsValid() == false)
	{
		return false;
	}

	PSharedPtr<JGType> classType = classObject->GetClassType();
	if (classType.IsValid() == false)
	{
		JG_LOG(Core, ELogLevel::Critical, "{0} : Invalid Type", classObject->GetName());
		return false;
	}
	
	if (registerType(classType) == false)
	{
		return false;
	}

	for (const JGType& type : classObject->ParentTypeSet)
	{
		if (registerType(Allocate<JGType>(type)) == false)
		{
			return false;
		}
	}
	
	_classMap.emplace(*classType, classObject);
	_createClassFuncPool.emplace(*classType, func);
	_createObjectFuncPool.emplace(*classType, createObjectFunc);

	return true;
}

bool GObjectGlobalSystem::RegisterJGEnum(PSharedPtr<JGEnum> enumObject)
{
	if (enumObject.IsValid() == false)
	{
		return false;
	}

	PSharedPtr<JGType> enumType = enumObject->GetEnumType();
	if (enumType.IsValid() == false)
	{
		JG_LOG(Core, ELogLevel::Critical, "{0} : Invalid Type", enumObject->GetName());
		return false;
	}

	if (registerType(enumType) == false)
	{
		return false;
	}

	_enumMap.emplace(*enumType, enumObject);

	return true;
}

bool GObjectGlobalSystem::IsRegisteredType(const JGType& type) const
{
	const PName& typeName = type.GetName();

	if (_typeMap.find(typeName) != _typeMap.end())
	{
		return true;
	}

	return false;
}

bool GObjectGlobalSystem::registerType(PSharedPtr<JGType> type)
{
	if (type.IsValid() == false)
	{
		return false;
	}

	if (IsRegisteredType(*type) == true)
	{
		return true;
	}

	_typeMap.emplace(type->GetName(), *type);

	return true;
}

bool GObjectGlobalSystem::canCastInternal(const JGType& destType, const JGType& srcType) const
{
	if (IsRegisteredType(destType) == false || IsRegisteredType(srcType) == false)
	{
		return false;
	}

	PSharedPtr<JGClass> destClass = GetStaticClass(destType);
	PSharedPtr<JGClass> srcClass  = GetStaticClass(srcType);

	if (destClass == nullptr || srcClass == nullptr)
	{
		return false;
	}

	if (srcClass == nullptr)
	{
		return false;
	}

	if (destClass->ParentTypeSet.contains(srcType) == true)
	{
		return true;
	}

	for (const JGType& type : destClass->ParentTypeSet)
	{
		if (canCastInternal(type, srcType) == true)
		{
			return true;
		}
	}

	return false;
}

bool GObjectGlobalSystem::codeGen()
{
	const HModuleSystemInfo& moduleSysInfo = HModuleSystemInfo::Get();

	for (const PString& moduleName : moduleSysInfo.CodeGenableModuleSet)
	{
		HJInstance ins = HPlatform::LoadDll(PString::Format("%s.dll", moduleName));
		if (ins == nullptr)
		{
			// Error Log
			continue;
		}

		HPlatformFunction<void, GCoreSystem*> linkModuleFunc = HPlatform::LoadFuncInDll<void, GCoreSystem*>(ins, "Link_Module");
		if (linkModuleFunc.IsVaild() == false)
		{
			continue;
		}

		linkModuleFunc(&GCoreSystem::GetInstance());

		HPlatformFunction<bool, GObjectGlobalSystem*> codeGenFunc = HPlatform::LoadFuncInDll<bool, GObjectGlobalSystem*>(ins, "Engine_CodeGenerate");
		if (codeGenFunc.IsVaild() == false)
		{
			continue;
		}

		codeGenFunc(this);
		HPlatform::UnLoadDll(ins);
	}
	
	return true;
}

bool GObjectGlobalSystem::auditClassMultipleInheritance() const
{
	for (const HPair<JGType, PSharedPtr<JGClass>>& pair : _classMap)
	{
		PSharedPtr<JGClass> Class = pair.second;

		HHashSet<JGType> typeVisitor;

		// �˻� Ÿ��
		for (const JGType& type : Class->ParentTypeSet)
		{
			if (typeVisitor.contains(type) == true)
			{
				return false;
			}

			typeVisitor.insert(type);

			if (auditClassMultipleInheritanceInteral(type, typeVisitor) == false)
			{
				return false;
			}
		}
	}

	return true;
}

bool GObjectGlobalSystem::auditClassMultipleInheritanceInteral(const JGType& inType, HHashSet<JGType>& typeVisitor) const
{
	if (inType == JGTYPE(JGObject))
	{
		return typeVisitor.contains(JGTYPE(JGObject)) == false;
	}

	PSharedPtr<JGClass> Class = GetStaticClass(inType);
	if (Class == nullptr)
	{
		return true;
	}

	for (const JGType& type : Class->ParentTypeSet)
	{
		if (typeVisitor.contains(type) == true)
		{
			return false;
		}
		typeVisitor.insert(type);

		if (auditClassMultipleInheritanceInteral(type, typeVisitor) == false)
		{
			return false;
		}
	}

	return true;
}
