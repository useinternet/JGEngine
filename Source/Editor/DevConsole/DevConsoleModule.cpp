#include "PCH/PCH.h"
#include "DevConsoleModule.h"
#include "GUIModule.h"

 JG_MODULE_IMPL(HDevConsoleModule, DEVCONSOLE_C_API)

 HDevConsoleModule::~HDevConsoleModule()
 {

 }

 JGType HDevConsoleModule::GetModuleType() const
 {
 	return JGTYPE(HDevConsoleModule);
 }

 void HDevConsoleModule::StartupModule()
 {
	 HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	 if (GUIModule == nullptr)
	 {
		 JG_LOG(DevConsole, ELogLevel::Error, "DevConsoleModule Need GUI Module");
		 return;
	 }

	 HMainMenuItem MenuItem;
	 MenuItem.MenuPath = "Windows/DevConsole";
	 MenuItem.Action.BindLambda([]()
		 {
			 HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			 GUIModule->OpenWidget<JGDevConsole>();
		 });

	 GUIModule->AddMainMenuItem(MenuItem);
 }

 void HDevConsoleModule::ShutdownModule()
 {

 }
