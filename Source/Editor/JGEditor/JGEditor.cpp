#include "PCH/PCH.h"
#include "JGEditor.h"
#include "Platform/Platform.h"
#include "Platform/JWindow.h"
#include "FileIO/ProjectDescriptor.h"
#include "GUIModule.h"
#include "Menu/MenuTree.h"
#include "Core/GameInstance.h"
#include "Widgets/SceneViewport.h"
#include "Widgets/GameplayDevView.h"
#include "Widgets/DataTableEditor.h"
#include "Widgets/SessionLobby.h"
#include "Widgets/DevConsole.h"
#include "Widgets/MemoryStatistics.h"

#ifdef _PLATFORM_WINDOWS
#include "Platform/Windows/WindowsJWindow.h"
#endif

JG_MODULE_IMPL(HJGEditorModule, JGEDITOR_C_API)

namespace
{
	// 에디터가 쓰는 엔진 모듈. 연결은 이 순서, 종료는 역순이다(JGDev_Graphics 에서 옮긴 순서 그대로).
	// GameFrameWork 는 프로젝트 없이 띄운 에디터에서도 그 모듈의 콘솔 명령(gmtest)을 쓰게 연결한다(DevConsole 5-1, 사용자 결정 2026-10-02).
	// 맨 뒤에 두어 게임 모듈이 연결하던 때와 같은 순서를 지킨다: 다른 엔진 모듈이 모두 올라간 뒤 시작하고, 게임 모듈 다음 · 다른 엔진 모듈보다 먼저 내린다.
	// 게임 모듈의 ConnectModule("GameFrameWork") 는 이미 연결돼 있으면 true 만 돌려준다(참조 카운트 없음).
	// 에셋 DB 는 GameFrameWork 가 시작할 때 Graphics 가 이미 켜져 있으면 켠다 — 그래서 Graphics 가 맨 앞이다.
	const char* const EDITOR_ENGINE_MODULES[] = { "Graphics", "GUI", "GameFrameWork" };
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
	// 기본 창 크기는 100%(96 DPI) 기준 1936x1119(클라이언트 1920x1080)다. 프로세스가 DPI 인식이면 창 크기가 실제 픽셀이므로 화면 배율을 곱한다
	// (D-22 ②, 인식은 JGLauncher 시작 코드). 화면보다 크면 Windows 가 줄인다(인식 전과 같다).
	const float32 dpiScale = HPlatform::GetDpiScale();
	winArgs.Size = HVector2Int((int32)(1936.0f * dpiScale + 0.5f), (int32)(1119.0f * dpiScale + 0.5f));
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
	
	// 메모리 통계 · 콘솔 창. 메뉴는 넣은 차례대로 보이므로, 따로 된 모듈(DevStatistics · DevConsole)이 달던 때와 같은 순서가 되게 맨 앞에 둔다.
	HMainMenuItem MemoryStatisticsItem;
	MemoryStatisticsItem.MenuPath = "Windows/Statistics/Memory";
	MemoryStatisticsItem.Action.BindLambda([]()
		{
			HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			if (GUIModule != nullptr)
			{
				GUIModule->OpenWidget<JGMemoryStatistcs>();
			}
		});
	GUIModule->AddMainMenuItem(MemoryStatisticsItem);

	HMainMenuItem DevConsoleItem;
	DevConsoleItem.MenuPath = "Windows/DevConsole";
	DevConsoleItem.Action.BindLambda([]()
		{
			HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			if (GUIModule != nullptr)
			{
				GUIModule->OpenWidget<JGDevConsole>();
			}
		});
	GUIModule->AddMainMenuItem(DevConsoleItem);

	// 게임 월드 창. 월드가 없으면(프로젝트 없이 띄운 에디터) 창은 안내 문구만 그린다.
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

	// 세션 로비 개발용 창(호스트 · 참가 · 떠나기 · 새 판). 게임 인스턴스의 세션을 보여 주고 바꾼다.
	HMainMenuItem SessionLobbyItem;
	SessionLobbyItem.MenuPath = "Windows/Session Lobby";
	SessionLobbyItem.Action.BindLambda([]()
		{
			HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
			if (GUIModule != nullptr)
			{
				GUIModule->OpenWidget<JGSessionLobby>();
			}
		});
	GUIModule->AddMainMenuItem(SessionLobbyItem);
}

void HJGEditorModule::openSceneViewport()
{
	// 프로젝트 모드에서 게임 모듈을 연결했을 때만 연다(프로젝트 없이 띄운 에디터에 빈 월드 창이 저절로 뜨지 않게).
	// GameFrameWork 는 엔진 모듈로 늘 연결돼 게임 인스턴스가 있으므로 게임 인스턴스 유무만으로는 가를 수 없다.
	// 게임 모듈은 openDefaultWidgets 뒤에 연결되므로 여기서 본다.
	if (_connectedProjectModules.empty() || JGGameInstance::HasInstance() == false)
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

	// GameFrameWork 는 게임 모듈 바로 뒤, 창을 내리기 전에 내린다(게임 모듈이 연결하던 때의 순서). 엔진 모듈 목록에도 있어
	// 역순 해제가 한 번 더 부르지만, 이미 내려간 모듈이면 DisconnectModule 은 아무 일도 하지 않고 true 를 돌려준다.
	GModuleGlobalSystem::GetInstance().DisconnectModule("GameFrameWork");
}
