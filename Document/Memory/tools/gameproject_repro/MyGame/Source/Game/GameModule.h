#pragma once
#include "GameDefines.h"
#include "Misc/Module.h"

// 인게임 전용 모듈. 게임 규칙 · 액터 · 게임 인스턴스를 둔다.
class GAME_API HGameModule : public IModuleInterface
{
protected:
	virtual JGType GetModuleType() const override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
