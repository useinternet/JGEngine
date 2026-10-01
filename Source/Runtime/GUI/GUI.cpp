#include "PCH/PCH.h"
#include "GUI.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_internal.h"
#include "Imgui/implot.h"
#include "GUIModule.h"
#include "Backends/GUIBackend.h"

#pragma warning( disable : 4244 )

ImColor ToImGui(const HLinearColor& InColor)
{
	return ImColor(InColor.R, InColor.G, InColor.B, InColor.A);
}

#define BEGIN_TOIMGUIFLAGS(JGFlagType, ImGuiFlagType) \
ImGuiFlagType ToImGui(JGFlagType InFlags) \
{ \
	ImGuiFlagType Result = 0; \

#define END_TO_IMGUIFLAGS() \
	return Result; \
} \

#define TO_IMGUIFLAGS(JG_Flag, ImGui_Flag) \
if (EnumHasAnyFlags(InFlags, JG_Flag)) \
{ \
	Result |= ImGui_Flag; \
} \


BEGIN_TOIMGUIFLAGS(ESelectableFlags, ImGuiSelectableFlags)
TO_IMGUIFLAGS(ESelectableFlags::AllowDoubleClick, ImGuiSelectableFlags_AllowDoubleClick);
TO_IMGUIFLAGS(ESelectableFlags::Disabled, ImGuiSelectableFlags_Disabled);
TO_IMGUIFLAGS(ESelectableFlags::Highlight, ImGuiSelectableFlags_Highlight);
TO_IMGUIFLAGS(ESelectableFlags::SpanAllColumns, ImGuiSelectableFlags_SpanAllColumns);
END_TO_IMGUIFLAGS()

ImGuiCol ToImGui(EGUIColor InColor)
{
	switch (InColor)
	{
	case EGUIColor::Text:                  return ImGuiCol_Text;
	case EGUIColor::Border:                return ImGuiCol_Border;
	case EGUIColor::PopupBackground:       return ImGuiCol_PopupBg;
	case EGUIColor::TableHeaderBackground: return ImGuiCol_TableHeaderBg;
	case EGUIColor::TableBorderStrong:     return ImGuiCol_TableBorderStrong;
	case EGUIColor::TableBorderLight:      return ImGuiCol_TableBorderLight;
	case EGUIColor::Tab:                   return ImGuiCol_Tab;
	case EGUIColor::TabHovered:            return ImGuiCol_TabHovered;
	case EGUIColor::TabSelected:           return ImGuiCol_TabSelected;
	case EGUIColor::TabSelectedOverline:   return ImGuiCol_TabSelectedOverline;
	case EGUIColor::TabDimmed:             return ImGuiCol_TabDimmed;
	case EGUIColor::TabDimmedSelected:     return ImGuiCol_TabDimmedSelected;
	case EGUIColor::ChildBackground:        return ImGuiCol_ChildBg;
	case EGUIColor::FrameBackground:        return ImGuiCol_FrameBg;
	case EGUIColor::FrameBackgroundHovered: return ImGuiCol_FrameBgHovered;
	case EGUIColor::FrameBackgroundActive:  return ImGuiCol_FrameBgActive;
	case EGUIColor::CheckMark:              return ImGuiCol_CheckMark;
	case EGUIColor::Header:                 return ImGuiCol_Header;
	case EGUIColor::HeaderHovered:          return ImGuiCol_HeaderHovered;
	case EGUIColor::HeaderActive:           return ImGuiCol_HeaderActive;
	case EGUIColor::Button:                 return ImGuiCol_Button;
	case EGUIColor::ButtonHovered:          return ImGuiCol_ButtonHovered;
	case EGUIColor::ButtonActive:           return ImGuiCol_ButtonActive;
	case EGUIColor::TextDisabled:           return ImGuiCol_TextDisabled;
	case EGUIColor::ScrollbarBackground:    return ImGuiCol_ScrollbarBg;
	case EGUIColor::ScrollbarGrab:          return ImGuiCol_ScrollbarGrab;
	case EGUIColor::ScrollbarGrabHovered:   return ImGuiCol_ScrollbarGrabHovered;
	case EGUIColor::ScrollbarGrabActive:    return ImGuiCol_ScrollbarGrabActive;
	case EGUIColor::Separator:              return ImGuiCol_Separator;
	}
	return ImGuiCol_Text;
}

ImGuiStyleVar ToImGui(EGUIStyleVar InStyleVar)
{
	switch (InStyleVar)
	{
	case EGUIStyleVar::FrameRounding:     return ImGuiStyleVar_FrameRounding;
	case EGUIStyleVar::FrameBorderSize:   return ImGuiStyleVar_FrameBorderSize;
	case EGUIStyleVar::PopupRounding:     return ImGuiStyleVar_PopupRounding;
	case EGUIStyleVar::ChildRounding:     return ImGuiStyleVar_ChildRounding;
	case EGUIStyleVar::ScrollbarSize:     return ImGuiStyleVar_ScrollbarSize;
	case EGUIStyleVar::ScrollbarRounding: return ImGuiStyleVar_ScrollbarRounding;
	case EGUIStyleVar::FramePadding:      return ImGuiStyleVar_FramePadding;
	case EGUIStyleVar::ItemSpacing:       return ImGuiStyleVar_ItemSpacing;
	case EGUIStyleVar::WindowPadding:     return ImGuiStyleVar_WindowPadding;
	}
	return ImGuiStyleVar_FrameRounding;
}


void HGUI::SameLine()
{
	ImGui::SameLine();
}

void HGUI::NextLine()
{
	ImGui::NewLine();
}

// 문자열을 printf 서식으로 넘기지 않는다. ImGui::Text(str) 는 str 안의 % 를 변환 지정자로 읽어
// "12.5%   123" 같은 줄이 깨지고(메모리 통계 창 peak 열), "%s" 가 들어가면 빈 가변 인자를 읽는다(DevConsole_TODO 2-1).
void HGUI::Text(const PString& InStr)
{
	ImGui::TextUnformatted(InStr.GetCStr());
}

void HGUI::Text(const PString& InStr, const HLinearColor& InColor)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ToImGui(InColor).Value);
	ImGui::TextUnformatted(InStr.GetCStr());
	ImGui::PopStyleColor();
}

