#pragma once
#include "UI/GameUIDefines.h"
#include "UI/GameUIStyle.h"
#include "UI/GameUIDrawList.h"

class IRawTexture;
class JGClass;
class JGGameWidget;
class PGameUIElement;
class PGameUIFont;
class PGameUIManager;
class PGameUIRenderer;
class PGameUITextInput;

// 끌기 이벤트(SetDraggable 을 켠 요소). 위치는 논리 단위. 시작한 끌기는 반드시 끝 이벤트 하나로 끝난다.
// 시작: 끄는 요소, 누른 자리. 바로 이어서 이동이 한 번 온다.
JG_DECLARE_EVENT(HOnGameUIDragBegin, PGameUIManager, const PSharedPtr<PGameUIElement>&, const HVector2&);
// 이동: 끄는 요소, 지금 자리, 그 아래 입력 받는 요소(끄는 요소와 그 자손은 빼고 판정. 없으면 nullptr — 놓을 곳 강조에 쓴다).
JG_DECLARE_EVENT(HOnGameUIDragMove, PGameUIManager, const PSharedPtr<PGameUIElement>&, const HVector2&, const PSharedPtr<PGameUIElement>&);
// 끝: 끄는 요소, 뗀 자리, 놓은 곳(이동과 같은 판정. 없으면 nullptr), 취소했나(취소면 놓은 곳은 nullptr, 자리는 마지막 포인터 자리).
// 취소 = 위젯 스택이 바뀜 · CancelDrag · 끄는 중 왼쪽 누름이 또 옴(호스트가 뗌을 놓침).
JG_DECLARE_EVENT(HOnGameUIDragEnd, PGameUIManager, const PSharedPtr<PGameUIElement>&, const HVector2&, const PSharedPtr<PGameUIElement>&, bool);

// 게임 UI 관리자(Lyra 의 PrimaryGameLayout + CommonUI 입력 라우팅 자리). JGGameInstance 가 하나를 가진다(JGGameInstance::Get().GetUI()).
// 레이어 4개(Game < GameMenu < Menu < Modal), 레이어마다 위젯 스택. 각 스택의 맨 위 위젯만 보이고 활성이다.
// 열림 · 닫힘 전환(GG-16, JGGameWidget::SetOpenTransition): 맨 위가 열림 전환 중이면 바로 아래 위젯도 그린다(새 화면이 그 위로 나타난다).
// 닫힘 전환 중인 위젯은 스택에서 빠진 채 그 레이어 맨 위에 그린다(입력 · 뒤로가기는 받지 않는다). 전환이 끝나면 OnShutdown.
// 입력은 위 레이어부터 각 레이어 맨 위 위젯이 받고, 입력 모드 Menu 위젯에서 멈춘다(그 아래 레이어 · 월드는 못 받는다).
// 호스트가 하는 일: 월드를 그린 렌더 타깃 위에 Render, 포인터를 먼저 HandlePointer(true 면 월드 입력을 건너뜀), 뒤로가기(Esc)를 HandleBackAction.
// 글자 입력 포커스(입력란)가 있으면(WantsTextInput) 확정 글자를 HandleTextInput, 편집 키(Esc 포함)를 HandleKey 로 먼저 넘긴다.
// 위젯 스택이 바뀌면(올림 · 내림 · 비움) 호버 · 누름 · 끌기가 풀린다. 끌기는 취소 이벤트가 위젯 훅보다 먼저 온다.
// 휠(GG-20 · GG-21): 휠 받기를 켠 요소(SetWheelInput — 스크롤 칸은 기본으로 켜짐)가 받는다. 포인터 아래 판정 요소에서 위젯 루트까지 가장 안쪽부터 바깥으로,
// 처음 받는 요소에서 끝난다(스크롤 칸은 그 방향 끝이면 바깥으로 넘긴다). 끄는 중이면 휠을 켠 끄는 요소가 먼저. 받을 요소가 없으면 UI 위에서 소비만 한다(월드 줌을 막는다).
// 그래픽 없이(헤드리스) 써도 된다. 그때는 그리기 · 텍스처 로드만 하지 않는다. 메인 스레드 전용.
class GAMEFRAMEWORK_API PGameUIManager : public IMemoryObject
{
	friend class PGameUITextInput;

	HList<PSharedPtr<JGGameWidget>> _layers[(int32)EGameUILayer::Count];
	HList<PSharedPtr<JGGameWidget>> _closing[(int32)EGameUILayer::Count];   // 스택에서 빠져 닫힘 전환 중(그 레이어 맨 위에 그린다)
	float32                         _tweenTimeScale = 1.0f;

