#include "PCH/PCH.h"
#include "UI/GameUIFont.h"
#include "UI/GameUIFile.h"
#include "UI/GameUIUtf8.h"
#include "JGGraphics.h"
#include "Classes/Texture.h"

// stb_truetype 구현을 이 DLL 안에만 둔다(STBTT_STATIC). GUI.dll 의 ImGui 사본과 기호가 겹치지 않는다.
#pragma warning(push, 0)
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#pragma warning(pop)

namespace
{
	// 딱 맞는 폭에서 실수 오차로 줄이 바뀌지 않게 둔 여유(논리 단위)
	constexpr float32 WrapTolerance = 0.01f;
	constexpr float32 UnlimitedWidth = 3.0e38f;

	// 줄을 바꿀 때 지우는 공백(줄 끝 · 다음 줄 앞)
	bool isBreakSpace(uint32 codepoint)
	{
		return codepoint == ' ' || codepoint == '\t' || codepoint == 0x3000;
	}

	// 띄어쓰기 없이 글자 사이에서 줄을 바꿀 수 있는 글자: 한자 · 가나 · 전각 형태. 한글은 어절(공백) 단위로 둔다.
	bool isIdeographic(uint32 codepoint)
	{
		return (codepoint >= 0x2E80 && codepoint <= 0x2FDF)
			|| (codepoint >= 0x3040 && codepoint <= 0x30FF)
			|| (codepoint >= 0x3400 && codepoint <= 0x4DBF)
			|| (codepoint >= 0x4E00 && codepoint <= 0x9FFF)
			|| (codepoint >= 0xF900 && codepoint <= 0xFAFF)
			|| (codepoint >= 0xFF00 && codepoint <= 0xFFEF);
	}

	// 줄 첫머리에 두지 않는 글자(닫는 부호 · 문장부호). 줄을 바꿀 때 앞 글자와 함께 다음 줄로 넘긴다.
	const uint32 NoLineStartCodepoints[] =
	{
		',', '.', '!', '?', ';', ':', ')', ']', '}', '%',
		0x2019, 0x201D, 0x2026,                     // ’ ” …
		0x3001, 0x3002, 0x3009, 0x300B, 0x300D, 0x300F,   // 、 。 〉 》 」 』
		0xFF01, 0xFF09, 0xFF0C, 0xFF0E, 0xFF1F,     // ！ ） ， ． ？
	};

	// 줄 끝에 두지 않는 글자(여는 부호). 줄을 바꿀 때 뒤 글자와 함께 다음 줄로 넘긴다.
	const uint32 NoLineEndCodepoints[] =
	{
		'(', '[', '{',
		0x2018, 0x201C,                             // ‘ “
		0x3008, 0x300A, 0x300C, 0x300E,             // 〈 《 「 『
		0xFF08,                                     // （
	};

	bool isNoLineStart(uint32 codepoint)
	{
		for (uint32 noLineStart : NoLineStartCodepoints)
		{
			if (noLineStart == codepoint)
			{
				return true;
			}
		}
		return false;
	}

	bool isNoLineEnd(uint32 codepoint)
	{
		for (uint32 noLineEnd : NoLineEndCodepoints)
		{
			if (noLineEnd == codepoint)
			{
				return true;
			}
		}
		return false;
	}
}

PGameUIFont::~PGameUIFont()
{
	delete _fontInfo;
	_fontInfo = nullptr;
}

