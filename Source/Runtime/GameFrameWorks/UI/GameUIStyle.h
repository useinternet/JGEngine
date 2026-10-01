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
