#pragma once
#include "UI/GameUIDefines.h"
#include "UI/GameUIStyle.h"
#include "UI/GameUIDrawList.h"

class IRawTexture;
class JGClass;
class JGGameWidget;
class PGameUIElement;
class PGameUIFont;
class PGameUIRenderer;
class PGameUITextInput;

// 게임 UI 관리자(Lyra 의 PrimaryGameLayout + CommonUI 입력 라우팅 자리). JGGameInstance 가 하나를 가진다(JGGameInstance::Get().GetUI()).
// 레이어 4개(Game < GameMenu < Menu < Modal), 레이어마다 위젯 스택. 각 스택의 맨 위 위젯만 보이고 활성이다.
// 입력은 위 레이어부터 각 레이어 맨 위 위젯이 받고, 입력 모드 Menu 위젯에서 멈춘다(그 아래 레이어 · 월드는 못 받는다).
// 호스트가 하는 일: 월드를 그린 렌더 타깃 위에 Render, 포인터를 먼저 HandlePointer(true 면 월드 입력을 건너뜀), 뒤로가기(Esc)를 HandleBackAction.
// 글자 입력 포커스(입력란)가 있으면(WantsTextInput) 확정 글자를 HandleTextInput, 편집 키(Esc 포함)를 HandleKey 로 먼저 넘긴다.
// 그래픽 없이(헤드리스) 써도 된다. 그때는 그리기 · 텍스처 로드만 하지 않는다. 메인 스레드 전용.
class GAMEFRAMEWORKS_API PGameUIManager : public IMemoryObject
{
	friend class PGameUITextInput;

	HList<PSharedPtr<JGGameWidget>> _layers[(int32)EGameUILayer::Count];

	HVector2 _referenceSize = HVector2(1920.0f, 1080.0f);
	HVector2 _targetSize;
	float32  _scale = 1.0f;

	// 포인터 상태. 요소가 사라져도 매달리지 않도록 약참조.
	PWeakPtr<PGameUIElement> _hovered;
	PWeakPtr<PGameUIElement> _captured;
	EGameUIPointerButton     _capturedButton = EGameUIPointerButton::Left;
	uint32                   _stackVersion   = 0;   // 스택이 바뀔 때마다 +1 (입력을 전하는 중에 화면이 바뀌었는지 본다)

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

public:
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
	// 이미 만든 위젯을 올린다. 다른 스택에 있으면 false.
	bool PushWidgetInstance(PSharedPtr<JGGameWidget> widget, EGameUILayer layer);
	// 스택에서 뺀다(= widget->DeactivateWidget()). 맨 위였으면 아래 위젯이 다시 활성.
	bool RemoveWidget(PSharedPtr<JGGameWidget> widget);
	void ClearLayer(EGameUILayer layer);
	void ClearAllWidgets();
	PSharedPtr<JGGameWidget> GetTopWidget(EGameUILayer layer) const;
	const HList<PSharedPtr<JGGameWidget>>& GetLayerStack(EGameUILayer layer) const;
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

	// ---- 호스트 · 게임 인스턴스
	// 활성 위젯의 OnUpdate (JGGameInstance::Tick, 월드 틱 뒤).
	void Update(float32 deltaSeconds);
	// 보이는 위젯을 레이어 아래부터 target(Allow_RenderTarget) 위에 그린다.
	void Render(const PSharedPtr<IRawTexture>& target);
	// 보이는 위젯을 outDrawList 에 담는다(목록은 먼저 비운다). Render 가 쓰고, 검사에서도 쓴다.
	void BuildDrawList(const HVector2& targetSize, HGameUIDrawList& outDrawList);
	// 포인터 이벤트를 위젯에 전한다. UI 가 받았으면(월드로 넘기지 말아야 하면) true.
	bool HandlePointer(const HGameUIPointerEvent& event, const HVector2& targetSize);
	// 뒤로가기. 위 레이어부터 첫 뒤로가기 처리 위젯이 받는다(Menu 위젯 아래로는 가지 않는다). 처리했으면 true.
	bool HandleBackAction();
	// 활성 위젯 중 입력 모드 Menu 가 있나(있으면 월드 입력은 막힌다).
	bool IsWorldInputBlocked() const;
	// 지금 포인터 아래 있는(호버) 요소. 없으면 nullptr.
	PSharedPtr<PGameUIElement> GetHoveredElement() const;

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
	// 개발용 기본 글꼴: 엔진 Content/Fonts/GameUIDefault.ttf → 시스템 맑은 고딕 → Arial. 시스템 글꼴은 배포하지 않는다(게임은 자기 글꼴을 LoadFont 로).
	PSharedPtr<PGameUIFont> GetDefaultFont();
	// PNG · JPG 등(stb_image)을 RGBA8 텍스처로(경로별로 한 번만). 그래픽 API 가 없거나 실패하면 nullptr.
	PSharedPtr<IRawTexture> LoadTexture(const PString& path);
	// 글자 요소가 정하지 않은 필드 · 스타일을 정하지 않은 버튼이 쓰는 기본(CommonUI 의 기본 스타일 자리).
	void SetDefaultTextStyle(const HGameUITextStyle& style);
	const HGameUITextStyle& GetDefaultTextStyle() const;
	void SetDefaultButtonStyle(const HGameUIButtonStyle& style);
	const HGameUIButtonStyle& GetDefaultButtonStyle() const;
	void SetDefaultTextInputStyle(const HGameUITextInputStyle& style);
	const HGameUITextInputStyle& GetDefaultTextInputStyle() const;

	// 위젯을 모두 내리고(OnShutdown) 글꼴 · 텍스처를 놓는다(게임 인스턴스 종료).
	void Shutdown();

	static const char* GetLayerName(EGameUILayer layer);

private:
	// 입력을 받을 위젯: 위 레이어부터 각 레이어의 활성 맨 위 위젯, Menu 위젯에서 멈춘다.
	HList<PSharedPtr<JGGameWidget>> getInputWidgets() const;
	PSharedPtr<PGameUIElement> hitTestWidgets(const HList<PSharedPtr<JGGameWidget>>& widgets, const HVector2& point) const;
	void activate(PSharedPtr<JGGameWidget> widget);
	void deactivate(PSharedPtr<JGGameWidget> widget);
	// 스택에서 빠진 위젯: 관리자를 끊고 OnShutdown, 트리를 비운다(다시 Push 하면 OnInitialize 부터).
	void retire(PSharedPtr<JGGameWidget> widget);
	void setHovered(const PSharedPtr<PGameUIElement>& element);
	void resetPointerState();
	// 포커스 입력란을 지금 쓸 수 있으면 돌려준다(입력을 받는 위젯 안 · 보이고 켜짐). 아니면 nullptr.
	PSharedPtr<PGameUITextInput> getUsableTextInputFocus() const;
	bool isTextInputUsable(const PGameUITextInput* input) const;
	void setTextInputFocus(const PSharedPtr<PGameUITextInput>& input);
	// 누름: 입력란(왼쪽 버튼)이면 포커스 + 커서, 다른 곳이면 포커스를 푼다.
	void updateTextInputFocusOnDown(const PSharedPtr<PGameUIElement>& hit, EGameUIPointerButton button, const HVector2& point);
	void focusNextTextInput(const PSharedPtr<PGameUITextInput>& current, bool bBackward);
	bool isCaretBlinkOn() const;
};
