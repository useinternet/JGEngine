#include "PCH/PCH.h"
#include "WidgetComponent.h"
#include "Imgui/imgui.h"

void JGWidgetComponent::SetupLayout(const HWidgetComponentLayout& InLayout)
{
	HVector2 WidgetSize;
	OnLayout(InLayout);
	WidgetComponentSize = InLayout.ContentSize;
}

void JGWidgetComponent::GenerateGUI()
{
	HVector2 Size = WidgetComponentSize.GetValue();
	uint64 GUIDHash = GetGUID().GetHashCode();

	ImGuiChildFlags ImChildFlags = ImGuiChildFlags_None;
	if (EnumHasAnyFlags(WidgetComponentFlags,  EWidgetComponentFlags::Border))
	{
		ImChildFlags |= ImGuiChildFlags_Border;
	}

	ImGui::BeginChild((int)GUIDHash, ImVec2(Size.x, Size.y), ImChildFlags);
	OnGenerateGUI();
	ImGui::EndChild();
}

void JGWidgetComponent::Initialize()
{
	WidgetComponentFlags = EWidgetComponentFlags::AutoSize;
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