bool HGUI::InputText(const PString& InName, PString& OutStr)
{
	char Buf[512] = { 0, };
	memcpy_s(Buf, 512, OutStr.GetCStr(), OutStr.Length());
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(InName.GetCStr()); ImGui::SameLine();
	PString InputTextKey = PString("##") + InName;
	bool bResult = ImGui::InputText(InputTextKey.GetCStr(), Buf, 512, ImGuiInputTextFlags_EnterReturnsTrue);

	OutStr = Buf;
	return bResult;
}

void HGUI::PlotBarGroups(const HPlotBarGroupsArguments& InArgs)
{
	HList<const char*> GroupLabels;
	HList<const char*> DataLabels;
	for (const PString& GroupLabel : InArgs.GroupLabels)
	{
		GroupLabels.push_back(GroupLabel.GetCStr());
	}

	for (const PString& DataLabel : InArgs.DataLabels)
	{
		DataLabels.push_back(DataLabel.GetCStr());
	}

	if (GroupLabels.empty() || DataLabels.empty())
	{
		return;
	}

	// 틱 위치는 배열로 넘긴다. (v_min, v_max, n) 판은 n 을 최소 2로 올려 그룹이 하나일 때 라벨 배열을 넘겨 읽는다.
	// (2026-09-28: 메모리 위젯이 청크 하나만 있는 첫 프레임에 열리자 strlen 크래시)
	HList<double> TickPositions;
	TickPositions.reserve(GroupLabels.size());
	for (size_t i = 0; i < GroupLabels.size(); ++i)
	{
		TickPositions.push_back(static_cast<double>(i));
	}

	if (ImPlot::BeginPlot(InArgs.TitleName.GetCStr(), ImVec2(InArgs.PlotSize.x, InArgs.PlotSize.y), ImPlotFlags_NoMouseText)) {
		ImPlot::SetupLegend(ImPlotLocation_South, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);
		ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_Invert);
		ImPlot::SetupAxisTicks(ImAxis_Y1, TickPositions.data(), static_cast<int32>(TickPositions.size()), GroupLabels.data(), false);
		ImPlot::PlotBarGroups(DataLabels.data(), InArgs.Datas.data(), static_cast<int32>(DataLabels.size()), static_cast<int32>(GroupLabels.size()), 0.75, 0, ImPlotBarGroupsFlags_Stacked | ImPlotBarGroupsFlags_Horizontal);
		ImPlot::EndPlot();
	}
}

void HGUI::PlotTest()
{
}

bool HGUI::Selectable(const PString& InName, bool bSelected, ESelectableFlags InFlags, const HVector2& InSize)
{
	ImGuiSelectableFlags ImGuiFlags = ToImGui(InFlags);
	ImVec2 ImSize = ImVec2(InSize.x, InSize.y);

	bool bResult = ImGui::Selectable(InName.GetCStr(), bSelected, ImGuiFlags, ImSize);

	return bResult;
}

void HGUI::Image(uint64 InTextureID, const HVector2& InSize)
{
	HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	if (GUIModule == nullptr)
	{
		return;
	}

	ImTextureID ImTexID = (ImTextureID)(GUIModule->GetGUIBackend().Pin()->GPUAllocate(InTextureID));
	ImVec2 ImSize = ImVec2(InSize.x, InSize.y);

	ImGui::Image(ImTexID, ImSize);
}

bool HGUI::Checkbox(const PString& InName, bool& InOutValue)
{
	return ImGui::Checkbox(InName.GetCStr(), &InOutValue);
}

namespace
{
	// 가변 길이 버퍼 (imgui_stdlib 의 std::string 판과 같은 방식). UserData 는 std::string*.
	int inputTextResizeCallback(ImGuiInputTextCallbackData* InData)
	{
		if (InData->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			std::string* Buffer = static_cast<std::string*>(InData->UserData);
			Buffer->resize(InData->BufTextLen);
			InData->Buf = Buffer->data();
		}

		return 0;
	}
}

bool HGUI::InputTextWithHint(const PString& InName, const PString& InHint, PString& InOutStr, float32 InWidth)
{
	std::string Buffer = InOutStr.GetRawString();
	const PString InputTextKey = PString("##") + InName;

	if (InWidth != 0.0f)
	{
		ImGui::PushItemWidth(InWidth);
	}

	const bool bChanged = ImGui::InputTextWithHint(InputTextKey.GetCStr(), InHint.GetCStr(), Buffer.data(), Buffer.capacity() + 1,
		ImGuiInputTextFlags_CallbackResize, &inputTextResizeCallback, &Buffer);

	if (InWidth != 0.0f)
	{
		ImGui::PopItemWidth();
	}

	if (bChanged)
	{
		InOutStr = Buffer.c_str();
	}

	return bChanged;
}

bool HGUI::IsItemActive()
{
	return ImGui::IsItemActive();
}

