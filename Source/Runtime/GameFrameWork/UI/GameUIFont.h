#pragma once
#include "UI/GameUIDefines.h"

class IRawTexture;
struct stbtt_fontinfo;

// 글자 서식(요소 전체 또는 구간 하나): 합성 굵게 · 합성 기울임 · 장평 · 색.
struct HGameUITextRunStyle
{
	bool         bBold           = false;
	bool         bItalic         = false;
	float32      HorizontalScale = 1.0f;    // 장평(1.0 = 100%)
	float32      BoldStrength    = 0.0f;    // 합성 굵게 세기(em 비율, 0 = 엔진 기본). 구간은 요소 값을 받는다(구간은 굵게 켜기만)
	bool         bHasColor       = false;   // false 면 요소 색
	HLinearColor Color;
};

// 구간 서식: 글(마크업을 뺀 표시 글)의 바이트 범위 [Begin, End) 에 Style 을 겹친다. 바이트 순서로, 겹치지 않게 둔다.
struct HGameUITextSpan
{
	uint32              Begin = 0;
	uint32              End   = 0;
	HGameUITextRunStyle Style;
};

// 배치된 글자 하나(논리 단위). X = 줄 왼쪽에서 펜 위치(자간 · 양쪽 맞춤 포함), Run = HGameUITextLayout::Runs 번호.
struct HGameUITextGlyph
{
	uint32  Codepoint = 0;
	float32 X         = 0.0f;
	uint32  Run       = 0;
};

// 배치된 줄 하나: 글의 바이트 범위 [Begin, End), 논리 폭, 그 줄의 글자 Glyphs[GlyphBegin, GlyphBegin + GlyphCount).
// bEllipsis 면 끝에 말줄임(…) 글자가 붙어 있다(폭에 포함). Top = 글 상자 위에서 이 줄 상자 위까지.
struct HGameUITextLine
{
	uint32  Begin           = 0;
	uint32  End             = 0;
	float32 Width           = 0.0f;
	bool    bEllipsis       = false;
	float32 Top             = 0.0f;
	uint32  GlyphBegin      = 0;
	uint32  GlyphCount      = 0;
	bool    bParagraphStart = false;   // 문단(글 처음 또는 '\n' 다음)의 첫 줄
	bool    bWrapped        = false;   // 자동으로 바뀐 줄(문단의 마지막 줄이 아님)
	bool    bJustified      = false;   // 양쪽 맞춤으로 폈다(Width = 맞춘 폭)
};

// PGameUIFont::LayoutText 서식. 기본값이면 서식 없는 배치(지금까지와 같음). 값은 논리 단위.
struct HGameUITextLayoutOptions
{
	float32             FontSize         = 24.0f;
	float32             WrapWidth        = 0.0f;
	EGameUITextWrap     Wrap             = EGameUITextWrap::None;
	int32               MaxLines         = 0;
	bool                bEllipsis        = false;
	bool                bJustify         = false;   // 자동으로 바뀐 줄을 WrapWidth 에 맞춰 편다
	float32             LineHeight       = 0.0f;    // 줄 간격(기준선 사이). 0 = 글꼴 기본
	float32             LetterSpacing    = 0.0f;    // 자간(1/1000 em). 글자 사이에만
	float32             ParagraphSpacing = 0.0f;    // '\n' 다음 문단 앞에 더하는 간격
	HGameUITextRunStyle BaseStyle;                  // 요소 전체 서식. 구간 서식은 이 위에 겹친다(굵게 · 기울임은 둘 중 하나라도, 장평은 곱, 색은 구간이 이김)
	const HList<HGameUITextSpan>* Spans = nullptr;  // 구간 서식(없으면 BaseStyle 하나)
};

// 글 배치 결과(논리 단위). 줄 나누기는 논리 글꼴 크기(실수)의 글자 너비로 계산하므로 화면 배율이 달라도 같은 자리에서 바뀐다.
// 그리기 · 높이 측정 · 커서가 같은 결과(글자 자리)를 쓴다.
struct HGameUITextLayout
{
	HList<HGameUITextLine>     Lines;
	HList<HGameUITextGlyph>    Glyphs;
	HList<HGameUITextRunStyle> Runs;                         // [0] = BaseStyle, [i + 1] = 구간 i 를 겹친 서식
	float32                    LineHeight        = 0.0f;     // 줄 상자 높이(기준선 사이 거리)
	float32                    BaselineOffset    = 0.0f;     // 줄 상자 위에서 기준선까지(남는 줄 간격의 절반을 위에 둔다)
	HVector2                   Size;                         // 폭 = 가장 긴 줄, 높이 = 줄 높이 합 + 문단 간격
	bool                       bTruncated        = false;    // 최대 줄 수로 잘렸나
	uint32                     EllipsisCodepoint = 0x2026;   // 글꼴에 "…" 이 없으면 '.'
	int32                      EllipsisCount     = 1;        // '.' 이면 3
};

