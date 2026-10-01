#include "PCH/PCH.h"
#include "UI/GameUIText.h"
#include "UI/GameUIFont.h"
#include "UI/GameUIDrawList.h"
#include "UI/GameUIManager.h"
#include "UI/GameUIUtf8.h"

namespace
{
	void addGlyphQuad(HGameUIDrawContext& context, PGameUIFont& font, uint32 codepoint, int32 pixelSize, float32 penX, float32 baseline, const HLinearColor& color)
	{
		const PGameUIFont::HGlyph& glyph = font.GetGlyph(codepoint, pixelSize);
		if (glyph.bInAtlas == false)
		{
			return;
		}

		// 글리프 위치를 정수 픽셀에 맞춘다. 아틀라스 텍셀과 화면 픽셀이 1:1 이 되어 흐려지지 않는다.
		const float32 left = HMath::FloorToFloat32(penX + 0.5f) + (float32)glyph.OffsetX;
		const float32 top  = baseline + (float32)glyph.OffsetY;
		const HRect glyphRect(left, top, left + (float32)glyph.Width, top + (float32)glyph.Height);
		context.DrawList->AddQuad(glyphRect, glyph.UV, color, nullptr, &font, context.Clip);
	}
}

void PGameUIText::SetText(const PString& text)
{
	_text = text;
}

const PString& PGameUIText::GetText() const
{
	return _text;
}

void PGameUIText::SetStyle(const HGameUITextStyle& style)
{
	_style     = style;
	_overrides = OverrideFont | OverrideSize | OverrideColor;
}

void PGameUIText::SetFont(const PSharedPtr<PGameUIFont>& font)
{
	_style.Font = font;
	_overrides |= OverrideFont;
}

void PGameUIText::SetFontSize(float32 fontSize)
{
	_style.FontSize = fontSize;
	_overrides |= OverrideSize;
}

void PGameUIText::SetColor(const HLinearColor& color)
{
	_style.Color = color;
	_overrides |= OverrideColor;
}

void PGameUIText::ClearStyle()
{
	_style     = HGameUITextStyle();
	_overrides = 0;
}

void PGameUIText::SetAlignment(EGameUIHorizontalAlign horizontal, EGameUIVerticalAlign vertical)
{
	_horizontalAlign = horizontal;
	_verticalAlign   = vertical;
}

void PGameUIText::SetWrap(EGameUITextWrap wrap)
{
	_wrap = wrap;
}

EGameUITextWrap PGameUIText::GetWrap() const
{
	return _wrap;
}

void PGameUIText::SetMaxLines(int32 maxLines)
{
	_maxLines = HMath::Max(0, maxLines);
}

int32 PGameUIText::GetMaxLines() const
{
	return _maxLines;
}

void PGameUIText::SetEllipsis(bool bEllipsis)
{
	_bEllipsis = bEllipsis;
}

bool PGameUIText::IsEllipsis() const
{
	return _bEllipsis;
}

void PGameUIText::SetClip(bool bClip)
{
	_bClip = bClip;
}

bool PGameUIText::IsClip() const
{
	return _bClip;
}

HVector2 PGameUIText::MeasureContent(float32 width, PGameUIManager* manager) const
{
	const HGameUITextStyle style = ResolveStyle(manager);
	PSharedPtr<PGameUIFont> font = ResolveFont(style.Font, manager);
	if (font.IsValid() == false)
	{
		return HVector2(0.0f, 0.0f);
	}

	HGameUITextLayout layout;
	font->LayoutText(_text, style.FontSize, width, _wrap, _maxLines, _bEllipsis, layout);
	return layout.Size;
}

HGameUITextStyle PGameUIText::ResolveStyle(const PGameUIManager* manager) const
{
	HGameUITextStyle style = (manager != nullptr) ? manager->GetDefaultTextStyle() : HGameUITextStyle();
	if ((_overrides & OverrideFont) != 0)
	{
		style.Font = _style.Font;
	}
	if ((_overrides & OverrideSize) != 0)
	{
		style.FontSize = _style.FontSize;
	}
	if ((_overrides & OverrideColor) != 0)
	{
		style.Color = _style.Color;
	}
	return style;
}

PSharedPtr<PGameUIFont> PGameUIText::ResolveFont(const PSharedPtr<PGameUIFont>& font, PGameUIManager* manager)
{
	if (font.IsValid() && font->IsValid())
	{
		return font;
	}
	if (manager == nullptr)
	{
		return nullptr;
	}

	const PSharedPtr<PGameUIFont>& styleFont = manager->GetDefaultTextStyle().Font;
	if (styleFont.IsValid() && styleFont->IsValid())
	{
		return styleFont;
	}
	return manager->GetDefaultFont();
}

