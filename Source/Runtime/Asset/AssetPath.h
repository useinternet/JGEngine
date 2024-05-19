#pragma once
#include "Core.h"
#include "AssetDefines.h"



class ASSET_API HAssetPath : public IJsonable
{
private:
	PName RawAssetPath;
	PName AssetPath;
	PName AssetName;

	bool bIsValid;
public:
	HAssetPath();
	HAssetPath(const PString& inAssetPath);

	const PName& GetRawAssetPath() const { return RawAssetPath; }
	const PName& GetAssetPath() const { return AssetPath; }
	const PName& GetAssetName() const { return AssetName; }

	bool IsValid() const { return bIsValid; }

protected:
	virtual void WriteJson(PJsonData& json) const;
	virtual void ReadJson(const PJsonData& json);

private:
	void setupAssetPath(const PString& inAssetPath);
};