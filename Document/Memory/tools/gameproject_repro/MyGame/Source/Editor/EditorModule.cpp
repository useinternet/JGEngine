#include "PCH/PCH.h"
#include "EditorModule.h"

JG_MODULE_IMPL(HEditorModule, EDITOR_C_API)

JGType HEditorModule::GetModuleType() const
{
	return JGTYPE(HEditorModule);
}

void HEditorModule::StartupModule()
{
	if (GModuleGlobalSystem::GetInstance().ConnectModule("Game") == false)
	{
		JG_LOG(Editor, ELogLevel::Critical, "Fail Connect Game Module...");
	}

	JG_LOG(Editor, ELogLevel::Info, "Startup Editor Module...");
}

void HEditorModule::ShutdownModule()
{
	JG_LOG(Editor, ELogLevel::Info, "Shutdown Editor Module...");
}
