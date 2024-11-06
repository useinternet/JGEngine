#include "PCH/PCH.h"
#include "WidgetComponent.h"
#include "Imgui/imgui.h"

void JGWidgetComponent::GenerateGUI()
{
	if (ImGui::BeginChild((int)GetGUID().GetHashCode()))
	{
		OnGenerateGUI();
		ImGui::EndChild();
	}
}

void JGWidgetComponent::Initialize()
{
	OnInitialize();
}

void JGWidgetComponent::Shutdown()
{
	OnShutdown();
}

void JGWidgetComponent::Construct()
{
	GUID = HGuid::New();
}