void HGUI::BeginTooltipAboveItem(const PString& InName, float32 InExtraGap)
{
	// 창의 왼쪽 아래를 직전 항목의 왼쪽 위(줄 간격 + InExtraGap 만큼 위)에 맞춘다. 그래서 내용이 늘면 위로 자란다.
	const ImVec2 ItemMin = ImGui::GetItemRectMin();
	ImGui::SetNextWindowPos(ImVec2(ItemMin.x, ItemMin.y - ImGui::GetStyle().ItemSpacing.y - InExtraGap), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
	// 배경을 불투명하게 한다. 기본 PopupBg 알파 0.94 는 GUI 가 sRGB 로 출력되면서 뒤 글자가 밝게 비친다(로그 뷰 위에 겹쳐 읽기 어렵다).
	ImGui::SetNextWindowBgAlpha(1.0f);

	// BeginTooltip 과 같은 창 설정이지만 이름을 따로 둔다(마우스 툴팁 "##Tooltip_00" 과 내용이 섞이지 않게).
	// 입력·포커스·내비게이션을 가져가지 않으므로 입력줄에 계속 칠 수 있다.
	const ImGuiWindowFlags Flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	ImGui::Begin(InName.GetCStr(), nullptr, Flags);
}

void HGUI::EndTooltip()
{
	ImGui::End();
}

void HGUI::BeginChild(const PString& InName, const HVector2& InSize)
{
	// 반환값과 상관없이 EndChild 를 불러야 한다(ImGui 규칙).
	ImGui::BeginChild(InName.GetCStr(), ImVec2(InSize.x, InSize.y), ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar);
}

void HGUI::EndChild(bool bStickToBottom)
{
	// 지난 프레임에 맨 아래였으면 이번 프레임에 늘어난 내용의 맨 아래로 다시 붙인다(로그 뷰).
	if (bStickToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
}

float32 HGUI::GetFrameHeightWithSpacing()
{
	return ImGui::GetFrameHeightWithSpacing();
}

namespace
{
	struct HInputTextHistoryContext
	{
		std::string*                    Buffer;
		const HList<PString>*           History;
		int32*                          HistoryPos;
		const HGUIInputTextKeyDelegate* KeyHandler;
	};

	// ↑/↓/Tab 을 호출 쪽 키 처리기에 넘긴다. 처리기가 처리했으면(true) 입력줄을 처리기가 돌려준 글자로 바꾸고 true.
	bool dispatchInputTextKey(ImGuiInputTextCallbackData* InData, const HGUIInputTextKeyDelegate& InKeyHandler)
	{
		if (InKeyHandler.IsBound() == false)
		{
			return false;
		}

		EGUIInputTextKey Key = EGUIInputTextKey::Tab;
		if (InData->EventFlag == ImGuiInputTextFlags_CallbackHistory)
		{
			Key = (InData->EventKey == ImGuiKey_UpArrow) ? EGUIInputTextKey::Up : EGUIInputTextKey::Down;
		}

		PString Text(InData->Buf);
		if (InKeyHandler.Execute(Key, Text) == false)
		{
			return false;
		}

		if (Text.GetRawString() != InData->Buf)
		{
			InData->DeleteChars(0, InData->BufTextLen);
			InData->InsertChars(0, Text.GetCStr());
		}

		return true;
	}

	int inputTextHistoryCallback(ImGuiInputTextCallbackData* InData)
	{
		HInputTextHistoryContext* Context = static_cast<HInputTextHistoryContext*>(InData->UserData);

		// 가변 길이 버퍼 (imgui_stdlib 의 std::string 판과 같은 방식)
		if (InData->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			Context->Buffer->resize(InData->BufTextLen);
			InData->Buf = Context->Buffer->data();
			return 0;
		}

		if (InData->EventFlag != ImGuiInputTextFlags_CallbackHistory && InData->EventFlag != ImGuiInputTextFlags_CallbackCompletion)
		{
			return 0;
		}

		// 호출 쪽이 먼저 처리한다(콘솔 명령 자동완성). 처리하지 않은 ↑/↓ 만 히스토리를 오간다.
		if (dispatchInputTextKey(InData, *Context->KeyHandler) || InData->EventFlag != ImGuiInputTextFlags_CallbackHistory)
		{
			return 0;
		}

		const int32 Count = static_cast<int32>(Context->History->size());
		if (Count == 0)
		{
			return 0;
		}

		// HistoryPos: -1 = 새 줄(히스토리를 보지 않는 중), 0..Count-1 = 오래된 것부터
		int32& Pos = *Context->HistoryPos;
		const int32 PrevPos = Pos;
		if (InData->EventKey == ImGuiKey_UpArrow)
		{
			if (Pos == -1)
			{
				Pos = Count - 1;
			}
			else if (Pos > 0)
			{
				--Pos;
			}
		}
		else if (InData->EventKey == ImGuiKey_DownArrow)
		{
			if (Pos != -1)
			{
				Pos = (Pos + 1 >= Count) ? -1 : Pos + 1;
			}
		}

		if (PrevPos != Pos)
		{
			const char* Text = (Pos >= 0) ? (*Context->History)[Pos].GetCStr() : "";
			InData->DeleteChars(0, InData->BufTextLen);
			InData->InsertChars(0, Text);
		}

		return 0;
	}
}

bool HGUI::InputTextWithHistory(const PString& InName, PString& InOutStr, const HList<PString>& InHistory, int32& InOutHistoryPos,
	const HGUIInputTextKeyDelegate& InKeyHandler, const PString& InHint)
{
	std::string Buffer = InOutStr.GetRawString();
	HInputTextHistoryContext Context{ &Buffer, &InHistory, &InOutHistoryPos, &InKeyHandler };

	// 창이 처음 뜰 때 입력줄에 포커스를 둔다.
	if (ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere();
	}

	const PString InputTextKey = PString("##") + InName;
	ImGuiInputTextFlags Flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackResize;
	if (InKeyHandler.IsBound())
	{
		Flags |= ImGuiInputTextFlags_CallbackCompletion;
	}

	ImGui::PushItemWidth(-1.0f);
	const bool bEntered = InHint.Empty()
		? ImGui::InputText(InputTextKey.GetCStr(), Buffer.data(), Buffer.capacity() + 1, Flags, &inputTextHistoryCallback, &Context)
		: ImGui::InputTextWithHint(InputTextKey.GetCStr(), InHint.GetCStr(), Buffer.data(), Buffer.capacity() + 1, Flags, &inputTextHistoryCallback, &Context);
	ImGui::PopItemWidth();

	// ImGui 는 Enter 에서 입력줄의 포커스를 푼다(ConfigInputTextEnterKeepActive = false). 연속 입력을 위해 다시 잡는다.
	if (bEntered)
	{
		ImGui::SetKeyboardFocusHere(-1);
	}

	InOutStr = Buffer.c_str();
	return bEntered;
}

bool HGUI::Button(const PString& InName, const HVector2& InSize)
{
	return ImGui::Button(InName.GetCStr(), ImVec2(InSize.x, InSize.y));
}

void HGUI::Separator()
{
	ImGui::Separator();
}

bool HGUI::CollapsingHeader(const PString& InName, bool bDefaultOpen)
{
	const ImGuiTreeNodeFlags Flags = bDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None;
	return ImGui::CollapsingHeader(InName.GetCStr(), Flags);
}

HGUIImageInput HGUI::InteractiveImage(const PString& InName, uint64 InTextureID, const HVector2& InSize)
{
	HGUIImageInput Input;

	// InvisibleButton 은 크기 0 을 받지 않는다(ImGui assert).
	if (InSize.x <= 0.0f || InSize.y <= 0.0f)
	{
		return Input;
	}

	// 이미지를 그린 자리에 같은 크기의 보이지 않는 버튼을 겹친다. 버튼이 누름을 잡아 창이 끌려가지 않는다.
	const ImVec2 ItemMin = ImGui::GetCursorScreenPos();
	Image(InTextureID, InSize);
	ImGui::SetCursorScreenPos(ItemMin);
	ImGui::InvisibleButton(InName.GetCStr(), ImVec2(InSize.x, InSize.y),
		ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

	const ImGuiIO& IO = ImGui::GetIO();
	const bool bActive = ImGui::IsItemActive();
	Input.bHovered      = ImGui::IsItemHovered();
	Input.LocalPosition = HVector2(IO.MousePos.x - ItemMin.x, IO.MousePos.y - ItemMin.y);
	for (int32 ButtonIndex = 0; ButtonIndex < (int32)EGUIMouseButton::Count; ++ButtonIndex)
	{
		Input.bClicked[ButtonIndex] = ImGui::IsItemClicked(ButtonIndex);
		Input.bDown[ButtonIndex]    = bActive && ImGui::IsMouseDown(ButtonIndex);
	}
	Input.MouseDelta     = HVector2(IO.MouseDelta.x, IO.MouseDelta.y);
	Input.Wheel          = Input.bHovered ? IO.MouseWheel : 0.0f;
	Input.ScreenPosition = HVector2(ItemMin.x, ItemMin.y);
	return Input;
}


void HGUI::SameLine(float32 InSpacing)
{
	ImGui::SameLine(0.0f, InSpacing);
}

void HGUI::Spacing()
{
	ImGui::Spacing();
}

HVector2 HGUI::GetContentRegionAvail()
{
	const ImVec2 Avail = ImGui::GetContentRegionAvail();
	return HVector2(Avail.x, Avail.y);
}

float64 HGUI::GetTime()
{
	return ImGui::GetTime();
}

void HGUI::PushFont(EGUIFont InFont)
{
	// 글꼴 순서는 DX12GUIBackend::Initialize 가 넣은 순서(EGUIFont)와 같다. 없으면 기본 글꼴(nullptr).
	ImFontAtlas* Atlas = ImGui::GetIO().Fonts;
	const int32 Index = static_cast<int32>(InFont);
	ImGui::PushFont((Index < Atlas->Fonts.Size) ? Atlas->Fonts[Index] : nullptr);
}

void HGUI::PopFont()
{
	ImGui::PopFont();
}

void HGUI::PushStyleColor(EGUIColor InTarget, const HLinearColor& InColor)
{
	ImGui::PushStyleColor(ToImGui(InTarget), ToImGui(InColor).Value);
}

void HGUI::PopStyleColor(int32 InCount)
{
	ImGui::PopStyleColor(InCount);
}

void HGUI::TextRight(const PString& InStr)
{
	const float32 TextWidth = ImGui::CalcTextSize(InStr.GetCStr()).x;
	const float32 Avail     = ImGui::GetContentRegionAvail().x;
	if (Avail > TextWidth)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Avail - TextWidth);
	}
	ImGui::TextUnformatted(InStr.GetCStr());
}

void HGUI::TextRight(const PString& InStr, const HLinearColor& InColor)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ToImGui(InColor).Value);
	TextRight(InStr);
	ImGui::PopStyleColor();
}

