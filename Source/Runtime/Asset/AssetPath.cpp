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
		// 토큰 뒤의 상대 경로를 정규화한다: 역슬래시 -> 슬래시, 앞쪽 슬래시 제거, 확장자 보정.
		// 같은 파일을 가리키는 문자열("/JGEngine//A/B", "/JGEngine/A/B.jgasset")이 AssetDatabase에서 같은 키가 되어야 한다.
		PString relativePath = inAssetPath;
		relativePath.Remove(0, PString(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN).Length());
		relativePath.ReplaceAll("\\", "/");
		while (relativePath.Empty() == false && relativePath[0] == '/')
		{
			relativePath.Remove(0, 1);
		}

		PString FileExtension;
		HFileHelper::FileExtension(relativePath, &FileExtension);
		if (FileExtension.Empty())
		{
			relativePath += JG_ASSET_FORMAT;
		}
		else if (FileExtension.Equal(JG_ASSET_FORMAT) == false)
		{
			JG_LOG(Asset, ELogLevel::Warning, "NOT Support Format : %s", inAssetPath);
		}

		PString rawAssetPathStr;
		HFileHelper::CombinePath(HFileHelper::EngineContentDirectory(), relativePath, &rawAssetPathStr);

		PString AssetNameStr;
		HFileHelper::FileNameOnly(relativePath, &AssetNameStr);

		AssetPath    = PString(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN) + relativePath;
		RawAssetPath = rawAssetPathStr;
		AssetName    = AssetNameStr;
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
		

		PString assetPath;
		HFileHelper::AbsolutePath(inAssetPath, &assetPath);

		//const PString& gameContentPath = HFileHelper::GameContentDirectory();
		if(assetPath.StartWidth(engineContentPath) == true)
		{
			assetPath.Remove(0, engineContentPath.Length());
			HFileHelper::CombinePath(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN, assetPath, &assetPath);
			setupAssetPath(assetPath);
		}
		else
		{
			bIsValid = false;
			//JG_CHECK(false);
			JG_LOG(Asset, ELogLevel::Warning, "NOT Support Asset Path");
		}
	}
}
