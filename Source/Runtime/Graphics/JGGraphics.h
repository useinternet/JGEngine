#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsAPI.h"
#include "JGGraphicsCommand.h"
#include "Misc/Module.h"


class PJGGraphicsAPI;
class PRenderSelfTest;
class GRAPHICS_API HJGGraphicsModule : public IModuleInterface
{

	PSharedPtr<PJGGraphicsAPI> _graphicsAPI;
	HDelegateHandle _windowResizeHandle;
	// 렌더 결과 자체 시험(graphics.selftest). 마지막에 돈 시험을 들고 있다가 다음 시작 · 모듈 종료 때 놓는다.
	PSharedPtr<PRenderSelfTest> _renderSelfTest;
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
	// graphics.selftest 를 시작한다. 결과는 끝날 때 로그(OK/FAILED)로 나온다. 이미 돌고 있거나 API 가 없으면 false.
	bool StartRenderSelfTest(bool bExitWhenDone);
};

inline GRAPHICS_API PJGGraphicsAPI& GetGraphicsAPI();

