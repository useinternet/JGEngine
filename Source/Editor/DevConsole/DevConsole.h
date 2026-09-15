#pragma once
#include "Widget.h"
#include "WidgetComponent.h"
#include "DevConsoleDefines.h"
// PString <= Ύξ¶² Έν
//

#include "DevConsole.generation.h"


JG_DECLARE_EVENT(HOnDevConsole, JGDevConsole, HDevConsoleArguments)

JGCLASS()
class JGDevConsole : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

private:
	PString CommandText;

	HOnDevConsole OnDevConsole;
public:
	virtual void OnInitialize() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnGenerateGUI() override;


	virtual PString  GetTitleName() const override;
	virtual const  HGuid& GetGUID() const override;

public:
	HDelegateHandle RegisterConsoleCommand(HOnDevConsole::DelegateT Delegate);
	void UnRegisterConsoleCommand(HDelegateHandle& InHandle);
};




