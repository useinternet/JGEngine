#include "PCH/PCH.h"
#include "DevStatistics.h"
#include "GUIModule.h"
#include "MemoryStatistics.h"

 JG_MODULE_IMPL(HDevStatisticsModule, DEVSTATISTICS_C_API)

 HDevStatisticsModule::~HDevStatisticsModule()
 {
	
 }

 JGType HDevStatisticsModule::GetModuleType() const
 {
 	return JGTYPE(HDevStatisticsModule);
 }

 void HDevStatisticsModule::StartupModule()
 {
	 HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	 if (GUIModule == nullptr)
	 {
		 JG_LOG(DevStatistics, ELogLevel::Error, "DevStatisticsModule Need GUI Module");
		 return;
	 }

	 HMainMenuItem MenuItem;
	 MenuItem.MenuPath = "Windows/Statistics/Memory";
	 MenuItem.Action.BindLambda([]()
		 {
			 HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			 GUIModule->OpenWidget<JGMemoryStatistcs>();
		 });

	 GUIModule->AddMainMenuItem(MenuItem);
 }

 void HDevStatisticsModule::ShutdownModule()
 {

 }

