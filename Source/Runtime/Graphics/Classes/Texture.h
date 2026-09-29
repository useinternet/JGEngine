#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"
#include "Asset.h"


// GPU 텍스처. 픽셀을 CPU로 읽는 일은 텍스처가 아니라 그래픽 API가 맡는다.
// (PJGGraphicsAPI::RequestTextureReadback / ReadbackTextureImmediate. DEFAULT 힙은 Map할 수 없어 READBACK 스테이징을 거쳐야 한다)
class GRAPHICS_API IRawTexture : public IJGGraphicsObject
{
public:
	virtual uint64 GetTextureID() const = 0;
	virtual const HTextureInfo& GetTextureInfo() const = 0;

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

	HTextureConstructArguments() {}
};

JGCLASS()
class GRAPHICS_API JGTexture : public JGAsset
{
	// 생성 본문(JG_GENERATED_CLASS_BODY)은 WriteJson/ReadJson을 정의해 아래 직접 구현과 겹치므로 GetType()만 둔다.
	// 없으면 GetType()이 JGAsset을 돌려줘 로드한 텍스처 에셋의 타입 검사가 실패한다. (생성은 이름으로 하므로 LoadObject는 원래 동작했다)
	JG_GENERATED_SIMPLE_BODY

	friend class PJGGraphicsAPI;
private:

	PSharedPtr<IRawTexture> _texture;

public:
	// JGAsset
	virtual bool IsValid() const;

	// ~JGAsset

	// 바인딩·리드백·GUI 표시에 쓰는 GPU 텍스처
	PSharedPtr<IRawTexture> GetRawTexture() const { return _texture; }
protected:

	// IJsonable
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
	// ~ IJsonable

};