	HVector2 _referenceSize = HVector2(1920.0f, 1080.0f);
	HVector2 _targetSize;
	float32  _scale = 1.0f;

	// 포인터 상태. 요소가 사라져도 매달리지 않도록 약참조.
	PWeakPtr<PGameUIElement> _hovered;
	PWeakPtr<PGameUIElement> _captured;
	EGameUIPointerButton     _capturedButton = EGameUIPointerButton::Left;
	bool                     _bCapturedHeld  = false;   // 누른 요소가 누르는 동안 이동을 받는다(onPointerHeldBegin 이 true)
	uint32                   _stackVersion   = 0;   // 스택이 바뀔 때마다 +1 (입력을 전하는 중에 화면이 바뀌었는지 본다)
	HVector2                 _pointerPosition;      // 마지막 포인터 이벤트 자리(논리 단위)
	bool                     _bPointerInside = false;
	bool                     _bPointerDown[(int32)EGameUIPointerButton::Count] = {};

	// 끌기: 왼쪽으로 누른 끌 수 있는 요소가 후보가 되고, 누른 자리에서 문턱을 넘게 움직이면 끄는 요소가 된다.
	// 끄는 요소는 강참조다(끝 이벤트를 줄 때까지 살아 있어야 한다).
	PWeakPtr<PGameUIElement>   _dragCandidate;
	PSharedPtr<PGameUIElement> _dragged;
	HVector2                   _dragStart;
	float32                    _dragThreshold = 6.0f;

	// 글자 입력 포커스(입력란 하나). 입력란을 쓸 수 없게 되면(입력을 받는 위젯 밖 · 안 보임 · 꺼짐) 포커스가 없는 것으로 본다.
	PWeakPtr<PGameUITextInput> _textInputFocus;
	float32                    _caretBlinkTime = 0.0f;   // 입력 · 커서 이동 때 0. 커서 깜빡임

	PSharedPtr<PGameUIRenderer>              _renderer;
	HGameUIDrawList                          _drawList;
	HHashMap<PName, PSharedPtr<PGameUIFont>> _fonts;      // 키: 경로
	HHashMap<PName, PSharedPtr<IRawTexture>> _textures;   // 키: 경로
	PSharedPtr<PGameUIFont>                  _defaultFont;
	bool                                     _bDefaultFontTried = false;
	HGameUITextStyle                         _defaultTextStyle;
	HGameUIButtonStyle                       _defaultButtonStyle;
	HGameUITextInputStyle                    _defaultTextInputStyle;
	HGameUIScrollStyle                       _defaultScrollStyle;

public:
	// 끌기 이벤트. 게임 위젯이 구독하고 요소로 가린다(요소마다 대리자를 두지 않는다). 엔진은 끄는 요소를 옮기지 않는다 —
	// 게임이 이동에서 위치(SetPosition 또는 그리기 변환의 옮기기)를 바꾸고, 끝에서 놓을지 · 되돌릴지 정한다.
	// 위젯이 구독할 때: OnActivated 에서 AddRaw(this, ...), OnDeactivated 에서 RemoveObject(this) (취소는 비활성 훅보다 먼저 온다).
	HOnGameUIDragBegin OnDragBegin;
	HOnGameUIDragMove  OnDragMove;
	HOnGameUIDragEnd   OnDragEnd;

	PGameUIManager() = default;
	virtual ~PGameUIManager();

