#pragma once
#include "GUIDefines.h"
#include "Misc/Module.h"
#include "Menu/MenuTree.h"
#include "Widget.h"

class JGWidget;
class PGUIBackend;

class GUI_API HGUIModule : public IModuleInterface
{

    PSharedPtr<PGUIBackend> GUIBackend;
    HMainMenuTree MainMenuTree;

    HHashMap<HGuid, PSharedPtr<JGWidget>> Widgets;

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
        HGuid WidgetGUID = T::GetStaticGUID();
        if (Widgets.contains(WidgetGUID) == false)
        {
            Widgets[WidgetGUID] = Allocate<T>();
            Widgets[WidgetGUID]->Initialize();
        }

       bool bResult =  OpenWidgetInternal(Widgets[WidgetGUID]);
       return bResult;
    }

    template<class T>
    void CloseWidget()
    {
        HGuid WidgetGUID = T::GetStaticGUID();
		if (Widgets.contains(WidgetGUID) == false)
		{
            return;
		}

        CloseWidgetInternal(Widgets[WidgetGUID]);
    }

    template<class T>
    PSharedPtr<T> FindWidget() const
    {
        HGuid WidgetGUID = T::GetStaticGUID();
		if (Widgets.contains(WidgetGUID))
		{
			return Cast<T>(Widgets.at(WidgetGUID));
		}

        return nullptr;
    }

    PWeakPtr<PGUIBackend> GetGUIBackend() const;
private:
    bool OpenWidgetInternal(PSharedPtr<JGWidget> InWidget);
    void CloseWidgetInternal(PSharedPtr<JGWidget> InWidget);
    void UpdateWidgets();
    void UpdateFrameWidgets();
};

