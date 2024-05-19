#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"
#include "Asset.h"


JG_DECLARE_DELEGATE(HOnAccessTexturePixels, const void*)
class GRAPHICS_API IRawTexture : public IJGGraphicsObject
{
public:
	virtual uint64 GetTextureID() const = 0;
	virtual const HTextureInfo& GetTextureInfo() const = 0;
	virtual void AccessPixels(HOnAccessTexturePixels onAccessTexturePixels) const = 0;

	virtual void Reset() = 0;
	virtual bool IsValid() const = 0;
protected:
	virtual void Initialize(const HTextureInfo& inTextureInfo) = 0;
	virtual void InitializeByMemory(const uint8* pixels, const HTextureInfo& inTextureInfo) = 0;
};

struct HTextureConstructArguments
{
	HTextureInfo TextureInfo;
	HList<uint8> Pixels;
};

JGCLASS()
class GRAPHICS_API JGTexture : public JGAsset
{
	friend class PJGGraphicsAPI;
private:

	PSharedPtr<IRawTexture> _texture;

public:
	// JGAsset
	virtual bool IsValid() const;

	// ~JGAsset
protected:

	// IJsonable
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
	// ~ IJsonable

};