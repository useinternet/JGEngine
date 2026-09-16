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
class IRawMaterial;
class IJGGraphicsCommand;

JGCLASS()
class DEVKIT_API JGDevScene : public JGWidgetComponent
{
	JG_GENERATED_WIDGETCOMPONENT_BODY

private:
	PSharedPtr<IRawTexture>        SceneTexture;
	PSharedPtr<IRawMaterial>       TestMaterial;      // 첫 드로우 검증용 Scene 도메인 풀스크린 머터리얼
	PSharedPtr<IJGGraphicsCommand> GraphicsCommand;   // 프레임마다 재사용 (GetGraphicsCommand는 호출마다 새 객체를 만든다)
	HVector2 SceneSize;
public:
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnLayout(const HWidgetComponentLayout& InLayout) override;
	virtual void OnGenerateGUI() override;

private:
	// SceneTexture를 지우고 TestMaterial로 풀스크린을 그린다. 매 프레임 OnGenerateGUI에서 호출.
	void RenderScene();
};