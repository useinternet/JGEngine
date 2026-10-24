#pragma once
#include "Core.h"
#include "Misc/Module.h"
#include "GameHostDefines.h"
#include "FileIO/PackageDescriptor.h"
#include "Input/InputEvent.h"

class PJWindow;
class PGameViewport;

// 게임만 띄우는 호스트(DevOps GP-D3, 2026-10-02 — 사용자 요청 "exe 를 실행하면 게임만, 화면 전체가 게임 화면").
// JGEditor 자리에 들어가는 LaunchModule 이다: 창 · Graphics · GameFrameWork · 게임 모듈만 연결하고(에디터 · GUI(ImGui) 없음),
// GameFrameWork 의 게임 뷰포트(PGameViewport, GP-F1)가 그린 월드 + 게임 UI 를 스왑체인에 바로 낸다.
//   패키지(JG_WITH_EDITOR 0): JGGameLaunch → GCoreSystem::Create(LaunchModule = GameHost). 설정은 Bin/JGPackage.json.
//   개발 실행: JGLauncher.exe -game. 설정은 .jgproject 의 GameModules · "Package"."Window".
// 창: 기본 1920×1080 클라이언트(사용자 지시) 창 모드. 모니터 작업 영역보다 크면 비율을 지켜 줄인다.
//   명령줄 -windowed · -fullscreen(테두리 없는 전체 화면) · -res=WxH 가 설정보다 앞선다.
// 프레임: GFW 가 Update 단계에 월드 · UI 를 갱신 → 이 모듈이 End 단계에 Render + 제출 → Graphics 가 GraphicsEnd 에 Present.
class GAMEHOST_API HGameHostModule : public IModuleInterface
{
	PSharedPtr<PJWindow>      _window;
	PSharedPtr<PGameViewport> _viewport;
	HList<PString>            _connectedEngineModules;   // 연결한 순서. 종료는 역순
	HList<PString>            _connectedGameModules;
	HGameWindowSettings       _windowSettings;

	HVector2Int _pendingSize;            // 창 크기 변경이 멈추면 뷰포트에 반영
	int32       _framesSinceResize = -1; // -1 = 반영할 것 없음
	bool        _bLoggedRenderProblem = false;

public:
	virtual ~HGameHostModule() = default;

protected:
	virtual JGType GetModuleType() const override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	bool loadSettings(HList<PString>* outGameModules);
	bool createWindow();
	void connectModules(const HList<PString>& gameModules);
	void renderFrame();
	void onResize(uint32 width, uint32 height);
	void onInput(const HInputEvent& event);
};
