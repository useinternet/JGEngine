#include "PCH/PCH.h"
#include "AssetDatabase.h"
#include "Asset.h"

void GAssetDatabase::Start()
{
	loadAssets();
}

void GAssetDatabase::Update()
{
	uint32 loadingAssetCount = (uint32)_loadingAssets.size();

	HList<PName> loadedAssetPaths;
	for (const HPair<const PName, HLoadingAssetData>& _pair : _loadingAssets)
	{
		if (_pair.second.TaskHandle.IsCompelete() == true)
		{
			loadedAssetPaths.push_back(_pair.first);
		}
	}

	for (PName loadedAssetPath : loadedAssetPaths)
	{
		PSharedPtr<JGAsset> asset = _loadingAssets[loadedAssetPath].LoadedAsset;
		if (asset != nullptr)
		{
			_assetPool[asset->GetGuid()] = asset;
			_assetsByAssetPath[loadedAssetPath] = asset;

			for (const POnLoadCompelete& OnLoadCompelete : _loadingAssets[loadedAssetPath].OnLoadCompelete)
			{
				OnLoadCompelete.ExecuteIfBound(asset);
			}
			
			JG_LOG(Asset, ELogLevel::Trace, "%s : Success Load Asset");
		}
		else
		{
			JG_LOG(Asset, ELogLevel::Error, "%s : Failed Load Asset, Loaded Asset is nullptr");
		}

		_loadingAssets.erase(loadedAssetPath);
	}
}

void GAssetDatabase::Destroy()
{
}

PWeakPtr<JGAsset> GAssetDatabase::GetLoadedAsset(const HAssetPath& inAssetPath) const
{
	if (_assetsByAssetPath.contains(inAssetPath.GetAssetPath()))
	{
		return _assetsByAssetPath.at(inAssetPath.GetAssetPath());
	}

	return PWeakPtr<JGAsset>();
}

bool GAssetDatabase::LoadAssetAsync(const HAssetPath& inAssetPath, const POnLoadCompelete& OnLoadCompelete)
{
	PName AssetPath = inAssetPath.GetAssetPath();
	// 이미 로딩중이면 바로 콜백함수 호출
	if (_assetsByAssetPath.contains(AssetPath))
	{
		OnLoadCompelete.ExecuteIfBound(_assetsByAssetPath[AssetPath]);
		return true;
	}

	// 로딩 중이라면, 콜백함수 추가
	if (_loadingAssets.contains(AssetPath))
	{
		if (OnLoadCompelete.IsBound())
		{
			_loadingAssets[AssetPath].OnLoadCompelete.push_back(OnLoadCompelete);
		}

		return true;
	}

	HLoadingThreadData threadData;
	threadData.AssetPath  = inAssetPath;


	HLoadingAssetData assetData;
	assetData.LoadedAsset     = nullptr;
	if (OnLoadCompelete.IsBound())
	{
		assetData.OnLoadCompelete.push_back(OnLoadCompelete);
	}

	assetData.TaskHandle  = GScheduleGlobalSystem::GetInstance().ScheduleAsync(ENamedThread::AssetLoadThread, PTaskDelegate::CreateRaw(this, &GAssetDatabase::loadAsset_Thread, threadData));

	_loadingAssets.emplace(inAssetPath.GetAssetPath(), assetData);
	return true;
}

void GAssetDatabase::loadAssets()
{
	loadAssetsInternal(HFileHelper::EngineContentDirectory());
	//loadAssetsInternal(HFileHelper::GameContentDirectory());
}

void GAssetDatabase::loadAssetsInternal(const PString& inContentDir)
{
	HList<PString> assetFiles;
	HFileHelper::FileListInDirectory(inContentDir, &assetFiles, true, { JG_ASSET_FORMAT });

	JG_LOG(Asset, ELogLevel::Trace, "Start Load Assets in %s", inContentDir);
	for (const PString& assetFile : assetFiles)
	{
		LoadAssetAsync(HAssetPath(assetFile));
	}
}

void GAssetDatabase::loadAsset_Thread(HLoadingThreadData inThreadData)
{
	PName RawFilePath = inThreadData.AssetPath.GetRawAssetPath();
	PSharedPtr<JGAsset> Asset = LoadObject<JGAsset>(RawFilePath.ToString());

	if (Asset == nullptr)
	{
		JG_LOG(Asset, ELogLevel::Error, "%s : Fail Load Asset", inThreadData.AssetPath.GetAssetPath().ToString());
	}

	Asset->OnLoadAsset_Thread();
}
