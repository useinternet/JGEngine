#include "PCH/PCH.h"
#include "UI/GameUIImage.h"
#include "UI/GameUIDrawList.h"
#include "Classes/Texture.h"

void PGameUIImage::SetTexture(const PSharedPtr<IRawTexture>& texture)
{
	_texture = texture;
}

const PSharedPtr<IRawTexture>& PGameUIImage::GetTexture() const
{
	return _texture;
}

void PGameUIImage::SetColor(const HLinearColor& color)
{
	_color = color;
}

const HLinearColor& PGameUIImage::GetColor() const
{
	return _color;
}

void PGameUIImage::SetUV(const HRect& uv)
{
	_uv = uv;
}

void PGameUIImage::onBuildDraw(HGameUIDrawContext& context)
{
	const float32 scale = context.Scale;
	const HRect pixelRect(_layoutRect.left * scale, _layoutRect.top * scale, _layoutRect.right * scale, _layoutRect.bottom * scale);
	context.DrawList->AddQuad(pixelRect, _uv, _color, _texture, nullptr, context.Clip);
}
