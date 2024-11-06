#include "PCH/PCH.h"
#include "WidgetComponent.h"
#include "Widget.h"
#include "Imgui/imgui.h"

PString JGWidget::GetTitleName() const
{
	return PString();
}

const HGuid& JGWidget::GetGUID() const
{
	static HGuid NullGUID;
	return NullGUID;
}

void JGWidget::GenerateGUI()
{
	if (IsOpen() == false)
	{
		return;
	}

	bool bLocalOpen = bOpen;

	ImGui::PushID((int32)GetGUID().GetHashCode());
	ImGui::Begin(GetTitleName().GetCStr(), &bLocalOpen);
	OnGenerateGUI();
	ImGui::End();
	ImGui::PopID();

	if (bLocalOpen == false)
	{
		Close();
	}
}

void JGWidget::Open()
{
	if (IsOpen())
	{
		return;
	}

	bOpen = true;
	OnOpen();
	for (PSharedPtr<JGWidgetComponent> WidgetComp : WidgetComponents)
	{
		WidgetComp->OnOpen();
	}
}

void JGWidget::Close()
{
	if (IsOpen() == false)
	{
		return;
	}

	bOpen = false;
	OnClose();
	for (PSharedPtr<JGWidgetComponent> WidgetComp : WidgetComponents)
	{
		WidgetComp->OnClose();
	}
}
void JGWidget::Initialize()
{
	OnInitialize();
	for (PSharedPtr<JGWidgetComponent> WidgetComp : WidgetComponents)
	{
		WidgetComp->Initialize();
	}
}
void JGWidget::Shutdown()
{
	for (PSharedPtr<JGWidgetComponent> WidgetComp : WidgetComponents)
	{
		WidgetComp->Shutdown();
	}
	OnShutdown();

	WidgetComponents.clear();
}