bool PGameUIFont::LoadFromFile(const PString& path)
{
	_path = path;
	if (HGameUIFile::ReadAllBytes(path, _fontData) == false)
	{
		JG_LOG(GameUI, ELogLevel::Error, "Font file is not readable : %s", path);
		return false;
	}

	const int32 fontOffset = stbtt_GetFontOffsetForIndex(_fontData.data(), 0);
	if (fontOffset < 0)
	{
		JG_LOG(GameUI, ELogLevel::Error, "Not a TrueType font : %s", path);
		_fontData.clear();
		return false;
	}

	_fontInfo = new stbtt_fontinfo();
	if (stbtt_InitFont(_fontInfo, _fontData.data(), fontOffset) == 0)
	{
		JG_LOG(GameUI, ELogLevel::Error, "Fail to read font tables : %s", path);
		delete _fontInfo;
		_fontInfo = nullptr;
		_fontData.clear();
		return false;
	}

	// RGB 는 흰색, 알파(커버리지)만 0. 선형 필터가 글리프 가장자리에서 빈 텍셀을 섞어도 색이 어두워지지 않는다.
	_atlasPixels.assign((size_t)AtlasSize * AtlasSize * 4, 255);
	for (size_t i = 3; i < _atlasPixels.size(); i += 4)
	{
		_atlasPixels[i] = 0;
	}
	_bAtlasDirty = true;
	return true;
}

bool PGameUIFont::IsValid() const
{
	return _fontInfo != nullptr;
}

const PString& PGameUIFont::GetPath() const
{
	return _path;
}

float32 PGameUIFont::getScale(int32 pixelSize) const
{
	// 글자 크기 = em 높이(픽셀). CSS 의 font-size 와 같은 뜻.
	return stbtt_ScaleForMappingEmToPixels(_fontInfo, (float32)pixelSize);
}

float32 PGameUIFont::GetAscent(int32 pixelSize) const
{
	if (IsValid() == false)
	{
		return 0.0f;
	}

	int32 ascent  = 0;
	int32 descent = 0;
	int32 lineGap = 0;
	stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &lineGap);
	return ascent * getScale(pixelSize);
}

float32 PGameUIFont::GetLineHeight(int32 pixelSize) const
{
	if (IsValid() == false)
	{
		return 0.0f;
	}

	int32 ascent  = 0;
	int32 descent = 0;
	int32 lineGap = 0;
	stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &lineGap);
	return (ascent - descent + lineGap) * getScale(pixelSize);
}

float32 PGameUIFont::GetKerning(uint32 first, uint32 second, int32 pixelSize) const
{
	if (IsValid() == false)
	{
		return 0.0f;
	}
	return stbtt_GetCodepointKernAdvance(_fontInfo, (int32)first, (int32)second) * getScale(pixelSize);
}

bool PGameUIFont::HasGlyph(uint32 codepoint) const
{
	if (IsValid() == false)
	{
		return false;
	}
	return stbtt_FindGlyphIndex(_fontInfo, (int32)codepoint) != 0;
}

const PGameUIFont::HGlyph& PGameUIFont::GetGlyph(uint32 codepoint, int32 pixelSize)
{
	const uint64 key = ((uint64)(uint32)pixelSize << 32) | codepoint;
	auto found = _glyphs.find(key);
	if (found != _glyphs.end())
	{
		return found->second;
	}

	HGlyph glyph;
	if (IsValid() && pixelSize > 0)
	{
		const float32 scale = getScale(pixelSize);

		int32 advance     = 0;
		int32 leftBearing = 0;
		stbtt_GetCodepointHMetrics(_fontInfo, (int32)codepoint, &advance, &leftBearing);
		glyph.Advance = advance * scale;

		// 비트맵 상자. y 는 아래로 + 이고 기준선이 0 이라 y0 은 보통 음수다.
		int32 x0 = 0;
		int32 y0 = 0;
		int32 x1 = 0;
		int32 y1 = 0;
		stbtt_GetCodepointBitmapBox(_fontInfo, (int32)codepoint, scale, scale, &x0, &y0, &x1, &y1);
		const int32 width  = x1 - x0;
		const int32 height = y1 - y0;
		glyph.OffsetX = x0;
		glyph.OffsetY = y0;

		int32 atlasX = 0;
		int32 atlasY = 0;
		if (width > 0 && height > 0 && packGlyph(width, height, atlasX, atlasY))
		{
			std::vector<uint8> coverage((size_t)width * height);
			stbtt_MakeCodepointBitmap(_fontInfo, coverage.data(), width, height, width, scale, scale, (int32)codepoint);

			for (int32 y = 0; y < height; ++y)
			{
				uint8* row = &_atlasPixels[((size_t)(atlasY + y) * AtlasSize + atlasX) * 4];
				for (int32 x = 0; x < width; ++x)
				{
					row[x * 4 + 3] = coverage[(size_t)y * width + x];
				}
			}

			glyph.Width    = width;
			glyph.Height   = height;
			glyph.UV       = HRect((float32)atlasX / AtlasSize, (float32)atlasY / AtlasSize, (float32)(atlasX + width) / AtlasSize, (float32)(atlasY + height) / AtlasSize);
			glyph.bInAtlas = true;
			_bAtlasDirty   = true;
		}
	}

	return _glyphs.emplace(key, glyph).first->second;
}

