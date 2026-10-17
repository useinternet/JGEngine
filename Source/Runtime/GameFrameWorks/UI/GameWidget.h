#pragma once
#include "UI/GameUIElement.h"
#include "UI/GameUITween.h"
#include "GameWidget.generation.h"

class PGameUIManager;

// 게임 화면 하나(HUD · 메뉴 · 대화상자). 언리얼 CommonUI 의 ActivatableWidget 자리이고, 에디터의 JGWidget 처럼 상속해서 쓴다.
// 관리자(JGGameInstance::Get().GetUI())의 레이어 스택에 Push 하면 보이고, 스택 맨 위인 동안 활성이다.
//   OnInitialize    처음 스택에 올라갈 때 한 번. 요소 트리(AddChild)를 만든다
//   OnActivated     스택 맨 위가 될 때(Push 직후, 위 위젯이 빠졌을 때)
//   OnDeactivated   위에 다른 위젯이 올라오거나 자기가 빠질 때
//   OnUpdate(dt)    활성인 동안 매 프레임(게임 인스턴스 틱, 월드 틱 뒤). 값을 끌어와 갱신할 때만 쓴다
//   OnBackAction    뒤로가기(Esc). 뒤로가기 처리 위젯(SetBackHandler(true))만 받는다. 기본은 자기를 닫고 true
//   OnShutdown      화면에서 완전히 빠진 뒤(닫힘 전환이 있으면 그것이 끝난 뒤). 그다음 트리를 비운다 — 같은 객체를 다시 Push 하면 OnInitialize 부터 다시
// 루트 요소가 논리 화면 전체(관리자 기준 해상도)를 덮는다. 입력 모드 Menu 면 활성인 동안 아래 레이어 · 월드가 입력을 받지 못한다.
// 움직임(GG-16): PlayTimeline · PlayTween 으로 이 위젯이 가진 묶음을 재생한다 — 활성 · 닫히는 동안 돌고, 덮이면 멈추고, OnShutdown 전에 취소된다.
// 열림 · 닫힘 전환(GG-16): SetOpenTransition · SetCloseTransition. 전환은 루트 요소의 불투명도 · 그리기 변환(배율 · 옮기기)을 쓴다 — 게임은 루트 말고 자식에 쓴다.
//   닫힘 전환이 있으면 DeactivateWidget 때 스택에서는 바로 빠지고(OnDeactivated, 입력 · 뒤로가기를 받지 않음) 전환이 끝날 때까지 그 레이어 맨 위에 그려진 뒤 OnShutdown.
//   그 OnDeactivated 안에서 IsClosing() 이 true 다 — 자식 요소의 닫는 모습 트윈을 여기서 걸면 닫히는 동안 돈다(덮여서 비활성이 될 때는 false).
//   닫히는 중에 다시 Push 하면 지금 모습에서 다시 열린다(OnInitialize · OnShutdown 없음).
// 스크롤 칸 열쇠(GG-20, PGameUIScrollBox::SetScrollKey): 열쇠를 준 칸의 넘긴 양을 위젯이 열쇠별로 기억한다 — 칸째 다시 만들어도
//   같은 위젯 안 같은 열쇠의 새 칸이 그 자리에서 시작한다. OnShutdown 때 잊는다.
JGCLASS()
class GAMEFRAMEWORKS_API JGGameWidget : public JGObject
{
	JG_GENERATED_CLASS_BODY

	friend class PGameUIManager;
	friend class PGameUIScrollBox;

private:
	PSharedPtr<PGameUIElement> _root;
	PGameUIManager*            _manager      = nullptr;   // 스택에 있는 동안 · 닫히는 동안. 관리자는 스택의 위젯보다 오래 산다
	EGameUILayer               _layer        = EGameUILayer::Game;
	EGameWidgetInputMode       _inputMode    = EGameWidgetInputMode::Game;
	bool                       _bBackHandler = false;
	bool                       _bInitialized = false;
	bool                       _bActive      = false;
	bool                       _bClosing     = false;   // 스택에서 빠져 닫힘 전환 중

	HList<PSharedPtr<PGameUITimeline>> _timelines;     // 재생 중인 묶음(끝나면 다음 진행에서 뺀다)
	HGameUITransition                  _openTransition;
	HGameUITransition                  _closeTransition;
	PSharedPtr<PGameUITimeline>        _transition;    // 지금 도는 열림 · 닫힘 전환
	HHashMap<PName, HVector2>          _scrollMemory;  // 스크롤 칸 열쇠 → 넘긴 양(OnShutdown 때 비운다)

public:
	JGGameWidget();
	virtual ~JGGameWidget() = default;

	// 루트에 자식 요소를 만들어 맨 위에 붙인다.
	template<class T>
	PSharedPtr<T> AddChild(const PName& name)
	{
		return _root->AddChild<T>(name);
	}
	PSharedPtr<PGameUIElement> GetRoot() const;
	// 트리에서 이름으로 찾는다(자손까지).
	PSharedPtr<PGameUIElement> FindElement(const PName& name) const;

	bool            IsInStack() const;
	bool            IsActive() const;
	EGameUILayer    GetLayer() const;
	PGameUIManager* GetManager() const;
	// 스택에서 뺀다(맨 위였으면 아래 위젯이 다시 활성). CommonUI 의 DeactivateWidget. 스택에 없으면 아무것도 하지 않는다.
	void            DeactivateWidget();

	void                 SetInputMode(EGameWidgetInputMode inputMode);
	EGameWidgetInputMode GetInputMode() const;
	void                 SetBackHandler(bool bBackHandler);
	bool                 IsBackHandler() const;

	// ---- 움직임(GG-16)
	// 묶음을 재생한다(이미 재생 중이면 그대로). 재생을 건 자리에서 첫 트윈의 시작값을 바로 쓴다. 돌려준 묶음으로 Stop · Skip · OnFinished.
	PSharedPtr<PGameUITimeline> PlayTimeline(const PSharedPtr<PGameUITimeline>& timeline);
	// 트윈 하나짜리 묶음을 만들어 재생한다.
	PSharedPtr<PGameUITimeline> PlayTween(const HGameUITween& tween);
	// 열림 · 닫힘 전환(길이 0 = 없음 — 지금까지처럼 바로). Push · DeactivateWidget 때 쓴다.
	void                     SetOpenTransition(const HGameUITransition& transition);
	void                     SetCloseTransition(const HGameUITransition& transition);
	const HGameUITransition& GetOpenTransition() const;
	const HGameUITransition& GetCloseTransition() const;
	// 스택에서 빠져 닫힘 전환 중인가(IsInStack 은 false).
	bool                     IsClosing() const;

protected:
	virtual void OnInitialize() {}
	virtual void OnShutdown() {}
	virtual void OnActivated() {}
	virtual void OnDeactivated() {}
	virtual void OnUpdate(float32 deltaSeconds) {}
	// 처리했으면 true(관리자가 아래 위젯으로 넘기지 않는다).
	virtual bool OnBackAction();

private:
	// 관리자가 부른다: 재생 중인 묶음을 dt 만큼 진행하고 끝난 것을 뺀다.
	void tickTimelines(float32 deltaSeconds);
	// 모든 묶음(전환 포함)을 취소한다 — 화면에서 완전히 빠질 때 OnShutdown 전에.
	void cancelTimelines();
	// 열림(bOpen) · 닫힘 전환을 시작한다. bFromCurrent 면 열림도 지금 모습에서(닫히다 다시 열기). 전환이 없으면 아무것도 하지 않는다.
	void beginTransition(bool bOpen, bool bFromCurrent);
	bool isTransitionPlaying() const;
};
