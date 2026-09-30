#include "PCH/PCH.h"
#include "JGEditor.h"
#include "Platform/Platform.h"
#include "Platform/JWindow.h"
#include "FileIO/ProjectDescriptor.h"
#include "GUIModule.h"
#include "Menu/MenuTree.h"
#include "Devkit.h"

#ifdef _PLATFORM_WINDOWS
#include "Platform/Windows/WindowsJWindow.h"
#endif

JG_MODULE_IMPL(HJGEditorModule, JGEDITOR_C_API)

namespace
{
	// 에디터가 쓰는 엔진 모듈. 연결은 이 순서, 종료는 역순이다(JGDev_Graphics 에서 옮긴 순서 그대로).
	const char* const EDITOR_ENGINE_MODULES[] = { "Graphics", "GUI", "Asset", "DevStatistics", "DevConsole", "Devkit" };
}

JGType HJGEditorModule::GetModuleType() const
{
	return JGTYPE(HJGEditorModule);
}

void HJGEditorModule::StartupModule()
{
	HJWindowArguments winArgs;
	winArgs.Title = "JGEditor";
	if (HFileHelper::IsProjectMode() == true)
	{
		winArgs.Title = PString::Format("JGEditor - %s", HFileHelper::ProjectName());
	}
	winArgs.Size = HVector2Int(1936, 1119);
	_window = HPlatform::CreateJWindow(winArgs);

	GCoreSystem::GetGlobalValues().MainWindow = _window.GetRawPointer();

	for (const char* moduleName : EDITOR_ENGINE_MODULES)
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule(moduleName) == false)
		{
			JG_LOG(JGEditor, ELogLevel::Critical, "Fail Connect %s Module...", moduleName);
		}
	}

	openDefaultWidgets();
	connectProjectModules();

	JG_LOG(JGEditor, ELogLevel::Trace, "Startup JGEditor Module...");
}

void HJGEditorModule::ShutdownModule()
{
	// 게임 모듈은 엔진 모듈을 쓰므로 먼저 내린다.
	disconnectProjectModules();

	_window = nullptr;

	const int32 numEngineModules = (int32)std::size(EDITOR_ENGINE_MODULES);
	for (int32 i = numEngineModules - 1; i >= 0; --i)
	{
		GModuleGlobalSystem::GetInstance().DisconnectModule(EDITOR_ENGINE_MODULES[i]);
	}

	JG_LOG(JGEditor, ELogLevel::Trace, "Shutdown JGEditor Module...");
}

void HJGEditorModule::openDefaultWidgets()
{
	HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	if (GUIModule == nullptr)
	{
		JG_LOG(JGEditor, ELogLevel::Error, "JGEditor needs the GUI module");
		return;
	}

	HMainMenuItem MenuItem;
	MenuItem.MenuPath = "Dev/DevAI";
	MenuItem.Action.BindLambda([]()
		{
			HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			if (GUIModule != nullptr)
			{
				GUIModule->OpenWidget<JGDevFeature>();
			}
		});

	GUIModule->AddMainMenuItem(MenuItem);

	// 시작 시 DevFeature(씬 뷰)를 바로 연다. 메뉴 Dev/DevAI 로도 열 수 있다.
	GUIModule->OpenWidget<JGDevFeature>();
}

void HJGEditorModule::connectProjectModules()
{
	if (HFileHelper::IsProjectMode() == false)
	{
		return;
	}

	HProjectDescriptor descriptor;
	if (HProjectDescriptor::Load(HFileHelper::ProjectDirectory(), &descriptor) == false)
	{
		JG_LOG(JGEditor, ELogLevel::Error, "Fail read project descriptor in %s", HFileHelper::ProjectDirectory());
		return;
	}

	// 에디터 환경은 게임 모듈과 에디터 모듈을 모두 올린다. 에디터 모듈이 게임 모듈에 의존하므로 게임 모듈이 먼저다.
	HList<PString> moduleNames = descriptor.GameModules;
	moduleNames.insert(moduleNames.end(), descriptor.EditorModules.begin(), descriptor.EditorModules.end());

	for (const PString& moduleName : moduleNames)
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule(moduleName) == false)
		{
			JG_LOG(JGEditor, ELogLevel::Error, "Fail Connect project module %s", moduleName);
			continue;
		}

		_connectedProjectModules.push_back(moduleName);
	}
}

void HJGEditorModule::disconnectProjectModules()
{
	while (_connectedProjectModules.empty() == false)
	{
		GModuleGlobalSystem::GetInstance().DisconnectModule(_connectedProjectModules.back());
		_connectedProjectModules.pop_back();
	}

	// 게임 모듈이 연결한 GameFrameWorks 도 엔진 모듈보다 먼저 내린다. 연결돼 있지 않으면 아무 일도 하지 않는다.
	GModuleGlobalSystem::GetInstance().DisconnectModule("GameFrameWorks");
}
