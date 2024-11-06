#pragma once
#include "WidgetComponent.h"

JGCLASS()
class JGMemoryStatisticsCategory : public JGWidgetComponent
{
	JG_GENERATED_WIDGETCOMPONENT_BODY


protected:
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnGenerateGUI() override;
};