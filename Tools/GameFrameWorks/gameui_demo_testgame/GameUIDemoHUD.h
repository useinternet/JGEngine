#pragma once
#include "GameUIDemoDefines.h"
#include "UI/GameWidget.h"
#include "GameUIDemoHUD.generation.h"

class PGameUIElement;
class PGameUIImage;
class PGameUIText;
class PGameUIFont;
class PGameUIButton;
class PGameUITimeline;
class JGGameUIDemoPauseMenu;

// 게임 UI 검증 게임의 HUD(Game 레이어, 입력 모드 Game). 위 띠(패널 · 턴 글자 · 안내 · Menu 버튼), 카드 뒷면 2장, 손패 글자,
// 오른쪽 아래 주황 End Turn 버튼(누르면 턴 +1), 비활성 Draw 버튼(누르면 아무 일 없음 — 월드 클릭도 막는다).
// 오른쪽 패널: 자동 줄바꿈 문단(폭 600, 높이 = MeasureContent) + 말줄임 한 줄. 그 아래 입력란 2개(주소 · 이름, Enter 로 확정).
// 왼쪽 패널(입력은 월드로 넘긴다): 글 서식(ER-010 — 마크업 구간 서식, 양쪽 맞춤 문단, 요소 전체 서식),
// 글꼴(ER-011 — Arial + 맑은 고딕 대체, 따로 연 글꼴의 아틀라스 두 페이지에 걸친 글자).
// 요소 변환(GG-14): 첫 카드 뒷면은 그리기만 -5° 기울어 있다(배치 · 판정 자리는 기울인 모양).
// 끌어다 놓기(GG-15): 두 번째 카드 뒷면을 끌 수 있다. 끄는 동안 그리기 변환으로 포인터를 따라가며 기울고(4° · 1.05배 · 불투명도 0.85),
// 가운데 아래 놓을 곳(Drop zone) 위면 놓을 곳이 밝아진다. 놓을 곳에 놓으면 그 안에 남고, 다른 곳이면 원래 자리로 돌아간다.
// Menu 버튼 → 일시정지 메뉴(JGGameUIDemoPauseMenu)를 Menu 레이어에 올린다. 한 객체를 다시 올린다 — 닫히는 중이면 지금 모습에서 다시 열린다(GG-16).
// 움직임(GG-16): 처음 활성 때 카드 두 장이 왼쪽 밖에서 날아오고(옮기기 · 불투명도 트윈), End Turn 을 누르면 버튼이 눌렸다가 튕기며 돌아온다.
// 스크롤 칸(GG-20): 카드와 놓을 곳 사이 목록 칸(버튼 40줄, 줄이 칸보다 넓어 세로 · 가로 막대, 바로 넘김)과
// 가운데 이야기 칸(20줄 글, 세로 막대, 부드럽게 넘기기 0.15초). 넘길 때마다 · 줄을 누르면 로그를 남긴다.
// 휠 받기(GG-21): 오른쪽 입력란 위 지도 틀(자식 잘라내기 · 끌기 · 휠 받기) 안의 지도(칸 16개)를 휠로 포인터 기준 확대 · 축소하고(배율 · 옮기기 트윈 한 묶음),
// 틀을 끌어 옮긴다(끄는 동안의 휠은 바로 확대). 확대가 끝나면 그 휠 때 포인터 아래였던 지도 점이 얼마나 움직였는지 로그를 남긴다.
JGCLASS()
class GAMEUIDEMO_API JGGameUIDemoHUD : public JGGameWidget
{
	JG_GENERATED_CLASS_BODY

private:
	PSharedPtr<PGameUIText> _turnText;
	int32                   _turn = 1;

	// 아틀라스 페이지 확인: 첫 프레임에 위 줄 글자가 페이지 0 에 들어간 뒤, 큰 글자로 페이지 0 을 채우고 아래 줄 글자를 페이지 1 에 넣는다.
	PSharedPtr<PGameUIFont> _atlasFont;
	PSharedPtr<PGameUIText> _atlasLate;
	PSharedPtr<PGameUIText> _atlasStats;
	bool                    _bAtlasFilled = false;

	// 끌어다 놓기: 관리자 끌기 이벤트를 활성인 동안 구독한다(OnActivated · OnDeactivated).
	PSharedPtr<PGameUIImage> _dragCard;
	PSharedPtr<PGameUIImage> _dropZone;
	HVector2                 _dragGrab;    // 누른 자리(시작 이벤트)
	int32                    _dragMoves = 0;

	// 움직임(GG-16)
	PSharedPtr<PGameUIButton>         _endTurn;
	PSharedPtr<JGGameUIDemoPauseMenu> _pauseMenu;          // 한 객체를 다시 올린다
	bool                              _bCardsFlown = false;

	// 휠 받기(GG-21): 틀이 휠 · 끌기를 받고 안의 지도를 그리기 변환으로 움직인다.
	PSharedPtr<PGameUIImage>    _mapFrame;
	PSharedPtr<PGameUIElement>  _mapContent;
	PSharedPtr<PGameUITimeline> _mapZoom;
	float32                     _mapTargetScale = 0.5f;   // 연달아 굴리면 목표 배율을 쌓는다
	HVector2                    _mapZoomLocal;            // 마지막 휠 때 포인터 아래 지도 점(지도 자리)
	HVector2                    _mapZoomPointer;          // 그때 포인터 자리(캔버스)
	HVector2                    _mapDragGrab;
	HVector2                    _mapDragTranslation;
	int32                       _mapDragMoves = 0;

public:
	JGGameUIDemoHUD() = default;
	virtual ~JGGameUIDemoHUD() = default;

	int32 GetTurn() const;

protected:
	virtual void OnInitialize() override;
	virtual void OnActivated() override;
	virtual void OnDeactivated() override;
	virtual void OnUpdate(float32 deltaSeconds) override;

private:
	void updateTurnText();
	void onEndTurnClicked();
	void onMenuClicked();
	void fillAtlasPage();
	void addScrollBoxes();
	void addMap();
	void zoomMap(const HVector2& pointerPoint, float32 wheel);
	void logMapAnchor();
	HVector2 mapPivotPoint() const;
	void onDragBegin(const PSharedPtr<PGameUIElement>& element, const HVector2& position);
	void onDragMove(const PSharedPtr<PGameUIElement>& element, const HVector2& position, const PSharedPtr<PGameUIElement>& target);
	void onDragEnd(const PSharedPtr<PGameUIElement>& element, const HVector2& position, const PSharedPtr<PGameUIElement>& dropTarget, bool bCanceled);
};