// 글리프 그림 변형. 같은 글자도 변형마다 따로 래스터화해 아틀라스에 둔다.
struct HGameUIGlyphVariant
{
	float32 HorizontalScale = 1.0f;    // 가로 배율로 래스터화(늘인 그림이 아니라 그 폭으로 그린다)
	bool    bBold           = false;   // 커버리지를 넓혀 굵게(합성)
	float32 BoldStrength    = 0.0f;    // 굵게 세기(em 비율). 0 = 엔진 기본(지금 넓힘), > 0 = 정한 세기(선명한 굵게, GG-19). 세기마다 따로 래스터화한다
};

// 아틀라스 상태(확인용).
struct HGameUIFontAtlasStats
{
	int32   PageCount        = 0;      // 지금 연 페이지 수
	int32   PageLimit        = 0;      // 페이지 상한(SetAtlasPageLimit)
	float32 Usage            = 0.0f;   // 페이지를 채운 비율(0~1, 연 페이지 평균)
	uint64  EvictedPageCount = 0;      // 상한에서 비운 페이지 수(누적)
	uint64  OverLimitCount   = 0;      // 한 프레임에 상한보다 많이 필요해 상한을 넘겨 연 페이지 수(누적)
};

// TTF 글꼴. 글리프는 (코드포인트, 픽셀 크기, 변형)마다 처음 쓸 때 래스터화해 아틀라스 페이지(1024x1024 RGBA8)에 줄 단위로 넣는다.
// 아틀라스 텍셀은 RGB 흰색 + 알파 = 커버리지라, 색은 정점 색이 정한다(가장자리에 어두운 테가 생기지 않는다).
// 페이지가 차면 새 페이지를 연다. 페이지 수가 상한(SetAtlasPageLimit, 기본 4장 = 16MB)이면 이번 프레임에 쓰지 않은 페이지 중
// 가장 오래 안 쓴 것을 비우고 다시 채운다(비운 글자는 다음에 쓸 때 다시 래스터화된다). 프레임 = BeginFrame 사이(PGameUIManager::BuildDrawList 마다) —
// 이미 그리기 목록에 넣은 글자의 페이지는 그 프레임에 비우지 않는다. 한 프레임에 상한보다 많이 필요하면 상한을 넘겨 열고 경고를 한 번 남긴다
// (넘겨 연 페이지는 ClearGlyphs 까지 남아 비우고 다시 쓴다).
// GPU 텍스처는 그리기 직전 GetAtlasTexture(page) 가 페이지마다 만든다. 바뀐 페이지만 다시 만들고, 이전 텍스처는 GPU 가 끝낸 뒤 해제된다.
// 이 글꼴에 없는 글자는 대체 글꼴(SetFallbackFont, 사슬)의 폭 · 글리프 · 아틀라스로 배치 · 그린다. 줄 높이 · 기준선은 이 글꼴 것이다.
// 합성 굵게 = 글자 폭 + em × 세기, 그림도 양쪽 합 같은 만큼 넓힌다. 세기는 기본 BoldStrength(em/24), 글 요소가 문단 서식으로 정할 수 있다(HGameUIParagraphStyle::BoldStrength, GG-18).
//   세기를 정하지 않은 굵게: 커버리지를 넓힌다(1픽셀 미만은 바깥 한 겹을 부분 픽셀로 — 지금까지의 그림 그대로).
//   세기를 정한 굵게(GG-19): 8배로 그려 원판으로 넓힌 뒤 줄인다 — 넓힌 양이 그대로 들어가 획 가운데가 진하고 옅은 테가 없다(같은 세기 값이면 앞의 것보다 굵다).
// 합성 기울임 = 그릴 때 사각형을 ItalicSkew 만큼 비튼다(폭은 그대로).
// 그래픽 없이(헤드리스) 래스터화 · 측정만 해도 된다.
class GAMEFRAMEWORK_API PGameUIFont : public IMemoryObject
{
public:
	static constexpr int32   AtlasSize             = 1024;
	static constexpr int32   GlyphPadding          = 1;               // 선형 필터가 옆 글리프를 섞지 않게
	static constexpr int32   DefaultAtlasPageLimit = 4;
	static constexpr float32 BoldStrength          = 1.0f / 24.0f;    // 합성 굵게 기본 세기 — 더하는 폭(em 비율)
	static constexpr float32 MaxBoldStrength       = 1.0f / 6.0f;     // 글 요소가 정할 수 있는 가장 큰 세기
	static constexpr float32 ItalicSkew            = 0.2126f;         // 합성 기울임(tan 12°)

