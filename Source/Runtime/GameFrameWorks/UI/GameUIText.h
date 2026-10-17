#pragma once
#include "UI/GameUIElement.h"
#include "UI/GameUIStyle.h"
#include "UI/GameUIFont.h"

// 글자. UTF-8, '\n' 에서 줄을 바꾸고, 줄바꿈 방식(SetWrap)을 정하면 요소 폭에서도 바꾼다. 요소 사각형 안에서 가로 · 세로 정렬한다.
// 넘치는 글은 밖까지 그린다. SetClip(true) 면 요소 사각형 밖을 잘라낸다. SetMaxLines 로 줄 수를 제한하고, SetEllipsis 면 잘린 끝에 말줄임(…)을 붙인다.
// 문단 서식(SetParagraphStyle): 줄 간격 · 자간 · 장평 · 문단 간격 · 요소 전체 합성 굵게/기울임. 정렬 Justify = 양쪽 맞춤.
// 구간 서식(SetRichText(true)): 글 안 마크업 <b> <i> <color=#RRGGBB(AA)> <hscale=퍼센트> 와 닫는 태그(겹쳐 쓸 수 있다),
//   글자 그대로의 < > & 는 &lt; &gt; &amp;. 모르는 태그 · 짝 없는 닫는 태그는 글자 그대로 그린다(글이 조용히 사라지지 않게).
// 줄 나누기는 논리 단위(실수 글꼴 크기의 글자 너비)로 하므로 화면 배율이 달라도 같은 자리에서 바뀐다. 배치한 크기는 MeasureContent 로 읽는다.
// 스타일은 필드별로 덮어쓴다: 정하지 않은 필드(글꼴 · 크기 · 색)는 관리자 기본 글자 스타일(PGameUIManager::GetDefaultTextStyle)에서 받는다.
// 크기는 논리 단위이고, 화면 배율만큼 큰 픽셀 크기로 래스터화해 선명하다.
class GAMEFRAMEWORKS_API PGameUIText : public PGameUIElement
{
	static constexpr uint8 OverrideFont  = 1 << 0;
	static constexpr uint8 OverrideSize  = 1 << 1;
	static constexpr uint8 OverrideColor = 1 << 2;

	PString                _text;          // SetText 그대로(마크업 포함)
	PString                _displayText;   // 그리는 글(리치 텍스트면 마크업을 뺀 글)
	HList<HGameUITextSpan> _spans;         // 마크업에서 읽은 구간 서식(_displayText 바이트 기준)
	bool                   _bRichText       = false;
	HGameUITextStyle       _style;
	HGameUIParagraphStyle  _paragraph;
	uint8                  _overrides       = 0;
	EGameUIHorizontalAlign _horizontalAlign = EGameUIHorizontalAlign::Left;
	EGameUIVerticalAlign   _verticalAlign   = EGameUIVerticalAlign::Top;
	EGameUITextWrap        _wrap            = EGameUITextWrap::None;
	int32                  _maxLines        = 0;
	bool                   _bEllipsis       = false;
	bool                   _bClip           = false;

public:
	PGameUIText() = default;
	virtual ~PGameUIText() = default;

