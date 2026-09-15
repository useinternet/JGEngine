#pragma once
#include "DevkitDefines.h"
#include "Widget.h"
#include "DevFeature.generation.h"


class JGDevScene;
class JGDevSettings;


JGCLASS()
class DEVKIT_API JGDevFeature : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

private:
	PSharedPtr<JGWidgetComponent> DevScene;
	PSharedPtr<JGWidgetComponent> DevSettings;

protected:
	virtual PSharedPtr<JGClass> GetDevSceneClass() const;
	virtual PSharedPtr<JGClass> GetDevSettingsClass() const;

	virtual PString  GetTitleName() const override;
private:
	virtual void OnInitialize() override;
	virtual void OnGenerateGUI() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnShutdown() override;
};