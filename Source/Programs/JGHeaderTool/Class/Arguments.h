#pragma once
#include "Core.h"


class PArguments : public IMemoryObject, public IJsonable
{
public:
	PString EngineWorkDirectory;
	HHashSet<PString> EngineWorkCategories;
	

	PString UserWorkDirectory;
	HHashSet<PString> UserWorkCategories;

	PArguments()
	{
		EngineWorkDirectory = HFileHelper::EngineSourceDirectory();
		EngineWorkCategories.insert("Editor");
		EngineWorkCategories.insert("Runtime");
		EngineWorkCategories.insert("Programs");

		HFileHelper::CombinePath(HFileHelper::EngineSourceDirectory(), "Dummy", &UserWorkDirectory);
	}

	virtual ~PArguments() = default;
protected:
	virtual void WriteJson(PJsonData& json) const override
	{
		json.AddMember("EngineWorkDirectory", EngineWorkDirectory);
		json.AddMember("EngineWorkCategories", EngineWorkCategories);
		json.AddMember("UserWorkDirectory", UserWorkDirectory);
		json.AddMember("UserWorkCategories", UserWorkCategories);
	}

	virtual void ReadJson(const PJsonData& json) override
	{
		if (json.GetData("EngineWorkDirectory", &EngineWorkDirectory) == false)
		{
			JG_LOG(HeaderTool, ELogLevel::Error, "EngineWorkDirectory: fail read json data");
		}

		if (json.GetData("EngineWorkCategories", &EngineWorkCategories) == false)
		{
			JG_LOG(HeaderTool, ELogLevel::Error, "EngineWorkCategories: fail read json data");
		}

		if (json.GetData("UserWorkDirectory", &UserWorkDirectory) == false)
		{
			JG_LOG(HeaderTool, ELogLevel::Error, "UserWorkDirectory: fail read json data");
		}

		if (json.GetData("UserWorkCategories", &UserWorkCategories) == false)
		{
			JG_LOG(HeaderTool, ELogLevel::Error, "UserWorkCategories: fail read json data");
		}
	}
};