void PGameUIText::onBuildDraw(HGameUIDrawContext& context)
{
	const HGameUITextStyle style = ResolveStyle(context.Manager);
	PSharedPtr<PGameUIFont> font = ResolveFont(style.Font, context.Manager);
	if (font.IsValid() == false || _text.Empty())
	{
		return;
	}

	HGameUITextLayout layout;
	font->LayoutText(_text, style.FontSize, _layoutRect.right - _layoutRect.left, _wrap, _maxLines, _bEllipsis, layout);

	if (_bClip == false)
	{
		AddTextDraw(context, *font, _text, layout, style.FontSize, _layoutRect, style.Color, _horizontalAlign, _verticalAlign);
		return;
	}

	// 잘라내기: 부모 클립 ∩ 요소 사각형(픽셀). 이 요소의 글자에만 쓴다(자식은 받은 클립 그대로).
	HGameUIDrawContext clipped = context;
	clipped.Clip.left   = HMath::Max(context.Clip.left, _layoutRect.left * context.Scale);
	clipped.Clip.top    = HMath::Max(context.Clip.top, _layoutRect.top * context.Scale);
	clipped.Clip.right  = HMath::Min(context.Clip.right, _layoutRect.right * context.Scale);
	clipped.Clip.bottom = HMath::Min(context.Clip.bottom, _layoutRect.bottom * context.Scale);
	if (clipped.Clip.right <= clipped.Clip.left || clipped.Clip.bottom <= clipped.Clip.top)
	{
		return;
	}
	AddTextDraw(clipped, *font, _text, layout, style.FontSize, _layoutRect, style.Color, _horizontalAlign, _verticalAlign);
}

void PGameUIText::AddTextDraw(HGameUIDrawContext& context, PGameUIFont& font, const PString& text, float32 fontSize,
	const HRect& rect, const HLinearColor& color, EGameUIHorizontalAlign horizontal, EGameUIVerticalAlign vertical)
{
	if (context.DrawList == nullptr || font.IsValid() == false || text.Empty())
	{
		return;
	}

	HGameUITextLayout layout;
	font.LayoutText(text, fontSize, 0.0f, EGameUITextWrap::None, 0, false, layout);
	AddTextDraw(context, font, text, layout, fontSize, rect, color, horizontal, vertical);
}

void PGameUIText::AddTextDraw(HGameUIDrawContext& context, PGameUIFont& font, const PString& text, const HGameUITextLayout& layout, float32 fontSize,
	const HRect& rect, const HLinearColor& color, EGameUIHorizontalAlign horizontal, EGameUIVerticalAlign vertical)
{
	if (context.DrawList == nullptr || font.IsValid() == false || layout.Lines.empty())
	{
		return;
	}

	// 글리프는 화면 배율만큼 큰 픽셀 크기로 래스터화하고(배율이 바뀌어도 선명하다), 자리는 논리 배치 × 배율로 정한다.
	const float32 scale       = context.Scale;
	const int32   pixelSize   = HMath::Max(1, (int32)(fontSize * scale + 0.5f));
	const float32 lineHeight  = layout.LineHeight * scale;
	const float32 ascent      = font.MeasureAscent(fontSize) * scale;
	const HRect   pixelRect(rect.left * scale, rect.top * scale, rect.right * scale, rect.bottom * scale);
	const float32 blockHeight = lineHeight * (float32)layout.Lines.size();

	float32 lineTop = pixelRect.top;
	if (vertical == EGameUIVerticalAlign::Middle)
	{
		lineTop = pixelRect.top + ((pixelRect.bottom - pixelRect.top) - blockHeight) * 0.5f;
	}
	else if (vertical == EGameUIVerticalAlign::Bottom)
	{
		lineTop = pixelRect.bottom - blockHeight;
	}

	const char* base = text.GetCStr();
	for (const HGameUITextLine& line : layout.Lines)
	{
		const float32 lineWidth = line.Width * scale;
		float32 penX = pixelRect.left;
		if (horizontal == EGameUIHorizontalAlign::Center)
		{
			penX = pixelRect.left + ((pixelRect.right - pixelRect.left) - lineWidth) * 0.5f;
		}
		else if (horizontal == EGameUIHorizontalAlign::Right)
		{
			penX = pixelRect.right - lineWidth;
		}

		// 기준선을 정수 픽셀에 맞춘다.
		const float32 baseline = HMath::FloorToFloat32(lineTop + ascent + 0.5f);

		uint32 previous = 0;
		const char* cursor  = base + line.Begin;
		const char* lineEnd = base + line.End;
		uint32 codepoint = 0;
		while (HGameUIUtf8::Next(cursor, lineEnd, codepoint))
		{
			if (previous != 0)
			{
				penX += font.MeasureKerning(previous, codepoint, fontSize) * scale;
			}
			addGlyphQuad(context, font, codepoint, pixelSize, penX, baseline, color);
			penX += font.MeasureAdvance(codepoint, fontSize) * scale;
			previous = codepoint;
		}

		if (line.bEllipsis)
		{
			for (int32 i = 0; i < layout.EllipsisCount; ++i)
			{
				addGlyphQuad(context, font, layout.EllipsisCodepoint, pixelSize, penX, baseline, color);
				penX += font.MeasureAdvance(layout.EllipsisCodepoint, fontSize) * scale;
			}
		}

		lineTop += lineHeight;
	}
}
