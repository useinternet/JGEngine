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
	JG_LOG(Asset, ELogLevel::Trace, "Shutdown Asset Module...");
}
