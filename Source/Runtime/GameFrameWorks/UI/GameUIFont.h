#pragma once
#include "UI/GameUIDefines.h"

class IRawTexture;
struct stbtt_fontinfo;

// 배치된 줄 하나: 글의 바이트 범위 [Begin, End) 와 논리 폭. bEllipsis 면 끝에 말줄임(…)을 붙여 그린다(폭에 포함).
struct HGameUITextLine
{
	uint32  Begin     = 0;
	uint32  End       = 0;
	float32 Width     = 0.0f;
	bool    bEllipsis = false;
};

// 글 배치 결과(논리 단위). 줄 나누기는 논리 글꼴 크기(실수)의 글자 너비로 계산하므로 화면 배율이 달라도 같은 자리에서 바뀐다.
struct HGameUITextLayout
{
	HList<HGameUITextLine> Lines;
	float32                LineHeight        = 0.0f;
	HVector2               Size;                       // 폭 = 가장 긴 줄, 높이 = 줄 수 × 줄 간격
	bool                   bTruncated        = false;  // 최대 줄 수로 잘렸나
	uint32                 EllipsisCodepoint = 0x2026; // 글꼴에 "…" 이 없으면 '.'
	int32                  EllipsisCount     = 1;      // '.' 이면 3
};

// TTF 글꼴. 글리프는 (코드포인트, 픽셀 크기)마다 처음 쓸 때 래스터화해 아틀라스(1024x1024 RGBA8)에 줄 단위로 넣는다.
// 아틀라스 텍셀은 RGB 흰색 + 알파 = 커버리지라, 색은 정점 색이 정한다(가장자리에 어두운 테가 생기지 않는다).
// GPU 텍스처는 그리기 직전 GetAtlasTexture() 가 만든다. 아틀라스가 바뀌면 다시 만들고, 이전 텍스처는 GPU 가 끝낸 뒤 해제된다.
// 그래픽 없이(헤드리스) 래스터화 · 측정만 해도 된다.
class GAMEFRAMEWORKS_API PGameUIFont : public IMemoryObject
{
public:
	static constexpr int32 AtlasSize    = 1024;
	static constexpr int32 GlyphPadding = 1;   // 선형 필터가 옆 글리프를 섞지 않게

	struct HGlyph
	{
		int32   Width   = 0;    // 비트맵 픽셀. 공백은 0
		int32   Height  = 0;
		int32   OffsetX = 0;    // 펜 위치(기준선)에서 비트맵 왼쪽 위까지
		int32   OffsetY = 0;
		float32 Advance = 0.0f; // 다음 글자까지(픽셀)
		HRect   UV;             // 아틀라스 UV(0~1)
		bool    bInAtlas = false;
	};

private:
	PString            _path;
	std::vector<uint8> _fontData;
	stbtt_fontinfo*    _fontInfo = nullptr;

	std::vector<uint8> _atlasPixels;   // RGBA8, AtlasSize x AtlasSize
	int32 _packX         = GlyphPadding;
	int32 _packY         = GlyphPadding;
	int32 _packRowHeight = 0;
	bool  _bAtlasFull    = false;
	bool  _bAtlasDirty   = false;
	PSharedPtr<IRawTexture> _atlasTexture;

	HHashMap<uint64, HGlyph> _glyphs;   // 키 = (픽셀 크기 << 32) | 코드포인트

public:
	PGameUIFont() = default;
	virtual ~PGameUIFont();

	bool LoadFromFile(const PString& path);
	bool IsValid() const;
	const PString& GetPath() const;

	// 줄 맨 위에서 기준선까지(픽셀).
	float32 GetAscent(int32 pixelSize) const;
	// 줄 간격(픽셀) = ascent - descent + lineGap.
	float32 GetLineHeight(int32 pixelSize) const;
	float32 GetKerning(uint32 first, uint32 second, int32 pixelSize) const;
	bool    HasGlyph(uint32 codepoint) const;
	// 없으면 래스터화해 아틀라스에 넣는다. 아틀라스가 차면 비트맵 없이(Advance 만) 돌려준다.
	const HGlyph& GetGlyph(uint32 codepoint, int32 pixelSize);
	// UTF-8 글자 묶음의 크기(픽셀). 폭 = 가장 긴 줄, 높이 = 줄 수 x 줄 간격. '\n' 에서 줄을 바꾼다.
	HVector2 MeasureText(const PString& text, int32 pixelSize);

	// 래스터화 없는 수치(실수 글꼴 크기 — 논리 단위 배치용).
	float32 MeasureAdvance(uint32 codepoint, float32 fontSize) const;
	float32 MeasureKerning(uint32 first, uint32 second, float32 fontSize) const;
	float32 MeasureAscent(float32 fontSize) const;
	float32 MeasureLineHeight(float32 fontSize) const;
	// text 를 fontSize(논리 단위) 글자로 배치한다. wrap 이 None 이 아니고 wrapWidth > 0 이면 그 폭에서 줄을 바꾼다.
	// maxLines > 0 이면 그 줄 수까지만 두고(bTruncated), bEllipsis 면 잘린 마지막 줄 끝에 말줄임을 붙인다(폭 안에 들어가게 줄인다).
	// wrap 이 None 이고 bEllipsis 이면 wrapWidth 를 넘는 줄을 말줄임으로 줄인다. 빈 글은 줄 0개.
	void LayoutText(const PString& text, float32 fontSize, float32 wrapWidth, EGameUITextWrap wrap, int32 maxLines, bool bEllipsis, HGameUITextLayout& outLayout) const;

	// 그리기용 GPU 아틀라스. 그래픽 API 가 없으면 nullptr.
	PSharedPtr<IRawTexture> GetAtlasTexture();

	// 확인용
	uint64 GetGlyphCount() const;
	bool   IsAtlasFull() const;
	const std::vector<uint8>& GetAtlasPixels() const;

private:
	float32 getScale(int32 pixelSize) const;
	bool    packGlyph(int32 width, int32 height, int32& outX, int32& outY);
	// 한 단락(줄바꿈 문자 없는 범위)을 줄로 나눠 outLayout.Lines 에 붙인다. 바이트 위치는 base 기준.
	void    layoutParagraph(const char* base, const char* begin, const char* end, float32 fontSize, float32 wrapWidth, EGameUITextWrap wrap, HGameUITextLayout& outLayout) const;
	// 줄 끝을 말줄임이 availableWidth 안에 들어가게 줄인다.
	void    applyEllipsis(const char* base, HGameUITextLine& line, float32 fontSize, float32 availableWidth, const HGameUITextLayout& layout) const;
};