void HGUI::ItemTooltip(const PString& InText)
{
	if (ImGui::BeginItemTooltip())
	{
		ImGui::TextUnformatted(InText.GetCStr());
		ImGui::EndTooltip();
	}
}

void HGUI::FillWindowBackground(const HLinearColor& InColor)
{
	// 창 배경색(스타일)은 모든 창이 같이 쓴다. 이 창만 바꾸려고 안쪽 사각형을 먼저 칠한다. 안쪽 사각형은 여백을 포함한다.
	ImGuiWindow* Window = ImGui::GetCurrentWindow();
	Window->DrawList->PushClipRect(Window->InnerRect.Min, Window->InnerRect.Max, false);
	Window->DrawList->AddRectFilled(Window->InnerRect.Min, Window->InnerRect.Max, ToImGui(InColor));
	Window->DrawList->PopClipRect();
}

void HGUI::BeginPanel(const PString& InName, const HVector2& InSize, const HLinearColor& InBackground, const HLinearColor& InBorder)
{
	ImGuiChildFlags ChildFlags = ImGuiChildFlags_Border | ImGuiChildFlags_AlwaysUseWindowPadding;
	if (InSize.y <= 0.0f)
	{
		ChildFlags |= ImGuiChildFlags_AutoResizeY;
	}

	// 색 · 모서리 · 여백 · 테두리 두께는 BeginChild 가 창을 만들 때 읽으므로 바로 되돌린다. 안쪽 항목에는 번지지 않는다.
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ToImGui(InBackground).Value);
	ImGui::PushStyleColor(ImGuiCol_Border, ToImGui(InBorder).Value);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
	// 반환값과 상관없이 EndChild 를 불러야 한다(ImGui 규칙). 휠은 바깥 창이 스크롤한다.
	ImGui::BeginChild(InName.GetCStr(), ImVec2(InSize.x, (InSize.y > 0.0f) ? InSize.y : 0.0f), ChildFlags, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);
}

void HGUI::EndPanel()
{
	ImGui::EndChild();
}

