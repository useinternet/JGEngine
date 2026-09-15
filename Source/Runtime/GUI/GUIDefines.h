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

/*
	ImGuiSelectableFlags_None               = 0,
	ImGuiSelectableFlags_NoAutoClosePopups  = 1 << 0,   // Clicking this doesn't close parent popup window (overrides ImGuiItemFlags_AutoClosePopups)
	ImGuiSelectableFlags_SpanAllColumns     = 1 << 1,   // Frame will span all columns of its container table (text will still fit in current column)
	ImGuiSelectableFlags_AllowDoubleClick   = 1 << 2,   // Generate press events on double clicks too
	ImGuiSelectableFlags_Disabled           = 1 << 3,   // Cannot be selected, display grayed out text
	ImGuiSelectableFlags_AllowOverlap       = 1 << 4,   // (WIP) Hit testing to allow subsequent widgets to overlap this one
	ImGuiSelectableFlags_Highlight          = 1 << 5,   // Make the item be displayed as if it is hovered
*/