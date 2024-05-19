#pragma once
#include "Core.h"
#include "AssetDefines.h"
#include "Misc/Module.h"

class ASSET_API HAssetModule : public IModuleInterface
{
protected:
	virtual JGType GetModuleType() const override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