	// ---- 위젯 스택
	// 새 위젯을 만들어 layer 스택 맨 위에 올린다(활성). 같은 스택의 이전 맨 위는 비활성 · 안 보임. 템플릿이라 게임 DLL 에서 인스턴스화된다.
	template<class T>
	PSharedPtr<T> PushWidget(EGameUILayer layer)
	{
		PSharedPtr<T> widget = Allocate<T>();
		if (PushWidgetInstance(widget, layer) == false)
		{
			return nullptr;
		}
		return widget;
	}
	// 리플렉션 클래스로 만든다. JGGameWidget 파생이 아니면 nullptr.
	PSharedPtr<JGGameWidget> PushWidgetByClass(PSharedPtr<JGClass> widgetClass, EGameUILayer layer);
	// 이미 만든 위젯을 올린다. 다른 스택에 있으면 false. 닫힘 전환 중인 위젯이면 지금 모습에서 다시 연다(OnInitialize · OnShutdown 없음).
	bool PushWidgetInstance(PSharedPtr<JGGameWidget> widget, EGameUILayer layer);
	// 스택에서 뺀다(= widget->DeactivateWidget()). 맨 위였으면 아래 위젯이 다시 활성.
	// 맨 위였고 닫힘 전환이 있으면 전환이 끝날 때까지 그린 뒤 OnShutdown, 아니면 바로 OnShutdown.
	bool RemoveWidget(PSharedPtr<JGGameWidget> widget);
	// 비운다. 닫힘 전환 중인 위젯도 바로 내린다(전환 없이).
	void ClearLayer(EGameUILayer layer);
	void ClearAllWidgets();
	PSharedPtr<JGGameWidget> GetTopWidget(EGameUILayer layer) const;
	const HList<PSharedPtr<JGGameWidget>>& GetLayerStack(EGameUILayer layer) const;
	// 스택에 있는 위젯 수(닫힘 전환 중인 위젯은 세지 않는다).
	uint64 GetWidgetCount() const;
	// 스택에 있는 첫 T(위 레이어 · 맨 위부터). 없으면 nullptr.
	template<class T>
	PSharedPtr<T> FindWidget() const
	{
		for (int32 layerIndex = (int32)EGameUILayer::Count - 1; layerIndex >= 0; --layerIndex)
		{
			const HList<PSharedPtr<JGGameWidget>>& stack = _layers[layerIndex];
			for (auto it = stack.rbegin(); it != stack.rend(); ++it)
			{
				PSharedPtr<JGGameWidget> widget = *it;
				PSharedPtr<T> found = RawDynamicCast<T>(widget);
				if (found.IsValid())
				{
					return found;
				}
			}
		}
		return nullptr;
	}

	// ---- 화면 크기 (논리 단위 = 기준 해상도. 배율 = min(타깃 폭 / 기준 폭, 타깃 높이 / 기준 높이))
	void SetReferenceSize(const HVector2& referenceSize);
	const HVector2& GetReferenceSize() const;
	float32 GetScale() const;
	HVector2 GetLogicalSize() const;
	HVector2 TargetToCanvas(const HVector2& targetPoint) const;
	// targetSize(렌더 타깃 픽셀) 기준으로 배율과 보이는 위젯의 사각형을 다시 계산한다.
	void UpdateLayout(const HVector2& targetSize);

	// ---- 움직임(GG-16, UI/GameUITween.h)
	// 위젯 묶음(트윈 · 열림 · 닫힘 전환)의 시간 배율. 기본 1, 0 = 멈춤, 음수는 0. 위젯 OnUpdate 의 dt 는 그대로다.
	void    SetTweenTimeScale(float32 timeScale);
	float32 GetTweenTimeScale() const;

	// ---- 호스트 · 게임 인스턴스
	// 활성 위젯의 묶음을 진행하고 OnUpdate, 닫힘 전환 중인 위젯의 묶음을 진행하고 전환이 끝났으면 내린다(JGGameInstance::Tick, 월드 틱 뒤).
	void Update(float32 deltaSeconds);
	// 보이는 위젯을 레이어 아래부터 target(Allow_RenderTarget) 위에 그린다.
	void Render(const PSharedPtr<IRawTexture>& target);
	// 보이는 위젯을 outDrawList 에 담는다(목록은 먼저 비운다). Render 가 쓰고, 검사에서도 쓴다.
	void BuildDrawList(const HVector2& targetSize, HGameUIDrawList& outDrawList);
	// 포인터 이벤트를 위젯에 전한다. UI 가 받았으면(월드로 넘기지 말아야 하면) true. 끄는 동안의 이동 · 왼쪽 뗌은 늘 true.
	// Leave 는 호버만 푼다 — 끄는 중이면 끌기는 이어지고, 밖에서 떼면 놓은 곳 없이 끝난다. 뗌이 오지 않게 되면 호스트가 CancelDrag.
	bool HandlePointer(const HGameUIPointerEvent& event, const HVector2& targetSize);
	// 뒤로가기. 위 레이어부터 첫 뒤로가기 처리 위젯이 받는다(Menu 위젯 아래로는 가지 않는다). 처리했으면 true.
	bool HandleBackAction();
	// 활성 위젯 중 입력 모드 Menu 가 있나(있으면 월드 입력은 막힌다).
	bool IsWorldInputBlocked() const;
	// 지금 포인터 아래 있는(호버) 요소. 없으면 nullptr. 끄는 동안은 끄는 요소를 뺀 그 아래 요소.
	PSharedPtr<PGameUIElement> GetHoveredElement() const;

