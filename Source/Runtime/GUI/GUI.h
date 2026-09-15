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
private:

};