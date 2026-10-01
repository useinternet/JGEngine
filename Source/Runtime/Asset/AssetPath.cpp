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

namespace
{
	// inFullPath 가 inContentDirectory 안의 파일이면 "<inToken><Content 기준 상대경로>" 로 바꾼다.
	bool toTokenAssetPath(const PString& inFullPath, const PString& inContentDirectory, const char* inToken, PString* outTokenPath)
	{
		// 빈 Content(엔진 단독의 게임 Content)를 AbsolutePath 에 넘기면 작업 폴더가 되어 아무 경로나 걸린다.
		if (inContentDirectory.Empty())
		{
			return false;
		}

		PString contentPath;
		HFileHelper::AbsolutePath(inContentDirectory, &contentPath);
		if (inFullPath.StartWidth(contentPath) == false)
		{
			return false;
		}

		PString relativePath = inFullPath;
		relativePath.Remove(0, contentPath.Length());
		HFileHelper::CombinePath(inToken, relativePath, outTokenPath);
		return true;
	}
}

void HAssetPath::setupAssetPath(const PString& inAssetPath)
{
	bIsValid = false;

	// 토큰이 에셋이 속한 Content 를 정한다. /JGEngine/ = 엔진 Content, /JGGame/ = 게임 프로젝트 Content.
	const char* token = nullptr;
	PString contentDirectory;
	if (inAssetPath.StartWidth(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN))
	{
		token = JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN;
		contentDirectory = HFileHelper::EngineContentDirectory();
	}
	else if (inAssetPath.StartWidth(JG_ASSET_GAME_PATH_RECOGNITION_TOEKN))
	{
		if (HFileHelper::GameContentDirectory().Empty())
		{
			JG_LOG(Asset, ELogLevel::Error, "%s : Game content is only available in a game project", inAssetPath);
			return;
		}

		token = JG_ASSET_GAME_PATH_RECOGNITION_TOEKN;
		contentDirectory = HFileHelper::GameContentDirectory();
	}
	else
	{
		// Full Path : 엔진 또는 게임 Content 안의 파일이면 토큰 경로로 바꿔 다시 푼다.
		PString assetPath;
		HFileHelper::AbsolutePath(inAssetPath, &assetPath);

		PString tokenPath;
		if (toTokenAssetPath(assetPath, HFileHelper::EngineContentDirectory(), JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN, &tokenPath)
			|| toTokenAssetPath(assetPath, HFileHelper::GameContentDirectory(), JG_ASSET_GAME_PATH_RECOGNITION_TOEKN, &tokenPath))
		{
			setupAssetPath(tokenPath);
		}
		else
		{
			JG_LOG(Asset, ELogLevel::Warning, "NOT Support Asset Path");
		}

		return;
	}

	PString relativePath = inAssetPath;
	relativePath.Remove(0, PString(token).Length());
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
	HFileHelper::CombinePath(contentDirectory, relativePath, &rawAssetPathStr);

	PString AssetNameStr;
	HFileHelper::FileNameOnly(relativePath, &AssetNameStr);

	AssetPath    = PString(token) + relativePath;
	RawAssetPath = rawAssetPathStr;
	AssetName    = AssetNameStr;
	bIsValid = true;
}
