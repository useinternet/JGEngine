#pragma once
#include "Core.h"
#include "GUIDefines.h"
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

enum class EWidgetFlags
{
	None = 0,

};
JG_ENUM_FLAG(EWidgetFlags)


class JGWidgetComponent;

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


public:
	bool IsOpen() const { return bOpen; }

private:
	void GenerateGUI();
	void Open();
	void Close();
	void Initialize();
	void Shutdown();
};

