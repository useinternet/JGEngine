#include "PCH/PCH.h"
#include "DevConsole.h"
#include "GUI.h"

void JGDevConsole::OnInitialize()
{

}

void JGDevConsole::OnLayout(const HWidgetLayout& InLayout)
{

}

void JGDevConsole::OnGenerateGUI()
{
	if (HGUI::InputText("Cmd", CommandText))
	{
		HDevConsoleArguments Args = HDevConsoleArguments(CommandText);
		OnDevConsole.BroadCast(Args);

		CommandText.Reset();
	}
}


PString  JGDevConsole::GetTitleName() const
{
	return "DevConsole";
}

const  HGuid& JGDevConsole::GetGUID() const
{
	return GetStaticGUID();
}

HDelegateHandle JGDevConsole::RegisterConsoleCommand(HOnDevConsole::DelegateT Delegate)
{
	return OnDevConsole.Add(std::move(Delegate));
}

void JGDevConsole::UnRegisterConsoleCommand(HDelegateHandle& InHandle)
{
	OnDevConsole.Remove(InHandle);
}