	// 굵게 세기(em 비율): 0 이하 = 기본 BoldStrength, MaxBoldStrength 를 넘으면 그 값.
	static float32 ResolveBoldStrength(float32 strength);

	struct HGlyph
	{
		int32        Width    = 0;         // 비트맵 픽셀. 공백은 0
		int32        Height   = 0;
		int32        OffsetX  = 0;         // 펜 위치(기준선)에서 비트맵 왼쪽 위까지
		int32        OffsetY  = 0;
		float32      Advance  = 0.0f;      // 다음 글자까지(픽셀)
		HRect        UV;                   // 아틀라스 페이지 UV(0~1)
		bool         bInAtlas = false;
		int32        Page     = 0;         // 아틀라스 페이지
		PGameUIFont* Font     = nullptr;   // 그림이 있는 글꼴(대체 글꼴이 그렸으면 그 글꼴). 그릴 때 이 글꼴 · 페이지로 묶는다
	};

private:
	struct HAtlasPage
	{
		std::vector<uint8>      Pixels;               // RGBA8, AtlasSize x AtlasSize
		int32                   PackX         = GlyphPadding;
		int32                   PackY         = GlyphPadding;
		int32                   PackRowHeight = 0;
		bool                    bFull         = false;
		bool                    bDirty        = true;
		uint64                  LastUsedFrame = 0;
		PSharedPtr<IRawTexture> Texture;

		HAtlasPage() = default;
		// 픽셀(4MB)을 복사하지 않고 옮긴다(목록이 커질 때).
		HAtlasPage(HAtlasPage&&) = default;
		HAtlasPage& operator=(HAtlasPage&&) = default;
	};

	PString            _path;
	std::vector<uint8> _fontData;
	stbtt_fontinfo*    _fontInfo = nullptr;

	HList<HAtlasPage>        _pages;
	int32                    _currentPage      = -1;   // 채우는 중인 페이지
	int32                    _atlasPageLimit   = DefaultAtlasPageLimit;
	uint64                   _evictedPageCount = 0;
	uint64                   _overLimitCount   = 0;
	bool                     _bOverLimitWarned = false;
	HHashMap<uint64, HGlyph> _glyphs;   // 키 = 코드포인트 | 픽셀 크기 | 장평(%) | 굵게 세기
	PSharedPtr<PGameUIFont>  _fallbackFont;

public:
	PGameUIFont() = default;
	// 복사하지 않는다(페이지마다 4MB. DLL 내보내기 클래스라 막지 않으면 복사 대입이 만들어진다).
	PGameUIFont(const PGameUIFont&) = delete;
	PGameUIFont& operator=(const PGameUIFont&) = delete;
	virtual ~PGameUIFont();

	bool LoadFromFile(const PString& path);
	bool IsValid() const;
	const PString& GetPath() const;

	// 프레임 시작(그리기 목록을 만들기 전에 — PGameUIManager::BuildDrawList 가 부른다). 이번 프레임에 쓴 페이지는 비우지 않는다.
	static void BeginFrame();

	// 줄 맨 위에서 기준선까지(픽셀).
	float32 GetAscent(int32 pixelSize) const;
	// 줄 간격(픽셀) = ascent - descent + lineGap.
	float32 GetLineHeight(int32 pixelSize) const;
	// 두 글자를 그리는 글꼴이 다르면(대체 글꼴) 0.
	float32 GetKerning(uint32 first, uint32 second, int32 pixelSize) const;
	// 이 글꼴 또는 대체 글꼴 사슬로 그릴 수 있나.
	bool    HasGlyph(uint32 codepoint) const;
	// 없으면 래스터화해 아틀라스에 넣는다. 이 글꼴에 없는 글자는 대체 글꼴이 돌려준다(Font = 그 글꼴).
	// 한 페이지보다 큰 글리프는 비트맵 없이(Advance 만) 돌려준다. 값으로 돌려준다(페이지를 비우면 캐시 항목이 지워진다).
	HGlyph GetGlyph(uint32 codepoint, int32 pixelSize);
	HGlyph GetGlyph(uint32 codepoint, int32 pixelSize, const HGameUIGlyphVariant& variant);
	// UTF-8 글자 묶음의 크기(픽셀). 폭 = 가장 긴 줄, 높이 = 줄 수 x 줄 간격. '\n' 에서 줄을 바꾼다.
	HVector2 MeasureText(const PString& text, int32 pixelSize);

