#pragma once
#include "UI/GameUIDefines.h"

class HGameUIDrawList;
class JGGameWidget;
class PGameUIElement;
class PGameUIManager;

// 휠 알림(GG-21 — SetWheelInput 을 켠 요소). 인자: 받은 요소, 세로 칸 수(앞으로 굴리면 +), 가로 칸 수(오른쪽으로 +), 포인터 자리(캔버스 논리 단위).
// 칸 수는 소수일 수 있다(정밀 휠 · 터치패드 — 0.25 칸 같은 작은 값이 자주 온다). 방송은 요소만 한다(PGameUIElement::onWheel 기본 구현).
JG_DECLARE_EVENT(HOnGameUIWheel, PGameUIElement, PGameUIElement&, float32, float32, const HVector2&);

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
// 그리기 변환(SetRenderTransform: 회전 · 배율 · 옮기기, 피벗 기준) · 불투명도(SetOpacity)는 배치를 바꾸지 않고 그릴 때와 판정 때만 쓰며 자식에 곱해진다.
// 돌린 요소는 돌린 모양 안에서만 맞는다. 돌린 요소 안의 잘라내기(글자 SetClip · 입력란 여백)는 돌린 사각형을 감싸는 축 정렬 사각형으로 자른다.
// 끌기(SetDraggable)를 켠 요소는 관리자가 끌기 이벤트를 준다(PGameUIManager::OnDragBegin 참고). 엔진은 끄는 요소를 옮기지 않는다.
// 자식 잘라내기(SetClipChildren, GG-20)를 켠 요소는 자손을 자기 사각형 안에만 그리고, 포인터가 자기 사각형 밖이면 자손을 판정하지 않는다
// (잘려 안 보이는 자손은 올리기 · 누르기 · 끌기 · 놓을 곳 · 휠을 받지 않는다). 스크롤 칸(PGameUIScrollBox)이 쓴다.
// 휠 받기(SetWheelInput, GG-21)를 켠 요소는 그 위에서 굴린 휠을 OnWheel(파생은 onWheel)로 받는다 — 지도 · 판 확대 · 축소 등. 스크롤 칸은 기본으로 켜져 있다.
class GAMEFRAMEWORKS_API PGameUIElement : public IMemoryObject
{
	friend class PGameUIManager;
	friend class JGGameWidget;
	friend class PGameUITimeline;

protected:
	PName    _name;
	bool     _bVisible        = true;
	bool     _bHitTestVisible = false;
	bool     _bEnabled        = true;
	bool     _bDraggable      = false;
	bool     _bClipChildren   = false;
	bool     _bWheelInput     = false;
	HVector2 _anchor;
	HVector2 _pivot;
	HVector2 _position;
	HVector2 _size;
	HList<PSharedPtr<PGameUIElement>> _children;

	HGameUIRenderTransform _renderTransform;
	bool                   _bHasRenderTransform = false;
	float32                _opacity             = 1.0f;

	HRect         _layoutRect;                     // 마지막 레이아웃 결과(논리 단위, 변환 전)
	HGameUIAffine _worldTransform;                 // 마지막 레이아웃 때 조상 · 자기 그리기 변환의 곱(논리 → 논리)
	HGameUIAffine _worldInverse;
	bool          _bWorldTransformed = false;
	float32       _worldOpacity      = 1.0f;      // 조상 · 자기 불투명도의 곱

	// 속성마다 지금 값을 쓰는 묶음의 번호(GG-16, 0 = 없음). 다른 묶음이 같은 속성에 트윈을 시작하면 번호가 바뀌어 옛 묶음이 취소로 끝난다.
	uint64        _tweenWriters[(int32)EGameUITweenProperty::Count] = {};

public:
	// 휠(GG-21). SetWheelInput 을 켠 요소가 받는다 — 파생 없이 람다로(AddLambda). 받으면 휠은 거기서 끝난다(월드 · 바깥 요소로 가지 않는다).
	// 람다가 이 요소나 조상의 PSharedPtr 를 잡으면 서로 붙잡아 풀리지 않는다 — 인자로 오는 요소를 쓴다.
	HOnGameUIWheel OnWheel;

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
	// 끌기. 켠 요소(또는 그 자손)를 왼쪽 버튼으로 누른 채 문턱을 넘게 움직이면 이 요소가 끌린다(누른 요소부터 위로 가장 가까운 켠 요소).
	// 누를 수 있어야 한다(SetHitTestVisible(true) — 버튼 · 입력란은 기본으로 켜짐). 꺼진 요소는 끌리지 않는다.
	void SetDraggable(bool bDraggable);
	bool IsDraggable() const;
	// 자식 잘라내기. 켜면 자손을 이 요소 사각형 안에만 그리고(돌린 · 배율을 준 요소면 그 모양을 감싸는 축 정렬 사각형),
	// 포인터가 이 요소 사각형(돌린 모양) 밖이면 자손을 판정하지 않는다. 이 요소 자신의 모습은 자르지 않는다.
	void SetClipChildren(bool bClipChildren);
	bool IsClipChildren() const;
	// 휠 받기(GG-21). 켜면 이 요소 위에서 굴린 휠을 OnWheel(파생은 onWheel)로 받는다. 기본은 꺼짐(스크롤 칸만 켜짐).
	// 휠은 누르기와 같은 판정 길로 간다: 포인터 아래 가장 위 · 가장 깊은 입력 받는 요소에서 위젯 루트까지 가장 안쪽부터 바깥으로,
	// 휠을 켜고 켜진(SetEnabled) 요소에 주고 받으면 끝(스크롤 칸은 그 방향 끝이면 바깥으로 넘긴다). 빈 곳(입력 받는 자식이 없는 자리)에서도
	// 받으려면 입력 받기(SetHitTestVisible(true))도 켠다(끌기와 같은 규칙). 끄는 중이면 휠을 켠 끄는 요소가 먼저 받는다(지도를 끌면서 확대).
	void SetWheelInput(bool bWheelInput);
	bool IsWheelInput() const;

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
	// 마지막으로 그리거나 입력을 받을 때 계산한 사각형(논리 단위, 그리기 변환 전).
	const HRect& GetLayoutRect() const;
	// 부모 사각형 안에서 지금 배치 값(앵커 · 피벗 · 위치 · 크기)으로 정해지는 사각형(배치 식 그대로, 그리기 변환 전). 배치 전에도 쓸 수 있다.
	HRect CalculateLayoutRect(const HRect& parentRect) const;

