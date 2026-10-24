#pragma once
#include "Core.h"
#include "Classes/Texture.h"
#include "Asset/Asset.h"

struct HTextureConstructArguments
{
	HTextureInfo TextureInfo;
	HList<uint8> Pixels;

	HTextureConstructArguments() {}
};

// 텍스처 에셋. GPU 텍스처(IRawTexture, Graphics)를 들고, 저장 · 읽기 때 픽셀을 압축해 .jgasset 에 쓴다.
JGCLASS()
class GAMEFRAMEWORK_API JGTexture : public JGAsset
{
	// 생성 본문(JG_GENERATED_CLASS_BODY)은 WriteJson/ReadJson을 정의해 아래 직접 구현과 겹치므로 GetType()만 둔다.
	// 없으면 GetType()이 JGAsset을 돌려줘 로드한 텍스처 에셋의 타입 검사가 실패한다. (생성은 이름으로 하므로 LoadObject는 원래 동작했다)
	JG_GENERATED_SIMPLE_BODY

private:

	PSharedPtr<IRawTexture> _texture;

public:
	// 픽셀로 GPU 텍스처를 만든 텍스처 에셋. FBX 가져오기가 쓴다(옛 PJGGraphicsAPI::CreateTexture).
	static PSharedPtr<JGTexture> Create(const HTextureConstructArguments& inArgs);

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
