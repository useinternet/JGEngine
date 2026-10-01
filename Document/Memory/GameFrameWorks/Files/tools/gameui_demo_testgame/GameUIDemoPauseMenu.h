#pragma once
#include "GameUIDemoDefines.h"
#include "UI/GameWidget.h"
#include "GameUIDemoPauseMenu.generation.h"

// 게임 UI 검증 게임의 일시정지 메뉴(Menu 레이어, 입력 모드 Menu, 뒤로가기 처리).
// 떠 있는 동안 아래 HUD 버튼 · 월드 클릭이 막힌다. Resume 버튼이나 Esc(뒤로가기)로 닫는다.
JGCLASS()
class GAMEUIDEMO_API JGGameUIDemoPauseMenu : public JGGameWidget
{
	JG_GENERATED_CLASS_BODY

public:
	JGGameUIDemoPauseMenu() = default;
	virtual ~JGGameUIDemoPauseMenu() = default;

protected:
	virtual void OnInitialize() override;
	virtual void OnActivated() override;
	virtual void OnShutdown() override;
};
