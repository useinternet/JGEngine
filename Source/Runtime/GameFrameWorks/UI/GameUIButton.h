#pragma once
#include "UI/GameUIElement.h"
#include "UI/GameUIStyle.h"

class PGameUIButton;

// 왼쪽 버튼으로 누른 뒤 버튼 위에서 떼면 한 번 알린다. 방송은 버튼만 한다.
JG_DECLARE_EVENT(HOnGameUIButtonClicked, PGameUIButton);

// 공통 버튼(CommonUI 의 CommonButton 자리). 모습은 스타일(HGameUIButtonStyle)이 정한다: 버튼이 정하지 않으면 관리자 기본 버튼 스타일.
// 누른 상태에서 밖으로 나가 떼면 클릭이 아니다(관리자가 누른 요소를 캡처해 뗌을 전한다). 입력 받기가 기본으로 켜져 있다.
// 꺼진 버튼(SetEnabled(false), 또는 꺼진 부모 아래)은 비활성 색으로 그리고 클릭하지 않지만, 입력은 받아 아래 · 월드로 넘기지 않는다.
class GAMEFRAMEWORKS_API PGameUIButton : public PGameUIElement
{
	HGameUIButtonStyle _style;
	bool               _bHasStyle = false;
	PString            _label;
	bool               _bHovered  = false;
	bool               _bPressed  = false;

public:
	HOnGameUIButtonClicked OnClicked;

public:
	PGameUIButton();
	virtual ~PGameUIButton() = default;

	void SetStyle(const HGameUIButtonStyle& style);
	// 직접 정한 스타일을 지운다(관리자 기본 버튼 스타일을 따른다).
	void ClearStyle();
	bool HasStyle() const;
	void SetLabel(const PString& label);
	const PString& GetLabel() const;
	bool IsHovered() const;
	bool IsPressed() const;
	// 그릴 때 쓰는 스타일: 직접 정한 것, 없으면 관리자 기본(manager 가 nullptr 이면 타입 기본값).
	HGameUIButtonStyle ResolveStyle(const PGameUIManager* manager) const;
	// 상태에 따른 배경 색. 꺼짐 > 누름(포인터가 위에 있을 때) > 호버 > 보통. bEnabled 는 조상까지 본 켜짐.
	static const HLinearColor& PickStateColor(const HGameUIButtonStyle& style, bool bEnabled, bool bHovered, bool bPressed);

protected:
	virtual void onBuildDraw(HGameUIDrawContext& context) override;
	virtual void onPointerEnter() override;
	virtual void onPointerLeave() override;
	virtual void onPointerDown(EGameUIPointerButton button) override;
	virtual void onPointerUp(EGameUIPointerButton button, bool bInside) override;
};
