#include "PCH/PCH.h"
#include "AssetPath.h"

HAssetPath::HAssetPath()
{
	bIsValid = false;
}

HAssetPath::HAssetPath(const PString& inAssetPath)
{
	setupAssetPath(inAssetPath);
}

HAssetPath::HAssetPath(const char* inAssetPath)
{
	setupAssetPath(PString(inAssetPath));
}

HAssetPath& HAssetPath::operator=(const PString& inAssetPath)
{
	setupAssetPath(inAssetPath);
	return *this;
}

HAssetPath& HAssetPath::operator=(const char* inAssetPath)
{
	setupAssetPath(PString(inAssetPath));
	return *this;
}

void HAssetPath::WriteJson(PJsonData& json) const
{
	json.AddMember("AssetPath", AssetPath);
}

void HAssetPath::ReadJson(const PJsonData& json)
{
	PString assetPathStr;
	if (json.GetData("AssetPath" , &assetPathStr) == false)
	{
		JG_LOG(Asset, ELogLevel::Error, "HAssetPath : Fail Read Json");
	}

	setupAssetPath(assetPathStr);
}

void HAssetPath::setupAssetPath(const PString& inAssetPath)
{
	if (inAssetPath.StartWidth(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN))
	{
		PString rawAssetPathStr = inAssetPath;
		rawAssetPathStr.Remove(0, PString(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN).Length());
		HFileHelper::CombinePath(HFileHelper::EngineContentDirectory(), rawAssetPathStr, &rawAssetPathStr);
		rawAssetPathStr += JG_ASSET_FORMAT;

		AssetPath = inAssetPath;

		PString AssetNameStr;
		HFileHelper::FileNameOnly(inAssetPath, &AssetNameStr);


		RawAssetPath = rawAssetPathStr;
		AssetName = AssetNameStr;
		bIsValid = true;
	}
	else if (inAssetPath.StartWidth(JG_ASSET_GAME_PATH_RECOGNITION_TOEKN))
	{
		bIsValid = false;

		JG_CHECK(false);
		JG_LOG(Asset, ELogLevel::Critical, "NOT Support Game Path");
	}
	else
	{
		// Full Path
		PString engineContentPath = HFileHelper::EngineContentDirectory();
		HFileHelper::AbsolutePath(engineContentPath, &engineContentPath);
		
		//const PString& gameContentPath = HFileHelper::GameContentDirectory();
		if(inAssetPath.StartWidth(engineContentPath) == true)
		{
			PString assetPath = inAssetPath;
			assetPath.Remove(0, engineContentPath.Length());
			HFileHelper::CombinePath(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN, assetPath, &assetPath);
			setupAssetPath(assetPath);
		}
		else
		{
			bIsValid = false;
			JG_CHECK(false);
			JG_LOG(Asset, ELogLevel::Critical, "NOT Support Asset Path");
		}
	}
}
