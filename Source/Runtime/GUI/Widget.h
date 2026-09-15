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

enum class GUI_API EWidgetFlags
{
	None = 0,
	AllowAlwaysUpdate = 0x001,
	AllowAlwaysUpdateFrame  = 0x002,
	AllowUpdate = 0x004,
	AllowUpdateFrame = 0x008,
};
JG_ENUM_FLAG(EWidgetFlags)

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

	
	// Flags 및 윈도우 사이즈 
	EWidgetFlags WidgetFlags;

protected:
	virtual void OnInitialize() {}
	virtual void OnShutdown() {}
	virtual void OnOpen() {}
	virtual void OnClose() {}
	virtual void OnUpdate() {}
	virtual void OnUpdateFrame() {}
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
	EWidgetFlags GetFlags() const { return WidgetFlags; }

private:
	void SetupLayout();
	void GenerateGUI();
	void Update();
	void UpdateFrame();
	void Open();
	void Close();
	void Initialize();
	void Shutdown();
};

