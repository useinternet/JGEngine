#include "PCH/PCH.h"
#include "FileIO/PackageDescriptor.h"
#include "FileIO/FileHelper.h"
#include "FileIO/Json.h"
#include "Misc/Log.h"

void HGameWindowSettings::WriteJson(PJsonData& json) const
{
	json.AddMember("Title", Title);
	json.AddMember("Width", Width);
	json.AddMember("Height", Height);
	json.AddMember("Mode", Mode);
	json.AddMember("VSync", bVSync);
}

void HGameWindowSettings::ReadJson(const PJsonData& json)
{
	// 빠진 값은 기본값(1920×1080 창 모드 · VSync)을 그대로 둔다.
	json.GetData("Title", &Title);
	json.GetData("Width", &Width);
	json.GetData("Height", &Height);
	json.GetData("Mode", &Mode);
	json.GetData("VSync", &bVSync);
}

void HPackageDescriptor::FilePath(const PString& packageDirectory, PString* outPath)
{
	if (outPath == nullptr)
	{
		return;
	}

	PString binDirectory;
	HFileHelper::CombinePath(packageDirectory, BinDirectoryName, &binDirectory);
	HFileHelper::CombinePath(binDirectory, FileName, outPath);
}

bool HPackageDescriptor::Load(const PString& packageDirectory, HPackageDescriptor* outDescriptor)
{
	if (outDescriptor == nullptr || packageDirectory.Empty() == true)
	{
		return false;
	}

	PString path;
	FilePath(packageDirectory, &path);
	if (HFileHelper::Exists(path) == false)
	{
		return false;
	}

	PString text;
	if (HFileHelper::ReadAllText(path, &text) == false)
	{
		JG_LOG(Core, ELogLevel::Error, "Fail read %s", path);
		return false;
	}

	PJson json;
	if (PJson::ToObject(text, &json) == false)
	{
		JG_LOG(Core, ELogLevel::Error, "Invalid json: %s", path);
		return false;
	}

	if (json.GetData("Package", outDescriptor) == false || outDescriptor->Name.Empty() == true)
	{
		JG_LOG(Core, ELogLevel::Error, "%s needs a \"Package\" object with a Name", path);
		return false;
	}

	return true;
}

bool HPackageDescriptor::Save(const PString& packageDirectory) const
{
	PString path;
	FilePath(packageDirectory, &path);

	PJson json;
	json.AddMember("Package", *this);

	PString text;
	if (PJson::ToString(json, &text) == false)
	{
		return false;
	}

	return HFileHelper::WriteAllText(path, text);
}

void HPackageDescriptor::WriteJson(PJsonData& json) const
{
	json.AddMember("Name", Name);
	json.AddMember("Version", Version);
	json.AddMember("BuildConfig", BuildConfig);
	json.AddMember("EngineCommit", EngineCommit);
	json.AddMember("CookedAt", CookedAt);
	json.AddMember("LaunchModule", LaunchModule);
	json.AddMember("RuntimeModules", RuntimeModules);
	json.AddMember("GameModules", GameModules);
	json.AddMember("Window", Window);
}

void HPackageDescriptor::ReadJson(const PJsonData& json)
{
	json.GetData("Name", &Name);
	json.GetData("Version", &Version);
	json.GetData("BuildConfig", &BuildConfig);
	json.GetData("EngineCommit", &EngineCommit);
	json.GetData("CookedAt", &CookedAt);
	json.GetData("LaunchModule", &LaunchModule);
	json.GetData("RuntimeModules", &RuntimeModules);
	json.GetData("GameModules", &GameModules);
	json.GetData("Window", &Window);
}
