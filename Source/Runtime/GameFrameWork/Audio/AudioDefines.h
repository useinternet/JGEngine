#pragma once
#include "Core.h"
#include "Core/GameFrameWorkDefines.h"

// 소리 묶음(버스). 묶음마다 크기를 정한다(HAudio::SetBusVolume). Master 는 Music · Effects 위의 전체 크기다.
enum class EAudioBus : uint8
{
	Master,
	Music,
	Effects,
	Count
};

// 창(앱)이 뒤로 갔을 때 소리를 어떻게 할지(HAudio::SetBackgroundMode). 기본은 Keep(그대로).
enum class EAudioBackgroundMode : uint8
{
	Keep,
	Lower,
	Mute
};

// 재생 하나를 가리킨다. 소리가 끝나거나 멈추면 무효가 되고, 같은 값은 다시 쓰이지 않는다.
struct HAudioHandle
{
	uint64 ID = 0;

	bool IsValid() const
	{
		return ID != 0;
	}

	bool operator==(const HAudioHandle& other) const
	{
		return ID == other.ID;
	}
};

// HAudio::Play 인자
struct HAudioPlayArguments
{
	EAudioBus Bus           = EAudioBus::Effects;   // Music 또는 Effects. Master 를 주면 Effects 로 낸다
	float32   Volume        = 1.0f;                 // 이 소리의 크기(0 이상). 묶음 크기와 곱한다
	float32   Pitch         = 1.0f;                 // 높낮이 · 빠르기(1 = 그대로, 2 = 한 옥타브 위 · 두 배 빠르게). 0 보다 커야 한다
	bool      bLoop         = false;                // 끝나면 처음부터 되풀이한다(멈출 때까지)
	float32   FadeInSeconds = 0.0f;                 // 0 에서 Volume 까지 서서히 키우는 시간
	bool      bStream       = false;                // 긴 소리(배경음 · 분위기 소리): 통째로 풀지 않고 조금씩 풀며 재생한다. 짧은 효과음은 false
};
