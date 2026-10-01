#pragma once
#include "UI/GameUIElement.h"
#include "GameWidget.generation.h"

class PGameUIManager;

// 게임 화면 하나(HUD · 메뉴 · 대화상자). 언리얼 CommonUI 의 ActivatableWidget 자리이고, 에디터의 JGWidget 처럼 상속해서 쓴다.
// 관리자(JGGameInstance::Get().GetUI())의 레이어 스택에 Push 하면 보이고, 스택 맨 위인 동안 활성이다.
//   OnInitialize    처음 스택에 올라갈 때 한 번. 요소 트리(AddChild)를 만든다
//   OnActivated     스택 맨 위가 될 때(Push 직후, 위 위젯이 빠졌을 때)
//   OnDeactivated   위에 다른 위젯이 올라오거나 자기가 빠질 때
//   OnUpdate(dt)    활성인 동안 매 프레임(게임 인스턴스 틱, 월드 틱 뒤). 값을 끌어와 갱신할 때만 쓴다
//   OnBackAction    뒤로가기(Esc). 뒤로가기 처리 위젯(SetBackHandler(true))만 받는다. 기본은 자기를 닫고 true
//   OnShutdown      스택에서 빠진 뒤. 그다음 트리를 비운다 — 같은 객체를 다시 Push 하면 OnInitialize 부터 다시
// 루트 요소가 논리 화면 전체(관리자 기준 해상도)를 덮는다. 입력 모드 Menu 면 활성인 동안 아래 레이어 · 월드가 입력을 받지 못한다.
JGCLASS()
class GAMEFRAMEWORKS_API JGGameWidget : public JGObject
{
	JG_GENERATED_CLASS_BODY

	friend class PGameUIManager;

private:
	PSharedPtr<PGameUIElement> _root;
	PGameUIManager*            _manager      = nullptr;   // 스택에 있는 동안만. 관리자는 스택의 위젯보다 오래 산다
	EGameUILayer               _layer        = EGameUILayer::Game;
	EGameWidgetInputMode       _inputMode    = EGameWidgetInputMode::Game;
	bool                       _bBackHandler = false;
	bool                       _bInitialized = false;
	bool                       _bActive      = false;

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

protected:
	virtual void OnInitialize() {}
	virtual void OnShutdown() {}
	virtual void OnActivated() {}
	virtual void OnDeactivated() {}
	virtual void OnUpdate(float32 deltaSeconds) {}
	// 처리했으면 true(관리자가 아래 위젯으로 넘기지 않는다).
	virtual bool OnBackAction();
};
