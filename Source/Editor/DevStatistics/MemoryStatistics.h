#pragma once
#include "Widget.h"
#include "WidgetComponent.h"
#include "MemoryStatistics.generation.h"


JGCLASS()
class JGMemoryStatistcs : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

public:
	virtual void OnInitialize() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnGenerateGUI() override;


	virtual PString  GetTitleName() const override;
	virtual const  HGuid& GetGUID() const override;
};





