#pragma once
#include "Core.h"
#include "AudioDefines.h"

/*
소리 재생(효과음 · 배경음) — 오디오 재생(D-37, 요청 ER-022). miniaudio 0.11.25 를 감싼다(Source/ThirdParty/miniaudio).

쓰는 법(게임 모듈)
	- #include "Audio/Audio.h" — GameFrameWork 에 들어 있어 module.json 의존 · ConnectModule 이 따로 필요 없다
	- 소리 이름은 Content 토큰 경로 + 확장자: "/JGGame/Sounds/UI/Click.wav", "/JGEngine/…" (wav · ogg. mp3 · flac 도 풀린다)
	- 효과음:  HAudio::Play("/JGGame/Sounds/Card/Draw.wav");   같은 소리를 겹쳐 내도 된다
	- 배경음:  HAudio::PlayMusic("/JGGame/Sounds/Music/Night.ogg", 2.0f);   앞 곡은 2초 동안 줄며 멈추고 새 곡은 2초 동안 커지며 되풀이한다
	- 분위기 소리를 하나 더 겹치기:
		HAudioPlayArguments rain;
		rain.Bus = EAudioBus::Music;  rain.bLoop = true;  rain.bStream = true;  rain.FadeInSeconds = 2.0f;
		HAudioHandle rainHandle = HAudio::Play("/JGGame/Sounds/Ambience/Rain.ogg", rain);
	- 크기:    HAudio::SetBusVolume(EAudioBus::Effects, 0.8f);   설정 저장 · 불러오기는 게임 몫(게임 인스턴스 · Saved/)
	- 창이 뒤로 가면: HAudio::SetBackgroundMode(EAudioBackgroundMode::Lower, 0.3f);

- 장치는 처음 소리를 낼 때(Play · PlayMusic · Preload) 연다. 크기(SetBusVolume) · 창 뒤 설정(SetBackgroundMode)은 그 전에 불러도 저장했다가 열 때 적용한다.
- 메인 스레드에서만 부른다. GameFrameWork 가 연결되지 않았거나 종료 중이면 아무 일도 하지 않는다(핸들은 무효).
- 소리 파일은 처음 쓸 때 HFileHelper::ReadAllBytes 로 통째 읽어 둔다(패키지 읽기 검사를 거친다). 첫 재생이 끊기면 Preload 로 미리 풀어 둔다.
- 장치가 없거나, -nosound 거나, 창 없는 실행(JGConsole)이면 무음 장치로 돈다 — API 는 그대로 동작하고 소리만 나지 않는다.
- 함수 이름이 PlaySound 가 아닌 것은 Windows 의 PlaySound 매크로와 부딪치지 않게 하려고다.
*/
class GAMEFRAMEWORK_API HAudio
{
public:
	// 소리 하나를 낸다. 경로가 Content 토큰 경로가 아니거나 파일이 없거나 못 풀면 무효 핸들(오류 로그는 경로마다 한 번).
	static HAudioHandle Play(const PString& soundPath, const HAudioPlayArguments& args = HAudioPlayArguments());
	// 멈추고 놓는다. fadeSeconds > 0 이면 그동안 줄인 뒤 멈춘다.
	static void Stop(HAudioHandle handle, float32 fadeSeconds = 0.0f);
	static void Pause(HAudioHandle handle);
	static void Resume(HAudioHandle handle);
	// 재생 중이면(일시 멈춤 포함) true. 끝났거나 멈춘 소리는 false.
	static bool IsPlaying(HAudioHandle handle);
	static void SetVolume(HAudioHandle handle, float32 volume);

	// 배경음 한 칸: Music 묶음 · 되풀이 · 흘려 풀기. 다른 곡이 돌고 있으면 fadeSeconds 동안 줄이며 멈추고, 새 곡은 같은 시간 동안 키운다.
	// 같은 곡이 이미 돌고 있으면 그대로 두고 그 핸들을 돌려준다.
	static HAudioHandle PlayMusic(const PString& soundPath, float32 fadeSeconds = 1.0f, float32 volume = 1.0f);
	static void StopMusic(float32 fadeSeconds = 1.0f);

	static void    SetBusVolume(EAudioBus bus, float32 volume);
	static float32 GetBusVolume(EAudioBus bus);

	// 창(앱)이 뒤로 갔을 때: Keep 그대로 · Lower 전체 크기를 lowerVolume 배로 · Mute 끄기. 앞으로 돌아오면 원래대로(0.25초에 걸쳐).
	static void SetBackgroundMode(EAudioBackgroundMode mode, float32 lowerVolume = 0.3f);

	// 짧은 소리를 미리 풀어 둔다(첫 재생 끊김 막기). 파일이 없거나 못 풀면 false.
	static bool Preload(const PString& soundPath);
	static void StopAll(float32 fadeSeconds = 0.0f);

	// 실제 출력 장치로 나가는가(무음 장치 · 장치 없음 · -nosound 면 false)
	static bool  IsDeviceActive();
	// 지금 재생 중인(일시 멈춤 포함) 소리 수
	static int32 GetPlayingCount();
};
