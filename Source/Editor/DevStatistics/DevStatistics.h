#pragma once
#include "Core.h"
#include "Misc/Module.h"

#ifdef _DEVSTATISTICS
#define DEVSTATISTICS_API __declspec(dllexport)
#define DEVSTATISTICS_C_API extern "C" __declspec(dllexport)
#else
#define DEVSTATISTICS_API __declspec(dllimport)
#define DEVSTATISTICS_C_API extern "C" __declspec(dllimport)
#endif

class DEVSTATISTICS_API HDevStatisticsModule : public IModuleInterface
{

public:
	virtual ~HDevStatisticsModule();

protected:
	virtual JGType GetModuleType() const override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

