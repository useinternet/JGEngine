#pragma once
#include "Core.h"
#include "Misc/Module.h"

#ifdef _DEVKIT
#define DEVKIT_API __declspec(dllexport)
#define DEVKIT_C_API extern "C" __declspec(dllexport)
#else
#define DEVKIT_API __declspec(dllimport)
#define DEVKIT_C_API extern "C" __declspec(dllimport)
#endif


class PJWindow;
class HMenuBuilder;
class DEVKIT_API HDevKitModule : public IModuleInterface
{
public:
	virtual ~HDevKitModule();

protected:
	virtual JGType GetModuleType() const override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

