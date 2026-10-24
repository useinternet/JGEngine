#include "PCH/PCH.h"
#include "Asset.h"

JGAsset::JGAsset()
{
	_guid = HGuid::New();
	Version = 0;
}

void JGAsset::SetName(const PName& name)
{
	JG_SUPER::SetName(name);

	AssetPath = HAssetPath(name.ToString());
}