	// 래스터화 없는 수치(실수 글꼴 크기 — 논리 단위 배치용).
	float32 MeasureAdvance(uint32 codepoint, float32 fontSize) const;
	float32 MeasureKerning(uint32 first, uint32 second, float32 fontSize) const;
	float32 MeasureAscent(float32 fontSize) const;
	float32 MeasureLineHeight(float32 fontSize) const;
	// 서식(장평 · 합성 굵게)을 넣은 글자 폭.
	float32 MeasureStyledAdvance(uint32 codepoint, float32 fontSize, const HGameUITextRunStyle& style) const;
	// text 를 서식대로 배치한다. Wrap 이 None 이 아니고 WrapWidth > 0 이면 그 폭에서 줄을 바꾼다.
	// MaxLines > 0 이면 그 줄 수까지만 두고(bTruncated), bEllipsis 면 잘린 마지막 줄 끝에 말줄임을 붙인다(폭 안에 들어가게 줄인다).
	// Wrap 이 None 이고 bEllipsis 이면 WrapWidth 를 넘는 줄을 말줄임으로 줄인다. bJustify 면 자동으로 바뀐 줄을 WrapWidth 에 맞춰 편다
	// (남는 폭을 글자 사이에 나누되 띄어쓰기 뒤에 4배를 준다 — 띄어쓰기가 적은 한글 줄도 고르게 펴진다). 빈 글은 줄 0개.
	void LayoutText(const PString& text, const HGameUITextLayoutOptions& options, HGameUITextLayout& outLayout) const;
	// 서식 없는 배치(버튼 라벨 · 입력란 · 지금까지의 호출).
	void LayoutText(const PString& text, float32 fontSize, float32 wrapWidth, EGameUITextWrap wrap, int32 maxLines, bool bEllipsis, HGameUITextLayout& outLayout) const;

	// 이 글꼴에 없는 글자를 그릴 글꼴(그 글꼴의 대체 글꼴까지 사슬로 찾는다). nullptr 이면 끊는다. 사슬이 돌면 받지 않는다.
	// 그리기 전에(글꼴을 준비할 때) 정한다.
	void SetFallbackFont(const PSharedPtr<PGameUIFont>& font);
	const PSharedPtr<PGameUIFont>& GetFallbackFont() const;

	// 아틀라스 페이지 상한(1 이상). 줄여도 이미 연 페이지는 ClearGlyphs 까지 남는다(새 페이지를 열 때부터 지킨다).
	void  SetAtlasPageLimit(int32 pageLimit);
	int32 GetAtlasPageLimit() const;
	// 글리프 캐시와 아틀라스 페이지를 모두 비운다(화면을 크게 바꿀 때). 글자는 다음에 쓸 때 다시 래스터화된다.
	// 이번 프레임에 이미 만든 그리기 목록의 글자는 깨지므로 그리기 목록을 만들기 전에 부른다.
	void  ClearGlyphs();

	// 그리기용 GPU 아틀라스(페이지마다). 그래픽 API 가 없거나 없는 페이지면 nullptr.
	PSharedPtr<IRawTexture> GetAtlasTexture(int32 page);

	// 확인용
	uint64 GetGlyphCount() const;
	int32  GetAtlasPageCount() const;
	HGameUIFontAtlasStats GetAtlasStats() const;
	// 페이지 픽셀(RGBA8). 없는 페이지면 빈 배열.
	const std::vector<uint8>& GetAtlasPixels(int32 page) const;

private:
	float32 getScale(int32 pixelSize) const;
	bool    hasOwnGlyph(uint32 codepoint) const;
	// codepoint 를 그리는 글꼴: 이 글꼴 → 대체 글꼴 사슬. 아무도 없으면 이 글꼴(빈 글리프 상자).
	const PGameUIFont* findGlyphFont(uint32 codepoint) const;
	PGameUIFont*       findGlyphFont(uint32 codepoint);
	bool    packGlyph(int32 width, int32 height, int32& outPage, int32& outX, int32& outY);
	int32   openPage();
	int32   takeFreshPage();
	void    evictPage(int32 page);
};