void HGUI::SegmentedBar(const HList<HGUIBarSegment>& InSegments, const HVector2& InSize, const HLinearColor& InTrack)
{
	const float32 Width      = (InSize.x > 0.0f) ? InSize.x : ImGui::GetContentRegionAvail().x;
	const float32 BarHeight  = (InSize.y > 0.0f) ? InSize.y : 8.0f;
	const float32 LineHeight = ImMax(BarHeight, ImGui::GetTextLineHeight());
	if (Width <= 0.0f)
	{
		return;
	}

	const ImVec2  Cursor   = ImGui::GetCursorScreenPos();
	const float32 Top      = Cursor.y + (LineHeight - BarHeight) * 0.5f;
	const float32 Bottom   = Top + BarHeight;
	const float32 Right    = Cursor.x + Width;
	const float32 Rounding = ImMin(4.0f, BarHeight * 0.5f);
	const float32 Gap      = 2.0f;     // 구간은 선 대신 틈으로 나눈다
	ImDrawList*   DrawList = ImGui::GetWindowDrawList();

	float32 Position = Cursor.x;
	bool    bFirst   = true;
	for (const HGUIBarSegment& Segment : InSegments)
	{
		float32 SegmentWidth = Width * ImClamp(Segment.Fraction, 0.0f, 1.0f);
		if (SegmentWidth <= 0.0f)
		{
			continue;
		}

		// 아주 작은 구간도 보이게 틈을 뺀 뒤 2px 는 남긴다.
		SegmentWidth = ImMax(SegmentWidth, bFirst ? 2.0f : Gap + 2.0f);
		const float32 PieceLeft  = bFirst ? Position : Position + Gap;
		const float32 PieceRight = ImMin(Position + SegmentWidth, Right);
		if (PieceRight > PieceLeft)
		{
			ImDrawFlags Corners = 0;
			if (bFirst)
			{
				Corners |= ImDrawFlags_RoundCornersLeft;
			}
			if (PieceRight >= Right - 0.5f)
			{
				Corners |= ImDrawFlags_RoundCornersRight;
			}
			DrawList->AddRectFilled(ImVec2(PieceLeft, Top), ImVec2(PieceRight, Bottom), ToImGui(Segment.Color), Rounding, (Corners != 0) ? Corners : ImDrawFlags_RoundCornersNone);
		}
		Position = PieceRight;
		bFirst   = false;
	}

	// 남은 칸
	const float32 TrackLeft = bFirst ? Position : Position + Gap;
	if (Right - TrackLeft > 0.5f)
	{
		const ImDrawFlags Corners = bFirst ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight;
		DrawList->AddRectFilled(ImVec2(TrackLeft, Top), ImVec2(Right, Bottom), ToImGui(InTrack), Rounding, Corners);
	}

	ImGui::Dummy(ImVec2(Width, LineHeight));
}

void HGUI::Badge(const PString& InText, const HLinearColor& InBackground, const HLinearColor& InTextColor)
{
	// 글자 줄 높이를 그대로 써서 표의 행 높이를 바꾸지 않는다. 배경만 위아래로 1px 넓힌다.
	const ImVec2  TextSize = ImGui::CalcTextSize(InText.GetCStr());
	const float32 PaddingX = 5.0f;
	const ImVec2  Min      = ImGui::GetCursorScreenPos();
	const ImVec2  Max(Min.x + TextSize.x + PaddingX * 2.0f, Min.y + TextSize.y);
	ImDrawList*   DrawList = ImGui::GetWindowDrawList();
	DrawList->AddRectFilled(ImVec2(Min.x, Min.y - 1.0f), ImVec2(Max.x, Max.y + 1.0f), ToImGui(InBackground), 3.0f);
	DrawList->AddText(ImVec2(Min.x + PaddingX, Min.y), ToImGui(InTextColor), InText.GetCStr());
	ImGui::Dummy(ImVec2(Max.x - Min.x, Max.y - Min.y));
}

void HGUI::LegendItem(const PString& InLabel, const HLinearColor& InColor)
{
	const float32 LineHeight = ImGui::GetTextLineHeight();
	const float32 SwatchSize = 8.0f;
	const ImVec2  Cursor     = ImGui::GetCursorScreenPos();
	const ImVec2  SwatchMin(Cursor.x, Cursor.y + (LineHeight - SwatchSize) * 0.5f);
	ImGui::GetWindowDrawList()->AddRectFilled(SwatchMin, ImVec2(SwatchMin.x + SwatchSize, SwatchMin.y + SwatchSize), ToImGui(InColor), 2.0f);
	ImGui::Dummy(ImVec2(SwatchSize, LineHeight));
	ImGui::SameLine(0.0f, 5.0f);
	ImGui::TextUnformatted(InLabel.GetCStr());
}

bool HGUI::BeginTable(const PString& InName, int32 InColumnCount)
{
	// 칸 여백은 행마다 다시 읽으므로 EndTable 까지 유지한다.
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, 4.0f));
	const ImGuiTableFlags Flags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;
	if (ImGui::BeginTable(InName.GetCStr(), InColumnCount, Flags) == false)
	{
		ImGui::PopStyleVar();
		return false;
	}
	return true;
}

void HGUI::TableSetupColumn(const PString& InLabel, float32 InWidth, bool bAlignRight)
{
	const ImGuiTableColumnFlags Flags = (InWidth > 0.0f) ? ImGuiTableColumnFlags_WidthFixed : ImGuiTableColumnFlags_WidthStretch;
	// 정렬은 열의 UserID 에 담아 TableHeadersRow 가 읽는다 (ImGui 머리글은 왼쪽 정렬만 한다).
	ImGui::TableSetupColumn(InLabel.GetCStr(), Flags, InWidth, bAlignRight ? 1u : 0u);
}

void HGUI::TableHeadersRow()
{
	ImGuiTable* Table = ImGui::GetCurrentTable();
	if (Table == nullptr)
	{
		return;
	}

	ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
	for (int32 ColumnIndex = 0; ColumnIndex < Table->ColumnsCount; ++ColumnIndex)
	{
		if (ImGui::TableSetColumnIndex(ColumnIndex) == false)
		{
			continue;
		}

		const PString Name = ImGui::TableGetColumnName(ColumnIndex);
		if (Table->Columns[ColumnIndex].UserID == 1u)
		{
			TextRight(Name);
		}
		else
		{
			ImGui::TextUnformatted(Name.GetCStr());
		}
	}
}

void HGUI::TableNextRow()
{
	ImGui::TableNextRow();
}

bool HGUI::TableNextColumn()
{
	return ImGui::TableNextColumn();
}

void HGUI::EndTable()
{
	ImGui::EndTable();
	ImGui::PopStyleVar();
}

bool HGUI::BeginTabBar(const PString& InName)
{
	// 선택된 탭 위에 강조선(TabSelectedOverline 색)을 긋는다.
	return ImGui::BeginTabBar(InName.GetCStr(), ImGuiTabBarFlags_DrawSelectedOverline);
}

void HGUI::EndTabBar()
{
	ImGui::EndTabBar();
}

bool HGUI::BeginTabItem(const PString& InLabel)
{
	return ImGui::BeginTabItem(InLabel.GetCStr());
}

void HGUI::EndTabItem()
{
	ImGui::EndTabItem();
}

