#pragma once
#include "Core.h"
#include "GUIDefines.h"
#include "WidgetComponent.generation.h"

#define JG_GENERATED_WIDGETCOMPONENT_BODY JG_GENERATED_CLASS_BODY

class JGWidget;
enum class GUI_API EWidgetComponentFlags
{
	None = 0,
	AutoSize = 0x000001,
	Border   = 0x000002,
};
JG_ENUM_FLAG(EWidgetComponentFlags)

struct HWidgetComponentLayout
{
	HVector2 ContentSize;
};


JGCLASS()
class GUI_API JGWidgetComponent : public JGObject
{
	JG_GENERATED_WIDGETCOMPONENT_BODY

	friend JGWidget;

	JGPROPERTY()
	HGuid GUID;

protected:
	EWidgetComponentFlags WidgetComponentFlags;

public:
	HAttribute<HVector2> WidgetComponentSize;

protected:
	virtual void OnInitialize() {}
	virtual void OnShutdown() {}
	virtual void OnOpen()  {}
	virtual void OnClose() {}
	virtual void OnUpdate() {}
	virtual void OnUpdateFrame() {}
	virtual void OnLayout(const HWidgetComponentLayout& InLayout) {}
	virtual void OnGenerateGUI() {}

	const  HGuid& GetGUID() const { return GUID; }
	EWidgetComponentFlags GetFlags() const { return WidgetComponentFlags; }
private:
	void Initialize();
	void Shutdown();
	
	virtual void Construct() override;
public:
	void SetupLayout(const HWidgetComponentLayout& InLayout);
	void GenerateGUI();
};

