#pragma once
#include "GUIDefines.h"


struct HPlotBarGroupsArguments
{
	PString  TitleName;
	HVector2 PlotSize = HVector2(-1, -1);;
	HList<PString>   GroupLabels;
	HList<PString>   DataLabels;
	HList<float64> Datas;
};


class GUI_API HGUI
{
public:
	static void SameLine();
	static void NextLine();
	static void Text(const PString& InStr);
	static void Text(const PString& InStr, const HLinearColor& InColor);
	static bool InputText(const PString& InName, PString& OutStr);
	static void PlotBarGroups(const HPlotBarGroupsArguments& InArgs);
	static void PlotTest();
	static bool Selectable(const PString& InName, bool bSelected, ESelectableFlags InFlags = ESelectableFlags::None, const HVector2& InSize = HVector2(0, 0));
	static void Image(uint64 InTextureID, const HVector2& InSize);

	// 스크롤 영역. InSize 의 0 은 남은 공간 전부, 음수는 남은 공간에서 그만큼 뺀 크기다(ImGui 규칙). EndChild 는 항상 부른다.
	static void BeginChild(const PString& InName, const HVector2& InSize);
	// bStickToBottom: 스크롤이 맨 아래였으면 새 줄이 생겨도 맨 아래를 유지한다(로그 뷰).
	static void EndChild(bool bStickToBottom = false);
	// 한 줄 높이 + 줄 간격. BeginChild 의 음수 높이로 아래에 입력줄 자리를 남길 때 쓴다.
	static float32 GetFrameHeightWithSpacing();
	// 한 줄 입력(가로 전체). Enter 면 true. 창이 처음 뜰 때와 Enter 뒤에 포커스를 둔다.
	// ↑/↓ 로 InHistory 를 오간다. InOutHistoryPos 는 호출 쪽이 보관한다(-1 = 새 줄, 0.. = 오래된 것부터).
	static bool InputTextWithHistory(const PString& InName, PString& InOutStr, const HList<PString>& InHistory, int32& InOutHistoryPos);
private:

};