#include "PCH/PCH.h"
#include "{PROJECT_NAME}Module.h"
#include "Core/GameInstance.h"
#include "{PROJECT_NAME}EntryActor.h"

JG_MODULE_IMPL(H{PROJECT_NAME}Module, {PROJECT_NAME_UPPER}_C_API)

JGType H{PROJECT_NAME}Module::GetModuleType() const
{
	return JGTYPE(H{PROJECT_NAME}Module);
}

void H{PROJECT_NAME}Module::StartupModule()
{
	// 런처는 GameFrameWorks 를 연결하지 않으므로 게임 모듈이 연결한다.
	if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
	{
		JG_LOG({PROJECT_NAME}, ELogLevel::Critical, "Fail Connect GameFrameWorks Module...");
		return;
	}

	// 월드를 만들 때마다 엔진이 이 엔트리 액터를 스폰하고 OnEnterWorld 를 부른다.
	JGGameInstance::Get().SetEntryClass<JG{PROJECT_NAME}EntryActor>();
	JGGameInstance::Get().LoadWorld(PName("{PROJECT_NAME}"));

	JG_LOG({PROJECT_NAME}, ELogLevel::Info, "Startup {PROJECT_NAME} Module...");
}

void H{PROJECT_NAME}Module::ShutdownModule()
{
	// 의존 모듈(GameFrameWorks)은 여기서 Disconnect 하지 않는다. 모듈 연결에는 참조 카운트가 없어서
	// 다른 모듈이 아직 쓰고 있을 수 있다. 종료 순서는 호스트와 GModuleGlobalSystem::Destroy 가 맡는다.
	// 이 모듈이 PSharedPtr 로 들고 있는 전역 객체가 생기면 여기서 Reset 한다.
	JG_LOG({PROJECT_NAME}, ELogLevel::Info, "Shutdown {PROJECT_NAME} Module...");
}
