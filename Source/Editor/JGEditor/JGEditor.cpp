#include "PCH/PCH.h"
#include "JGEditor.h"
#include "Platform/Platform.h"
#include "Platform/JWindow.h"
#include "FileIO/ProjectDescriptor.h"
#include "GUIModule.h"
#include "Menu/MenuTree.h"
#include "Devkit.h"
#include "Core/GameInstance.h"
#include "Widgets/SceneViewport.h"
#include "Widgets/GameplayDevView.h"
#include "Widgets/DataTableEditor.h"

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
	openSceneViewport();

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
	
	// 게임 월드 창. GameFrameWorks 가 돌지 않으면(프로젝트 없이 띄운 에디터) 창은 안내 문구만 그린다.
	HMainMenuItem SceneViewportItem;
	SceneViewportItem.MenuPath = "Windows/Scene Viewport";
	SceneViewportItem.Action.BindLambda([]()
		{
			HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			if (GUIModule != nullptr)
			{
				GUIModule->OpenWidget<JGSceneViewport>();
			}
		});
	GUIModule->AddMainMenuItem(SceneViewportItem);

	HMainMenuItem DevViewItem;
	DevViewItem.MenuPath = "Windows/Gameplay DevView";
	DevViewItem.Action.BindLambda([]()
		{
			HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			if (GUIModule != nullptr)
			{
				GUIModule->OpenWidget<JGGameplayDevView>();
			}
		});
	GUIModule->AddMainMenuItem(DevViewItem);

	// 데이터 테이블(.jgasset JSON) 편집 창. 프로젝트 없이도 엔진 Content 테이블을 연다.
	HMainMenuItem DataTableEditorItem;
	DataTableEditorItem.MenuPath = "Windows/Data Table Editor";
	DataTableEditorItem.Action.BindLambda([]()
		{
			HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			if (GUIModule != nullptr)
			{
				GUIModule->OpenWidget<JGDataTableEditor>();
			}
		});
	GUIModule->AddMainMenuItem(DataTableEditorItem);
}

void HJGEditorModule::openSceneViewport()
{
	// 게임 모듈이 GameFrameWorks 를 연결했을 때(프로젝트 모드)만 연다. 게임 모듈은 openDefaultWidgets 뒤에 연결되므로 여기서 본다.
	if (JGGameInstance::HasInstance() == false)
	{
		return;
	}

	HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	if (GUIModule == nullptr)
	{
		return;
	}

	// 기본 창이다. 사용자가 닫아 둔 레이아웃(imgui.ini)이면 열지 않는다. 그 밖에 열려 있던 창은 GUI 모듈이 첫 프레임에 다시 연다.
	GUIModule->OpenWidgetByDefault<JGSceneViewport>();
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
