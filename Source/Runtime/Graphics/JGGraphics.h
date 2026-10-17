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
	HDelegateHandle _windowResizeHandle;
protected:
	JGType GetModuleType() const override;

	void StartupModule() override;
	void ShutdownModule() override;

private:
	void BeginFrame();
	void EndFrame();
	// 메인 창 크기 변경(WM_SIZE) → 스왑체인. GUI 가 없는 호스트(게임 실행)에서도 돈다. (GP-G1)
	void OnWindowResize(uint32 inWidth, uint32 inHeight);
public:
	PSharedPtr<PJGGraphicsAPI> GetGraphicsAPI() const;
};

inline GRAPHICS_API PJGGraphicsAPI& GetGraphicsAPI();

