#pragma once
#include "Core.h"
#include "Misc/Module.h"

#ifdef _JGDEV_GRAPHICS
#define JGDEV_GRAPHICS_API __declspec(dllexport)
#define JGDEV_GRAPHICS_C_API extern "C" __declspec(dllexport)
#else
#define JGDEV_GRAPHICS_API __declspec(dllimport)
#define JGDEV_GRAPHICS_C_API extern "C" __declspec(dllimport)
#endif

#include "JGDevGraphicsTest.h"

class PJWindow;
class HMenuBuilder;
class JGDEV_GRAPHICS_API HJGDev_GraphicsModule : public IModuleInterface
{
	PSharedPtr<PJWindow> _window;
	HJGDevGraphicsTest _test;
public:
	virtual ~HJGDev_GraphicsModule();

protected:
	virtual JGType GetModuleType() const override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

