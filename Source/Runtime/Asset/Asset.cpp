#include "PCH/PCH.h"
#include "Asset.h"

JGAsset::JGAsset()
{
	_guid = HGuid::New();
	Version = 0;
}
