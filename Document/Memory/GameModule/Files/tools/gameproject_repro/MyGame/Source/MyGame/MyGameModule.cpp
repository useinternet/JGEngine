#include "PCH/PCH.h"
#include "MyGameModule.h"
#include "Core/GameInstance.h"
#include "MyGameEntryActor.h"

JG_MODULE_IMPL(HMyGameModule, MYGAME_C_API)

JGType HMyGameModule::GetModuleType() const
{
	return JGTYPE(HMyGameModule);
}

void HMyGameModule::StartupModule()
{
	if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
	{
		JG_LOG(MyGame, ELogLevel::Critical, "Fail Connect GameFrameWorks Module...");
		return;
	}

	JGGameInstance::Get().SetEntryClass<JGMyGameEntryActor>();
	JG_LOG(MyGame, ELogLevel::Info, "Startup MyGame Module...");
}

void HMyGameModule::ShutdownModule()
{
	// 의존 모듈(GameFrameWorks)은 여기서 Disconnect 하지 않는다. 종료 순서는 GModuleGlobalSystem::Destroy 가 연결 역순으로 보장한다.
	JG_LOG(MyGame, ELogLevel::Info, "Shutdown MyGame Module...");
}