HVector2 PGameUIFont::MeasureText(const PString& text, int32 pixelSize)
{
	if (IsValid() == false || text.Empty())
	{
		return HVector2(0.0f, 0.0f);
	}

	float32 maxWidth  = 0.0f;
	float32 lineWidth = 0.0f;
	int32   lineCount = 1;
	uint32  previous  = 0;

	const char* cursor = text.GetCStr();
	const char* end    = cursor + text.Size();
	uint32 codepoint = 0;
	while (HGameUIUtf8::Next(cursor, end, codepoint))
	{
		if (codepoint == '\n')
		{
			maxWidth  = HMath::Max(maxWidth, lineWidth);
			lineWidth = 0.0f;
			previous  = 0;
			++lineCount;
			continue;
		}

		if (previous != 0)
		{
			lineWidth += GetKerning(previous, codepoint, pixelSize);
		}
		lineWidth += GetGlyph(codepoint, pixelSize).Advance;
		previous = codepoint;
	}
	maxWidth = HMath::Max(maxWidth, lineWidth);

	return HVector2(maxWidth, lineCount * GetLineHeight(pixelSize));
}

float32 PGameUIFont::MeasureAdvance(uint32 codepoint, float32 fontSize) const
{
	if (IsValid() == false || fontSize <= 0.0f)
	{
		return 0.0f;
	}

	int32 advance     = 0;
	int32 leftBearing = 0;
	stbtt_GetCodepointHMetrics(_fontInfo, (int32)codepoint, &advance, &leftBearing);
	return advance * stbtt_ScaleForMappingEmToPixels(_fontInfo, fontSize);
}

float32 PGameUIFont::MeasureKerning(uint32 first, uint32 second, float32 fontSize) const
{
	if (IsValid() == false || fontSize <= 0.0f)
	{
		return 0.0f;
	}
	return stbtt_GetCodepointKernAdvance(_fontInfo, (int32)first, (int32)second) * stbtt_ScaleForMappingEmToPixels(_fontInfo, fontSize);
}

float32 PGameUIFont::MeasureAscent(float32 fontSize) const
{
	if (IsValid() == false || fontSize <= 0.0f)
	{
		return 0.0f;
	}

	int32 ascent  = 0;
	int32 descent = 0;
	int32 lineGap = 0;
	stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &lineGap);
	return ascent * stbtt_ScaleForMappingEmToPixels(_fontInfo, fontSize);
}

float32 PGameUIFont::MeasureLineHeight(float32 fontSize) const
{
	if (IsValid() == false || fontSize <= 0.0f)
	{
		return 0.0f;
	}

	int32 ascent  = 0;
	int32 descent = 0;
	int32 lineGap = 0;
	stbtt_GetFontVMetrics(_fontInfo, &ascent, &descent, &lineGap);
	return (ascent - descent + lineGap) * stbtt_ScaleForMappingEmToPixels(_fontInfo, fontSize);
}

