#pragma once
#include "Core.h"
#include "AssetDefines.h"
#include "AssetPath.h"
#include "Asset.generation.h"

JGCLASS()
class ASSET_API JGAsset : public JGObject
{
	friend class GAssetDatabase;

	JG_GENERATED_CLASS_BODY

private:
	JGPROPERTY()
	HGuid _guid;

	JGPROPERTY()
	HAssetPath AssetPath;

	JGPROPERTY()
	uint32 Version;
public:
	JGAsset();
	virtual ~JGAsset() = default;

	const HGuid& GetGuid() const { return _guid; }
	uint32 GetVersion() const { return Version; }
protected:
	virtual bool IsValid() const { return _guid.IsValid();  }
	virtual void OnLoadAsset_Thread() {}
};
