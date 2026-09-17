#include "PCH/PCH.h"
#include "AssetDatabase.h"
#include "Asset.h"
#include <thread>
#include <chrono>

GAssetDatabase::~GAssetDatabase()
{
	releaseAssets();
}

void GAssetDatabase::Start()
{
	loadAssets();
}

void GAssetDatabase::Update()
{
	// 완료된 항목은 잠금 안에서 꺼내고, 콜백은 잠금 밖에서 부른다. (콜백이 LoadAssetAsync를 다시 부를 수 있다)
	HList<HPair<PName, HLoadingAssetData>> completedAssets;
	{
		HLockGuard<HMutex> lock(_loadingMutex);
		for (const HPair<const PName, HLoadingAssetData>& _pair : _loadingAssets)
		{
			if (_pair.second.bCompleted == true)
			{
				completedAssets.push_back(HPair<PName, HLoadingAssetData>(_pair.first, _pair.second));
			}
		}

		for (const HPair<PName, HLoadingAssetData>& completed : completedAssets)
		{
			_loadingAssets.erase(completed.first);
		}
	}

	for (const HPair<PName, HLoadingAssetData>& completed : completedAssets)
	{
		const PName& loadedAssetPath = completed.first;
		PSharedPtr<JGAsset> asset = completed.second.LoadedAsset;
		if (asset != nullptr)
		{
			_assetPool[asset->GetGuid()] = asset;
			_assetsByAssetPath[loadedAssetPath] = asset;

			JG_LOG(Asset, ELogLevel::Trace, "%s : Success Load Asset", loadedAssetPath.ToString());
		}
		else
		{
			JG_LOG(Asset, ELogLevel::Error, "%s : Failed Load Asset, Loaded Asset is nullptr", loadedAssetPath.ToString());
		}

		// 실패해도 콜백은 부른다. 기다리는 쪽이 빈 포인터를 보고 실패를 알 수 있어야 한다.
		for (const POnLoadCompelete& OnLoadCompelete : completed.second.OnLoadCompelete)
		{
			OnLoadCompelete.ExecuteIfBound(PWeakPtr<JGAsset>(asset));
		}
	}
}

void GAssetDatabase::Destroy()
{
	releaseAssets();
}

void GAssetDatabase::releaseAssets()
{
	// 로드 스레드가 아직 _loadingAssets를 쓰고 있을 수 있다. 결과가 기록될 때까지 잠시 기다린다.
	const int32 maxWaitCount = 500; // 10ms * 500 = 5초
	for (int32 i = 0; i < maxWaitCount; ++i)
	{
		bool bAllCompleted = true;
		{
			HLockGuard<HMutex> lock(_loadingMutex);
			for (const HPair<const PName, HLoadingAssetData>& _pair : _loadingAssets)
			{
				if (_pair.second.bCompleted == false)
				{
					bAllCompleted = false;
					break;
				}
			}
		}

		if (bAllCompleted)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	{
		HLockGuard<HMutex> lock(_loadingMutex);
		if (_loadingAssets.empty() == false)
		{
			JG_LOG(Asset, ELogLevel::Warning, "AssetDatabase : %d loading asset(s) dropped at shutdown", (int32)_loadingAssets.size());
		}
		_loadingAssets.clear();
	}

	_assetsByAssetPath.clear();
	_assetPool.clear();
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
	if (inAssetPath.IsValid() == false)
	{
		JG_LOG(Asset, ELogLevel::Error, "%s : Invalid Asset Path", inAssetPath.GetAssetPath().ToString());
		return false;
	}

	PName AssetPath = inAssetPath.GetAssetPath();
	// 이미 로드되어 있으면 바로 콜백함수 호출
	if (_assetsByAssetPath.contains(AssetPath))
	{
		OnLoadCompelete.ExecuteIfBound(_assetsByAssetPath[AssetPath]);
		return true;
	}

	HLockGuard<HMutex> lock(_loadingMutex);

	// 로딩 중이라면, 콜백함수 추가
	auto iter = _loadingAssets.find(AssetPath);
	if (iter != _loadingAssets.end())
	{
		if (OnLoadCompelete.IsBound())
		{
			iter->second.OnLoadCompelete.push_back(OnLoadCompelete);
		}

		return true;
	}

	// 항목을 먼저 넣고 나서 작업을 예약한다. 반대 순서면 작업이 emplace보다 먼저 끝나 결과를 잃는다.
	HLoadingAssetData& assetData = _loadingAssets[AssetPath];
	assetData.LoadedAsset = nullptr;
	if (OnLoadCompelete.IsBound())
	{
		assetData.OnLoadCompelete.push_back(OnLoadCompelete);
	}

	HLoadingThreadData threadData;
	threadData.AssetPath = inAssetPath;
	assetData.TaskHandle = GScheduleGlobalSystem::GetInstance().ScheduleAsync(ENamedThread::AssetLoadThread, PTaskDelegate::CreateRaw(this, &GAssetDatabase::loadAsset_Thread, threadData));

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
	else
	{
		Asset->OnLoadAsset_Thread();
	}

	// 결과(실패면 null)를 로딩 목록에 기록하고 완료 표시한다. Update()가 이 플래그를 보고 회수한다.
	// (이전 코드는 여기서 결과를 버려 항상 "Loaded Asset is nullptr"로 보고됐다)
	HLockGuard<HMutex> lock(_loadingMutex);
	auto iter = _loadingAssets.find(inThreadData.AssetPath.GetAssetPath());
	if (iter != _loadingAssets.end())
	{
		iter->second.LoadedAsset = Asset;
		iter->second.bCompleted  = true;
	}
	else
	{
		JG_LOG(Asset, ELogLevel::Error, "%s : Loading entry not found. Loaded asset is discarded", inThreadData.AssetPath.GetAssetPath().ToString());
	}
}
