#pragma once
#include "Core.h"
#include "GUIDefines.h"
#include "WidgetComponent.generation.h"


#define JG_GENERATED_WIDGETCOMPONENT_BODY JG_GENERATED_CLASS_BODY

JGCLASS()
class GUI_API JGWidgetComponent : public JGObject
{
	JG_GENERATED_WIDGETCOMPONENT_BODY

	friend class JGWidget;

	JGPROPERTY()
	HGuid GUID;

protected:
	virtual void OnInitialize() {}
	virtual void OnShutdown() {}
	virtual void OnOpen() {}
	virtual void OnClose() {}
	virtual void OnGenerateGUI() {}

	const  HGuid& GetGUID() const { return GUID; }
private:
	void Initialize();
	void Shutdown();

	virtual void Construct() override;
public:
	void GenerateGUI();
};