void PGameUIFont::LayoutText(const PString& text, float32 fontSize, float32 wrapWidth, EGameUITextWrap wrap, int32 maxLines, bool bEllipsis, HGameUITextLayout& outLayout) const
{
	outLayout.Lines.clear();
	outLayout.Size              = HVector2(0.0f, 0.0f);
	outLayout.LineHeight        = 0.0f;
	outLayout.bTruncated        = false;
	outLayout.EllipsisCodepoint = 0x2026;
	outLayout.EllipsisCount     = 1;
	if (IsValid() == false || fontSize <= 0.0f || text.Empty())
	{
		return;
	}

	outLayout.LineHeight = MeasureLineHeight(fontSize);
	if (HasGlyph(0x2026) == false)
	{
		outLayout.EllipsisCodepoint = '.';
		outLayout.EllipsisCount     = 3;
	}

	// 단락('\n' 사이)마다 줄로 나눈다. 최대 줄 수를 넘었으면 남은 단락은 볼 필요가 없다.
	const char* base      = text.GetCStr();
	const char* end       = base + text.Size();
	const char* paragraph = base;
	while (true)
	{
		const char* paragraphEnd = paragraph;
		while (paragraphEnd < end && *paragraphEnd != '\n')
		{
			++paragraphEnd;
		}
		layoutParagraph(base, paragraph, paragraphEnd, fontSize, wrapWidth, wrap, outLayout);

		if (maxLines > 0 && (int32)outLayout.Lines.size() > maxLines)
		{
			break;
		}
		if (paragraphEnd >= end)
		{
			break;
		}
		paragraph = paragraphEnd + 1;
	}

	if (maxLines > 0 && (int32)outLayout.Lines.size() > maxLines)
	{
		outLayout.Lines.resize((size_t)maxLines);
		outLayout.bTruncated = true;
		if (bEllipsis)
		{
			applyEllipsis(base, outLayout.Lines.back(), fontSize, wrapWidth > 0.0f ? wrapWidth : UnlimitedWidth, outLayout);
		}
	}

	// 줄을 바꾸지 않는 글자는 폭을 넘는 줄만 말줄임으로 줄인다(한 줄 이름 칸 등).
	if (bEllipsis && wrap == EGameUITextWrap::None && wrapWidth > 0.0f)
	{
		for (HGameUITextLine& line : outLayout.Lines)
		{
			if (line.bEllipsis == false && line.Width > wrapWidth + WrapTolerance)
			{
				applyEllipsis(base, line, fontSize, wrapWidth, outLayout);
			}
		}
	}

	float32 maxWidth = 0.0f;
	for (const HGameUITextLine& line : outLayout.Lines)
	{
		maxWidth = HMath::Max(maxWidth, line.Width);
	}
	outLayout.Size = HVector2(maxWidth, outLayout.LineHeight * (float32)outLayout.Lines.size());
}

