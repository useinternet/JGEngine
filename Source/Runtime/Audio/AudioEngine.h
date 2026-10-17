#pragma once
#include "Core.h"
#include "AudioDefines.h"
#include <vector>

// 소리를 내보낼 곳
enum class EAudioOutput : uint8
{
	Device,       // 기본 장치(WASAPI → DirectSound → WinMM 순, 장치가 없으면 miniaudio 가 무음 장치로 연다)
	NullDevice,   // 무음 장치 강제(-nosound · 창 없는 실행). 실제 시간대로 돌고 소리만 나지 않는다
	Manual        // 장치 없음. ReadOutput 을 부를 때만 시간이 간다(자체 검사 audio.selftest)
};

struct HAudioEngineArguments
{
	EAudioOutput Output     = EAudioOutput::Device;
	uint32       SampleRate = 0;   // Manual 일 때만(0 이면 48000)
	uint32       Channels   = 0;   // Manual 일 때만(0 이면 2)
};

// miniaudio 엔진 하나를 감싼 섞개(믹서). 모듈 밖으로 내보내지 않는다 — 게임은 HAudio 를 쓴다.
// GAudioGlobalSystem 이 장치 출력용 하나를 갖고, 자체 검사는 Manual 출력용을 따로 만들어 섞은 결과를 직접 읽는다.
// 메인 스레드 전용이다. miniaudio 의 오디오 스레드 · 작업 스레드는 이 클래스의 파일 갈고리(메모리에서 읽기)만 부른다.
class HAudioEngine
{
public:
	HAudioEngine();
	~HAudioEngine();
	HAudioEngine(const HAudioEngine&) = delete;
	HAudioEngine& operator=(const HAudioEngine&) = delete;

	bool Initialize(const HAudioEngineArguments& args);
	// 모든 소리 · 장치를 놓는다. 여러 번 불러도 된다. 그 뒤의 부르기는 아무 일도 하지 않는다.
	void Shutdown();
	bool IsInitialized() const;
	// 실제 출력 장치로 나가는가(Manual · 무음 장치면 false)
	bool IsDeviceActive() const;
	// 로그용 "<장치 이름> (<backend>)"
	PString GetDeviceDescription() const;

	HAudioHandle Play(const PString& soundPath, const HAudioPlayArguments& args);
	void Stop(HAudioHandle handle, float32 fadeSeconds);
	void Pause(HAudioHandle handle);
	void Resume(HAudioHandle handle);
	bool IsPlaying(HAudioHandle handle) const;
	void SetVolume(HAudioHandle handle, float32 volume);

	HAudioHandle PlayMusic(const PString& soundPath, float32 fadeSeconds, float32 volume);
	void StopMusic(float32 fadeSeconds);
	HAudioHandle GetMusicHandle() const;

	void    SetBusVolume(EAudioBus bus, float32 volume);
	float32 GetBusVolume(EAudioBus bus) const;
	// 묶음 크기와 따로, 전체에 곱하는 크기를 seconds 동안 바꾼다(창이 뒤로 갔을 때 줄이기 · 끄기)
	void FadeOutputGain(float32 gain, float32 seconds);

	bool  Preload(const PString& soundPath);
	void  StopAll(float32 fadeSeconds);
	int32 GetPlayingCount() const;

	// 끝난 소리와 페이드로 멈춘 소리를 놓는다. 메인 스레드에서 매 프레임 부른다(GAudioGlobalSystem::Update).
	void Update();

	// 자체 검사용
	// Manual 출력만: 섞은 결과를 frameCount 프레임 읽는다(outFrames 는 frameCount × 채널 수 개의 float).
	bool   ReadOutput(float32* outFrames, uint64 frameCount);
	uint32 GetSampleRate() const;
	uint32 GetChannels() const;
	// 소리 파일 내용을 디스크 대신 메모리에서 준다(soundPath 는 Play 에 줄 토큰 경로).
	bool   RegisterMemoryFile(const PString& soundPath, const std::vector<uint8>& bytes);

private:
	struct HImpl;
	HImpl* _impl;
};
