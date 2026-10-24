#pragma once
#include "GameUIDemoDefines.h"
#include "UI/GameWidget.h"
#include "GameUIDemoPauseMenu.generation.h"

class PGameUIImage;

// 게임 UI 검증 게임의 일시정지 메뉴(Menu 레이어, 입력 모드 Menu, 뒤로가기 처리).
// 떠 있는 동안 아래 HUD 버튼 · 월드 클릭이 막힌다. Resume 버튼이나 Esc(뒤로가기)로 닫는다.
// 열림 · 닫힘 전환(GG-16): 화면 전체(어두운 막 + 패널)가 흐려지며 나타나고(0.3초) 사라진다(0.4초). 패널은 열릴 때 40 아래에서 올라오고
// 닫힐 때 내려간다(OnActivated · OnDeactivated 에서 건 트윈 — 닫히는 동안에도 돈다). 닫히는 동안은 입력 · 뒤로가기를 받지 않는다.
JGCLASS()
class GAMEUIDEMO_API JGGameUIDemoPauseMenu : public JGGameWidget
{
	JG_GENERATED_CLASS_BODY

private:
	PSharedPtr<PGameUIImage> _panel;

public:
	JGGameUIDemoPauseMenu() = default;
	virtual ~JGGameUIDemoPauseMenu() = default;

protected:
	virtual void OnInitialize() override;
	virtual void OnActivated() override;
	virtual void OnDeactivated() override;
	virtual void OnShutdown() override;
};