void PGameUIFont::layoutParagraph(const char* base, const char* begin, const char* end, float32 fontSize, float32 wrapWidth, EGameUITextWrap wrap, HGameUITextLayout& outLayout) const
{
	const bool bWrap = wrap != EGameUITextWrap::None && wrapWidth > 0.0f;
	const char* lineStart  = begin;
	bool        bFirstLine = true;

	while (true)
	{
		if (bFirstLine == false)
		{
			// 바꾼 줄 앞의 공백은 버린다(단락 첫 줄의 들여쓰기는 둔다).
			while (lineStart < end)
			{
				const char* next = lineStart;
				uint32 codepoint = 0;
				HGameUIUtf8::Next(next, end, codepoint);
				if (isBreakSpace(codepoint) == false)
				{
					break;
				}
				lineStart = next;
			}
			if (lineStart >= end)
			{
				return;
			}
		}
		bFirstLine = false;

		float32     width              = 0.0f;       // 줄 시작부터 지금까지(공백 포함)
		float32     contentWidth       = 0.0f;       // 마지막 공백 아닌 글자까지
		const char* contentEnd         = lineStart;
		const char* breakAt            = nullptr;    // 여기서 바꾸면 다음 줄이 시작하는 곳
		const char* breakContentEnd    = nullptr;    // 그때 이 줄의 끝(끝 공백 제외)
		float32     breakWidth         = 0.0f;
		const char* previousGlyph      = nullptr;    // 앞 글자의 시작
		float32     previousStartWidth = 0.0f;       // 앞 글자 직전까지의 폭
		uint32      previous           = 0;
		bool        bBroken            = false;

		const char* cursor = lineStart;
		while (cursor < end)
		{
			const char* glyphBegin = cursor;
			uint32 codepoint = 0;
			HGameUIUtf8::Next(cursor, end, codepoint);

			float32 advance = MeasureAdvance(codepoint, fontSize);
			if (previous != 0)
			{
				advance += MeasureKerning(previous, codepoint, fontSize);
			}
			const bool bSpace = isBreakSpace(codepoint);

			if (bWrap)
			{
				// 이 글자 앞이 바꿀 수 있는 자리인가
				if (glyphBegin > lineStart && bSpace == false && isNoLineStart(codepoint) == false && isNoLineEnd(previous) == false)
				{
					bool bOpportunity = true;
					if (wrap == EGameUITextWrap::Word)
					{
						bOpportunity = isBreakSpace(previous) || isIdeographic(codepoint) || isIdeographic(previous);
					}
					if (bOpportunity)
					{
						breakAt         = glyphBegin;
						breakContentEnd = contentEnd;
						breakWidth      = contentWidth;
					}
				}

				// 넘친다: 마지막 바꿀 자리에서, 없으면 이 글자 앞에서 끊는다(공백은 넘쳐도 줄 끝에 매달아 둔다).
				if (bSpace == false && contentEnd > lineStart && width + advance > wrapWidth + WrapTolerance)
				{
					HGameUITextLine line;
					line.Begin = (uint32)(lineStart - base);
					if (breakAt != nullptr && breakContentEnd > lineStart)
					{
						line.End   = (uint32)(breakContentEnd - base);
						line.Width = breakWidth;
						lineStart  = breakAt;
					}
					else
					{
						// 바꿀 자리가 없다(줄보다 긴 단어): 이 글자 앞에서 끊는다. 닫는 부호면 앞 글자와 함께 넘긴다.
						const char* cut      = glyphBegin;
						float32     cutWidth = width;
						if (isNoLineStart(codepoint) && previousGlyph != nullptr && previousGlyph > lineStart)
						{
							cut      = previousGlyph;
							cutWidth = previousStartWidth;
						}
						if (contentEnd <= cut)
						{
							line.End   = (uint32)(contentEnd - base);
							line.Width = contentWidth;
						}
						else
						{
							line.End   = (uint32)(cut - base);
							line.Width = cutWidth;
						}
						lineStart = cut;
					}
					outLayout.Lines.push_back(line);
					bBroken = true;
					break;
				}
			}

			previousGlyph      = glyphBegin;
			previousStartWidth = width;
			width += advance;
			if (bSpace == false)
			{
				contentEnd   = cursor;
				contentWidth = width;
			}
			previous = codepoint;
		}

		if (bBroken)
		{
			continue;
		}

		// 단락의 마지막 줄. 줄을 바꾸는 글자는 끝 공백을 폭에서 뺀다(바꾸지 않으면 지금까지처럼 그대로).
		HGameUITextLine line;
		line.Begin = (uint32)(lineStart - base);
		if (bWrap)
		{
			line.End   = (uint32)(contentEnd - base);
			line.Width = contentWidth;
		}
		else
		{
			line.End   = (uint32)(end - base);
			line.Width = width;
		}
		outLayout.Lines.push_back(line);
		return;
	}
}

