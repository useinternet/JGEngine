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

PSharedPtr<JGWidgetComponent> JGWidget::MakeWidgetComponent(PSharedPtr<JGClass> InClass)
{
	PSharedPtr<JGObject> Object = AllocateByClass(InClass);
	PSharedPtr<JGWidgetComponent> WidgetComp = Cast<JGWidgetComponent>(Object);
	if (WidgetComp.IsValid())
	{
		WidgetComponents.push_back(WidgetComp);
	}
	
	return WidgetComp;
}


void JGWidget::SetupLayout()
{
	HWidgetLayout Layout;
	ImVec2 ContentRegionMax  = ImGui::GetWindowContentRegionMax();
	ImVec2 ContentRegionMin  = ImGui::GetWindowContentRegionMin();
	
	Layout.ContentSize = HVector2(ContentRegionMax.x - ContentRegionMin.x - ImGui::GetStyle().FramePadding.x, ContentRegionMax.y - ContentRegionMin.y);
	OnLayout(Layout);
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
	SetupLayout();
	OnGenerateGUI();
	ImGui::End();
	ImGui::PopID();

	if (bLocalOpen == false)
	{
		Close();
	}
}

void JGWidget::Update()
{
	OnUpdate();
	for (PSharedPtr<JGWidgetComponent> WidgetCom : WidgetComponents)
	{
		WidgetCom->OnUpdate();
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