void HGUI::PlotLines(const HPlotLinesArguments& InArgs)
{
	const int32 Count       = static_cast<int32>(InArgs.XValues.size());
	const int32 SeriesCount = static_cast<int32>(InArgs.SeriesLabels.size());
	if (SeriesCount == 0 || InArgs.Datas.size() < static_cast<size_t>(Count) * static_cast<size_t>(SeriesCount))
	{
		return;
	}

	float64 MaxValue = 0.0;
	for (float64 Value : InArgs.Datas)
	{
		MaxValue = ImMax(MaxValue, Value);
	}

	HList<ImVec4> SeriesColors;
	for (int32 SeriesIndex = 0; SeriesIndex < SeriesCount; ++SeriesIndex)
	{
		const bool bHasColor = SeriesIndex < static_cast<int32>(InArgs.SeriesColors.size());
		SeriesColors.push_back(bHasColor ? ToImGui(InArgs.SeriesColors[SeriesIndex]).Value : ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// 그래프 바탕 · 테두리 · 눈금 표시는 지우고 격자와 글자만 남긴다(뒤 표면이 비친다).
	const ImVec4 Transparent(0.0f, 0.0f, 0.0f, 0.0f);
	const ImVec4 AxisText = ToImGui(InArgs.AxisTextColor).Value;
	ImPlot::PushStyleColor(ImPlotCol_PlotBg, Transparent);
	ImPlot::PushStyleColor(ImPlotCol_PlotBorder, Transparent);
	ImPlot::PushStyleColor(ImPlotCol_LegendBg, Transparent);
	ImPlot::PushStyleColor(ImPlotCol_LegendBorder, Transparent);
	ImPlot::PushStyleColor(ImPlotCol_LegendText, AxisText);
	ImPlot::PushStyleColor(ImPlotCol_AxisText, AxisText);
	ImPlot::PushStyleColor(ImPlotCol_AxisGrid, ToImGui(InArgs.GridColor).Value);
	ImPlot::PushStyleColor(ImPlotCol_AxisTick, Transparent);
	ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(4.0f, 6.0f));
	ImPlot::PushStyleVar(ImPlotStyleVar_LegendSpacing, ImVec2(14.0f, 0.0f));

	const PString PlotID = PString("##") + InArgs.TitleName;
	ImPlotFlags PlotFlags = ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotFlags_NoFrame;
	if (InArgs.bShowLegend == false)
	{
		PlotFlags |= ImPlotFlags_NoLegend;
	}
	if (ImPlot::BeginPlot(PlotID.GetCStr(), ImVec2(InArgs.PlotSize.x, InArgs.PlotSize.y), PlotFlags))
	{
		// 범위는 매 프레임 고정한다. 끌기 · 확대는 막는다.
		const ImPlotAxisFlags AxisFlags = ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_Lock;
		ImPlot::SetupAxes(nullptr, nullptr, AxisFlags, AxisFlags);
		ImPlot::SetupAxisLimits(ImAxis_X1, InArgs.XMin, InArgs.XMax, ImPlotCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, (MaxValue > 0.0) ? MaxValue * 1.25 : 1.0, ImPlotCond_Always);
		ImPlot::SetupAxisFormat(ImAxis_X1, InArgs.XAxisFormat.GetCStr());
		ImPlot::SetupAxisFormat(ImAxis_Y1, InArgs.YAxisFormat.GetCStr());
		// 범례는 그래프 영역 밖(위)에 둔다. 안에 두면 선이 범례 높이를 지날 때 글자와 겹친다.
		ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside);

		for (int32 SeriesIndex = 0; SeriesIndex < SeriesCount; ++SeriesIndex)
		{
			const char*    Label     = InArgs.SeriesLabels[SeriesIndex].GetCStr();
			const float64* Values    = InArgs.Datas.data() + static_cast<size_t>(SeriesIndex) * static_cast<size_t>(Count);
			const bool     bHasFill  = SeriesIndex < static_cast<int32>(InArgs.SeriesFillAlphas.size());
			const float32  FillAlpha = bHasFill ? InArgs.SeriesFillAlphas[SeriesIndex] : 0.0f;
			if (FillAlpha > 0.0f)
			{
				ImPlot::SetNextFillStyle(SeriesColors[SeriesIndex], FillAlpha);
				ImPlot::PlotShaded(Label, InArgs.XValues.data(), Values, Count, 0.0);
			}
			ImPlot::SetNextLineStyle(SeriesColors[SeriesIndex], 2.0f);
			ImPlot::PlotLine(Label, InArgs.XValues.data(), Values, Count);
		}

		// 마우스 위치: 가장 가까운 표본에 세로선 · 표식을 긋고 모든 계열 값을 풍선 도움말로 보여 준다.
		if (Count > 0 && ImPlot::IsPlotHovered())
		{
			const float64 MouseX  = ImPlot::GetPlotMousePos().x;
			int32         Nearest = 0;
			for (int32 Index = 1; Index < Count; ++Index)
			{
				if (ImAbs(InArgs.XValues[Index] - MouseX) < ImAbs(InArgs.XValues[Nearest] - MouseX))
				{
					Nearest = Index;
				}
			}

			const float64 NearestX = InArgs.XValues[Nearest];
			ImPlot::SetNextLineStyle(AxisText, 1.0f);
			ImPlot::PlotInfLines("##hover_line", &NearestX, 1);
			for (int32 SeriesIndex = 0; SeriesIndex < SeriesCount; ++SeriesIndex)
			{
				const float64* Values = InArgs.Datas.data() + static_cast<size_t>(SeriesIndex) * static_cast<size_t>(Count);
				ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4.0f, SeriesColors[SeriesIndex], 2.0f, ToImGui(InArgs.SurfaceColor).Value);
				ImPlot::PlotScatter(PString::Format("##hover_point%d", SeriesIndex).GetCStr(), &NearestX, &Values[Nearest], 1);
			}

			float32 LabelWidth = 0.0f;
			for (const PString& SeriesLabel : InArgs.SeriesLabels)
			{
				LabelWidth = ImMax(LabelWidth, ImGui::CalcTextSize(SeriesLabel.GetCStr()).x);
			}

			ImGui::BeginTooltip();
			ImGui::TextUnformatted(PString::Format(InArgs.XAxisFormat, NearestX).GetCStr());
			for (int32 SeriesIndex = 0; SeriesIndex < SeriesCount; ++SeriesIndex)
			{
				const float64* Values = InArgs.Datas.data() + static_cast<size_t>(SeriesIndex) * static_cast<size_t>(Count);
				const bool bHasColor  = SeriesIndex < static_cast<int32>(InArgs.SeriesColors.size());
				LegendItem(InArgs.SeriesLabels[SeriesIndex], bHasColor ? InArgs.SeriesColors[SeriesIndex] : HLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
				// 값 열: 여백 + 색 사각형 + 간격 + 가장 긴 이름 + 16px
				ImGui::SameLine(ImGui::GetStyle().WindowPadding.x + 8.0f + 5.0f + LabelWidth + 16.0f);
				ImGui::TextUnformatted(PString::Format(InArgs.ValueFormat, Values[Nearest]).GetCStr());
			}
			ImGui::EndTooltip();
		}

		ImPlot::EndPlot();
	}

	ImPlot::PopStyleVar(2);
	ImPlot::PopStyleColor(8);
}

