#include "PCH/PCH.h"
#include "JGDev_Graphics.h"
#include "Platform/Platform.h"
#include "Platform/JWindow.h"
#include "JGGraphics.h"
#ifdef _PLATFORM_WINDOWS
#include "Platform/Windows/WindowsJWindow.h"
#endif
#include "Importers/FBXAssetImporter.h"
#include "Devkit.h"
#include "Menu/MenuTree.h"
#include "GUIModule.h"

 JG_MODULE_IMPL(HJGDev_GraphicsModule, JGDEV_GRAPHICS_C_API)

 HJGDev_GraphicsModule::~HJGDev_GraphicsModule()
 {
	
 }

 JGType HJGDev_GraphicsModule::GetModuleType() const
 {
 	return JGTYPE(HJGDev_GraphicsModule);
 }

 void HJGDev_GraphicsModule::StartupModule()
 {
 	HJWindowArguments winArgs;
 	winArgs.Title = "JGDev_Graphics";
 	winArgs.Size  = HVector2Int(1936, 1119);
 	_window = HPlatform::CreateJWindow(winArgs);

 	GCoreSystem::GetGlobalValues().MainWindow = _window.GetRawPointer();


 	if (GModuleGlobalSystem::GetInstance().ConnectModule("Graphics") == false)
 	{
 		JG_LOG(JGDev_GraphicsModule, ELogLevel::Critical, "Fail Connect Graphics Module...");
 	}

	if (GModuleGlobalSystem::GetInstance().ConnectModule("GUI") == false)
	{
		JG_LOG(JGDev_GraphicsModule, ELogLevel::Critical, "Fail Connect Graphics Module...");
	}

	if (GModuleGlobalSystem::GetInstance().ConnectModule("Asset") == false)
	{
		JG_LOG(JGDev_GraphicsModule, ELogLevel::Critical, "Fail Connect Asset Module...");

	}

	if (GModuleGlobalSystem::GetInstance().ConnectModule("DevStatistics") == false)
	{
		JG_LOG(JGDev_GraphicsModule, ELogLevel::Critical, "Fail Connect Asset Module...");

	}

	if (GModuleGlobalSystem::GetInstance().ConnectModule("DevConsole") == false)
	{
		JG_LOG(JGDev_GraphicsModule, ELogLevel::Critical, "Fail Connect DevConsole Module...");

	}

	if (GModuleGlobalSystem::GetInstance().ConnectModule("Devkit") == false)
	{
		JG_LOG(JGDev_GraphicsModule, ELogLevel::Critical, "Fail Connect Asset Module...");

	}

 	JG_LOG(JGDev_GraphicsModule, ELogLevel::Trace, "Startup JGDev_GraphicsModule Module...");

    _test.Init();
    GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::Update, PTaskDelegate::CreateRaw(&_test, &HJGDevGraphicsTest::Update));

	HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	if (GUIModule == nullptr)
	{
		JG_LOG(DevStatistics, ELogLevel::Error, "DevStatisticsModule Need GUI Module");
		return;
	}

	HMainMenuItem MenuItem;
	MenuItem.MenuPath = "Dev/DevAI";
	MenuItem.Action.BindLambda([]()
		{
			HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			GUIModule->OpenWidget<JGDevFeature>();
		});

	GUIModule->AddMainMenuItem(MenuItem);
 }

 void HJGDev_GraphicsModule::ShutdownModule()
 {
     _test.Shutdown();

 	_window = nullptr;
	GModuleGlobalSystem::GetInstance().DisconnectModule("Devkit");
    GModuleGlobalSystem::GetInstance().DisconnectModule("DevConsole");
    GModuleGlobalSystem::GetInstance().DisconnectModule("DevStatistics");
    GModuleGlobalSystem::GetInstance().DisconnectModule("Asset");
    GModuleGlobalSystem::GetInstance().DisconnectModule("GUI");
    GModuleGlobalSystem::GetInstance().DisconnectModule("Graphics");

 	JG_LOG(JGDev_GraphicsModule, ELogLevel::Trace, "Shutdown JGDev_GraphicsModule Module...");
 }

