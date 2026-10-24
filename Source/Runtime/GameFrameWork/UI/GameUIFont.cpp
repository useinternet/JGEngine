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

	// 양쪽 맞춤: 띄어쓰기 뒤 사이가 글자 사이보다 이만큼 더 받는다.
	constexpr float32 JustifySpaceWeight = 4.0f;
	// 장평 범위
	constexpr float32 MinHorizontalScale = 0.1f;
	constexpr float32 MaxHorizontalScale = 10.0f;

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

	// 장평 → 글리프 키의 퍼센트(1 ~ 2047)
	int32 toScalePercent(float32 horizontalScale)
	{
		return HMath::Clamp((int32)(horizontalScale * 100.0f + 0.5f), 1, 2047);
	}

	// 굵게 → 글리프 키 값(11비트). 0 = 굵게 아님, 2047 = 세기를 정하지 않은 굵게(기본 em/24 — 지금 넓힘),
	// 그 밖 = 정한 세기(1/10000 em, 1 ~ 2046 — 선명한 굵게, GG-19). 같은 em/24 라도 정한 것과 정하지 않은 것은 그림이 달라 따로 둔다.
	constexpr int32 DefaultBoldCode = 2047;

	int32 toBoldCode(bool bBold, float32 explicitStrength)
	{
		if (bBold == false)
		{
			return 0;
		}
		if (explicitStrength <= 0.0f)
		{
			return DefaultBoldCode;
		}
		const float32 strength = PGameUIFont::ResolveBoldStrength(explicitStrength);
		return HMath::Clamp((int32)(strength * 10000.0f + 0.5f), 1, DefaultBoldCode - 1);
	}

	// 글리프 키 = 코드포인트(21비트) | 픽셀 크기(12비트) | 장평 %(11비트) | 굵게 세기(11비트, 0 = 굵게 아님) — 세기가 다른 요소의 같은 글자는 따로 둔다
	uint64 makeGlyphKey(uint32 codepoint, int32 pixelSize, int32 scalePercent, int32 boldCode)
	{
		return (uint64)(codepoint & 0x1FFFFF)
			| ((uint64)(pixelSize & 0xFFF) << 21)
			| ((uint64)(scalePercent & 0x7FF) << 33)
			| ((uint64)(boldCode & 0x7FF) << 44);
	}

	uint32 glyphKeyCodepoint(uint64 key)
	{
		return (uint32)(key & 0x1FFFFF);
	}

	// 아틀라스 프레임(PGameUIFont::BeginFrame 마다 하나 는다). 페이지는 마지막으로 쓴 프레임을 기억한다.
	uint64 GFontFrame = 1;

	// 아틀라스 페이지를 비운다: RGB 는 흰색 그대로, 알파(커버리지)만 0.
	void clearCoverage(std::vector<uint8>& pixels)
	{
		for (size_t i = 3; i < pixels.size(); i += 4)
		{
			pixels[i] = 0;
		}
	}

	// 합성 굵게의 한쪽 넓힘(픽셀). 양쪽 합 = 글자 폭 증가(em × 세기)라 그림과 폭이 같이 는다.
	float32 boldRadius(int32 pixelSize, float32 strength)
	{
		return (float32)pixelSize * strength * 0.5f;
	}

	// 비트맵이 사방으로 늘어나는 칸 수(반지름의 올림).
	int32 boldPadding(float32 radius)
	{
		return HMath::CeilToInt(radius - 0.0001f);
	}

	// 줄 하나(또는 열 하나)를 radius 만큼 넓힌다: 거리 ≤ 정수부는 그대로 최댓값, 바로 바깥 한 겹은 커버리지 × 소수부(1픽셀보다 작은 넓힘).
	// source 는 stride 간격의 count 칸, out 은 같은 간격의 count + 2 × padding 칸.
	void dilateLine(const uint8* source, int32 count, int32 sourceStride, uint8* out, int32 outStride, float32 radius, int32 padding)
	{
		const int32   full     = (int32)radius;
		const float32 fraction = radius - (float32)full;
		const int32   outCount = count + padding * 2;
		for (int32 i = 0; i < outCount; ++i)
		{
			const int32 center = i - padding;   // 이 칸이 놓이는 원래 자리
			float32 value = 0.0f;
			for (int32 sourceIndex = center - full; sourceIndex <= center + full; ++sourceIndex)
			{
				if (sourceIndex >= 0 && sourceIndex < count)
				{
					value = HMath::Max(value, (float32)source[sourceIndex * sourceStride]);
				}
			}
			if (fraction > 0.0f)
			{
				const int32 ring[2] = { center - full - 1, center + full + 1 };
				for (int32 sourceIndex : ring)
				{
					if (sourceIndex >= 0 && sourceIndex < count)
					{
						value = HMath::Max(value, (float32)source[sourceIndex * sourceStride] * fraction);
					}
				}
			}
			out[i * outStride] = (uint8)(value + 0.5f);
		}
	}

	// 커버리지를 사방으로 radius(실수) 픽셀 넓힌다(가로 → 세로). 결과는 (width + 2p) x (height + 2p), p = 반지름의 올림.
	void dilateCoverage(const std::vector<uint8>& source, int32 width, int32 height, float32 radius, std::vector<uint8>& outCoverage)
	{
		const int32 padding   = boldPadding(radius);
		const int32 outWidth  = width + padding * 2;
		const int32 outHeight = height + padding * 2;

		std::vector<uint8> horizontal((size_t)outWidth * height, 0);
		for (int32 y = 0; y < height; ++y)
		{
			dilateLine(&source[(size_t)y * width], width, 1, &horizontal[(size_t)y * outWidth], 1, radius, padding);
		}

		outCoverage.assign((size_t)outWidth * outHeight, 0);
		for (int32 x = 0; x < outWidth; ++x)
		{
			dilateLine(&horizontal[(size_t)x], height, outWidth, &outCoverage[(size_t)x], outWidth, radius, padding);
		}
	}

	// ---- 선명한 굵게(GG-19): 세기를 정한 굵은 글리프를 이 배수로 그려 원판으로 넓힌 뒤 배수 × 배수 평균으로 줄인다.
	// 넓힘이 1/8 픽셀 단위로 정확히 들어가 반쯤 칠해진 가장자리가 먼저 차고, 바깥에 옅은 테가 생기지 않는다.
	constexpr int32 SharpBoldSupersample = 8;

	int32 floorDivide(int32 value, int32 divisor)
	{
		return (value >= 0) ? value / divisor : -((-value + divisor - 1) / divisor);
	}

	int32 ceilDivide(int32 value, int32 divisor)
	{
		return -floorDivide(-value, divisor);
	}

	// 선명한 굵게 상자: 배수 래스터 상자(배수 픽셀) · 넓힘(배수 칸)과, 넓힌 그림을 덮는 1배 픽셀 상자(펜 기준 — 1배 래스터와 같은 격자).
	struct HSharpBoldBox
	{
		int32 RasterX0     = 0;
		int32 RasterY0     = 0;
		int32 RasterWidth  = 0;
		int32 RasterHeight = 0;
		int32 Grow         = 0;
		int32 X0           = 0;
		int32 Y0           = 0;
		int32 Width        = 0;
		int32 Height       = 0;
	};

	HSharpBoldBox measureSharpBold(const stbtt_fontinfo* fontInfo, uint32 codepoint, float32 scaleX, float32 scaleY, float32 radius)
	{
		const int32 sample = SharpBoldSupersample;
		HSharpBoldBox box;
		int32 x1 = 0;
		int32 y1 = 0;
		stbtt_GetCodepointBitmapBox(fontInfo, (int32)codepoint, scaleX * (float32)sample, scaleY * (float32)sample, &box.RasterX0, &box.RasterY0, &x1, &y1);
		box.RasterWidth  = x1 - box.RasterX0;
		box.RasterHeight = y1 - box.RasterY0;
		box.Grow         = HMath::RoundToInt(radius * (float32)sample);
		box.X0           = floorDivide(box.RasterX0 - box.Grow, sample);
		box.Y0           = floorDivide(box.RasterY0 - box.Grow, sample);
		box.Width        = ceilDivide(x1 + box.Grow, sample) - box.X0;
		box.Height       = ceilDivide(y1 + box.Grow, sample) - box.Y0;
		return box;
	}

	// 커버리지를 원판(반지름 radius 칸)으로 넓힌다 — 원판 = 가로 구간의 합. 가로 최댓값의 반폭을 0 부터 한 칸씩 넓히며
	// 그 반폭(= floor(sqrt(r² − dy²)))을 쓰는 세로 자리 dy 마다 결과에 겹친다. 칸마다 반지름 × 3 번 남짓. 바깥 radius 칸은 비워 둔 캔버스여야 한다.
	void dilateDisk(std::vector<uint8>& coverage, int32 width, int32 height, int32 radius)
	{
		if (radius <= 0)
		{
			return;
		}
		if (width < 2)
		{
			return;
		}
		// 세로 자리 dy(−r ~ r)마다 쓰는 가로 반폭
		std::vector<int32> halfWidths((size_t)(radius * 2 + 1));
		for (int32 dy = -radius; dy <= radius; ++dy)
		{
			halfWidths[(size_t)(dy + radius)] = (int32)std::sqrt((float32)(radius * radius - dy * dy));
		}

		std::vector<uint8> rows = coverage;
		std::vector<uint8> wider(coverage.size());
		std::vector<uint8> out(coverage.size(), 0);
		for (int32 halfWidth = 0; halfWidth <= radius; ++halfWidth)
		{
			if (halfWidth > 0)
			{
				// 반폭 한 칸 더: 양옆 이웃과의 최댓값(끝 칸은 안쪽 이웃만)
				for (int32 y = 0; y < height; ++y)
				{
					const uint8* row  = &rows[(size_t)y * width];
					uint8*       next = &wider[(size_t)y * width];
					next[0] = HMath::Max(row[0], row[1]);
					for (int32 x = 1; x < width - 1; ++x)
					{
						const uint8 side = (row[x - 1] > row[x + 1]) ? row[x - 1] : row[x + 1];
						next[x] = (row[x] > side) ? row[x] : side;
					}
					next[width - 1] = HMath::Max(row[width - 2], row[width - 1]);
				}
				rows.swap(wider);
			}
			for (int32 dy = -radius; dy <= radius; ++dy)
			{
				if (halfWidths[(size_t)(dy + radius)] != halfWidth)
				{
					continue;
				}
				const int32 firstY = HMath::Max(0, -dy);
				const int32 endY   = HMath::Min(height, height - dy);
				for (int32 y = firstY; y < endY; ++y)
				{
					const uint8* source = &rows[(size_t)(y + dy) * width];
					uint8*       target = &out[(size_t)y * width];
					for (int32 x = 0; x < width; ++x)
					{
						target[x] = (source[x] > target[x]) ? source[x] : target[x];
					}
				}
			}
		}
		coverage.swap(out);
	}

	void rasterizeSharpBold(const stbtt_fontinfo* fontInfo, uint32 codepoint, float32 scaleX, float32 scaleY, const HSharpBoldBox& box, std::vector<uint8>& outCoverage)
	{
		const int32 sample       = SharpBoldSupersample;
		const int32 canvasWidth  = box.Width * sample;
		const int32 canvasHeight = box.Height * sample;
		std::vector<uint8> canvas((size_t)canvasWidth * canvasHeight, 0);
		if (box.RasterWidth > 0 && box.RasterHeight > 0)
		{
			// 배수 래스터를 캔버스의 제 자리(1배 픽셀 격자에 맞춘 자리)에 바로 그린다.
			const int32 left = box.RasterX0 - box.X0 * sample;
			const int32 top  = box.RasterY0 - box.Y0 * sample;
			stbtt_MakeCodepointBitmap(fontInfo, &canvas[(size_t)top * canvasWidth + left], box.RasterWidth, box.RasterHeight, canvasWidth,
				scaleX * (float32)sample, scaleY * (float32)sample, (int32)codepoint);
		}
		dilateDisk(canvas, canvasWidth, canvasHeight, box.Grow);

		outCoverage.assign((size_t)box.Width * box.Height, 0);
		const int32 area = sample * sample;
		for (int32 y = 0; y < box.Height; ++y)
		{
			for (int32 x = 0; x < box.Width; ++x)
			{
				int32 sum = 0;
				for (int32 sy = 0; sy < sample; ++sy)
				{
					const uint8* row = &canvas[(size_t)(y * sample + sy) * canvasWidth + x * sample];
					for (int32 sx = 0; sx < sample; ++sx)
					{
						sum += row[sx];
					}
				}
				outCoverage[(size_t)y * box.Width + x] = (uint8)((sum + area / 2) / area);
			}
		}
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

void PGameUIFont::BeginFrame()
{
	++GFontFrame;
}

float32 PGameUIFont::getScale(int32 pixelSize) const
{
	// 글자 크기 = em 높이(픽셀). CSS 의 font-size 와 같은 뜻.
	return stbtt_ScaleForMappingEmToPixels(_fontInfo, (float32)pixelSize);
}

bool PGameUIFont::hasOwnGlyph(uint32 codepoint) const
{
	return _fontInfo != nullptr && stbtt_FindGlyphIndex(_fontInfo, (int32)codepoint) != 0;
}

const PGameUIFont* PGameUIFont::findGlyphFont(uint32 codepoint) const
{
	if (_fallbackFont.IsValid() == false || hasOwnGlyph(codepoint))
	{
		return this;
	}
	for (const PGameUIFont* font = _fallbackFont.GetRawPointer(); font != nullptr; font = font->_fallbackFont.GetRawPointer())
	{
		if (font->hasOwnGlyph(codepoint))
		{
			return font;
		}
	}
	return this;
}

PGameUIFont* PGameUIFont::findGlyphFont(uint32 codepoint)
{
	return const_cast<PGameUIFont*>(static_cast<const PGameUIFont*>(this)->findGlyphFont(codepoint));
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

	// 다른 글꼴(대체 글꼴)의 두 글자 사이에는 커닝이 없다.
	const PGameUIFont* font = findGlyphFont(first);
	if (font != findGlyphFont(second))
	{
		return 0.0f;
	}
	return stbtt_GetCodepointKernAdvance(font->_fontInfo, (int32)first, (int32)second) * font->getScale(pixelSize);
}

bool PGameUIFont::HasGlyph(uint32 codepoint) const
{
	return findGlyphFont(codepoint)->hasOwnGlyph(codepoint);
}

PGameUIFont::HGlyph PGameUIFont::GetGlyph(uint32 codepoint, int32 pixelSize)
{
	return GetGlyph(codepoint, pixelSize, HGameUIGlyphVariant());
}

PGameUIFont::HGlyph PGameUIFont::GetGlyph(uint32 codepoint, int32 pixelSize, const HGameUIGlyphVariant& variant)
{
	const int32   scalePercent = toScalePercent(variant.HorizontalScale);
	const float32 boldStrength = variant.bBold ? ResolveBoldStrength(variant.BoldStrength) : 0.0f;
	const uint64  key          = makeGlyphKey(codepoint, pixelSize, scalePercent, toBoldCode(variant.bBold, variant.BoldStrength));
	auto found = _glyphs.find(key);
	if (found != _glyphs.end())
	{
		if (found->second.bInAtlas)
		{
			_pages[found->second.Page].LastUsedFrame = GFontFrame;
		}
		return found->second;
	}

	// 이 글꼴에 없는 글자는 대체 글꼴이 래스터화해 자기 캐시 · 아틀라스에 둔다.
	PGameUIFont* owner = findGlyphFont(codepoint);
	if (owner != this)
	{
		return owner->GetGlyph(codepoint, pixelSize, variant);
	}

	HGlyph glyph;
	glyph.Font = this;
	if (IsValid() && pixelSize > 0)
	{
		// 장평은 가로 배율로 래스터화한다(늘인 그림이 아니라 그 폭의 그림).
		const float32 scaleY = getScale(pixelSize);
		const float32 scaleX = scaleY * (float32)scalePercent / 100.0f;

		int32 advance     = 0;
		int32 leftBearing = 0;
		stbtt_GetCodepointHMetrics(_fontInfo, (int32)codepoint, &advance, &leftBearing);
		glyph.Advance = advance * scaleX;
		if (variant.bBold)
		{
			glyph.Advance += (float32)pixelSize * boldStrength;
		}

		// 비트맵 상자. y 는 아래로 + 이고 기준선이 0 이라 y0 은 보통 음수다.
		int32 x0 = 0;
		int32 y0 = 0;
		int32 x1 = 0;
		int32 y1 = 0;
		stbtt_GetCodepointBitmapBox(_fontInfo, (int32)codepoint, scaleX, scaleY, &x0, &y0, &x1, &y1);
		const int32 rasterWidth  = x1 - x0;
		const int32 rasterHeight = y1 - y0;

		// 합성 굵게: 그림을 사방으로 반지름(한쪽 넓힘)만큼 넓힌다 — 양쪽 합 = 폭 증가 em × 세기.
		//   세기를 정하지 않은 굵게(기본 em/24): 커버리지를 넓힌다. 1픽셀보다 작으면 바깥 한 겹만 그 비율로 진해진다(지금까지의 그림 그대로).
		//   세기를 정한 굵게(GG-19): 8배로 그려 원판으로 넓힌 뒤 줄인다 — 넓힌 양이 그대로 들어가 획 가운데가 진하다(옅은 테가 생기지 않는다).
		const bool    bSharpBold = variant.bBold && variant.BoldStrength > 0.0f;
		const float32 radius     = variant.bBold ? boldRadius(pixelSize, boldStrength) : 0.0f;
		int32         padding    = 0;
		int32         width      = rasterWidth;
		int32         height     = rasterHeight;
		HSharpBoldBox sharpBox;
		if (bSharpBold)
		{
			sharpBox      = measureSharpBold(_fontInfo, codepoint, scaleX, scaleY, radius);
			width         = sharpBox.Width;
			height        = sharpBox.Height;
			glyph.OffsetX = sharpBox.X0;
			glyph.OffsetY = sharpBox.Y0;
		}
		else
		{
			padding       = variant.bBold ? boldPadding(radius) : 0;
			width         = rasterWidth + padding * 2;
			height        = rasterHeight + padding * 2;
			glyph.OffsetX = x0 - padding;
			glyph.OffsetY = y0 - padding;
		}

		int32 page   = 0;
		int32 atlasX = 0;
		int32 atlasY = 0;
		if (rasterWidth > 0 && rasterHeight > 0 && packGlyph(width, height, page, atlasX, atlasY))
		{
			std::vector<uint8> coverage;
			if (bSharpBold)
			{
				rasterizeSharpBold(_fontInfo, codepoint, scaleX, scaleY, sharpBox, coverage);
			}
			else
			{
				coverage.resize((size_t)rasterWidth * rasterHeight);
				stbtt_MakeCodepointBitmap(_fontInfo, coverage.data(), rasterWidth, rasterHeight, rasterWidth, scaleX, scaleY, (int32)codepoint);
				if (padding > 0)
				{
					std::vector<uint8> bold;
					dilateCoverage(coverage, rasterWidth, rasterHeight, radius, bold);
					coverage.swap(bold);
				}
			}

			std::vector<uint8>& pixels = _pages[page].Pixels;
			for (int32 y = 0; y < height; ++y)
			{
				uint8* row = &pixels[((size_t)(atlasY + y) * AtlasSize + atlasX) * 4];
				for (int32 x = 0; x < width; ++x)
				{
					row[x * 4 + 3] = coverage[(size_t)y * width + x];
				}
			}

			glyph.Width    = width;
			glyph.Height   = height;
			glyph.UV       = HRect((float32)atlasX / AtlasSize, (float32)atlasY / AtlasSize, (float32)(atlasX + width) / AtlasSize, (float32)(atlasY + height) / AtlasSize);
			glyph.bInAtlas = true;
			glyph.Page     = page;
		}
	}

	_glyphs.emplace(key, glyph);
	return glyph;
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

	// 이 글꼴에 없는 글자는 그리는 글꼴(대체 글꼴)의 폭
	const PGameUIFont* font = findGlyphFont(codepoint);
	int32 advance     = 0;
	int32 leftBearing = 0;
	stbtt_GetCodepointHMetrics(font->_fontInfo, (int32)codepoint, &advance, &leftBearing);
	return advance * stbtt_ScaleForMappingEmToPixels(font->_fontInfo, fontSize);
}

float32 PGameUIFont::MeasureKerning(uint32 first, uint32 second, float32 fontSize) const
{
	if (IsValid() == false || fontSize <= 0.0f)
	{
		return 0.0f;
	}

	// 다른 글꼴(대체 글꼴)의 두 글자 사이에는 커닝이 없다.
	const PGameUIFont* font = findGlyphFont(first);
	if (font != findGlyphFont(second))
	{
		return 0.0f;
	}
	return stbtt_GetCodepointKernAdvance(font->_fontInfo, (int32)first, (int32)second) * stbtt_ScaleForMappingEmToPixels(font->_fontInfo, fontSize);
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

float32 PGameUIFont::MeasureStyledAdvance(uint32 codepoint, float32 fontSize, const HGameUITextRunStyle& style) const
{
	float32 advance = MeasureAdvance(codepoint, fontSize) * style.HorizontalScale;
	if (style.bBold)
	{
		advance += fontSize * ResolveBoldStrength(style.BoldStrength);
	}
	return advance;
}

float32 PGameUIFont::ResolveBoldStrength(float32 strength)
{
	if (strength <= 0.0f)
	{
		return BoldStrength;
	}
	return HMath::Min(strength, MaxBoldStrength);
}

namespace
{
	// 배치 중 공통 값. 글자 폭 = 서식 넣은 폭, 글자 사이 = 자간 보정 × 장평 + 자간.
	struct HLayoutContext
	{
		const PGameUIFont*              Font    = nullptr;
		const HGameUITextLayoutOptions* Options = nullptr;
		HGameUITextLayout*              Out     = nullptr;
		const char*                     Base    = nullptr;
		float32                         Spacing = 0.0f;

		// 바이트 위치 offset 의 서식 번호(Runs 번호). 구간 밖이면 0.
		uint32 RunAt(uint32 offset) const
		{
			const HList<HGameUITextSpan>* spans = Options->Spans;
			if (spans == nullptr || spans->empty())
			{
				return 0;
			}

			// 구간은 바이트 순서 → End 가 offset 보다 큰 첫 구간
			size_t low  = 0;
			size_t high = spans->size();
			while (low < high)
			{
				const size_t middle = (low + high) / 2;
				if ((*spans)[middle].End <= offset)
				{
					low = middle + 1;
				}
				else
				{
					high = middle;
				}
			}
			if (low < spans->size() && (*spans)[low].Begin <= offset)
			{
				return (uint32)low + 1;
			}
			return 0;
		}

		float32 Advance(uint32 codepoint, uint32 run) const
		{
			return Font->MeasureStyledAdvance(codepoint, Options->FontSize, Out->Runs[run]);
		}

		// 앞 글자와 이 글자 사이
		float32 Gap(uint32 previous, uint32 codepoint, uint32 run) const
		{
			return Font->MeasureKerning(previous, codepoint, Options->FontSize) * Out->Runs[run].HorizontalScale + Spacing;
		}
	};

	// 요소 서식 위에 구간 서식을 겹친다. 굵게 세기는 요소 값(구간은 굵게를 켜기만 한다).
	HGameUITextRunStyle combineRun(const HGameUITextRunStyle& base, const HGameUITextRunStyle& span)
	{
		HGameUITextRunStyle run = base;
		run.bBold           = base.bBold || span.bBold;
		run.bItalic         = base.bItalic || span.bItalic;
		run.HorizontalScale = base.HorizontalScale * span.HorizontalScale;
		if (span.bHasColor)
		{
			run.bHasColor = true;
			run.Color     = span.Color;
		}
		return run;
	}

	// 한 단락(줄바꿈 문자 없는 범위)을 줄로 나눠 Out->Lines 에 붙인다(바이트 범위 · 폭. 글자 자리는 buildLineGlyphs 가 만든다).
	void layoutParagraph(HLayoutContext& context, const char* begin, const char* end)
	{
		const HGameUITextLayoutOptions& options = *context.Options;
		const char*   base      = context.Base;
		const float32 wrapWidth = options.WrapWidth;
		const bool    bWrap     = options.Wrap != EGameUITextWrap::None && wrapWidth > 0.0f;
		const char*   lineStart  = begin;
		bool          bFirstLine = true;

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

			HGameUITextLine line;
			line.Begin           = (uint32)(lineStart - base);
			line.bParagraphStart = bFirstLine;
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

				const uint32 run = context.RunAt((uint32)(glyphBegin - base));
				float32 advance = context.Advance(codepoint, run);
				if (previous != 0)
				{
					advance += context.Gap(previous, codepoint, run);
				}
				const bool bSpace = isBreakSpace(codepoint);

				if (bWrap)
				{
					// 이 글자 앞이 바꿀 수 있는 자리인가
					if (glyphBegin > lineStart && bSpace == false && isNoLineStart(codepoint) == false && isNoLineEnd(previous) == false)
					{
						bool bOpportunity = true;
						if (options.Wrap == EGameUITextWrap::Word)
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
						line.bWrapped = true;
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
						context.Out->Lines.push_back(line);
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
			context.Out->Lines.push_back(line);
			return;
		}
	}

	// 줄 끝을 (자간 +) 말줄임이 availableWidth 안에 들어가게 줄인다. 말줄임 글자는 요소 서식(Runs[0])으로 그린다.
	void applyEllipsis(HLayoutContext& context, HGameUITextLine& line, float32 availableWidth)
	{
		const HGameUITextLayout& layout = *context.Out;
		const char*   base          = context.Base;
		const float32 ellipsisWidth = context.Advance(layout.EllipsisCodepoint, 0) * (float32)layout.EllipsisCount
			+ context.Spacing * (float32)(layout.EllipsisCount - 1);

		// 앞에서부터 글자를 더하며 (글자 + 자간 + 말줄임)이 들어가는 마지막 자리를 찾는다. 그 자리 앞의 공백은 뺀다.
		const char* lineBegin = base + line.Begin;
		const char* lineEnd   = base + line.End;
		const char* cut       = lineBegin;
		float32     cutWidth  = 0.0f;
		float32     width     = 0.0f;
		uint32      previous  = 0;

		const char* cursor = lineBegin;
		while (cursor < lineEnd)
		{
			const char* glyphBegin = cursor;
			uint32 codepoint = 0;
			HGameUIUtf8::Next(cursor, lineEnd, codepoint);

			const uint32 run = context.RunAt((uint32)(glyphBegin - base));
			float32 advance = context.Advance(codepoint, run);
			if (previous != 0)
			{
				advance += context.Gap(previous, codepoint, run);
			}
			if (width + advance + context.Spacing + ellipsisWidth > availableWidth + WrapTolerance)
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
		line.Width     = cutWidth + (cut > lineBegin ? context.Spacing : 0.0f) + ellipsisWidth;
		line.bEllipsis = true;
	}

	// 줄의 글자 자리(말줄임 글자 포함)를 Out->Glyphs 에 붙인다. 마지막 글자 끝 = line.Width.
	void buildLineGlyphs(HLayoutContext& context, HGameUITextLine& line)
	{
		HGameUITextLayout& layout = *context.Out;
		const char* base = context.Base;
		line.GlyphBegin = (uint32)layout.Glyphs.size();

		float32     x        = 0.0f;
		uint32      previous = 0;
		const char* cursor   = base + line.Begin;
		const char* lineEnd  = base + line.End;
		while (cursor < lineEnd)
		{
			const char* glyphBegin = cursor;
			uint32 codepoint = 0;
			HGameUIUtf8::Next(cursor, lineEnd, codepoint);

			const uint32 run = context.RunAt((uint32)(glyphBegin - base));
			if (previous != 0)
			{
				x += context.Gap(previous, codepoint, run);
			}

			HGameUITextGlyph glyph;
			glyph.Codepoint = codepoint;
			glyph.X         = x;
			glyph.Run       = run;
			layout.Glyphs.push_back(glyph);

			x += context.Advance(codepoint, run);
			previous = codepoint;
		}

		if (line.bEllipsis)
		{
			for (int32 i = 0; i < layout.EllipsisCount; ++i)
			{
				if (previous != 0)
				{
					x += context.Spacing;
				}

				HGameUITextGlyph glyph;
				glyph.Codepoint = layout.EllipsisCodepoint;
				glyph.X         = x;
				glyph.Run       = 0;
				layout.Glyphs.push_back(glyph);

				x += context.Advance(layout.EllipsisCodepoint, 0);
				previous = layout.EllipsisCodepoint;
			}
		}
		line.GlyphCount = (uint32)layout.Glyphs.size() - line.GlyphBegin;
	}

	// 양쪽 맞춤: 남는 폭을 글자 사이에 나눠 줄을 width 에 맞춘다(띄어쓰기 뒤 사이는 JustifySpaceWeight 배).
	void justifyLine(HLayoutContext& context, HGameUITextLine& line, float32 width)
	{
		if (line.GlyphCount < 2 || width - line.Width <= WrapTolerance)
		{
			return;
		}

		HList<HGameUITextGlyph>& glyphs = context.Out->Glyphs;
		float32 totalWeight = 0.0f;
		for (uint32 i = 1; i < line.GlyphCount; ++i)
		{
			totalWeight += isBreakSpace(glyphs[line.GlyphBegin + i - 1].Codepoint) ? JustifySpaceWeight : 1.0f;
		}

		const float32 extra  = width - line.Width;
		float32       offset = 0.0f;
		for (uint32 i = 1; i < line.GlyphCount; ++i)
		{
			const float32 weight = isBreakSpace(glyphs[line.GlyphBegin + i - 1].Codepoint) ? JustifySpaceWeight : 1.0f;
			offset += extra * weight / totalWeight;
			glyphs[line.GlyphBegin + i].X += offset;
		}
		line.Width      = width;
		line.bJustified = true;
	}
}

void PGameUIFont::LayoutText(const PString& text, float32 fontSize, float32 wrapWidth, EGameUITextWrap wrap, int32 maxLines, bool bEllipsis, HGameUITextLayout& outLayout) const
{
	HGameUITextLayoutOptions options;
	options.FontSize  = fontSize;
	options.WrapWidth = wrapWidth;
	options.Wrap      = wrap;
	options.MaxLines  = maxLines;
	options.bEllipsis = bEllipsis;
	LayoutText(text, options, outLayout);
}

void PGameUIFont::LayoutText(const PString& text, const HGameUITextLayoutOptions& options, HGameUITextLayout& outLayout) const
{
	outLayout.Lines.clear();
	outLayout.Glyphs.clear();
	outLayout.Runs.clear();
	outLayout.LineHeight        = 0.0f;
	outLayout.BaselineOffset    = 0.0f;
	outLayout.Size              = HVector2(0.0f, 0.0f);
	outLayout.bTruncated        = false;
	outLayout.EllipsisCodepoint = 0x2026;
	outLayout.EllipsisCount     = 1;
	if (IsValid() == false || options.FontSize <= 0.0f || text.Empty())
	{
		return;
	}

	// 서식: [0] = 요소 전체, [i + 1] = 구간 i 를 겹친 것
	HGameUITextRunStyle baseStyle = options.BaseStyle;
	baseStyle.HorizontalScale = HMath::Clamp(baseStyle.HorizontalScale, MinHorizontalScale, MaxHorizontalScale);
	outLayout.Runs.push_back(baseStyle);
	if (options.Spans != nullptr)
	{
		for (const HGameUITextSpan& span : *options.Spans)
		{
			HGameUITextRunStyle run = combineRun(baseStyle, span.Style);
			run.HorizontalScale = HMath::Clamp(run.HorizontalScale, MinHorizontalScale, MaxHorizontalScale);
			outLayout.Runs.push_back(run);
		}
	}

	// 줄 상자: 높이 = 줄 간격, 기준선 = 남는 높이의 절반을 위에 둔 자리
	const float32 fontLineHeight = MeasureLineHeight(options.FontSize);
	outLayout.LineHeight     = (options.LineHeight > 0.0f) ? options.LineHeight : fontLineHeight;
	outLayout.BaselineOffset = (outLayout.LineHeight - fontLineHeight) * 0.5f + MeasureAscent(options.FontSize);
	if (HasGlyph(0x2026) == false)
	{
		outLayout.EllipsisCodepoint = '.';
		outLayout.EllipsisCount     = 3;
	}

	HLayoutContext context;
	context.Font    = this;
	context.Options = &options;
	context.Out     = &outLayout;
	context.Base    = text.GetCStr();
	context.Spacing = options.LetterSpacing * options.FontSize / 1000.0f;

	// 단락('\n' 사이)마다 줄로 나눈다. 최대 줄 수를 넘었으면 남은 단락은 볼 필요가 없다.
	const char* base      = context.Base;
	const char* end       = base + text.Size();
	const char* paragraph = base;
	const int32 maxLines  = options.MaxLines;
	while (true)
	{
		const char* paragraphEnd = paragraph;
		while (paragraphEnd < end && *paragraphEnd != '\n')
		{
			++paragraphEnd;
		}
		layoutParagraph(context, paragraph, paragraphEnd);

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

	const float32 wrapWidth = options.WrapWidth;
	if (maxLines > 0 && (int32)outLayout.Lines.size() > maxLines)
	{
		outLayout.Lines.resize((size_t)maxLines);
		outLayout.bTruncated = true;
		if (options.bEllipsis)
		{
			applyEllipsis(context, outLayout.Lines.back(), wrapWidth > 0.0f ? wrapWidth : UnlimitedWidth);
		}
	}

	// 줄을 바꾸지 않는 글자는 폭을 넘는 줄만 말줄임으로 줄인다(한 줄 이름 칸 등).
	if (options.bEllipsis && options.Wrap == EGameUITextWrap::None && wrapWidth > 0.0f)
	{
		for (HGameUITextLine& line : outLayout.Lines)
		{
			if (line.bEllipsis == false && line.Width > wrapWidth + WrapTolerance)
			{
				applyEllipsis(context, line, wrapWidth);
			}
		}
	}

	// 글자 자리 · 양쪽 맞춤 · 줄 위치(문단 첫 줄 앞에 문단 간격)
	float32 top      = 0.0f;
	float32 maxWidth = 0.0f;
	for (size_t i = 0; i < outLayout.Lines.size(); ++i)
	{
		HGameUITextLine& line = outLayout.Lines[i];
		buildLineGlyphs(context, line);
		if (options.bJustify && wrapWidth > 0.0f && line.bWrapped && line.bEllipsis == false)
		{
			justifyLine(context, line, wrapWidth);
		}

		if (i > 0)
		{
			top += outLayout.LineHeight;
			if (line.bParagraphStart)
			{
				top += options.ParagraphSpacing;
			}
		}
		line.Top = top;
		maxWidth = HMath::Max(maxWidth, line.Width);
	}

	if (outLayout.Lines.empty() == false)
	{
		outLayout.Size = HVector2(maxWidth, top + outLayout.LineHeight);
	}
}

void PGameUIFont::SetFallbackFont(const PSharedPtr<PGameUIFont>& font)
{
	// 사슬이 이 글꼴로 돌아오면 받지 않는다.
	for (const PGameUIFont* chain = font.GetRawPointer(); chain != nullptr; chain = chain->_fallbackFont.GetRawPointer())
	{
		if (chain == this)
		{
			JG_LOG(GameUI, ELogLevel::Warning, "Fallback font chain loops back to %s. Ignored", _path);
			return;
		}
	}
	_fallbackFont = font;

	// 이 글꼴에 없어 빈 글리프 상자로 둔 캐시를 지운다(이제 대체 글꼴이 그릴 수 있다).
	for (auto it = _glyphs.begin(); it != _glyphs.end();)
	{
		if (hasOwnGlyph(glyphKeyCodepoint(it->first)) == false)
		{
			it = _glyphs.erase(it);
		}
		else
		{
			++it;
		}
	}
}

const PSharedPtr<PGameUIFont>& PGameUIFont::GetFallbackFont() const
{
	return _fallbackFont;
}

void PGameUIFont::SetAtlasPageLimit(int32 pageLimit)
{
	_atlasPageLimit = HMath::Max(1, pageLimit);
}

int32 PGameUIFont::GetAtlasPageLimit() const
{
	return _atlasPageLimit;
}

void PGameUIFont::ClearGlyphs()
{
	_glyphs.clear();
	_pages.clear();
	_currentPage      = -1;
	_bOverLimitWarned = false;
}

PSharedPtr<IRawTexture> PGameUIFont::GetAtlasTexture(int32 page)
{
	if (IsValid() == false || page < 0 || page >= (int32)_pages.size())
	{
		return nullptr;
	}
	HAtlasPage& atlasPage = _pages[page];
	if (atlasPage.Texture.IsValid() && atlasPage.bDirty == false)
	{
		return atlasPage.Texture;
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
	textureInfo.Name       = PString::Format("GameUIFontAtlas_%s_%d", fileName, page);
	textureInfo.Width      = AtlasSize;
	textureInfo.Height     = AtlasSize;
	textureInfo.Format     = ETextureFormat::R8G8B8A8_Unorm;
	textureInfo.Flags      = ETextureFlags::None;
	textureInfo.FilterMode = ETextureFilterMode::Linear;
	textureInfo.WrapMode   = ETextureWrapMode::Clamp;
	textureInfo.MipLevel   = 1;
	textureInfo.ArraySize  = 1;

	// 업로드는 이번 프레임 제출의 맨 앞(드로우보다 먼저)에 기록된다. 이전 텍스처는 GPU 가 끝낸 뒤 해제된다(DeferRelease).
	atlasPage.Texture = graphicsAPI->CreateRawTexture(atlasPage.Pixels.data(), textureInfo);
	atlasPage.bDirty  = false;
	return atlasPage.Texture;
}

uint64 PGameUIFont::GetGlyphCount() const
{
	return (uint64)_glyphs.size();
}

int32 PGameUIFont::GetAtlasPageCount() const
{
	return (int32)_pages.size();
}

HGameUIFontAtlasStats PGameUIFont::GetAtlasStats() const
{
	HGameUIFontAtlasStats stats;
	stats.PageCount        = (int32)_pages.size();
	stats.PageLimit        = _atlasPageLimit;
	stats.EvictedPageCount = _evictedPageCount;
	stats.OverLimitCount   = _overLimitCount;
	if (_pages.empty() == false)
	{
		// 페이지를 채운 비율 = 쓴 줄(선반)의 높이 / 페이지 높이
		float32 filled = 0.0f;
		for (const HAtlasPage& page : _pages)
		{
			filled += page.bFull ? 1.0f : (float32)HMath::Min(page.PackY + page.PackRowHeight, AtlasSize) / (float32)AtlasSize;
		}
		stats.Usage = filled / (float32)_pages.size();
	}
	return stats;
}

const std::vector<uint8>& PGameUIFont::GetAtlasPixels(int32 page) const
{
	static const std::vector<uint8> NoPixels;
	if (page < 0 || page >= (int32)_pages.size())
	{
		return NoPixels;
	}
	return _pages[page].Pixels;
}

bool PGameUIFont::packGlyph(int32 width, int32 height, int32& outPage, int32& outX, int32& outY)
{
	// 한 페이지보다 큰 글리프는 넣지 않는다.
	if (width + GlyphPadding * 2 > AtlasSize || height + GlyphPadding * 2 > AtlasSize)
	{
		return false;
	}

	// 채우는 중인 페이지에 넣는다. 차면 새 페이지(상한이면 오래 안 쓴 페이지를 비운 것)에 — 빈 페이지에는 반드시 들어간다.
	for (int32 attempt = 0; attempt < 2; ++attempt)
	{
		if (_currentPage < 0)
		{
			_currentPage = takeFreshPage();
		}

		// 줄(선반) 단위로 채운다. 줄이 넘치면 다음 줄로.
		HAtlasPage& page = _pages[_currentPage];
		if (page.PackX + width + GlyphPadding > AtlasSize)
		{
			page.PackX = GlyphPadding;
			page.PackY += page.PackRowHeight + GlyphPadding;
			page.PackRowHeight = 0;
		}
		if (page.PackY + height + GlyphPadding > AtlasSize)
		{
			page.bFull   = true;
			_currentPage = -1;
			continue;
		}

		outPage = _currentPage;
		outX    = page.PackX;
		outY    = page.PackY;
		page.PackX += width + GlyphPadding;
		page.PackRowHeight = HMath::Max(page.PackRowHeight, height);
		page.LastUsedFrame = GFontFrame;
		page.bDirty        = true;
		return true;
	}
	return false;
}

int32 PGameUIFont::openPage()
{
	// RGB 는 흰색, 알파(커버리지)만 0. 선형 필터가 글리프 가장자리에서 빈 텍셀을 섞어도 색이 어두워지지 않는다.
	HAtlasPage page;
	page.Pixels.assign((size_t)AtlasSize * AtlasSize * 4, 255);
	clearCoverage(page.Pixels);
	page.LastUsedFrame = GFontFrame;
	_pages.push_back(std::move(page));
	return (int32)_pages.size() - 1;
}

int32 PGameUIFont::takeFreshPage()
{
	if ((int32)_pages.size() < _atlasPageLimit)
	{
		return openPage();
	}

	// 상한: 이번 프레임에 쓰지 않은 페이지 중 가장 오래 안 쓴 것을 비운다.
	int32 oldest = -1;
	for (int32 i = 0; i < (int32)_pages.size(); ++i)
	{
		const uint64 lastUsed = _pages[i].LastUsedFrame;
		if (lastUsed < GFontFrame && (oldest < 0 || lastUsed < _pages[oldest].LastUsedFrame))
		{
			oldest = i;
		}
	}
	if (oldest >= 0)
	{
		evictPage(oldest);
		return oldest;
	}

	// 모든 페이지를 이번 프레임에 썼다(이미 그리기 목록에 들어간 글자): 상한을 넘겨 연다.
	++_overLimitCount;
	if (_bOverLimitWarned == false)
	{
		_bOverLimitWarned = true;
		JG_LOG(GameUI, ELogLevel::Warning, "Font atlas needs more than %d pages in one frame (%s). Opened one more page. Raise SetAtlasPageLimit", _atlasPageLimit, _path);
	}
	return openPage();
}

void PGameUIFont::evictPage(int32 page)
{
	// 그 페이지 글리프를 캐시에서 지운다(다음에 쓸 때 다시 래스터화된다).
	for (auto it = _glyphs.begin(); it != _glyphs.end();)
	{
		if (it->second.bInAtlas && it->second.Page == page)
		{
			it = _glyphs.erase(it);
		}
		else
		{
			++it;
		}
	}

	HAtlasPage& atlasPage = _pages[page];
	clearCoverage(atlasPage.Pixels);
	atlasPage.PackX         = GlyphPadding;
	atlasPage.PackY         = GlyphPadding;
	atlasPage.PackRowHeight = 0;
	atlasPage.bFull         = false;
	atlasPage.bDirty        = true;
	++_evictedPageCount;
}
