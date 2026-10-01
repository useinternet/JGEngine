#pragma once
#include "Core.h"
#include "GUIDefines.h"
#include "WidgetComponent.h"
#include "Widget.generation.h"

#define JG_GENERATED_WIDGET_BODY \
	JG_GENERATED_CLASS_BODY \
public: \
	static const HGuid& GetStaticGUID() \
	{ \
		static HGuid StaticGUID = HGuid::New(); \
		return StaticGUID; \
	} \
private: \

struct GUI_API HWidgetLayout
{
	HVector2 ContentSize;
};


JGCLASS()
class GUI_API JGWidget : public JGObject
{
	JG_GENERATED_WIDGET_BODY

	friend class HGUIModule;
private:

	JGPROPERTY()
	bool bOpen = false;

	JGPROPERTY()
	HList<PSharedPtr<JGWidgetComponent>> WidgetComponents;

protected:
	virtual void OnInitialize() {}
	virtual void OnShutdown() {}
	virtual void OnOpen() {}
	virtual void OnClose() {}
	// 열려 있는 동안 매 프레임 불린다. Update 단계라 GUI 생성(GraphicsBegin) 뒤, GUI 드로우(GraphicsEnd) 앞이다.
	virtual void OnUpdate() {}
	virtual void OnLayout(const HWidgetLayout& InLayout) {}
	virtual void OnGenerateGUI() {}

	virtual PString  GetTitleName() const;

	virtual const  HGuid& GetGUID() const;
protected:
	template<class T>
	PSharedPtr<T> MakeWidgetComponent()
	{
		PSharedPtr<T> WidgetComp = Allocate<T>();
		WidgetComponents.push_back(WidgetComp);
		return WidgetComp;
	}

	PSharedPtr<JGWidgetComponent> MakeWidgetComponent(PSharedPtr<JGClass> InClass);

public:
	bool IsOpen() const { return bOpen; }

private:
	void SetupLayout();
	void GenerateGUI();
	void Update();
	void Open();
	void Close();
	void Initialize();
	void Shutdown();
};

