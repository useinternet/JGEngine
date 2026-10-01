#pragma once
#include "UI/GameUIElement.h"

class IRawTexture;

// 이미지. 텍스처가 없으면 단색 사각형이라 패널 배경으로도 쓴다. 색을 곱해 그린다.
// 입력 받기는 기본으로 꺼져 있다. 패널이 아래(월드) 클릭을 막아야 하면 SetHitTestVisible(true).
class GAMEFRAMEWORKS_API PGameUIImage : public PGameUIElement
{
	PSharedPtr<IRawTexture> _texture;
	HLinearColor            _color = HLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	HRect                   _uv    = HRect(0.0f, 0.0f, 1.0f, 1.0f);

public:
	PGameUIImage() = default;
	virtual ~PGameUIImage() = default;

	void SetTexture(const PSharedPtr<IRawTexture>& texture);
	const PSharedPtr<IRawTexture>& GetTexture() const;
	void SetColor(const HLinearColor& color);
	const HLinearColor& GetColor() const;
	// 텍스처에서 쓸 영역(0~1). 기본은 전체.
	void SetUV(const HRect& uv);

protected:
	virtual void onBuildDraw(HGameUIDrawContext& context) override;
};
