#include "PCH/PCH.h"
#include "GUI.h"
#include "Imgui/imgui.h"
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
		std::string*          Buffer;
		const HList<PString>* History;
		int32*                HistoryPos;
	};

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

		if (InData->EventFlag != ImGuiInputTextFlags_CallbackHistory)
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

bool HGUI::InputTextWithHistory(const PString& InName, PString& InOutStr, const HList<PString>& InHistory, int32& InOutHistoryPos)
{
	std::string Buffer = InOutStr.GetRawString();
	HInputTextHistoryContext Context{ &Buffer, &InHistory, &InOutHistoryPos };

	// 창이 처음 뜰 때 입력줄에 포커스를 둔다.
	if (ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere();
	}

	const PString InputTextKey = PString("##") + InName;
	const ImGuiInputTextFlags Flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackResize;

	ImGui::PushItemWidth(-1.0f);
	const bool bEntered = ImGui::InputText(InputTextKey.GetCStr(), Buffer.data(), Buffer.capacity() + 1, Flags, &inputTextHistoryCallback, &Context);
	ImGui::PopItemWidth();

	// ImGui 는 Enter 에서 입력줄의 포커스를 푼다(ConfigInputTextEnterKeepActive = false). 연속 입력을 위해 다시 잡는다.
	if (bEntered)
	{
		ImGui::SetKeyboardFocusHere(-1);
	}

	InOutStr = Buffer.c_str();
	return bEntered;
}


