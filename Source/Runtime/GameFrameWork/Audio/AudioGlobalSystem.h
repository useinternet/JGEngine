#pragma once
#include "Core.h"
#include "AudioDefines.h"
#include "AudioEngine.h"

// 소리 기능의 전역 시스템. 장치 출력용 HAudioEngine 하나와 창 비활성 처리를 갖는다. GameFrameWork 밖으로 내보내지 않는다(게임은 HAudio).
// HGameFrameWorkModule::StartupModule 이 등록만 하고 장치는 열지 않는다 — 처음 소리를 낼 때(HAudio::Play · PlayMusic · Preload)
// EnsureInitialized 가 연다. 그래서 GameFrameWork 를 늘 켜는 에디터도 소리를 내기 전에는 장치를 열지 않는다.
// 종료: GCoreSystem::Destroy 는 시스템 Destroy 를 모든 모듈 ShutdownModule 보다 먼저 부른다 → Destroy 에서 장치 · 소리를 닫고,
// 그 뒤 게임 모듈이 종료 중에 HAudio 를 불러도 아무 일도 하지 않는다(IsActive false).
class GAudioGlobalSystem final : public GGlobalSystemInstance<GAudioGlobalSystem>
{
public:
	GAudioGlobalSystem() = default;

	// 처음 부를 때만 장치를 연다. 한 번 실패하면 다시 시도하지 않는다. 열려 있으면 true.
	bool EnsureInitialized();
	// 여러 번 불러도 된다(Destroy · ShutdownModule 둘 다 부른다)
	void Shutdown();
	bool IsActive() const;

	HAudioEngine& GetEngine();
	void SetBackgroundMode(EAudioBackgroundMode mode, float32 lowerVolume);

protected:
	virtual ~GAudioGlobalSystem();

	virtual void Update() override;
	virtual void Destroy() override;

private:
	void initialize();
	void onWndProc(HWND hWnd, uint32 message, WPARAM wParam, LPARAM lParam);
	void applyBackgroundGain(float32 seconds);

private:
	HAudioEngine         _engine;
	HDelegateHandle      _wndProcHandle;
	EAudioBackgroundMode _backgroundMode   = EAudioBackgroundMode::Keep;
	float32              _backgroundVolume = 0.3f;
	bool                 _bAppActive       = true;
	bool                 _bInitializeTried = false;
	bool                 _bShutdown        = false;
};
