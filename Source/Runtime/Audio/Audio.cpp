#include "PCH/PCH.h"
#include "Audio.h"
#include "AudioGlobalSystem.h"

namespace
{
	// 모듈이 연결돼 있고 종료 전일 때만 엔진을 준다. 아니면 nullptr — 부르는 쪽은 아무 일도 하지 않는다.
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
}

HAudioHandle HAudio::Play(const PString& soundPath, const HAudioPlayArguments& args)
{
	HAudioEngine* engine = activeEngine();
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
	HAudioEngine* engine = activeEngine();
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

void HAudio::SetBusVolume(EAudioBus bus, float32 volume)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return;
	}
	engine->SetBusVolume(bus, volume);
}

float32 HAudio::GetBusVolume(EAudioBus bus)
{
	HAudioEngine* engine = activeEngine();
	if (engine == nullptr)
	{
		return 0.0f;
	}
	return engine->GetBusVolume(bus);
}

void HAudio::SetBackgroundMode(EAudioBackgroundMode mode, float32 lowerVolume)
{
	if (GAudioGlobalSystem::IsValid() == false || GAudioGlobalSystem::GetInstance().IsActive() == false)
	{
		return;
	}
	GAudioGlobalSystem::GetInstance().SetBackgroundMode(mode, lowerVolume);
}

bool HAudio::Preload(const PString& soundPath)
{
	HAudioEngine* engine = activeEngine();
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
