#pragma once
#include "Core.h"

#ifdef _GUI
#define GUI_API __declspec(dllexport)
#define GUI_C_API extern "C" __declspec(dllexport)
#else
#define GUI_API __declspec(dllimport)
#define GUI_C_API extern "C" __declspec(dllimport)
#endif


enum class ESelectableFlags
{
	None = 0,
	SpanAllColumns = 0x0001,
	AllowDoubleClick = 0x0002,
	Disabled  = 0x0004,
	Highlight = 0x0008
};

JG_ENUM_FLAG(ESelectableFlags);

enum class EGUIMouseButton : int32
{
	Left = 0,
	Right,
	Middle,
	Count,
};

// HGUI::IsKeyPressed 대상. 쓰는 키만 둔다(게임 UI 뒤로가기 = Escape, 게임 UI 글자 입력 = 편집 키 · Ctrl+A/C/V/X).
enum class EGUIKey : int32
{
	Escape = 0,
	Backspace,
	Delete,
	Enter,
	KeypadEnter,
	Tab,
	LeftArrow,
	RightArrow,
	UpArrow,
	DownArrow,
	Home,
	End,
	PageUp,
	PageDown,
	Space,
	A,
	C,
	V,
	X,
	Count,
};

/*
	ImGuiSelectableFlags_None               = 0,
	ImGuiSelectableFlags_NoAutoClosePopups  = 1 << 0,   // Clicking this doesn't close parent popup window (overrides ImGuiItemFlags_AutoClosePopups)
	ImGuiSelectableFlags_SpanAllColumns     = 1 << 1,   // Frame will span all columns of its container table (text will still fit in current column)
	ImGuiSelectableFlags_AllowDoubleClick   = 1 << 2,   // Generate press events on double clicks too
	ImGuiSelectableFlags_Disabled           = 1 << 3,   // Cannot be selected, display grayed out text
	ImGuiSelectableFlags_AllowOverlap       = 1 << 4,   // (WIP) Hit testing to allow subsequent widgets to overlap this one
	ImGuiSelectableFlags_Highlight          = 1 << 5,   // Make the item be displayed as if it is hovered
*/

// HGUI::PushStyleColor 대상
enum class EGUIColor
{
	Text,
	Border,
	PopupBackground,
	TableHeaderBackground,
	TableBorderStrong,
	TableBorderLight,
	Tab,
	TabHovered,
	TabSelected,
	TabSelectedOverline,
	TabDimmed,
	TabDimmedSelected,
	// 편집기 창 테마(콘솔 등): 입력칸 · 체크박스 · 드롭다운 · 스크롤바
	ChildBackground,
	FrameBackground,
	FrameBackgroundHovered,
	FrameBackgroundActive,
	CheckMark,
	Header,
	HeaderHovered,
	HeaderActive,
	Button,
	ButtonHovered,
	ButtonActive,
	TextDisabled,
	ScrollbarBackground,
	ScrollbarGrab,
	ScrollbarGrabHovered,
	ScrollbarGrabActive,
	Separator,
};

// HGUI::PushStyleVar 대상. 값의 형이 정해져 있다(float32 / HVector2 — 다른 형으로 부르면 ImGui 가 assert 한다).
enum class EGUIStyleVar
{
	FrameRounding,      // float32
	FrameBorderSize,    // float32
	PopupRounding,      // float32
	ChildRounding,      // float32
	ScrollbarSize,      // float32
	ScrollbarRounding,  // float32
	FramePadding,       // HVector2
	ItemSpacing,        // HVector2
	WindowPadding,      // HVector2
};

// HGUI::PushFont 대상. 순서는 DX12GUIBackend::Initialize 가 글꼴을 넣는 순서와 같다.
enum class EGUIFont
{
	Default = 0,
	Large,          // 기본 글꼴의 2배 (통계 창의 큰 숫자)
};