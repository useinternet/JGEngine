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
		PString RawAssetPathStr = inAssetPath;
		RawAssetPathStr.Remove(0, PString(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN).Length());
		HFileHelper::CombinePath(HFileHelper::EngineContentsDirectory(), RawAssetPathStr, &RawAssetPathStr);
		RawAssetPathStr += JG_ASSET_FORMAT;

		AssetPath = inAssetPath;

		PString AssetNameStr;
		HFileHelper::FileNameOnly(inAssetPath, &AssetNameStr);

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
		bIsValid = false;
	}
}