	// 그리기 변환(배치는 그대로 — 그릴 때와 입력 판정 때만, 자식에 곱해진다). 기준점 = 이 요소 배치 사각형 안 피벗 자리.
	void SetRenderTransform(const HGameUIRenderTransform& transform);
	const HGameUIRenderTransform& GetRenderTransform() const;
	void ClearRenderTransform();
	// 불투명도(0~1). 자식에 곱해지고 정점 알파에 곱한다. 판정에는 영향이 없다(사라진 요소의 입력은 SetHitTestVisible(false) 로 끈다).
	void SetOpacity(float32 opacity);
	float32 GetOpacity() const;
	// 캔버스(논리) 점 → 이 요소의 변환 전 자리(마지막 레이아웃 기준). 변환이 없으면 그대로.
	HVector2 CanvasToLocal(const HVector2& canvasPoint) const;
	// 변환 전 자리(논리)의 사각형을 변환한 모양을 감싸는 축 정렬 사각형(논리). 변환이 없으면 그대로.
	HRect LocalToCanvasBounds(const HRect& localRect) const;

protected:
	// 자기 모습만 그린다. 자식은 관리자가 이어서 그린다.
	virtual void onBuildDraw(HGameUIDrawContext& context) {}
	virtual void onPointerEnter() {}
	virtual void onPointerLeave() {}
	virtual void onPointerDown(EGameUIPointerButton button) {}
	// 누른 요소가 받는다(캡처). bInside 는 뗄 때 포인터가 이 요소 위였는지.
	virtual void onPointerUp(EGameUIPointerButton button, bool bInside) {}
	// 끄는 요소만 받는다. 관리자 이벤트(OnDragBegin · OnDragMove · OnDragEnd)와 같은 인자로 그 직전에 부른다.
	virtual void onDragBegin(const HVector2& position) {}
	virtual void onDragMove(const HVector2& position, const PSharedPtr<PGameUIElement>& target) {}
	virtual void onDragEnd(const HVector2& position, const PSharedPtr<PGameUIElement>& dropTarget, bool bCanceled) {}
	// 누른 요소가 누르는 동안의 이동을 직접 받는다(GG-20 — 스크롤 손잡이 · 슬라이더). 끌기(SetDraggable)와 달리 문턱 없이 바로.
	// 누른 요소(캡처)가 onPointerDown 바로 다음에 받는다. true 를 돌려주면 뗄 때까지 이동마다 onPointerHeldMove 를 받고, 조상 요소의 끌기는 시작하지 않는다.
	// position = 포인터 자리(논리 단위, 캔버스 — 변환 전 자리는 CanvasToLocal). 뗌은 지금처럼 onPointerUp 으로 온다.
	virtual bool onPointerHeldBegin(EGameUIPointerButton button, const HVector2& position) { return false; }
	virtual void onPointerHeldMove(const HVector2& position) {}
	// 휠(GG-21, 휠 받기를 켠 요소만 — SetWheelInput). wheel = 세로 칸 수(앞으로 +), wheelHorizontal = 가로 칸 수(오른쪽 +, 둘 다 소수일 수 있다),
	// position = 포인터 자리(캔버스 논리 단위. 요소 자리는 CanvasToLocal — 배치는 휠 처리 직전에 다시 계산한다).
	// 받았으면 true(관리자가 멈춘다), false 면 바깥 요소에 넘긴다. 기본: OnWheel 을 알리고 true. 스크롤 칸은 넘기기에 쓴다(그 방향 끝이면 false).
	virtual bool onWheel(float32 wheel, float32 wheelHorizontal, const HVector2& position);
	// 배치 훅: 이 요소의 사각형 · 변환을 정한 직후, 자식을 배치하기 전에 부른다(자식 자리를 이 요소가 정할 때 — 스크롤 칸).
	// ownerWidget = 이 트리를 가진 위젯(관리자가 배치할 때 넘긴다).
	virtual void onLayout(JGGameWidget* ownerWidget) {}

private:
	void adoptChild(PSharedPtr<PGameUIElement> child, const PName& name);
	void layout(const HRect& parentRect, JGGameWidget* ownerWidget, const HGameUIAffine& parentWorld = HGameUIAffine(), float32 parentOpacity = 1.0f);
	void buildDraw(HGameUIDrawContext& context);
	// point(논리)에 있는 가장 위 · 가장 깊은 입력 받는 요소. 꺼진 요소 아래에서 맞으면 그 꺼진 요소. 없으면 nullptr.
	// excluded 와 그 자손은 판정에서 뺀다(끄는 중인 요소 아래 놓을 곳을 찾을 때).
	static PSharedPtr<PGameUIElement> hitTest(const PSharedPtr<PGameUIElement>& element, const HVector2& point, const PGameUIElement* excluded = nullptr);
};
