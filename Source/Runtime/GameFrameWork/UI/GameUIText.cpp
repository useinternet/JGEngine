#include "PCH/PCH.h"
#include "UI/GameUIText.h"
#include "UI/GameUIFont.h"
#include "UI/GameUIDrawList.h"
#include "UI/GameUIManager.h"
#include "UI/GameUIUtf8.h"
#include <cstring>
#include <cstdlib>

namespace
{
	// 글자 하나를 서식대로 그린다. 장평 · 굵게는 그 변형으로 래스터화한 글리프, 기울임은 기준선 축으로 비튼 사각형.
	// 배치는 글리프가 있는 글꼴(대체 글꼴일 수 있다) · 아틀라스 페이지로 묶인다.
	// rasterToPixel = 래스터 픽셀 → 변환 전 렌더 타깃 픽셀(변환이 없으면 1). 변환이 있으면 그 배율로 크게 래스터화해 두었으니 줄여 그리고,
	// 그리기 목록이 다시 키워 화면에서 1:1 이 된다. bSnap 이면 펜 위치를 정수 픽셀에 맞춘다(변환이 없을 때).
	void addGlyphQuad(HGameUIDrawContext& context, PGameUIFont& font, uint32 codepoint, int32 pixelSize, const HGameUITextRunStyle& run,
		float32 penX, float32 baseline, const HLinearColor& color, float32 rasterToPixel, bool bSnap)
	{
		HGameUIGlyphVariant variant;
		variant.HorizontalScale = run.HorizontalScale;
		variant.bBold           = run.bBold;
		variant.BoldStrength    = run.BoldStrength;
		const PGameUIFont::HGlyph glyph = font.GetGlyph(codepoint, pixelSize, variant);
		if (glyph.bInAtlas == false)
		{
			return;
		}

		// 글리프 위치를 정수 픽셀에 맞춘다. 아틀라스 텍셀과 화면 픽셀이 1:1 이 되어 흐려지지 않는다.
		const float32 pen    = bSnap ? HMath::FloorToFloat32(penX + 0.5f) : penX;
		const float32 left   = pen + (float32)glyph.OffsetX * rasterToPixel;
		const float32 top    = baseline + (float32)glyph.OffsetY * rasterToPixel;
		const float32 right  = left + (float32)glyph.Width * rasterToPixel;
		const float32 bottom = top + (float32)glyph.Height * rasterToPixel;
		if (run.bItalic == false)
		{
			context.DrawList->AddQuad(HRect(left, top, right, bottom), glyph.UV, color, nullptr, glyph.Font, context.Clip, glyph.Page);
			return;
		}

		// 합성 기울임: 기준선을 축으로 비튼다(기준선 위는 오른쪽, 아래는 왼쪽으로).
		const float32  topShift    = (baseline - top) * PGameUIFont::ItalicSkew;
		const float32  bottomShift = (baseline - bottom) * PGameUIFont::ItalicSkew;
		const HVector2 corners[4]  =
		{
			HVector2(left + topShift, top),
			HVector2(right + topShift, top),
			HVector2(right + bottomShift, bottom),
			HVector2(left + bottomShift, bottom),
		};
		context.DrawList->AddQuad(corners, glyph.UV, color, nullptr, glyph.Font, context.Clip, glyph.Page);
	}

	// ---- 글 안 마크업

	// 굵게 · 기울임은 깊이, 색 · 장평은 쌓는다(닫으면 바로 앞 값으로).
	struct HMarkupState
	{
		int32               BoldDepth   = 0;
		int32               ItalicDepth = 0;
		HList<HLinearColor> Colors;
		HList<float32>      Scales;

		HGameUITextRunStyle ToStyle() const
		{
			HGameUITextRunStyle style;
			style.bBold   = BoldDepth > 0;
			style.bItalic = ItalicDepth > 0;
			for (float32 scale : Scales)
			{
				style.HorizontalScale *= scale;
			}
			if (Colors.empty() == false)
			{
				style.bHasColor = true;
				style.Color     = Colors.back();
			}
			return style;
		}
	};

	bool isSameRunStyle(const HGameUITextRunStyle& a, const HGameUITextRunStyle& b)
	{
		if (a.bBold != b.bBold || a.bItalic != b.bItalic || a.HorizontalScale != b.HorizontalScale || a.bHasColor != b.bHasColor)
		{
			return false;
		}
		return a.bHasColor == false || (a.Color.R == b.Color.R && a.Color.G == b.Color.G && a.Color.B == b.Color.B && a.Color.A == b.Color.A);
	}