	void SetText(const PString& text);
	// SetText 로 넣은 글(마크업 포함).
	const PString& GetText() const;
	// 그리는 글(리치 텍스트면 마크업을 뺀 글).
	const PString& GetDisplayText() const;
	// 글 안 마크업을 읽는다(기본 꺼짐 — 꺼져 있으면 '<' 도 글자 그대로).
	void SetRichText(bool bRichText);
	bool IsRichText() const;
	// 마크업에서 읽은 구간 서식(확인용, GetDisplayText 바이트 기준).
	const HList<HGameUITextSpan>& GetSpans() const;
	// 스타일 전체(세 필드 모두 덮어쓴다).
	void SetStyle(const HGameUITextStyle& style);
	// 필드 하나씩 덮어쓴다.
	void SetFont(const PSharedPtr<PGameUIFont>& font);
	void SetFontSize(float32 fontSize);
	void SetColor(const HLinearColor& color);
	// 지금 글 색: 덮어쓴 색, 없으면 관리자 기본 글자 스타일 색(manager 가 없으면 흰색). 트윈이 지금 값에서 시작할 때 읽는다.
	HLinearColor GetColor(const PGameUIManager* manager = nullptr) const;
	// 덮어쓴 필드를 모두 지운다(관리자 기본을 따른다).
	void ClearStyle();
	// 문단 서식(줄 간격 · 자간 · 장평 · 문단 간격 · 요소 전체 굵게/기울임). 기본값이면 서식 없음.
	void SetParagraphStyle(const HGameUIParagraphStyle& style);
	const HGameUIParagraphStyle& GetParagraphStyle() const;
	// 가로 Justify = 양쪽 맞춤(자동으로 바뀐 줄을 폭에 맞춰 편다 — 줄바꿈을 켜야 의미가 있다).
	void SetAlignment(EGameUIHorizontalAlign horizontal, EGameUIVerticalAlign vertical);
	// 자동 줄바꿈(요소 폭). 기본 None 이면 '\n' 에서만 바꾼다.
	void SetWrap(EGameUITextWrap wrap);
	EGameUITextWrap GetWrap() const;
	// 최대 줄 수. 0 이면 제한 없음. 넘는 줄은 그리지 않는다.
	void SetMaxLines(int32 maxLines);
	int32 GetMaxLines() const;
	// 말줄임: 최대 줄 수로 잘린 마지막 줄 끝, 줄바꿈이 None 이면 요소 폭을 넘는 줄 끝을 줄이고 "…" 을 붙인다.
	void SetEllipsis(bool bEllipsis);
	bool IsEllipsis() const;
	// 요소 사각형 밖을 잘라낸다(부모 클립과 겹치는 부분만 그린다).
	void SetClip(bool bClip);
	bool IsClip() const;
	// 그릴 때 쓰는 스타일 = 덮어쓴 필드 + 관리자 기본 글자 스타일. manager 가 nullptr 이면 + 타입 기본값.
	HGameUITextStyle ResolveStyle(const PGameUIManager* manager) const;
	// 폭 width(논리 단위)에서 이 요소의 스타일 · 서식 · 줄바꿈 · 최대 줄 수 · 말줄임으로 배치한 글 상자 크기(논리 단위).
	// 높이 = 줄 높이 합 + 문단 간격. 목록에서 다음 요소를 아래에 놓을 때 쓴다. manager 는 기본 스타일 · 글꼴(위젯 안에서는 GetManager()).
	// 글꼴이 없으면 (0, 0).
	HVector2 MeasureContent(float32 width, PGameUIManager* manager) const;
	// 그리기와 같은 배치(줄 · 글자 자리). 글꼴이 없으면 false.
	bool LayoutContent(float32 width, PGameUIManager* manager, HGameUITextLayout& outLayout) const;

	// rect(논리 단위) 안에 글자를 정렬해 그리기 목록에 넣는다('\n' 에서만 줄을 바꾼다, 서식 없음). 버튼 라벨도 이것으로 그린다.
	// (이름이 DrawText 가 아닌 이유: Windows 헤더의 DrawText 매크로가 선언을 DrawTextW 로 바꾼다.)
	static void AddTextDraw(HGameUIDrawContext& context, PGameUIFont& font, const PString& text, float32 fontSize,
		const HRect& rect, const HLinearColor& color, EGameUIHorizontalAlign horizontal, EGameUIVerticalAlign vertical);
	// 배치한 글(PGameUIFont::LayoutText 결과)을 그린다. 글리프 자리는 논리 배치 × 화면 배율이라, 그린 줄 폭이 배치 폭과 같다.
	// 서식(굵게 · 기울임 · 장평 · 색)은 layout.Runs 를 따른다. color 는 색을 정하지 않은 글자의 색.
	static void AddTextDraw(HGameUIDrawContext& context, PGameUIFont& font, const PString& text, const HGameUITextLayout& layout, float32 fontSize,
		const HRect& rect, const HLinearColor& color, EGameUIHorizontalAlign horizontal, EGameUIVerticalAlign vertical);
	// font → 관리자 기본 글자 스타일의 글꼴 → 관리자 기본 글꼴. 모두 없으면 nullptr.
	static PSharedPtr<PGameUIFont> ResolveFont(const PSharedPtr<PGameUIFont>& font, PGameUIManager* manager);

protected:
	virtual void onBuildDraw(HGameUIDrawContext& context) override;

private:
	// _text → _displayText · _spans (리치 텍스트가 꺼져 있으면 그대로).
	void rebuildDisplayText();
	HGameUITextLayoutOptions makeLayoutOptions(float32 fontSize, float32 width) const;
};
