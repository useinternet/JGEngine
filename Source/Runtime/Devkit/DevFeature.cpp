#include "PCH/PCH.h"
#include "DevFeature.h"
#include "DevScene.h"
#include "DevSettings.h"
#include "GUI.h"

PSharedPtr<JGClass> JGDevFeature::GetDevSceneClass() const
{
	return StaticClass<JGDevScene>();
}

PSharedPtr<JGClass> JGDevFeature::GetDevSettingsClass() const
{
	return StaticClass<JGDevSettings>();
}

PString JGDevFeature::GetTitleName() const
{
	return "DevFeature";
}

void JGDevFeature::OnInitialize()
{
	DevScene    = MakeWidgetComponent(GetDevSceneClass());
	DevSettings = MakeWidgetComponent(GetDevSettingsClass());

}

void JGDevFeature::OnGenerateGUI()
{
	if (DevScene.IsValid())
	{
		DevScene->GenerateGUI();
	}
	HGUI::SameLine();
	if (DevSettings.IsValid())
	{
		DevSettings->GenerateGUI();
	}
}

void JGDevFeature::OnLayout(const HWidgetLayout& InLayout)
{
	const HVector2& WidgetContentSize = InLayout.ContentSize;
	if (DevScene.IsValid())
	{
		HWidgetComponentLayout Layout;
		Layout.ContentSize = HVector2(800, 600);
		DevScene->SetupLayout(Layout);
	}

	if (DevSettings.IsValid())
	{
		HWidgetComponentLayout Layout;
		Layout.ContentSize = HVector2(250, WidgetContentSize.y);
		DevSettings->SetupLayout(Layout);
	}

}

void JGDevFeature::OnShutdown()
{
}
