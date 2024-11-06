#include "PCH/PCH.h"
#include "MemoryStatistics.h"
#include "Imgui/imgui.h"

#include "MemoryStatistics/MemoryStatisticsCategory.h"
#include "MemoryStatistics/MemoryStatisticsContent.h"

void JGMemoryStatistcs::OnInitialize()
{
	Category = MakeWidgetComponent<JGMemoryStatisticsCategory>();
	Content  = MakeWidgetComponent<JGMemoryStatisticsContent>();
}

void JGMemoryStatistcs::OnGenerateGUI()
{


	Category->GenerateGUI();
	Content->GenerateGUI();
}

PString JGMemoryStatistcs::GetTitleName() const
{
	return "MemoryStatistcs";
}

const HGuid& JGMemoryStatistcs::GetGUID() const
{
	return GetStaticGUID();
}
