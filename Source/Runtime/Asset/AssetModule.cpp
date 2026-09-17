#include "PCH/PCH.h"
#include "AssetModule.h"
#include "AssetDatabase.h"

JG_MODULE_IMPL(HAssetModule, ASSET_C_API)

JGType HAssetModule::GetModuleType() const
{
	return JGTYPE(HAssetModule);
}

void HAssetModule::StartupModule()
{
	JG_LOG(Asset, ELogLevel::Trace, "Startup Asset Module...");

	GCoreSystem::RegisterSystemInstance<GAssetDatabase>();
}

void HAssetModule::ShutdownModule()
{
	// StartupModule에서 등록한 짝. 여기서 내려야 보유 에셋(메시/텍스처)이 Graphics 모듈보다 먼저 해제된다.
	GCoreSystem::UnRegisterSystemInstance<GAssetDatabase>();

	JG_LOG(Asset, ELogLevel::Trace, "Shutdown Asset Module...");
}
