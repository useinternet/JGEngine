#pragma once
#include "Core.h"
#include "AudioDefines.h"
#include "AudioEngine.h"

// 오디오 모듈의 전역 시스템. 장치 출력용 HAudioEngine 하나와 창 비활성 처리를 갖는다. 모듈 밖으로 내보내지 않는다(게임은 HAudio).
// HAudioModule::StartupModule 이 등록하고 Initialize 를 부른다 — JGConsole 처럼 Create 뒤에 연결하면 Start() 가 불리지 않아서다.
// 종료: GCoreSystem::Destroy 는 시스템 Destroy 를 모든 모듈 ShutdownModule 보다 먼저 부른다 → Destroy 에서 장치 · 소리를 닫고,
// 그 뒤 게임 모듈이 종료 중에 HAudio 를 불러도 아무 일도 하지 않는다(IsActive false).
class GAudioGlobalSystem final : public GGlobalSystemInstance<GAudioGlobalSystem>
{
public:
	GAudioGlobalSystem() = default;

	void Initialize();
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
	void onWndProc(HWND hWnd, uint32 message, WPARAM wParam, LPARAM lParam);
	void applyBackgroundGain(float32 seconds);

private:
	HAudioEngine         _engine;
	HDelegateHandle      _wndProcHandle;
	EAudioBackgroundMode _backgroundMode   = EAudioBackgroundMode::Keep;
	float32              _backgroundVolume = 0.3f;
	bool                 _bAppActive       = true;
	bool                 _bShutdown        = false;
};
