#include "PCH/PCH.h"
#include "GUIModule.h"
#include "Widget.h"
#include "Platform/JWindow.h"

#ifdef _DIRECTX12
#include "Backends/DX12GUIBackend.h"
#endif // _DIRECTX12

JG_MODULE_IMPL(HGUIModule, GUI_C_API)

JGType HGUIModule::GetModuleType() const
{
    return JGTYPE(HGUIModule);
}

void HGUIModule::StartupModule()
{
	if (GModuleGlobalSystem::GetInstance().ConnectModule("Graphics") == false)
	{
		JG_LOG(JGDev_GraphicsModule, ELogLevel::Critical, "Fail Connect Graphics Module...");
	}

	GUIBackend = Allocate<PDX12GUIBackend>();
	GUIBackend->Initialize();

	GUIBackend->OnMainMenuGUI.AddRaw(this, &HGUIModule::GenerateMainMenuGUI);
	GUIBackend->OnGUI.AddRaw(this, &HGUIModule::GenerateWidgetGUI);

	GScheduleGlobalSystem::GetInstance().Schedule(0.0f, 1.0f / 30.0f, EMainThreadExecutionOrder::Update, PTaskDelegate::CreateRaw(this, &HGUIModule::UpdateWidgets));
	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::Update, PTaskDelegate::CreateRaw(this, &HGUIModule::UpdateFrameWidgets));
}

void HGUIModule::ShutdownModule()
{
	for (HPair<const HGuid, PSharedPtr<JGWidget>>& Pair : Widgets)
	{
		Pair.second->Shutdown();
	}

	GUIBackend->Shutdown();
	GUIBackend.Reset();
	GUIBackend = nullptr;
}

void HGUIModule::GenerateMainMenuGUI()
{
	MainMenuTree.GenerateMainMenuGUI();
}

void HGUIModule::GenerateWidgetGUI()
{
	for (const HPair<const HGuid, PSharedPtr<JGWidget>>& Pair : Widgets)
	{
		PSharedPtr<JGWidget> Widget = Pair.second;
		if (Widget->IsOpen())
		{
			Widget->GenerateGUI();
		}
	}
}

void HGUIModule::AddMainMenuItem(const HMainMenuItem& InMainMenuItem)
{
	MainMenuTree.AddMainMenuItem(InMainMenuItem);
}

PWeakPtr<PGUIBackend> HGUIModule::GetGUIBackend() const
{
	return GUIBackend;
}

bool HGUIModule::OpenWidgetInternal(PSharedPtr<JGWidget> InWidget)
{
	if (InWidget == nullptr)
	{
		return false;
	}

	InWidget->Open();

	return true;
}

void HGUIModule::CloseWidgetInternal(PSharedPtr<JGWidget> InWidget)
{
	InWidget->Close();
}

void HGUIModule::UpdateWidgets()
{
	for (HPair<const HGuid, PSharedPtr<JGWidget>>& Pair : Widgets)
	{
		PSharedPtr<JGWidget> Widget = Pair.second;
		EWidgetFlags WidgetFlags = Widget->GetFlags();
		if (EnumHasAnyFlags(WidgetFlags, EWidgetFlags::AllowAlwaysUpdate) == false && EnumHasAnyFlags(WidgetFlags, EWidgetFlags::AllowUpdate) == false)
		{
			continue;
		}

		bool bEnableUpdate = Widget->IsOpen();
		if (EnumHasAnyFlags(WidgetFlags, EWidgetFlags::AllowAlwaysUpdate))
		{
			bEnableUpdate = true;
		}
		
		if (bEnableUpdate)
		{
			Widget->Update();
		}
	}
}

void HGUIModule::UpdateFrameWidgets()
{
	for (HPair<const HGuid, PSharedPtr<JGWidget>>& Pair : Widgets)
	{
		PSharedPtr<JGWidget> Widget = Pair.second;
		EWidgetFlags WidgetFlags = Widget->GetFlags();
		if (EnumHasAnyFlags(WidgetFlags, EWidgetFlags::AllowAlwaysUpdateFrame) == false && EnumHasAnyFlags(WidgetFlags, EWidgetFlags::AllowUpdateFrame) == false)
		{
			continue;
		}

		bool bEnableUpdate = Widget->IsOpen();
		if (EnumHasAnyFlags(WidgetFlags, EWidgetFlags::AllowAlwaysUpdateFrame))
		{
			bEnableUpdate = true;
		}

		if (bEnableUpdate)
		{
			Widget->Update();
		}
	}
}