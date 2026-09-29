#include "PCH/PCH.h"
#include "GameModule.h"
#include "Core/GameFrameWorksModule.h"
#include "Core/GameInstance.h"
#include "MyGameEntryActor.h"

JG_MODULE_IMPL(HGameModule, GAME_C_API)

JGType HGameModule::GetModuleType() const
{
	return JGTYPE(HGameModule);
}

void HGameModule::StartupModule()
{
	if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
	{
		JG_LOG(Game, ELogLevel::Critical, "Fail Connect GameFrameWorks Module...");
		return;
	}

	JGGameInstance::Get().SetEntryClass<JGMyGameEntryActor>();
	JG_LOG(Game, ELogLevel::Info, "Startup Game Module...");
}

void HGameModule::ShutdownModule()
{
	JG_LOG(Game, ELogLevel::Info, "Shutdown Game Module...");
}
