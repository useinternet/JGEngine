#include "PCH/PCH.h"
#include "Audio.h"
#include "AudioGlobalSystem.h"

namespace
{
	// 장치가 열려 있고 종료 전일 때만 엔진을 준다. 아니면 nullptr — 부르는 쪽은 아무 일도 하지 않는다. 장치를 열지 않는다(멈춤 · 묻기).
	HAudioEngine* activeEngine()
	{
		if (GAudioGlobalSystem::IsValid() == false)
		{
			return nullptr;
		}

		GAudioGlobalSystem& system = GAudioGlobalSystem::GetInstance();
		if (system.IsActive() == false)
		{
			return nullptr;
		}
		return &system.GetEngine();
	}

	// 처음 부르면 장치를 연다(EnsureInitialized). 소리를 내는 함수(Play · PlayMusic · Preload)만 쓴다.
	HAudioEngine* engineForSound()
	{
		if (GAudioGlobalSystem::IsValid() == false)
		{
			return nullptr;
		}

		GAudioGlobalSystem& system = GAudioGlobalSystem::GetInstance();
		if (system.EnsureInitialized() == false)
		{
			return nullptr;
		}
		return &system.GetEngine();
	}
}

HAudioHandle HAudio::Play(const PString& soundPath, const HAudioPlayArguments& args)
{
	HAudioEngine* engine = engineForSound();
	if (engine == nullptr)
	{
		return HAudioHandle();
	}
	return engine->Play(soundPath, args);
}

void HAudio::Stop(HAudioHandle handle, float32 fadeSeconds)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return;
	}
	engine->Stop(handle, fadeSeconds);
}

void HAudio::Pause(HAudioHandle handle)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return;
	}
	engine->Pause(handle);
}

void HAudio::Resume(HAudioHandle handle)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return;
	}
	engine->Resume(handle);
}

bool HAudio::IsPlaying(HAudioHandle handle)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return false;
	}
	return engine->IsPlaying(handle);
}

void HAudio::SetVolume(HAudioHandle handle, float32 volume)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return;
	}
	engine->SetVolume(handle, volume);
}

HAudioHandle HAudio::PlayMusic(const PString& soundPath, float32 fadeSeconds, float32 volume)
{
	HAudioEngine* engine = engineForSound();
	if (engine == nullptr)
	{
		return HAudioHandle();
	}
	return engine->PlayMusic(soundPath, fadeSeconds, volume);
}

void HAudio::StopMusic(float32 fadeSeconds)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return;
	}
	engine->StopMusic(fadeSeconds);
}

// 크기 · 창 뒤 설정은 장치가 닫혀 있어도 값을 저장한다 — 엔진 · 시스템이 장치를 열 때 적용한다(장치를 열지 않는다).
void HAudio::SetBusVolume(EAudioBus bus, float32 volume)
{
	if (GAudioGlobalSystem::IsValid() == false)
	{
		return;
	}
	GAudioGlobalSystem::GetInstance().GetEngine().SetBusVolume(bus, volume);
}

float32 HAudio::GetBusVolume(EAudioBus bus)
{
	if (GAudioGlobalSystem::IsValid() == false)
	{
		return 0.0f;
	}
	return GAudioGlobalSystem::GetInstance().GetEngine().GetBusVolume(bus);
}

void HAudio::SetBackgroundMode(EAudioBackgroundMode mode, float32 lowerVolume)
{
	if (GAudioGlobalSystem::IsValid() == false)
	{
		return;
	}
	GAudioGlobalSystem::GetInstance().SetBackgroundMode(mode, lowerVolume);
}

bool HAudio::Preload(const PString& soundPath)
{
	HAudioEngine* engine = engineForSound();
	if (engine == nullptr)
	{
		return false;
	}
	return engine->Preload(soundPath);
}

void HAudio::StopAll(float32 fadeSeconds)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return;
	}
	engine->StopAll(fadeSeconds);
}

bool HAudio::IsDeviceActive()
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return false;
	}
	return engine->IsDeviceActive();
}

int32 HAudio::GetPlayingCount()
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return 0;
	}
	return engine->GetPlayingCount();
}
