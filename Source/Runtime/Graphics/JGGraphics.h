#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsAPI.h"
#include "JGGraphicsCommand.h"
#include "Misc/Module.h"


class PJGGraphicsAPI;
class GRAPHICS_API HJGGraphicsModule : public IModuleInterface
{

	PSharedPtr<PJGGraphicsAPI> _graphicsAPI;
protected:
	JGType GetModuleType() const override;

	void StartupModule() override;
	void ShutdownModule() override;

private:
	void BeginFrame();
	void EndFrame();
public:
	PSharedPtr<PJGGraphicsAPI> GetGraphicsAPI() const;
};

inline GRAPHICS_API PJGGraphicsAPI& GetGraphicsAPI();

