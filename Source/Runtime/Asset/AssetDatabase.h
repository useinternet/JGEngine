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
		// 로드 스레드가 결과(성공/실패)를 기록했는지. 완료 판정은 이 플래그로만 한다.
		// HTaskHandle::IsCompelete()는 작업 객체가 파괴되면 false로 굳어 버려(약참조) Update가 완료를 놓칠 수 있다.
		bool bCompleted = false;

		HLoadingAssetData() = default;
	};

public:
	HHashMap<HGuid, PSharedPtr<JGAsset>> _assetPool;
	HHashMap<PName, PWeakPtr<JGAsset>>   _assetsByAssetPath;

	// 로딩 중인 에셋. 로드 스레드가 LoadedAsset을 채우고 메인 스레드(Update)가 회수하므로 _loadingMutex로 보호한다.
	HHashMap<PName, HLoadingAssetData> _loadingAssets;
	mutable HMutex _loadingMutex;

protected:
	virtual ~GAssetDatabase();

	virtual void Start();
	virtual void Update();
	virtual void Destroy();

public:
	PWeakPtr<JGAsset> GetLoadedAsset(const HAssetPath& inAssetPath) const;
	// 로드가 끝나면(또는 이미 로드되어 있으면) 메인 스레드에서 OnLoadCompelete를 부른다. 로드 실패 시에는 빈 포인터로 부른다.
	bool LoadAssetAsync(const HAssetPath& inAssetPath, const POnLoadCompelete& OnLoadCompelete = POnLoadCompelete());

//private:
	void loadAssets();
	void loadAssetsInternal(const PString& inContentDir);
	void loadAsset_Thread(HLoadingThreadData inAssetData);

private:
	// 보유한 에셋을 모두 놓는다. 진행 중인 로드는 잠시(최대 5초) 기다린다.
	// Asset 모듈 종료 시 불려야 한다. 그래야 DisconnectModule의 GC Flush가 Graphics 모듈이 살아 있을 때 메시/버퍼를 정리한다.
	// (놓지 않으면 GC 객체가 메모리 시스템 소멸의 강제 정리까지 살아남아, 먼저 해제된 객체의 카운터를 건드려 종료 크래시가 난다)
	void releaseAssets();
};
