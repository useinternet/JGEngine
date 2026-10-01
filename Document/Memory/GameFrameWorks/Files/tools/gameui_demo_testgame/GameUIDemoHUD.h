#pragma once
#include "GameUIDemoDefines.h"
#include "UI/GameWidget.h"
#include "GameUIDemoHUD.generation.h"

class PGameUIText;

// 게임 UI 검증 게임의 HUD(Game 레이어, 입력 모드 Game). 위 띠(패널 · 턴 글자 · 안내 · Menu 버튼), 카드 뒷면 2장, 손패 글자,
// 오른쪽 아래 주황 End Turn 버튼(누르면 턴 +1), 비활성 Draw 버튼(누르면 아무 일 없음 — 월드 클릭도 막는다).
// 오른쪽 패널: 자동 줄바꿈 문단(폭 600, 높이 = MeasureContent) + 말줄임 한 줄. 그 아래 입력란 2개(주소 · 이름, Enter 로 확정).
// Menu 버튼 → 일시정지 메뉴(JGGameUIDemoPauseMenu)를 Menu 레이어에 올린다.
JGCLASS()
class GAMEUIDEMO_API JGGameUIDemoHUD : public JGGameWidget
{
	JG_GENERATED_CLASS_BODY

private:
	PSharedPtr<PGameUIText> _turnText;
	int32                   _turn = 1;

public:
	JGGameUIDemoHUD() = default;
	virtual ~JGGameUIDemoHUD() = default;

	int32 GetTurn() const;

protected:
	virtual void OnInitialize() override;
	virtual void OnActivated() override;
	virtual void OnDeactivated() override;

private:
	void updateTurnText();
	void onEndTurnClicked();
	void onMenuClicked();
};
