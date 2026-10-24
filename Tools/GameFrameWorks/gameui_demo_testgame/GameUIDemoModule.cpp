#include "PCH/PCH.h"
#include "GameUIDemoModule.h"
#include "GameUIDemoEntryActor.h"
#include "GameUIDemoHUD.h"
#include "Core/GameInstance.h"
#include "UI/GameUI.h"

JG_MODULE_IMPL(HGameUIDemoModule, GAMEUIDEMO_C_API)

JGType HGameUIDemoModule::GetModuleType() const
{
	return JGTYPE(HGameUIDemoModule);
}

void HGameUIDemoModule::StartupModule()
{
	// 런처는 GameFrameWorks 를 연결하지 않으므로 게임 모듈이 연결한다. 게임 UI 는 GameFrameWorks 안에 있다.
	if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
	{
		JG_LOG(GameUIDemo, ELogLevel::Critical, "Fail Connect GameFrameWorks Module...");
		return;
	}

	// 월드를 만들 때마다 엔진이 이 엔트리 액터를 스폰하고 OnEnterWorld 를 부른다.
	JGGameInstance::Get().SetEntryClass<JGGameUIDemoEntryActor>();
	JGGameInstance::Get().LoadWorld(PName("GameUIDemo"));

	// HUD 는 게임 인스턴스의 UI 에 올린다(월드를 바꿔도 남는다).
	JGGameInstance::Get().GetUI()->PushWidget<JGGameUIDemoHUD>(EGameUILayer::Game);

	JG_LOG(GameUIDemo, ELogLevel::Info, "Startup GameUIDemo Module...");
}

void HGameUIDemoModule::ShutdownModule()
{
	// 게임이 만든 화면은 게임 모듈이 내려갈 때 내린다(위젯 클래스가 이 DLL 에 있다).
	if (JGGameInstance::HasInstance())
	{
		JGGameInstance::Get().GetUI()->ClearAllWidgets();
	}
	JG_LOG(GameUIDemo, ELogLevel::Info, "Shutdown GameUIDemo Module...");
}
