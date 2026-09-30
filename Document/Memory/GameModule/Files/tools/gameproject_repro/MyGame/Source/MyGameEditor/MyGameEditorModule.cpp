#include "PCH/PCH.h"
#include "MyGameEditorModule.h"

JG_MODULE_IMPL(HMyGameEditorModule, MYGAMEEDITOR_C_API)

JGType HMyGameEditorModule::GetModuleType() const
{
	return JGTYPE(HMyGameEditorModule);
}

void HMyGameEditorModule::StartupModule()
{
	// 게임 모듈이 먼저 떠 있어야 한다. 이미 연결돼 있으면 아무 일도 하지 않는다.
	if (GModuleGlobalSystem::GetInstance().ConnectModule("MyGame") == false)
	{
		JG_LOG(MyGameEditor, ELogLevel::Critical, "Fail Connect MyGame Module...");
	}

	JG_LOG(MyGameEditor, ELogLevel::Info, "Startup MyGameEditor Module...");
}

void HMyGameEditorModule::ShutdownModule()
{
	JG_LOG(MyGameEditor, ELogLevel::Info, "Shutdown MyGameEditor Module...");
}