	int32 hexDigit(char character)
	{
		if (character >= '0' && character <= '9')
		{
			return character - '0';
		}
		if (character >= 'a' && character <= 'f')
		{
			return character - 'a' + 10;
		}
		if (character >= 'A' && character <= 'F')
		{
			return character - 'A' + 10;
		}
		return -1;
	}

	// "#RRGGBB" 또는 "#RRGGBBAA" (화면에 보일 sRGB 값 그대로)
	bool parseHexColor(const std::string& value, HLinearColor& outColor)
	{
		if ((value.size() != 7 && value.size() != 9) || value[0] != '#')
		{
			return false;
		}

		int32 channels[4] = { 0, 0, 0, 255 };
		const size_t channelCount = (value.size() - 1) / 2;
		for (size_t i = 0; i < channelCount; ++i)
		{
			const int32 high = hexDigit(value[1 + i * 2]);
			const int32 low  = hexDigit(value[2 + i * 2]);
			if (high < 0 || low < 0)
			{
				return false;
			}
			channels[i] = high * 16 + low;
		}
		outColor = HLinearColor(channels[0] / 255.0f, channels[1] / 255.0f, channels[2] / 255.0f, channels[3] / 255.0f);
		return true;
	}

	// 장평 퍼센트(예: "140", "93.5")
	bool parseScalePercent(const std::string& value, float32& outScale)
	{
		if (value.empty())
		{
			return false;
		}

		char* parsedEnd = nullptr;
		const float32 percent = std::strtof(value.c_str(), &parsedEnd);
		if (parsedEnd == nullptr || *parsedEnd != '\0' || percent <= 0.0f)
		{
			return false;
		}
		outScale = HMath::Clamp(percent / 100.0f, 0.1f, 10.0f);
		return true;
	}

	// 꺾쇠 안쪽(tag)을 상태에 적용한다. 알아보면 true(모르는 태그 · 짝 없는 닫는 태그는 false — 글자 그대로 둔다).
	bool applyMarkupTag(const std::string& tag, HMarkupState& state)
	{
		if (tag == "b")
		{
			++state.BoldDepth;
			return true;
		}
		if (tag == "/b")
		{
			if (state.BoldDepth == 0)
			{
				return false;
			}
			--state.BoldDepth;
			return true;
		}
		if (tag == "i")
		{
			++state.ItalicDepth;
			return true;
		}
		if (tag == "/i")
		{
			if (state.ItalicDepth == 0)
			{
				return false;
			}
			--state.ItalicDepth;
			return true;
		}
		if (tag.compare(0, 6, "color=") == 0)
		{
			HLinearColor color;
			if (parseHexColor(tag.substr(6), color) == false)
			{
				return false;
			}
			state.Colors.push_back(color);
			return true;
		}
		if (tag == "/color")
		{
			if (state.Colors.empty())
			{
				return false;
			}
			state.Colors.pop_back();
			return true;
		}
		if (tag.compare(0, 7, "hscale=") == 0)
		{
			float32 scale = 1.0f;
			if (parseScalePercent(tag.substr(7), scale) == false)
			{
				return false;
			}
			state.Scales.push_back(scale);
			return true;
		}
		if (tag == "/hscale")
		{
			if (state.Scales.empty())
			{
				return false;
			}
			state.Scales.pop_back();
			return true;
		}
		return false;
	}

	// 마크업 → 표시 글 + 구간 서식(서식 없는 범위는 구간을 만들지 않는다).
	void parseMarkup(const PString& source, std::string& outDisplay, HList<HGameUITextSpan>& outSpans)
	{
		struct HEntity
		{
			const char* Name;
			char        Value;
		};
		static const HEntity Entities[] =
		{
			{ "&lt;", '<' },
			{ "&gt;", '>' },
			{ "&amp;", '&' },
		};

		outDisplay.clear();
		outSpans.clear();

		HMarkupState        state;
		HGameUITextRunStyle current;
		uint32              currentBegin = 0;
		auto closeRun = [&outDisplay, &outSpans, &current, &currentBegin]()
		{
			if (isSameRunStyle(current, HGameUITextRunStyle()) == false && (uint32)outDisplay.size() > currentBegin)
			{
				HGameUITextSpan span;
				span.Begin = currentBegin;
				span.End   = (uint32)outDisplay.size();
				span.Style = current;
				outSpans.push_back(span);
			}
		};

		const char* cursor = source.GetCStr();
		const char* end    = cursor + source.Size();
		while (cursor < end)
		{
			if (*cursor == '<')
			{
				const char* close = cursor + 1;
				while (close < end && *close != '>' && *close != '<')
				{
					++close;
				}
				if (close < end && *close == '>')
				{
					HMarkupState next = state;
					if (applyMarkupTag(std::string(cursor + 1, close), next))
					{
						const HGameUITextRunStyle style = next.ToStyle();
						if (isSameRunStyle(style, current) == false)
						{
							closeRun();
							current      = style;
							currentBegin = (uint32)outDisplay.size();
						}
						state  = next;
						cursor = close + 1;
						continue;
					}
				}
			}
			else if (*cursor == '&')
			{
				bool bMatched = false;
				for (const HEntity& entity : Entities)
				{
					const size_t length = std::strlen(entity.Name);
					if ((size_t)(end - cursor) >= length && std::strncmp(cursor, entity.Name, length) == 0)
					{
						outDisplay.push_back(entity.Value);
						cursor  += length;
						bMatched = true;
						break;
					}
				}
				if (bMatched)
				{
					continue;
				}
			}

			outDisplay.push_back(*cursor);
			++cursor;
		}
		closeRun();
	}
}