bool HGUI::IsWindowFocused()
{
	return ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
}

namespace
{
	// EGUIKey 순서와 같다.
	const ImGuiKey GUIKeyToImGuiKey[(int32)EGUIKey::Count] =
	{
		ImGuiKey_Escape,
		ImGuiKey_Backspace,
		ImGuiKey_Delete,
		ImGuiKey_Enter,
		ImGuiKey_KeypadEnter,
		ImGuiKey_Tab,
		ImGuiKey_LeftArrow,
		ImGuiKey_RightArrow,
		ImGuiKey_UpArrow,
		ImGuiKey_DownArrow,
		ImGuiKey_Home,
		ImGuiKey_End,
		ImGuiKey_PageUp,
		ImGuiKey_PageDown,
		ImGuiKey_Space,
		ImGuiKey_A,
		ImGuiKey_C,
		ImGuiKey_V,
		ImGuiKey_X,
	};

	// ClaimTextEditKeys 가 내비게이션에서 가져오는 키
	const ImGuiKey TextEditKeys[] =
	{
		ImGuiKey_LeftArrow,
		ImGuiKey_RightArrow,
		ImGuiKey_UpArrow,
		ImGuiKey_DownArrow,
		ImGuiKey_Home,
		ImGuiKey_End,
		ImGuiKey_PageUp,
		ImGuiKey_PageDown,
		ImGuiKey_Enter,
		ImGuiKey_KeypadEnter,
		ImGuiKey_Tab,
		ImGuiKey_Space,
		ImGuiKey_Backspace,
		ImGuiKey_Delete,
		ImGuiKey_Escape,
	};
}

bool HGUI::IsKeyPressed(EGUIKey InKey, bool bRepeat)
{
	if ((int32)InKey < 0 || (int32)InKey >= (int32)EGUIKey::Count)
	{
		return false;
	}

	const ImGuiKey Key = GUIKeyToImGuiKey[(int32)InKey];
	if (Key == ImGuiKey_None)
	{
		return false;
	}
	return ImGui::IsKeyPressed(Key, bRepeat);
}

bool HGUI::IsCtrlDown()
{
	return ImGui::GetIO().KeyCtrl;
}

bool HGUI::IsShiftDown()
{
	return ImGui::GetIO().KeyShift;
}

bool HGUI::IsAltDown()
{
	return ImGui::GetIO().KeyAlt;
}

void HGUI::GetInputCharacters(HList<uint32>& OutCodepoints)
{
	OutCodepoints.clear();
	const ImGuiIO& IO = ImGui::GetIO();
	for (int32 Index = 0; Index < IO.InputQueueCharacters.Size; ++Index)
	{
		OutCodepoints.push_back((uint32)IO.InputQueueCharacters[Index]);
	}
}

void HGUI::ClaimTextEditKeys()
{
	// 내비게이션은 주인 없는 키만 읽는다(ImGuiKeyOwner_NoOwner). 주인은 다음 프레임 시작까지 남는다.
	const ImGuiID Owner = ImGui::GetID("##JGTextEditKeys");
	for (ImGuiKey Key : TextEditKeys)
	{
		ImGui::SetKeyOwner(Key, Owner);
	}
}

void HGUI::SetTextInputPosition(const HVector2& InScreenPosition, float32 InLineHeight)
{
	// ImGui 입력란과 같은 길: 이번 프레임 IME 자리를 정하면 EndFrame 이 바뀐 때만 OS 에 알린다(PlatformSetImeDataFn).
	ImGuiContext& Context = *ImGui::GetCurrentContext();
	Context.PlatformImeData.WantVisible     = true;
	Context.PlatformImeData.InputPos        = ImVec2(InScreenPosition.x, InScreenPosition.y);
	Context.PlatformImeData.InputLineHeight = InLineHeight;
	Context.PlatformImeViewport             = ImGui::GetWindowViewport()->ID;
}

PString HGUI::GetClipboardText()
{
	const char* Text = ImGui::GetClipboardText();
	return (Text != nullptr) ? PString(Text) : PString();
}

void HGUI::SetClipboardText(const PString& InText)
{
	ImGui::SetClipboardText(InText.GetCStr());
}

namespace
{
	float32 srgbChannelToLinear(uint32 InChannel)
	{
		const float32 Value = static_cast<float32>(InChannel) / 255.0f;
		if (Value <= 0.04045f)
		{
			return Value / 12.92f;
		}
		return powf((Value + 0.055f) / 1.055f, 2.4f);
	}
}

HLinearColor HGUI::DisplayColor(uint32 InRGB, float32 InAlpha)
{
	return HLinearColor(srgbChannelToLinear((InRGB >> 16) & 0xFF), srgbChannelToLinear((InRGB >> 8) & 0xFF), srgbChannelToLinear(InRGB & 0xFF), InAlpha);
}

void HGUI::PushStyleVar(EGUIStyleVar InTarget, float32 InValue)
{
	ImGui::PushStyleVar(ToImGui(InTarget), InValue);
}

void HGUI::PushStyleVar(EGUIStyleVar InTarget, const HVector2& InValue)
{
	ImGui::PushStyleVar(ToImGui(InTarget), ImVec2(InValue.x, InValue.y));
}

void HGUI::PopStyleVar(int32 InCount)
{
	ImGui::PopStyleVar(InCount);
}

float32 HGUI::GetFrameHeight()
{
	return ImGui::GetFrameHeight();
}

float32 HGUI::CalcTextWidth(const PString& InText)
{
	return ImGui::CalcTextSize(InText.GetCStr()).x;
}

void HGUI::AlignTextToFramePadding()
{
	ImGui::AlignTextToFramePadding();
}

