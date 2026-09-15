#pragma once
#include "DevkitDefines.h"
#include "WidgetComponent.h"
#include "DevScene.generation.h"

/*
DevFeature

DevFeature은 기본적으로 아래 기능을 가지고 있음.

Scene => DataClass와 Widget 나누기
Setting => DataClass와 Widget 나누기
Log 출력 (DevConsole 기능도) => LogData와 Widget 나누기
*/
class IRawTexture;

JGCLASS()
class DEVKIT_API JGDevScene : public JGWidgetComponent
{
	JG_GENERATED_WIDGETCOMPONENT_BODY

private:
	PSharedPtr<IRawTexture> SceneTexture;
	HVector2 SceneSize;
public:
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnLayout(const HWidgetComponentLayout& InLayout) override;
	virtual void OnGenerateGUI() override;
};