	// ---- 포인터 상태 · 끌기 (논리 단위)
	// 마지막 포인터 이벤트 자리(Leave 는 바꾸지 않는다).
	const HVector2& GetPointerPosition() const;
	// 그 버튼을 누른 뒤 아직 떼지 않았나(호스트가 넘긴 누름 · 뗌 그대로).
	bool IsPointerDown(EGameUIPointerButton button) const;
	// 포인터가 그리는 영역 안인가(Leave 뒤 false, 다음 포인터 이벤트에서 true).
	bool IsPointerInside() const;
	// 끌기 문턱(기본 6). 누른 자리에서 이보다 멀리 움직여야 끌기가 된다 — 그 안에서 떼면 지금처럼 클릭. 음수는 0.
	void SetDragThreshold(float32 threshold);
	float32 GetDragThreshold() const;
	// 끄는 중인 요소. 없으면 nullptr.
	PSharedPtr<PGameUIElement> GetDraggedElement() const;
	// 끄는 중이면 취소한다(OnDragEnd, bCanceled = true). 창이 포커스를 잃는 등 뗌이 오지 않을 때 호스트가 부른다.
	void CancelDrag();

	// ---- 글자 입력 (입력란 포커스 하나. 호스트가 키보드를 넘긴다)
	// 입력란을 왼쪽 버튼으로 누르면 포커스를 받고(누른 자리에 커서), 다른 곳(다른 요소 · 빈 곳 · 월드)을 누르면 푼다.
	// 입력란이 입력을 받지 못하게 되면(위젯이 빠지거나 덮임 · Menu 위젯 아래 · 안 보임 · 꺼짐) 포커스도 없다.
	// 호스트: WantsTextInput 일 때만 확정 글자 · 편집 키를 넘기고, IME 조합 창을 GetTextInputCaretRect 에 둔다.
	bool WantsTextInput() const;
	PSharedPtr<PGameUITextInput> GetFocusedTextInput() const;
	// 코드로 포커스를 준다. 입력을 받는 위젯 안의 보이고 켜진 입력란만 받는다. 받았으면 true.
	bool SetTextInputFocus(const PSharedPtr<PGameUITextInput>& input);
	void ClearTextInputFocus();
	// 확정 글자(UTF-8. IME 확정 · 붙여넣기 포함). 포커스 입력란이 있으면 true. 제어 문자 · 허용 안 되는 글자는 버린다.
	bool HandleTextInput(const PString& text);
	// 편집 키. 포커스 입력란이 있으면 true. Esc = 포커스 해제(그 Esc 는 뒤로가기로 쓰지 않는다), Enter = 포커스 해제 후 OnCommitted,
	// Tab = 같은 위젯의 다음 입력란(bShift 면 앞, 글 전체 선택). bShift 는 ← → Home End 에서 선택을 넓힌다.
	bool HandleKey(EGameUIKey key, bool bShift = false);
	// 포커스 입력란의 커서 사각형(렌더 타깃 픽셀, IME 조합 창 자리). 없으면 false.
	bool GetTextInputCaretRect(HRect& outRect);
	// 포커스 입력란에서 선택한 글(복사용). 없으면 빈 글.
	PString GetTextInputSelection() const;

	// ---- 글꼴 · 텍스처 · 기본 스타일
	// TTF 글꼴(경로별로 한 번만 읽는다). 실패하면 nullptr.
	PSharedPtr<PGameUIFont> LoadFont(const PString& path);
	// 기본 글꼴(글꼴을 정하지 않은 글 · 버튼 라벨 · 입력란): SetDefaultFont 로 정한 것, 없으면 처음 부를 때 찾는다 —
	// 게임 Content/Fonts/GameUIDefault.ttf → 엔진 Content/Fonts/GameUIDefault.ttf → OS 맑은 고딕 → Arial.
	// OS 글꼴은 이 PC 에서 보기 위한 것이다(패키지에 실리지 않는다 — 패키지 빌드면 경고). 게임은 자기 글꼴을 Content/Fonts/GameUIDefault.ttf 로 두거나 SetDefaultFont.
	PSharedPtr<PGameUIFont> GetDefaultFont();
	// 게임이 기본 글꼴을 정한다(예: LoadFont 로 자기 Content 의 글꼴을 읽어서, 시작할 때). nullptr 이면 위 순서로 다시 찾는다.
	void SetDefaultFont(const PSharedPtr<PGameUIFont>& font);
	// PNG · JPG 등(stb_image)을 RGBA8 텍스처로(경로별로 한 번만). 그래픽 API 가 없거나 실패하면 nullptr.
	PSharedPtr<IRawTexture> LoadTexture(const PString& path);
	// 글자 요소가 정하지 않은 필드 · 스타일을 정하지 않은 버튼이 쓰는 기본(CommonUI 의 기본 스타일 자리).
	void SetDefaultTextStyle(const HGameUITextStyle& style);
	const HGameUITextStyle& GetDefaultTextStyle() const;
	void SetDefaultButtonStyle(const HGameUIButtonStyle& style);
	const HGameUIButtonStyle& GetDefaultButtonStyle() const;
	void SetDefaultTextInputStyle(const HGameUITextInputStyle& style);
	const HGameUITextInputStyle& GetDefaultTextInputStyle() const;
	void SetDefaultScrollStyle(const HGameUIScrollStyle& style);
	const HGameUIScrollStyle& GetDefaultScrollStyle() const;

