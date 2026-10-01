#pragma once
#include "UI/GameUIDefines.h"

class HGameUIDrawList;
class PGameUIManager;

// 요소가 자기 모습을 그릴 때 받는 정보. 논리 단위 × Scale = 렌더 타깃 픽셀.
struct HGameUIDrawContext
{
	HGameUIDrawList* DrawList = nullptr;
	float32          Scale    = 1.0f;
	HRect            Clip;                // 렌더 타깃 픽셀
	PGameUIManager*  Manager  = nullptr;  // 기본 글꼴 · 스타일. nullptr 이면 기본이 없다(글자는 글꼴을 직접 정해야 그려진다)
	bool             bEnabled = true;     // 이 요소와 조상이 모두 켜져 있나(꺼진 부모 아래 버튼은 비활성 모습)
};

// 게임 UI 요소의 바탕. 단위는 논리 단위(관리자 기준 해상도 기준).
// 사각형 = 부모.min + Anchor × 부모.size + Position − Pivot × Size   (Anchor · Pivot 은 0~1, (0,0)이 왼쪽 위)
// 자식은 부모 위에 그리고, 뒤에 추가한 자식이 앞 자식 위다. 포인터 판정도 위에 있는 것부터 한다.
// 입력 받기(bHitTestVisible)가 꺼진 요소는 판정에서 빠진다(아래 요소나 월드가 받는다). 버튼은 기본으로 켜져 있다.
// 꺼진(SetEnabled(false)) 요소는 판정은 되지만(아래 · 월드를 막는다) 이벤트를 무시하고, 그 아래 자식은 판정에서 그 요소로 바뀐다.
class GAMEFRAMEWORKS_API PGameUIElement : public IMemoryObject
{
	friend class PGameUIManager;
	friend class JGGameWidget;

protected:
	PName    _name;
	bool     _bVisible        = true;
	bool     _bHitTestVisible = false;
	bool     _bEnabled        = true;
	HVector2 _anchor;
	HVector2 _pivot;
	HVector2 _position;
	HVector2 _size;
	HList<PSharedPtr<PGameUIElement>> _children;

	HRect _layoutRect;   // 마지막 레이아웃 결과(논리 단위)

public:
	PGameUIElement() = default;
	virtual ~PGameUIElement() = default;

	// 자식 요소를 만들어 맨 위에 붙인다.
	template<class T>
	PSharedPtr<T> AddChild(const PName& name)
	{
		PSharedPtr<T> child = Allocate<T>();
		adoptChild(child, name);
		return child;
	}
	bool RemoveChild(const PSharedPtr<PGameUIElement>& child);
	void ClearChildren();
	const HList<PSharedPtr<PGameUIElement>>& GetChildren() const;
	// 이름으로 찾는다. bRecursive 면 자손까지.
	PSharedPtr<PGameUIElement> FindChild(const PName& name, bool bRecursive = true) const;

	const PName& GetName() const;
	void SetVisible(bool bVisible);
	bool IsVisible() const;
	void SetHitTestVisible(bool bHitTestVisible);
	bool IsHitTestVisible() const;
	void SetEnabled(bool bEnabled);
	bool IsEnabled() const;

	// 배치를 한 번에 정한다.
	void SetLayout(const HVector2& anchor, const HVector2& pivot, const HVector2& position, const HVector2& size);
	void SetAnchor(const HVector2& anchor);
	void SetPivot(const HVector2& pivot);
	void SetPosition(const HVector2& position);
	void SetSize(const HVector2& size);
	const HVector2& GetAnchor() const;
	const HVector2& GetPivot() const;
	const HVector2& GetPosition() const;
	const HVector2& GetSize() const;
	// 마지막으로 그리거나 입력을 받을 때 계산한 사각형(논리 단위).
	const HRect& GetLayoutRect() const;

protected:
	// 자기 모습만 그린다. 자식은 관리자가 이어서 그린다.
	virtual void onBuildDraw(HGameUIDrawContext& context) {}
	virtual void onPointerEnter() {}
	virtual void onPointerLeave() {}
	virtual void onPointerDown(EGameUIPointerButton button) {}
	// 누른 요소가 받는다(캡처). bInside 는 뗄 때 포인터가 이 요소 위였는지.
	virtual void onPointerUp(EGameUIPointerButton button, bool bInside) {}

private:
	void adoptChild(PSharedPtr<PGameUIElement> child, const PName& name);
	void layout(const HRect& parentRect);
	void buildDraw(HGameUIDrawContext& context);
	// point(논리)에 있는 가장 위 · 가장 깊은 입력 받는 요소. 꺼진 요소 아래에서 맞으면 그 꺼진 요소. 없으면 nullptr.
	static PSharedPtr<PGameUIElement> hitTest(const PSharedPtr<PGameUIElement>& element, const HVector2& point);
};
