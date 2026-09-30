#include "PCH/PCH.h"
#include "FileIO/ProjectDescriptor.h"
#include "FileIO/FileHelper.h"
#include "FileIO/Json.h"
#include "Misc/Log.h"
#include <cstdlib>

bool HProjectDescriptor::Load(const PString& projectDirectory, HProjectDescriptor* outDescriptor)
{
	if (outDescriptor == nullptr || projectDirectory.Empty() == true)
	{
		return false;
	}

	if (HFileHelper::IsDirectory(projectDirectory) == false)
	{
		return false;
	}

	HList<PString> descriptorFiles;
	HFileHelper::FileListInDirectory(projectDirectory, &descriptorFiles, false, { FileExtension });
	if (descriptorFiles.empty() == true)
	{
		return false;
	}

	if (descriptorFiles.size() > 1)
	{
		JG_LOG(Core, ELogLevel::Error, "More than one %s file in %s", FileExtension, projectDirectory);
		return false;
	}

	PString text;
	if (HFileHelper::ReadAllText(descriptorFiles[0], &text) == false)
	{
		JG_LOG(Core, ELogLevel::Error, "Fail read %s", descriptorFiles[0]);
		return false;
	}

	PJson json;
	if (PJson::ToObject(text, &json) == false)
	{
		JG_LOG(Core, ELogLevel::Error, "Invalid json: %s", descriptorFiles[0]);
		return false;
	}

	if (json.GetData("Project", outDescriptor) == false || outDescriptor->Name.Empty() == true)
	{
		JG_LOG(Core, ELogLevel::Error, "%s needs a \"Project\" object with a Name", descriptorFiles[0]);
		return false;
	}

	return true;
}

bool HProjectDescriptor::ResolveEngineRoot(const PString& projectDirectory, PString* outEngineRoot) const
{
	if (outEngineRoot == nullptr)
	{
		return false;
	}

	PString engineRoot = EngineRoot;
	if (engineRoot.Empty() == true)
	{
		const char* environmentValue = std::getenv(EngineRootEnvironmentVariable);
		if (environmentValue != nullptr)
		{
			engineRoot = environmentValue;
		}
	}

	if (engineRoot.Empty() == true)
	{
		return false;
	}

	if (fs::path(engineRoot.GetRawString()).is_relative() == true)
	{
		PString combined;
		HFileHelper::CombinePath(projectDirectory, engineRoot, &combined);
		engineRoot = combined;
	}

	HFileHelper::AbsoluteDirectory(engineRoot, outEngineRoot);
	return true;
}

void HProjectDescriptor::WriteJson(PJsonData& json) const
{
	json.AddMember("Name", Name);
	json.AddMember("EngineRoot", EngineRoot);
	json.AddMember("GameModules", GameModules);
	json.AddMember("EditorModules", EditorModules);
}

void HProjectDescriptor::ReadJson(const PJsonData& json)
{
	json.GetData("Name", &Name);
	json.GetData("EngineRoot", &EngineRoot);
	json.GetData("GameModules", &GameModules);
	json.GetData("EditorModules", &EditorModules);
}
