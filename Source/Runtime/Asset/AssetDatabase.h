#pragma once
#include "Core.h"
#include "AssetDefines.h"
#include "AssetPath.h"

class JGAsset;
JG_DECLARE_DELEGATE(POnLoadCompelete, PWeakPtr<JGAsset>);


class ASSET_API GAssetDatabase final : public GGlobalSystemInstance<GAssetDatabase>
{
	// Asset을 나누지
	struct HLoadingThreadData
	{
		HAssetPath       AssetPath;

		HLoadingThreadData() = default;
	};

	struct HLoadingAssetData
	{
		PSharedPtr<JGAsset> LoadedAsset;
		HTaskHandle         TaskHandle;
		HList<POnLoadCompelete> OnLoadCompelete;

		HLoadingAssetData() = default;
	};

public:
	HHashMap<HGuid, PSharedPtr<JGAsset>> _assetPool;
	HHashMap<PName, PWeakPtr<JGAsset>>   _assetsByAssetPath;

	HHashMap<PName, HLoadingAssetData> _loadingAssets;

protected:
	virtual void Start();
	virtual void Update();
	virtual void Destroy();

public:
	PWeakPtr<JGAsset> GetLoadedAsset(const HAssetPath& inAssetPath) const;
	bool LoadAssetAsync(const HAssetPath& inAssetPath, const POnLoadCompelete& OnLoadCompelete = POnLoadCompelete());

//private:
	void loadAssets();
	void loadAssetsInternal(const PString& inContentDir);
	void loadAsset_Thread(HLoadingThreadData inAssetData);
};