	// 위젯을 모두 내리고(OnShutdown) 글꼴 · 텍스처를 놓는다(게임 인스턴스 종료).
	void Shutdown();

	static const char* GetLayerName(EGameUILayer layer);

private:
	// 입력을 받을 위젯: 위 레이어부터 각 레이어의 활성 맨 위 위젯, Menu 위젯에서 멈춘다.
	HList<PSharedPtr<JGGameWidget>> getInputWidgets() const;
	// excluded 와 그 자손은 뺀다(끄는 요소).
	PSharedPtr<PGameUIElement> hitTestWidgets(const HList<PSharedPtr<JGGameWidget>>& widgets, const HVector2& point, const PGameUIElement* excluded = nullptr) const;
	void activate(PSharedPtr<JGGameWidget> widget);
	void deactivate(PSharedPtr<JGGameWidget> widget);
	// 스택에서 빠진 위젯: 묶음을 취소하고 관리자를 끊고 OnShutdown, 트리를 비운다(다시 Push 하면 OnInitialize 부터).
	void retire(PSharedPtr<JGGameWidget> widget);
	// 그 레이어에서 그릴 위젯을 아래부터 덧붙인다: (맨 위가 열림 전환 중이면 그 바로 아래), 맨 위, 닫힘 전환 중인 위젯.
	void getVisibleWidgets(EGameUILayer layer, HList<PSharedPtr<JGGameWidget>>& outWidgets) const;
	// 닫힘 전환 목록에서 뺀다(없으면 아무것도 하지 않는다).
	void removeClosing(const JGGameWidget* widget);
	bool hasClosingWidgets() const;
	void setHovered(const PSharedPtr<PGameUIElement>& element);
	// 호버 · 누름(캡처) · 끌기 후보를 풀고 끌기를 취소한다. 스택을 바꾸는 일은 맨 먼저 부른다(취소 처리기가 스택을 바꿀 수 있다).
	void resetPointerState();
	// 휠: 휠을 켠 끄는 요소, 그다음 hit 에서 바깥쪽으로 휠을 켠 요소에 차례로 준다(onWheel 이 true 면 끝). 받았으면 true.
	bool routeWheel(const HList<PSharedPtr<JGGameWidget>>& widgets, const PSharedPtr<PGameUIElement>& hit, float32 wheel, float32 wheelHorizontal, const HVector2& position);
	// 후보를 끄는 요소로: 누름은 클릭이 아니게 풀고 OnDragBegin.
	void beginDrag(PSharedPtr<PGameUIElement> element);
	void moveDrag(const HVector2& position, const PSharedPtr<PGameUIElement>& target);
	void endDrag(const HVector2& position, const PSharedPtr<PGameUIElement>& dropTarget, bool bCanceled);
	// 포커스 입력란을 지금 쓸 수 있으면 돌려준다(입력을 받는 위젯 안 · 보이고 켜짐). 아니면 nullptr.
	PSharedPtr<PGameUITextInput> getUsableTextInputFocus() const;
	bool isTextInputUsable(const PGameUITextInput* input) const;
	void setTextInputFocus(const PSharedPtr<PGameUITextInput>& input);
	// 누름: 입력란(왼쪽 버튼)이면 포커스 + 커서, 다른 곳이면 포커스를 푼다.
	void updateTextInputFocusOnDown(const PSharedPtr<PGameUIElement>& hit, EGameUIPointerButton button, const HVector2& point);
	void focusNextTextInput(const PSharedPtr<PGameUITextInput>& current, bool bBackward);
	bool isCaretBlinkOn() const;
};
