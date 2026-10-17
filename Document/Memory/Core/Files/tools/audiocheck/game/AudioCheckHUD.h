#pragma once
#include "AudioCheckDefines.h"
#include "UI/GameWidget.h"
#include "Audio.h"
#include "AudioCheckHUD.generation.h"

class PGameUIText;
class PGameUIButton;

// 오디오 재생(D-37) 검증 화면(엔진 밖 검증 장치). 왼쪽 단추로 효과음 · 8번 겹치기 · 곡 A(wav)/B(ogg) · 음악 멈춤 · 음악 일시 멈춤 ·
// 분위기 소리 · 묶음 크기(Master · Music · Effects 100/50/0) · 창이 뒤로 갔을 때 모드를 바꾼다. 위 글자에 장치 · 재생 수 · 묶음 크기.
// -audiocheck 로 띄우면 정해진 순서를 저절로 돌며 단계마다 "AUDIOCHECK <epoch ms> t=<초> …" 로그를 남기고 끝나면 종료한다
// (실제 출력 녹음 도구 audio_loopback_rms 의 시각과 맞춰 본다 — Document/Memory/Core/Files/tools/audiocheck/README.md).
JGCLASS()
class AUDIOCHECK_API JGAudioCheckHUD : public JGGameWidget
{
	JG_GENERATED_CLASS_BODY

private:
	PSharedPtr<PGameUIText>   _status;
	PSharedPtr<PGameUIButton> _effectsButton;
	PSharedPtr<PGameUIButton> _musicButton;
	PSharedPtr<PGameUIButton> _masterButton;
	PSharedPtr<PGameUIButton> _backgroundButton;
	PSharedPtr<PGameUIButton> _pauseButton;

	HAudioHandle         _music;
	HAudioHandle         _rain;
	bool                 _bMusicPaused   = false;
	EAudioBackgroundMode _backgroundMode = EAudioBackgroundMode::Keep;

	// 8번 겹치기: 40ms 간격
	int32   _burstRemaining = 0;
	float32 _burstTimer     = 0.0f;

	// -audiocheck 자동 순서
	bool    _bScript    = false;
	int32   _scriptStep = 0;
	float32 _clock      = 0.0f;

public:
	JGAudioCheckHUD() = default;
	virtual ~JGAudioCheckHUD() = default;

protected:
	virtual void OnInitialize() override;
	virtual void OnUpdate(float32 deltaSeconds) override;

private:
	PSharedPtr<PGameUIButton> addButton(const char* name, const PString& label, int32 row);
	void playEffect();
	void startBurst();
	void playMusic(bool bTrackB);
	void stopMusic();
	void toggleRain();
	void toggleMusicPause();
	void cycleBus(EAudioBus bus);
	void cycleBackgroundMode();
	void updateLabels();
	void runScript();
	void logStep(const char* step);
};
