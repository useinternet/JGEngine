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

void HGUI::Text(const PString& InStr)
{
	ImGui::Text(InStr.GetCStr());
}

void HGUI::Text(const PString& InStr, const HLinearColor& InColor)
{
	ImGui::TextColored(ToImGui(InColor), InStr.GetCStr());
}

bool HGUI::InputText(const PString& InName, PString& OutStr)
{
	char Buf[512] = { 0, };
	memcpy_s(Buf, 512, OutStr.GetCStr(), OutStr.Length());
	ImGui::AlignTextToFramePadding();
	ImGui::Text(InName.GetCStr()); ImGui::SameLine();
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


