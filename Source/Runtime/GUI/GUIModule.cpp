#include "PCH/PCH.h"
#include "GUIModule.h"
#include "Widget.h"
#include "Platform/JWindow.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_internal.h"

#ifdef _DIRECTX12
#include "Backends/DX12GUIBackend.h"
#endif // _DIRECTX12

JG_MODULE_IMPL(HGUIModule, GUI_C_API)

namespace
{
	// imgui.ini 의 절 이름. 내용은 "[JGWidget][Open]" 아래 "위젯 클래스 이름=0|1" 줄들이다.
	const char* const LayoutSettingsTypeName  = "JGWidget";
	const char* const LayoutSettingsEntryName = "Open";
}

JGType HGUIModule::GetModuleType() const
{
    return JGTYPE(HGUIModule);
}

void HGUIModule::StartupModule()
{
	if (GModuleGlobalSystem::GetInstance().ConnectModule("Graphics") == false)
	{
		JG_LOG(GUI, ELogLevel::Critical, "Fail Connect Graphics Module...");
	}

	GUIBackend = Allocate<PDX12GUIBackend>();
	GUIBackend->Initialize();

	RegisterLayoutSettings();

	GUIBackend->OnMainMenuGUI.AddRaw(this, &HGUIModule::GenerateMainMenuGUI);
	GUIBackend->OnGUI.AddRaw(this, &HGUIModule::GenerateWidgetGUI);

	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::Update, PTaskDelegate::CreateRaw(this, &HGUIModule::UpdateWidgets));
}

void HGUIModule::ShutdownModule()
{
	for (HPair<const JGType, PSharedPtr<JGWidget>>& Pair : Widgets)
	{
		Pair.second->Shutdown();
	}

	// ImGui 컨텍스트를 없애며 imgui.ini 를 저장한다. 열림 상태는 마지막 프레임 값이다(종료 중에 모듈이 닫은 창은 반영하지 않는다).
	GUIBackend->Shutdown();
	GUIBackend.Reset();
	GUIBackend = nullptr;
}

void HGUIModule::GenerateMainMenuGUI()
{
	MainMenuTree.GenerateMainMenuGUI();
}

void HGUIModule::GenerateWidgetGUI()
{
	// 저장된 레이아웃의 창은 첫 프레임에 연다. 이때는 엔진 · 게임 프로젝트 모듈이 모두 연결돼 위젯 클래스가 등록돼 있다.
	if (bLayoutWidgetsRestored == false)
	{
		RestoreLayoutWidgets();
		bLayoutWidgetsRestored = true;
	}

	for (const HPair<const JGType, PSharedPtr<JGWidget>>& Pair : Widgets)
	{
		PSharedPtr<JGWidget> Widget = Pair.second;
		if (Widget->IsOpen())
		{
			Widget->GenerateGUI();
		}
	}

	// 이번 프레임에 메뉴로 열거나 X 로 닫은 창까지 반영한다.
	UpdateLayoutWidgetStates();
}

void HGUIModule::AddMainMenuItem(const HMainMenuItem& InMainMenuItem)
{
	MainMenuTree.AddMainMenuItem(InMainMenuItem);
}

PWeakPtr<PGUIBackend> HGUIModule::GetGUIBackend() const
{
	return GUIBackend;
}

bool HGUIModule::OpenWidgetInternal(PSharedPtr<JGWidget> InWidget)
{
	if (InWidget == nullptr)
	{
		return false;
	}

	InWidget->Open();

	return true;
}

void HGUIModule::CloseWidgetInternal(PSharedPtr<JGWidget> InWidget)
{
	InWidget->Close();
}

void HGUIModule::UpdateWidgets()
{
	for (HPair<const JGType, PSharedPtr<JGWidget>>& Pair : Widgets)
	{
		PSharedPtr<JGWidget> Widget = Pair.second;
		if (Widget->IsOpen())
		{
			Widget->Update();
		}
	}
}