bool HGUI::Checkbox(const PString& InName, bool& InOutValue, bool bMixed)
{
	if (bMixed == false)
	{
		return Checkbox(InName, InOutValue);
	}

	// 일부만 켜진 상태는 ImGui 가 가로줄로 그린다. 누르면 InOutValue 가 뒤집힌다(호출 쪽은 false 를 넘기므로 "전부 켜기"가 된다).
	ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
	const bool bPressed = ImGui::Checkbox(InName.GetCStr(), &InOutValue);
	ImGui::PopItemFlag();
	return bPressed;
}

bool HGUI::ToggleChip(const PString& InId, const PString& InLabel, const PString& InTrailing, const HLinearColor& InAccent, bool& InOutOn)
{
	ImGuiWindow* Window = ImGui::GetCurrentWindow();
	if (Window->SkipItems)
	{
		return false;
	}

	const ImGuiStyle& Style        = ImGui::GetStyle();
	const ImGuiID     Id           = Window->GetID(InId.GetCStr());
	const float32     Height       = ImGui::GetFrameHeight();
	const float32     DotSize      = 6.0f;
	const float32     PaddingX     = 9.0f;
	const float32     Gap          = 6.0f;
	const ImVec2      LabelSize    = ImGui::CalcTextSize(InLabel.GetCStr());
	const ImVec2      TrailingSize = InTrailing.Empty() ? ImVec2(0.0f, 0.0f) : ImGui::CalcTextSize(InTrailing.GetCStr());
	const float32     Width        = PaddingX + DotSize + Gap + LabelSize.x + (InTrailing.Empty() ? 0.0f : Gap + TrailingSize.x) + PaddingX;

	const ImVec2 Min = Window->DC.CursorPos;
	const ImRect Bounds(Min, ImVec2(Min.x + Width, Min.y + Height));
	ImGui::ItemSize(Bounds, Style.FramePadding.y);
	if (ImGui::ItemAdd(Bounds, Id) == false)
	{
		return false;
	}

	bool bHovered = false;
	bool bHeld    = false;
	const bool bPressed = ImGui::ButtonBehavior(Bounds, Id, &bHovered, &bHeld);
	if (bPressed)
	{
		InOutOn = !InOutOn;
		ImGui::MarkItemEdited(Id);
	}

	// 켜짐: 강조색으로 옅게 물든 바탕 + 강조색 테두리 · 점. 꺼짐: 바탕 없이 흐린 글자 · 점(마우스를 올리면 바탕만).
	const ImVec4 Accent      = ToImGui(InAccent).Value;
	ImU32        Background  = 0;
	ImU32        BorderColor = ImGui::GetColorU32(ImGuiCol_Border);
	if (InOutOn)
	{
		// 알파는 선형 공간에서 섞이고 sRGB 로 나가 눈에는 더 진하게 보인다 → 낮게 둔다.
		Background  = ImGui::GetColorU32(ImVec4(Accent.x, Accent.y, Accent.z, bHovered ? 0.16f : 0.10f));
		BorderColor = ImGui::GetColorU32(ImVec4(Accent.x, Accent.y, Accent.z, 0.65f));
	}
	else if (bHovered)
	{
		Background = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
	}

	ImDrawList*   DrawList = Window->DrawList;
	const float32 Rounding = Height * 0.5f;
	if (Background != 0)
	{
		DrawList->AddRectFilled(Bounds.Min, Bounds.Max, Background, Rounding);
	}
	DrawList->AddRect(Bounds.Min, Bounds.Max, BorderColor, Rounding, 0, 1.0f);

	const ImVec2 DotCenter(Bounds.Min.x + PaddingX + DotSize * 0.5f, Bounds.Min.y + Height * 0.5f);
	DrawList->AddCircleFilled(DotCenter, DotSize * 0.5f, InOutOn ? ImGui::GetColorU32(Accent) : ImGui::GetColorU32(ImGuiCol_TextDisabled));

	const float32 TextY = Bounds.Min.y + Style.FramePadding.y;
	float32       TextX = Bounds.Min.x + PaddingX + DotSize + Gap;
	DrawList->AddText(ImVec2(TextX, TextY), ImGui::GetColorU32(InOutOn ? ImGuiCol_Text : ImGuiCol_TextDisabled), InLabel.GetCStr());
	if (InTrailing.Empty() == false)
	{
		TextX += LabelSize.x + Gap;
		DrawList->AddText(ImVec2(TextX, TextY), ImGui::GetColorU32(ImGuiCol_TextDisabled), InTrailing.GetCStr());
	}

	return bPressed;
}

bool HGUI::BeginCombo(const PString& InName, const PString& InPreview, float32 InWidth)
{
	const PString ComboKey = PString("##") + InName;
	if (InWidth != 0.0f)
	{
		ImGui::SetNextItemWidth(InWidth);
	}
	return ImGui::BeginCombo(ComboKey.GetCStr(), InPreview.GetCStr(), ImGuiComboFlags_HeightLarge);
}

void HGUI::EndCombo()
{
	ImGui::EndCombo();
}

void HGUI::HighlightLine(const HLinearColor& InBackground, const HLinearColor& InAccent, float32 InAccentWidth)
{
	ImGuiWindow* Window = ImGui::GetCurrentWindow();
	if (Window->SkipItems)
	{
		return;
	}

	// 줄 간격의 절반씩 위아래로 넓혀 이어진 줄이 한 띠로 보이게 한다. 폭은 보이는 안쪽 영역 전부(가로 스크롤과 상관없이).
	const float32 HalfGap = ImGui::GetStyle().ItemSpacing.y * 0.5f;
	const float32 Top     = Window->DC.CursorPos.y - HalfGap;
	const float32 Bottom  = Window->DC.CursorPos.y + ImGui::GetTextLineHeight() + HalfGap;
	if (Bottom < Window->ClipRect.Min.y || Top > Window->ClipRect.Max.y)
	{
		return;
	}

	const float32 Left  = Window->InnerClipRect.Min.x;
	const float32 Right = Window->InnerClipRect.Max.x;
	Window->DrawList->AddRectFilled(ImVec2(Left, Top), ImVec2(Right, Bottom), ToImGui(InBackground));
	if (InAccentWidth > 0.0f)
	{
		Window->DrawList->AddRectFilled(ImVec2(Left, Top), ImVec2(Left + InAccentWidth, Bottom), ToImGui(InAccent));
	}
}


