#pragma once
#include "UI/GameUIDefines.h"

class IRawTexture;
class PGameUIFont;

// 글자 스타일. Font 가 nullptr 이면 관리자 기본 글꼴(PGameUIManager::GetDefaultFont).
// 관리자에 기본 글자 스타일이 있고, 글자 요소는 정하지 않은 필드를 그 기본에서 받는다(PGameUIText).
struct HGameUITextStyle
{
	PSharedPtr<PGameUIFont> Font;
	float32                 FontSize = 24.0f;
	HLinearColor            Color    = HLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
};

// 글자 요소의 문단 서식(PGameUIText::SetParagraphStyle). 기본값이면 서식 없는 배치(지금까지와 같음). 값은 논리 단위.
// 줄 나누기 · 높이 측정(MeasureContent) · 말줄임이 이 값을 그대로 쓴다.
struct HGameUIParagraphStyle
{
	float32 LineHeight       = 0.0f;    // 줄 간격(기준선 사이 거리). 0 이면 글꼴 기본(ascent − descent + lineGap)
	float32 LetterSpacing    = 0.0f;    // 자간(1/1000 em, 음수 가능). 글자 사이에만 더한다(줄 끝에는 없다)
	float32 HorizontalScale  = 1.0f;    // 장평(1.0 = 100%). 글자 폭과 그림을 가로로 늘이거나 줄인다
	float32 ParagraphSpacing = 0.0f;    // '\n' 다음 문단 앞에 더하는 간격
	bool    bBold            = false;   // 요소 전체 굵게(합성 — 글꼴에 굵은 면이 없어도 된다)
	bool    bItalic          = false;   // 요소 전체 기울임(합성)
	// 합성 굵게 세기(em 비율 — 그림 넓힘 양쪽 합 = 글자 폭 증가). 이 요소의 요소 전체 굵게와 <b> 구간이 모두 쓴다.
	// 0 = 엔진 기본(PGameUIFont::BoldStrength = em/24, 지금까지와 같은 그림). 최대 PGameUIFont::MaxBoldStrength(em/6). 예: em/28 = 1.0f / 28.0f
	// 세기를 정하면 선명한 굵게(GG-19): 8배로 그려 넓힌 뒤 줄여 획 가운데가 진하다. 넓힌 양이 정한 값 그대로라 0(기본)의 그림보다 같은 값이 더 굵다
	// (기본 넓힘은 1픽셀 미만을 옅은 테로만 그렸다 — 그 넓힘으로 em/16 이던 굵기는 선명한 굵게 em/28 근처).
	float32 BoldStrength     = 0.0f;
};

// 버튼 스타일(CommonUI 의 ButtonStyle 자리). 배경 = Texture(없으면 단색) × 상태 색, 가운데 라벨은 Label 스타일.
// 버튼이 스타일을 정하지 않으면 관리자 기본 버튼 스타일을 통째로 쓴다. 게임은 몇 가지 스타일(주 버튼 · 보조 버튼)을 만들어 나눠 쓴다.
struct HGameUIButtonStyle
{
	PSharedPtr<IRawTexture> Texture;
	HLinearColor            Normal             = HLinearColor(0.20f, 0.22f, 0.26f, 1.0f);
	HLinearColor            Hovered            = HLinearColor(0.30f, 0.33f, 0.40f, 1.0f);
	HLinearColor            Pressed            = HLinearColor(0.12f, 0.13f, 0.16f, 1.0f);
	HLinearColor            Disabled           = HLinearColor(0.16f, 0.16f, 0.17f, 0.70f);
	HGameUITextStyle        Label;
	HLinearColor            DisabledLabelColor = HLinearColor(0.55f, 0.55f, 0.55f, 1.0f);

	HGameUIButtonStyle()
	{
		Label.FontSize = 28.0f;
	}
};

// 한 줄 입력란 스타일(PGameUITextInput). 배경 = Texture(없으면 단색) × 상태 색. 글은 Text 스타일, 빈 칸이면 자리 글을 PlaceholderColor 로.
// 입력란이 스타일을 정하지 않으면 관리자 기본 입력란 스타일을 통째로 쓴다.
struct HGameUITextInputStyle
{
	PSharedPtr<IRawTexture> Texture;
	HLinearColor            Normal            = HLinearColor(0.08f, 0.09f, 0.11f, 1.0f);
	HLinearColor            Hovered           = HLinearColor(0.11f, 0.12f, 0.15f, 1.0f);
	HLinearColor            Focused           = HLinearColor(0.13f, 0.16f, 0.22f, 1.0f);
	HLinearColor            Disabled          = HLinearColor(0.10f, 0.10f, 0.11f, 0.70f);
	HGameUITextStyle        Text;
	HLinearColor            PlaceholderColor  = HLinearColor(0.50f, 0.52f, 0.56f, 1.0f);
	HLinearColor            DisabledTextColor = HLinearColor(0.55f, 0.55f, 0.55f, 1.0f);
	HLinearColor            CaretColor        = HLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	HLinearColor            SelectionColor    = HLinearColor(0.26f, 0.46f, 0.82f, 0.65f);
	float32                 Padding           = 12.0f;   // 좌우 안쪽 여백(논리 단위). 글은 이 안에서만 보인다
};

// 스크롤 막대를 언제 보이나(PGameUIScrollBox, 넘기는 방향의 막대만).
enum class EGameUIScrollBarVisibility
{
	Auto,     // 내용이 칸보다 클 때만
	Always,   // 늘(내용이 다 들어가면 손잡이가 막대 전체)
	Never,    // 숨김(휠 · 코드로는 넘어간다)
};

// 스크롤 칸 스타일(PGameUIScrollBox). 막대 = 바탕(TrackTexture, 없으면 단색) × TrackColor 위에 손잡이(ThumbTexture) × 상태 색.
// 막대는 칸 안 오른쪽(세로) · 아래(가로)에 놓이고 보이는 칸을 그만큼 줄인다: 막대 자리 = Thickness + 2 × Margin.
// 스크롤 칸이 스타일을 정하지 않으면 관리자 기본 스크롤 스타일을 통째로 쓴다.
struct HGameUIScrollStyle
{
	PSharedPtr<IRawTexture>    TrackTexture;
	PSharedPtr<IRawTexture>    ThumbTexture;
	HLinearColor               TrackColor     = HLinearColor(0.10f, 0.11f, 0.13f, 0.60f);
	HLinearColor               ThumbNormal    = HLinearColor(0.42f, 0.45f, 0.52f, 0.85f);
	HLinearColor               ThumbHovered   = HLinearColor(0.56f, 0.60f, 0.68f, 0.95f);
	HLinearColor               ThumbPressed   = HLinearColor(0.70f, 0.74f, 0.82f, 1.00f);
	float32                    Thickness      = 12.0f;   // 막대 굵기(논리 단위)
	float32                    Margin         = 2.0f;    // 막대 둘레 여백(칸 가장자리 · 내용 쪽)
	float32                    MinThumbLength = 24.0f;   // 손잡이 최소 길이
	float32                    WheelStep      = 60.0f;   // 휠 한 칸에 넘기는 양(논리 단위)
	float32                    SmoothSeconds  = 0.0f;    // > 0 이면 휠 · 막대 빈 곳 누르기를 이 길이로 부드럽게(넘긴 양 트윈, OutCubic). 0 = 바로
	EGameUIScrollBarVisibility Visibility     = EGameUIScrollBarVisibility::Auto;
};