void PGameUIFont::applyEllipsis(const char* base, HGameUITextLine& line, float32 fontSize, float32 availableWidth, const HGameUITextLayout& layout) const
{
	const float32 ellipsisWidth = MeasureAdvance(layout.EllipsisCodepoint, fontSize) * (float32)layout.EllipsisCount;

	// 앞에서부터 글자를 더하며 (글자 + 말줄임)이 들어가는 마지막 자리를 찾는다. 그 자리 앞의 공백은 뺀다.
	const char* lineBegin = base + line.Begin;
	const char* lineEnd   = base + line.End;
	const char* cut       = lineBegin;
	float32     cutWidth  = 0.0f;
	float32     width     = 0.0f;
	uint32      previous  = 0;

	const char* cursor = lineBegin;
	while (cursor < lineEnd)
	{
		uint32 codepoint = 0;
		HGameUIUtf8::Next(cursor, lineEnd, codepoint);

		float32 advance = MeasureAdvance(codepoint, fontSize);
		if (previous != 0)
		{
			advance += MeasureKerning(previous, codepoint, fontSize);
		}
		if (width + advance + ellipsisWidth > availableWidth + WrapTolerance)
		{
			break;
		}

		width += advance;
		if (isBreakSpace(codepoint) == false)
		{
			cut      = cursor;
			cutWidth = width;
		}
		previous = codepoint;
	}

	line.End       = (uint32)(cut - base);
	line.Width     = cutWidth + ellipsisWidth;
	line.bEllipsis = true;
}

PSharedPtr<IRawTexture> PGameUIFont::GetAtlasTexture()
{
	if (IsValid() == false)
	{
		return nullptr;
	}
	if (_atlasTexture.IsValid() && _bAtlasDirty == false)
	{
		return _atlasTexture;
	}

	HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>();
	PSharedPtr<PJGGraphicsAPI> graphicsAPI = (graphicsModule != nullptr) ? graphicsModule->GetGraphicsAPI() : nullptr;
	if (graphicsAPI.IsValid() == false)
	{
		return nullptr;
	}

	PString fileName;
	HFileHelper::FileNameOnly(_path, &fileName);

	HTextureInfo textureInfo;
	textureInfo.Name       = PString::Format("GameUIFontAtlas_%s", fileName);
	textureInfo.Width      = AtlasSize;
	textureInfo.Height     = AtlasSize;
	textureInfo.Format     = ETextureFormat::R8G8B8A8_Unorm;
	textureInfo.Flags      = ETextureFlags::None;
	textureInfo.FilterMode = ETextureFilterMode::Linear;
	textureInfo.WrapMode   = ETextureWrapMode::Clamp;
	textureInfo.MipLevel   = 1;
	textureInfo.ArraySize  = 1;

	// 업로드는 이번 프레임 제출의 맨 앞(드로우보다 먼저)에 기록된다. 이전 텍스처는 GPU 가 끝낸 뒤 해제된다(DeferRelease).
	_atlasTexture = graphicsAPI->CreateRawTexture(_atlasPixels.data(), textureInfo);
	_bAtlasDirty  = false;
	return _atlasTexture;
}

uint64 PGameUIFont::GetGlyphCount() const
{
	return (uint64)_glyphs.size();
}

bool PGameUIFont::IsAtlasFull() const
{
	return _bAtlasFull;
}

const std::vector<uint8>& PGameUIFont::GetAtlasPixels() const
{
	return _atlasPixels;
}

bool PGameUIFont::packGlyph(int32 width, int32 height, int32& outX, int32& outY)
{
	if (_bAtlasFull)
	{
		return false;
	}
	if (width + GlyphPadding * 2 > AtlasSize || height + GlyphPadding * 2 > AtlasSize)
	{
		return false;
	}

	// 줄(선반) 단위로 채운다. 줄이 넘치면 다음 줄로.
	if (_packX + width + GlyphPadding > AtlasSize)
	{
		_packX = GlyphPadding;
		_packY += _packRowHeight + GlyphPadding;
		_packRowHeight = 0;
	}
	if (_packY + height + GlyphPadding > AtlasSize)
	{
		_bAtlasFull = true;
		JG_LOG(GameUI, ELogLevel::Warning, "Font atlas is full (%s). New glyphs are drawn without bitmaps", _path);
		return false;
	}

	outX = _packX;
	outY = _packY;
	_packX += width + GlyphPadding;
	_packRowHeight = HMath::Max(_packRowHeight, height);
	return true;
}
