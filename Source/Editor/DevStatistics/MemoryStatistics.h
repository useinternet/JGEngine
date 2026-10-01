#pragma once
#include "Widget.h"
#include "WidgetComponent.h"
#include "MemoryStatistics.generation.h"


JGCLASS()
class JGMemoryStatistcs : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

	// 풀 추이 (단위 MB). 창이 열려 있는 동안만 쌓고, 가로 범위보다 오래된 표본은 버린다.
	HList<float64> HistoryTimes;
	HList<float64> HistoryCommittedMB;
	HList<float64> HistoryAllocatedMB;
	float64        LastSampleTime = -1.0;

public:
	virtual void OnInitialize() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnGenerateGUI() override;


	virtual PString  GetTitleName() const override;
	virtual const  HGuid& GetGUID() const override;
};





