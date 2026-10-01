#pragma once
#include "GUIDefines.h"
#include "Misc/Module.h"
#include "Menu/MenuTree.h"
#include "Widget.h"

class JGWidget;
class PGUIBackend;

// 레이아웃 저장: 창 위치 · 크기 · 도킹은 ImGui 가 imgui.ini 에 저장한다. 어떤 위젯이 열려 있었는지는 이 모듈이 같은 파일의
// [JGWidget][Open] 절에 위젯 클래스 이름으로 저장하고(예: JGDevConsole=1), 다음 실행의 첫 프레임에 리플렉션으로 다시 연다.
class GUI_API HGUIModule : public IModuleInterface
{

    PSharedPtr<PGUIBackend> GUIBackend;
    HMainMenuTree MainMenuTree;

    // 위젯 클래스 타입마다 하나. 키가 클래스라서 저장된 클래스 이름으로 다시 만들 수 있다.
    HHashMap<JGType, PSharedPtr<JGWidget>> Widgets;

    // 저장된 레이아웃의 열림 상태(위젯 클래스 이름 → 열림). 매 프레임 열린 위젯으로 갱신하고, 이번 실행에 없는 위젯(다른 모듈)의 항목도 남긴다.
    HHashMap<PName, bool> LayoutWidgetOpenStates;
    bool bLayoutWidgetsRestored = false;

protected:
    JGType GetModuleType() const override;

    void StartupModule() override;
    void ShutdownModule() override;
    
    void GenerateMainMenuGUI();
    void GenerateWidgetGUI();

public:
    // 메뉴 추가
    void AddMainMenuItem(const HMainMenuItem& InMainMenuItem);

    // 이름 / Action
    // Context 메뉴 추가

    // 위젯 추가
    template<class T>
    bool OpenWidget()
    {
        const JGType WidgetType = JGTYPE(T);
        if (Widgets.contains(WidgetType) == false)
        {
            Widgets[WidgetType] = Allocate<T>();
            Widgets[WidgetType]->Initialize();
        }

       bool bResult =  OpenWidgetInternal(Widgets[WidgetType]);
       return bResult;
    }

    // 기본으로 여는 창(호스트가 시작할 때 여는 창). 저장된 레이아웃에서 사용자가 닫아 둔 창이면 열지 않는다. 저장된 상태가 없으면 연다.
    template<class T>
    bool OpenWidgetByDefault()
    {
        if (IsWidgetClosedInLayout(JGTYPE(T)) == true)
        {
            return false;
        }

        return OpenWidget<T>();
    }

    template<class T>
    void CloseWidget()
    {
        const JGType WidgetType = JGTYPE(T);
		if (Widgets.contains(WidgetType) == false)
		{
            return;
		}

        CloseWidgetInternal(Widgets[WidgetType]);
    }

    template<class T>
    PSharedPtr<T> FindWidget() const
    {
        const JGType WidgetType = JGTYPE(T);
		if (Widgets.contains(WidgetType))
		{
			return Cast<T>(Widgets.at(WidgetType));
		}

        return nullptr;
    }

    PWeakPtr<PGUIBackend> GetGUIBackend() const;
private:
    bool OpenWidgetInternal(PSharedPtr<JGWidget> InWidget);
    void CloseWidgetInternal(PSharedPtr<JGWidget> InWidget);
    void UpdateWidgets();

    // 레이아웃 저장 (위 클래스 주석)
    void RegisterLayoutSettings();
    void RestoreLayoutWidgets();
    void UpdateLayoutWidgetStates();
    bool IsWidgetClosedInLayout(const JGType& InWidgetType) const;
};

