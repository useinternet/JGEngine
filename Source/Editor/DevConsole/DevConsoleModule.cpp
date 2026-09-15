#include "PCH/PCH.h"
#include "DevConsoleModule.h"
#include "GUIModule.h"
#include "Misc/Delegate.h"

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
		 JG_LOG(DevStatistics, ELogLevel::Error, "DevStatisticsModule Need GUI Module");
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

 HDelegateHandle HDevConsoleModule::RegisterConsoleCommand(HOnDevConsole::DelegateT Delegate)
 {
	 PSharedPtr<JGDevConsole> DevConsole = GetCheckedGUIModule()->FindWidget<JGDevConsole>();
	 if (DevConsole.IsValid() == false)
	 {
		 return HDelegateHandle();
	 }

	 return DevConsole->RegisterConsoleCommand(Delegate);
 }

 void HDevConsoleModule::UnRegisterConsoleCommand(HDelegateHandle& InHandle)
 {
	 PSharedPtr<JGDevConsole> DevConsole = GetCheckedGUIModule()->FindWidget<JGDevConsole>();
	 if (DevConsole.IsValid() == false)
	 {
		 return;
	 }

	 DevConsole->UnRegisterConsoleCommand(InHandle);
 }

 HGUIModule* HDevConsoleModule::GetCheckedGUIModule() const
 {
	 static HGUIModule* GUIModule = nullptr;
	 if (GUIModule == nullptr)
	 {
		 GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	 }

	 JG_CHECK(GUIModule == nullptr);
	 return GUIModule;
 }

