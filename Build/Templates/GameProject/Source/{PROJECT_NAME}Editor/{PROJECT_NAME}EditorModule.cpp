#include "PCH/PCH.h"
#include "{PROJECT_NAME}EditorModule.h"
#include "Core/GameInstance.h"
#include "GUIModule.h"

JG_MODULE_IMPL(H{PROJECT_NAME}EditorModule, {PROJECT_NAME_UPPER}EDITOR_C_API)

JGType H{PROJECT_NAME}EditorModule::GetModuleType() const
{
	return JGTYPE(H{PROJECT_NAME}EditorModule);
}

void H{PROJECT_NAME}EditorModule::StartupModule()
{
	// 게임 모듈이 먼저 떠 있어야 한다. 이미 연결돼 있으면 아무 일도 하지 않는다.
	if (GModuleGlobalSystem::GetInstance().ConnectModule("{PROJECT_NAME}") == false)
	{
		JG_LOG({PROJECT_NAME}Editor, ELogLevel::Critical, "Fail Connect {PROJECT_NAME} Module...");
		return;
	}

	HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	if (GUIModule == nullptr)
	{
		JG_LOG({PROJECT_NAME}Editor, ELogLevel::Error, "{PROJECT_NAME}Editor needs the GUI module");
		return;
	}

	// 에디터에서만 쓰는 메뉴 예시: 게임 월드를 다시 만든다.
	HMainMenuItem MenuItem;
	MenuItem.MenuPath = "{PROJECT_NAME}/Reload World";
	MenuItem.Action.BindLambda([]()
		{
			JGGameInstance::Get().LoadWorld(PName("{PROJECT_NAME}"));
		});
	GUIModule->AddMainMenuItem(MenuItem);

	JG_LOG({PROJECT_NAME}Editor, ELogLevel::Info, "Startup {PROJECT_NAME}Editor Module...");
}

void H{PROJECT_NAME}EditorModule::ShutdownModule()
{
	// 의존 모듈({PROJECT_NAME}, GUI)은 여기서 Disconnect 하지 않는다. 종료 순서는 JGEditor 가 맡는다.
	JG_LOG({PROJECT_NAME}Editor, ELogLevel::Info, "Shutdown {PROJECT_NAME}Editor Module...");
}
