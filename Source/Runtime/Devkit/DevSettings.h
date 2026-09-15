#pragma once
#include "DevkitDefines.h"
#include "WidgetComponent.h"
#include "DevSettings.generation.h"

/*
DevFeature

DevFeature은 기본적으로 아래 기능을 가지고 있음.

Scene => DataClass와 Widget 나누기
Setting => DataClass와 Widget 나누기
Log 출력 (DevConsole 기능도) => LogData와 Widget 나누기
*/
class JGTexture;

JGCLASS()
class DEVKIT_API JGDevSettings : public JGWidgetComponent
{
	JG_GENERATED_WIDGETCOMPONENT_BODY


public:
	virtual void OnInitialize() override;
	virtual void OnGenerateGUI() override;
};