#pragma once
#include "GameUIDemoDefines.h"
#include "Misc/Module.h"

// 게임 UI(JGGameWidget, GameFrameWorks UI/) 검증 게임(엔진 밖). 월드(X Bot + 궤도 카메라) 위에 HUD 위젯을 Game 레이어에 올린다.
// HUD 의 Menu 버튼은 일시정지 메뉴 위젯을 Menu 레이어에 올린다(입력 모드 Menu, Esc 로 닫힘).
class GAMEUIDEMO_API HGameUIDemoModule : public IModuleInterface
{
protected:
	virtual JGType GetModuleType() const override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
