#pragma once
#include "Core.h"
#include "AudioDefines.h"
#include "Misc/Module.h"

// 오디오 재생(효과음 · 배경음) 모듈 — 오디오 재생(D-37). 소리를 쓰는 게임 모듈이 StartupModule 에서 ConnectModule("Audio") 로 연결한다.
class AUDIO_API HAudioModule : public IModuleInterface
{
protected:
	virtual JGType GetModuleType() const override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
