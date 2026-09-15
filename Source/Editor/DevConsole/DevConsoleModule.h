#pragma once
#include "Core.h"
#include "Misc/Module.h"
#include "DevConsoleDefines.h"
#include "DevConsole.h"

class HGUIModule;
class DEVCONSOLE_API HDevConsoleModule : public IModuleInterface
{
public:
	virtual ~HDevConsoleModule();

protected:
	virtual JGType GetModuleType() const override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	HDelegateHandle RegisterConsoleCommand(HOnDevConsole::DelegateT Delegate);
	void UnRegisterConsoleCommand(HDelegateHandle& InHandle);

private:
	HGUIModule* GetCheckedGUIModule() const;
};

