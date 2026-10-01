#pragma once
#include "UI/GameUIElement.h"
#include "UI/GameUIStyle.h"

class PGameUIFont;
class PGameUITextInput;

// 사용자가 글을 바꿨다(글자 · 지우기 · 붙여넣기). 인자 = 바뀐 글(UTF-8). SetText 는 알리지 않는다.
JG_DECLARE_EVENT(HOnGameUITextChanged, PGameUITextInput, const PString&);
// Enter 로 확정했다. 인자 = 확정한 글. 알리기 전에 포커스를 푼다.
JG_DECLARE_EVENT(HOnGameUITextCommitted, PGameUITextInput, const PString&);

// 한 줄 글자 입력란. 왼쪽 버튼으로 누르면 글자 입력 포커스를 받고(관리자가 하나만 든다), 호스트가 넘기는 확정 글자 · 편집 키로 고친다.
// 커서 · 선택(Shift + ← → Home End, 전체 선택), 누른 자리로 커서 이동, 최대 길이(글자 수) · 허용 문자, 빈 칸일 때 자리 글.
// Enter = OnCommitted(포커스 해제), Esc = 포커스만 해제, Tab = 같은 위젯의 다음 입력란. 줄바꿈 · 제어 문자는 받지 않는다.
// 글이 칸보다 길면 커서가 보이게 가로로 밀고, 좌우 여백 밖은 잘라낸다. 글자 수 = 코드포인트 수.
// 모습은 스타일(HGameUITextInputStyle): 입력란이 정하지 않으면 관리자 기본 입력란 스타일. 입력 받기가 기본으로 켜져 있다.
class GAMEFRAMEWORKS_API PGameUITextInput : public PGameUIElement
{
	friend class PGameUIManager;

	HList<uint32>         _codepoints;
	PString               _text;               // _codepoints 의 UTF-8
	PString               _placeholder;
	PString               _allowedCharacters;
	HList<uint32>         _allowedCodepoints;  // 비면 제어 문자 말고 모두
	int32                 _maxLength = 0;      // 0 이면 제한 없음
	int32                 _caret     = 0;      // 글자 위치 0 ~ 글자 수
	int32                 _anchor    = 0;      // 선택의 다른 끝. _caret 과 같으면 선택 없음
	float32               _scroll    = 0.0f;   // 가로 밀기(논리 단위). 마지막 그리기 · 커서 계산에서 정한다
	bool                  _bFocused  = false;
	bool                  _bHovered  = false;
	HGameUITextInputStyle _style;
	bool                  _bHasStyle = false;

public:
	HOnGameUITextChanged   OnTextChanged;
	HOnGameUITextCommitted OnCommitted;

public:
	PGameUITextInput();
	virtual ~PGameUITextInput() = default;

	// 글을 바꾼다(제어 문자 · 허용 안 되는 글자는 빼고 최대 길이로 자른다). 커서는 끝. OnTextChanged 는 부르지 않는다.
	void SetText(const PString& text);
	const PString& GetText() const;
	int32 GetLength() const;
	// 글이 비었을 때 흐리게 보이는 안내 글.
	void SetPlaceholder(const PString& placeholder);
	const PString& GetPlaceholder() const;
	// 최대 글자 수. 0 이면 제한 없음. 지금 글이 더 길면 자른다.
	void SetMaxLength(int32 maxLength);
	int32 GetMaxLength() const;
	// 받을 글자(예: "0123456789.:"). 빈 글이면 제어 문자 말고 모두 받는다. 지금 글에서 허용 안 되는 글자는 뺀다.
	void SetAllowedCharacters(const PString& characters);
	const PString& GetAllowedCharacters() const;
	void SetStyle(const HGameUITextInputStyle& style);
	// 직접 정한 스타일을 지운다(관리자 기본 입력란 스타일을 따른다).
	void ClearStyle();
	bool HasStyle() const;
	// 그릴 때 쓰는 스타일: 직접 정한 것, 없으면 관리자 기본(manager 가 nullptr 이면 타입 기본값).
	HGameUITextInputStyle ResolveStyle(const PGameUIManager* manager) const;

	bool IsFocused() const;
	bool IsHovered() const;
	int32 GetCaretPosition() const;
	// 선택 [outStart, outEnd) (글자 위치). 선택이 없으면 둘 다 커서 위치.
	void GetSelection(int32& outStart, int32& outEnd) const;
	PString GetSelectedText() const;

protected:
	virtual void onBuildDraw(HGameUIDrawContext& context) override;
	virtual void onPointerEnter() override;
	virtual void onPointerLeave() override;

private:
	// ---- 관리자가 부른다(포커스 · 글자 · 키)
	// 포커스를 잃으면 선택을 푼다.
	void setFocused(bool bFocused);
	// 확정 글자를 커서 자리에 넣는다(선택은 바꾼다). 바뀌었으면 OnTextChanged.
	void insertText(const PString& text);
	// Enter · Escape · Tab 말고 편집 키.
	void handleEditKey(EGameUIKey key, bool bShift);
	void selectAll();
	void commit();
	// 누른 자리(논리 단위 x)에서 가장 가까운 글자 경계에 커서를 둔다.
	void placeCaret(float32 canvasX, PGameUIManager* manager);
	// 커서 사각형(논리 단위). 글꼴이 없으면 false.
	bool getCaretRect(PGameUIManager* manager, HRect& outRect);

	void moveCaret(int32 position, bool bExtend);
	void deleteRange(int32 start, int32 end);
	bool isAllowed(uint32 codepoint) const;
	// UTF-8 → 받을 수 있는 글자만(제어 문자 · 허용 안 되는 글자 제외).
	void filterInto(const PString& text, HList<uint32>& outCodepoints) const;
	void rebuildText();
	void notifyChanged();
	// 글자 경계 x(논리 단위, 글 시작 기준). 크기 = 글자 수 + 1. 글자 요소와 같은 자간 계산이라 그린 글자와 맞는다.
	void measureBoundaries(const PGameUIFont& font, float32 fontSize, HList<float32>& outBoundaries) const;
	// 커서가 보이도록 _scroll 을 맞춘다.
	void updateScroll(const HList<float32>& boundaries, float32 visibleWidth);
};
