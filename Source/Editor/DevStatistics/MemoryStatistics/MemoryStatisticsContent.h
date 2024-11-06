#pragma once
#include "WidgetComponent.h"

JGCLASS()
class JGMemoryStatisticsContent : public JGWidgetComponent
{
	JG_GENERATED_WIDGETCOMPONENT_BODY


protected:
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnGenerateGUI() override;
};