void PGameUIText::SetText(const PString& text)
{
	_text = text;
	rebuildDisplayText();
}

const PString& PGameUIText::GetText() const
{
	return _text;
}

const PString& PGameUIText::GetDisplayText() const
{
	return _displayText;
}

void PGameUIText::SetRichText(bool bRichText)
{
	if (_bRichText == bRichText)
	{
		return;
	}
	_bRichText = bRichText;
	rebuildDisplayText();
}

bool PGameUIText::IsRichText() const
{
	return _bRichText;
}

const HList<HGameUITextSpan>& PGameUIText::GetSpans() const
{
	return _spans;
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

HLinearColor PGameUIText::GetColor(const PGameUIManager* manager) const
{
	return ResolveStyle(manager).Color;
}

void PGameUIText::ClearStyle()
{
	_style     = HGameUITextStyle();
	_overrides = 0;
}

void PGameUIText::SetParagraphStyle(const HGameUIParagraphStyle& style)
{
	_paragraph = style;
}

const HGameUIParagraphStyle& PGameUIText::GetParagraphStyle() const
{
	return _paragraph;
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
	HGameUITextLayout layout;
	if (LayoutContent(width, manager, layout) == false)
	{
		return HVector2(0.0f, 0.0f);
	}
	return layout.Size;
}

bool PGameUIText::LayoutContent(float32 width, PGameUIManager* manager, HGameUITextLayout& outLayout) const
{
	const HGameUITextStyle style = ResolveStyle(manager);
	PSharedPtr<PGameUIFont> font = ResolveFont(style.Font, manager);
	if (font.IsValid() == false)
	{
		outLayout = HGameUITextLayout();
		return false;
	}
	font->LayoutText(_displayText, makeLayoutOptions(style.FontSize, width), outLayout);
	return true;
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
	if (font.IsValid() == false || _displayText.Empty())
	{
		return;
	}

	HGameUITextLayout layout;
	font->LayoutText(_displayText, makeLayoutOptions(style.FontSize, _layoutRect.right - _layoutRect.left), layout);

	if (_bClip == false)
	{
		AddTextDraw(context, *font, _displayText, layout, style.FontSize, _layoutRect, style.Color, _horizontalAlign, _verticalAlign);
		return;
	}

	// 잘라내기: 부모 클립 ∩ 요소 사각형(픽셀). 이 요소의 글자에만 쓴다(자식은 받은 클립 그대로).
	// 그리기 변환이 있으면 변환한 사각형을 감싸는 축 정렬 사각형(시저는 축 정렬이라 돌린 모양 그대로는 못 자른다).
	const HRect bounds = LocalToCanvasBounds(_layoutRect);
	HGameUIDrawContext clipped = context;
	clipped.Clip.left   = HMath::Max(context.Clip.left, bounds.left * context.Scale);
	clipped.Clip.top    = HMath::Max(context.Clip.top, bounds.top * context.Scale);
	clipped.Clip.right  = HMath::Min(context.Clip.right, bounds.right * context.Scale);
	clipped.Clip.bottom = HMath::Min(context.Clip.bottom, bounds.bottom * context.Scale);
	if (clipped.Clip.right <= clipped.Clip.left || clipped.Clip.bottom <= clipped.Clip.top)
	{
		return;
	}
	AddTextDraw(clipped, *font, _displayText, layout, style.FontSize, _layoutRect, style.Color, _horizontalAlign, _verticalAlign);
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
	if (context.DrawList == nullptr || font.IsValid() == false || layout.Lines.empty() || layout.Runs.empty())
	{
		return;
	}

	// 글리프는 화면 배율만큼 큰 픽셀 크기로 래스터화하고(배율이 바뀌어도 선명하다), 자리는 논리 배치 × 배율로 정한다.
	// 그리기 변환이 있으면(ER-014) 화면 배율 × 변환 배율의 정수 픽셀로 래스터화해 키워도 선명하게 하고, 그만큼 줄여 넣는다
	// (그리기 목록이 변환으로 다시 키워 화면에서 1:1). 이때는 정수 픽셀 맞춤을 하지 않는다.
	const float32 scale          = context.Scale;
	const bool    bTransformed   = context.DrawList->Transform.IsIdentity() == false;
	const float32 transformScale = bTransformed ? context.DrawList->Transform.GetUniformScale() : 1.0f;
	const int32   pixelSize      = HMath::Max(1, (int32)(fontSize * scale * transformScale + 0.5f));
	const float32 rasterToPixel  = bTransformed ? (fontSize * scale) / (float32)pixelSize : 1.0f;
	const HRect   pixelRect(rect.left * scale, rect.top * scale, rect.right * scale, rect.bottom * scale);
	const float32 blockHeight = layout.Size.y * scale;

	float32 blockTop = pixelRect.top;
	if (vertical == EGameUIVerticalAlign::Middle)
	{
		blockTop = pixelRect.top + ((pixelRect.bottom - pixelRect.top) - blockHeight) * 0.5f;
	}
	else if (vertical == EGameUIVerticalAlign::Bottom)
	{
		blockTop = pixelRect.bottom - blockHeight;
	}

	for (const HGameUITextLine& line : layout.Lines)
	{
		// 양쪽 맞춤 줄은 폭이 가득 차 있고, 맞추지 않은 줄(문단 마지막 줄)은 왼쪽.
		const float32 lineWidth = line.Width * scale;
		float32 lineLeft = pixelRect.left;
		if (horizontal == EGameUIHorizontalAlign::Center)
		{
			lineLeft = pixelRect.left + ((pixelRect.right - pixelRect.left) - lineWidth) * 0.5f;
		}
		else if (horizontal == EGameUIHorizontalAlign::Right)
		{
			lineLeft = pixelRect.right - lineWidth;
		}

		// 기준선을 정수 픽셀에 맞춘다(변환이 없을 때).
		const float32 baselineExact = blockTop + (line.Top + layout.BaselineOffset) * scale;
		const float32 baseline      = bTransformed ? baselineExact : HMath::FloorToFloat32(baselineExact + 0.5f);
		for (uint32 i = 0; i < line.GlyphCount; ++i)
		{
			const HGameUITextGlyph&    glyph = layout.Glyphs[line.GlyphBegin + i];
			const HGameUITextRunStyle& run   = layout.Runs[glyph.Run];
			addGlyphQuad(context, font, glyph.Codepoint, pixelSize, run, lineLeft + glyph.X * scale, baseline, run.bHasColor ? run.Color : color,
				rasterToPixel, bTransformed == false);
		}
	}
}

void PGameUIText::rebuildDisplayText()
{
	if (_bRichText == false)
	{
		_displayText = _text;
		_spans.clear();
		return;
	}

	std::string display;
	parseMarkup(_text, display, _spans);
	_displayText = PString(display.c_str());
}

HGameUITextLayoutOptions PGameUIText::makeLayoutOptions(float32 fontSize, float32 width) const
{
	HGameUITextLayoutOptions options;
	options.FontSize                  = fontSize;
	options.WrapWidth                 = width;
	options.Wrap                      = _wrap;
	options.MaxLines                  = _maxLines;
	options.bEllipsis                 = _bEllipsis;
	options.bJustify                  = _horizontalAlign == EGameUIHorizontalAlign::Justify;
	options.LineHeight                = _paragraph.LineHeight;
	options.LetterSpacing             = _paragraph.LetterSpacing;
	options.ParagraphSpacing          = _paragraph.ParagraphSpacing;
	options.BaseStyle.bBold           = _paragraph.bBold;
	options.BaseStyle.bItalic         = _paragraph.bItalic;
	options.BaseStyle.HorizontalScale = _paragraph.HorizontalScale;
	options.BaseStyle.BoldStrength    = _paragraph.BoldStrength;
	options.Spans                     = _spans.empty() ? nullptr : &_spans;
	return options;
}