void HGUIModule::RegisterLayoutSettings()
{
	ImGuiSettingsHandler Handler;
	Handler.TypeName   = LayoutSettingsTypeName;
	Handler.TypeHash   = ImHashStr(LayoutSettingsTypeName);
	Handler.UserData   = this;
	Handler.ReadOpenFn = [](ImGuiContext* InContext, ImGuiSettingsHandler* InHandler, const char* InName) -> void*
	{
		if (strcmp(InName, LayoutSettingsEntryName) != 0)
		{
			return nullptr;
		}
		return InHandler->UserData;
	};
	Handler.ReadLineFn = [](ImGuiContext* InContext, ImGuiSettingsHandler* InHandler, void* InEntry, const char* InLine)
	{
		HGUIModule* Module = static_cast<HGUIModule*>(InEntry);
		const char* Equal  = strchr(InLine, '=');
		if (Equal == nullptr || Equal == InLine)
		{
			return;
		}

		PString WidgetName;
		PString(InLine).SubString(&WidgetName, 0, (uint64)(Equal - InLine));
		Module->LayoutWidgetOpenStates[PName(WidgetName)] = atoi(Equal + 1) != 0;
	};
	Handler.WriteAllFn = [](ImGuiContext* InContext, ImGuiSettingsHandler* InHandler, ImGuiTextBuffer* OutBuffer)
	{
		HGUIModule* Module = static_cast<HGUIModule*>(InHandler->UserData);

		// 이름순으로 쓴다. 해시 맵 순서대로 쓰면 저장할 때마다 줄 순서가 바뀐다.
		HList<HPair<PString, bool>> Entries;
		for (const HPair<const PName, bool>& Pair : Module->LayoutWidgetOpenStates)
		{
			Entries.push_back(HPair<PString, bool>(Pair.first.ToString(), Pair.second));
		}
		std::sort(Entries.begin(), Entries.end(), [](const HPair<PString, bool>& A, const HPair<PString, bool>& B)
			{
				return strcmp(A.first.GetCStr(), B.first.GetCStr()) < 0;
			});

		OutBuffer->appendf("[%s][%s]\n", LayoutSettingsTypeName, LayoutSettingsEntryName);
		for (const HPair<PString, bool>& Entry : Entries)
		{
			OutBuffer->appendf("%s=%d\n", Entry.first.GetCStr(), Entry.second ? 1 : 0);
		}
		OutBuffer->append("\n");
	};
	ImGui::AddSettingsHandler(&Handler);

	// 첫 프레임 전에 읽는다. 다른 모듈이 시작하며 여는 기본 창(OpenWidgetByDefault)이 저장된 상태를 봐야 한다.
	// NewFrame 은 이미 읽었으면 다시 읽지 않는다.
	ImGuiIO& IO = ImGui::GetIO();
	if (IO.IniFilename != nullptr)
	{
		ImGui::LoadIniSettingsFromDisk(IO.IniFilename);
	}
}

void HGUIModule::RestoreLayoutWidgets()
{
	PString RestoredNames;
	for (const HPair<const PName, bool>& Pair : LayoutWidgetOpenStates)
	{
		if (Pair.second == false)
		{
			continue;
		}

		// 그 위젯을 가진 모듈이 이번 실행에 없으면 클래스가 없다(예: 프로젝트 없이 띄운 에디터). 항목은 남겨 둔다.
		const JGType WidgetType = GObjectGlobalSystem::GetInstance().GetType(Pair.first);
		if (GObjectGlobalSystem::GetInstance().IsRegisteredType(WidgetType) == false)
		{
			continue;
		}

		if (Widgets.contains(WidgetType) == false)
		{
			PSharedPtr<JGWidget> Widget = RawDynamicCast<JGWidget>(GObjectGlobalSystem::GetInstance().NewObject(WidgetType));
			if (Widget == nullptr)
			{
				continue;
			}
			Widget->Initialize();
			Widgets[WidgetType] = Widget;
		}

		if (Widgets[WidgetType]->IsOpen() == false)
		{
			OpenWidgetInternal(Widgets[WidgetType]);
			RestoredNames += PString::Format("%s ", Pair.first.ToString());
		}
	}

	if (RestoredNames.Empty() == false)
	{
		JG_LOG(GUI, ELogLevel::Info, "Layout: reopened %s", RestoredNames);
	}
}

void HGUIModule::UpdateLayoutWidgetStates()
{
	for (const HPair<const JGType, PSharedPtr<JGWidget>>& Pair : Widgets)
	{
		const PName WidgetName = Pair.first.GetName();
		const bool  bOpen      = Pair.second->IsOpen();

		HHashMap<PName, bool>::iterator Iter = LayoutWidgetOpenStates.find(WidgetName);
		if (Iter != LayoutWidgetOpenStates.end() && Iter->second == bOpen)
		{
			continue;
		}

		LayoutWidgetOpenStates[WidgetName] = bOpen;
		ImGui::MarkIniSettingsDirty();
	}
}

bool HGUIModule::IsWidgetClosedInLayout(const JGType& InWidgetType) const
{
	HHashMap<PName, bool>::const_iterator Iter = LayoutWidgetOpenStates.find(InWidgetType.GetName());
	return Iter != LayoutWidgetOpenStates.end() && Iter->second == false;
}