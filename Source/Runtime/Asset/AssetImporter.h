#pragma once
#include "Core.h"
#include "AssetDefines.h"

class JGAssetImporter;
class ASSET_API PAssetImportArguments : public IMemoryObject
{
public:
	PString SrcPath;
	PString DestPath;

public:
	virtual JGType GetArgumentType() const { return JGTYPE(PAssetImportArguments); }
	virtual JGType GetImporterType() const { return JGTYPE(JGAssetImporter); }
	virtual ~PAssetImportArguments() = default;
};

JGCLASS()
class ASSET_API JGAssetImporter : public JGObject
{
	JG_GENERATED_CLASS_BODY
public:
	virtual ~JGAssetImporter() = default;

	virtual bool Import(PSharedPtr<PAssetImportArguments> inArgs) { return false; }
	virtual bool IsSupportedExtension(const PString& inExtension) const { return false; }
};