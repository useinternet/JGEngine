#include "PCH/PCH.h"
#include "GfwViewModule.h"
#include "Core/GameInstance.h"
#include "GfwViewEntryActor.h"

JG_MODULE_IMPL(HGfwViewModule, GFWVIEW_C_API)

JGType HGfwViewModule::GetModuleType() const
{
	return JGTYPE(HGfwViewModule);
}

void HGfwViewModule::StartupModule()
{
	// 런처는 GameFrameWorks 를 연결하지 않으므로 게임 모듈이 연결한다.
	if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
	{
		JG_LOG(GfwView, ELogLevel::Critical, "Fail Connect GameFrameWorks Module...");
		return;
	}

	// 월드를 만들 때마다 엔진이 이 엔트리 액터를 스폰하고 OnEnterWorld 를 부른다.
	JGGameInstance::Get().SetEntryClass<JGGfwViewEntryActor>();
	JGGameInstance::Get().LoadWorld(PName("GfwView"));

	// 창(씬 뷰포트 · Gameplay DevView)은 에디터가 가진다. 게임 모듈은 에디터 위젯을 참조하지 않는다.
	// 씬 뷰포트는 에디터가 자동으로 열고, DevView 는 메뉴 Windows/Gameplay DevView 로 연다.

	JG_LOG(GfwView, ELogLevel::Info, "Startup GfwView Module...");
}

void HGfwViewModule::ShutdownModule()
{
	// 의존 모듈(GameFrameWorks)은 여기서 Disconnect 하지 않는다. 모듈 연결에는 참조 카운트가 없어서
	// 다른 모듈이 아직 쓰고 있을 수 있다. 종료 순서는 호스트와 GModuleGlobalSystem::Destroy 가 맡는다.
	// 이 모듈이 PSharedPtr 로 들고 있는 전역 객체가 생기면 여기서 Reset 한다.
	JG_LOG(GfwView, ELogLevel::Info, "Shutdown GfwView Module...");
}
