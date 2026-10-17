#pragma once
#include "UI/GameUIElement.h"
#include "UI/GameUIStyle.h"

class PGameUIScrollBox;
class PGameUITimeline;

// 넘긴 양이 바뀌었다(휠 · 손잡이 끌기 · 막대 빈 곳 누르기 · 코드 · 넘긴 양 트윈 — 바뀔 때마다 한 번). 인자 = 새 넘긴 양. 방송은 스크롤 칸만 한다.
// 내용이 줄어 배치 때 넘긴 양이 잘리거나 열쇠 기억에서 시작할 때는 오지 않는다(배치 중).
JG_DECLARE_EVENT(HOnGameUIScrolled, PGameUIScrollBox, const HVector2&);

// 스크롤 칸이 넘기는 방향.
enum class EGameUIScrollDirection
{
	Vertical,     // 세로만(가로로 넘치는 내용은 잘린다)
	Horizontal,   // 가로만(세로 휠도 가로로 넘긴다)
	Both,
};

// 스크롤 칸(GG-20, ProjectAH ER-020). 칸(이 요소 사각형)보다 큰 내용을 칸 안에서 넘겨 보인다 — 칸 밖 내용은 그리지도 판정하지도 않는다.
// 넘길 자식은 내용 요소에 붙인다: box->GetContent()->AddChild<T>(이름). 내용 요소의 왼쪽 위가 (0, 0)이고 자식은 위 · 왼쪽 앵커(기본)로 둔다.
// 내용 크기: SetContentSize 로 정하거나(축마다 0 = 자동) 보이는 자식의 사각형을 감싸는 크기(자동은 앵커를 위 · 왼쪽으로 보고 계산한다).
//   글 하나를 넘기려면 글 요소 높이를 글에 맞춘다: text->SetSize(HVector2(폭, text->MeasureContent(폭, 관리자).y)).
//   넘기지 않는 방향의 내용 요소 크기는 보이는 칸과 같다(세로 칸의 내용 폭 = 보이는 칸 폭).
// 넘긴 양 = 내용 왼쪽 위가 보이는 칸 왼쪽 위에서 위 · 왼쪽으로 간 거리(논리 단위). 늘 0 ~ GetMaxScroll() 로 잘린다(내용이 줄면 따라 줄어든다).
// 막대: 넘기는 방향마다 칸 안 오른쪽(세로) · 아래(가로). 스타일(HGameUIScrollStyle) Visibility 가 Auto 면 내용이 칸보다 클 때만 보이고,
//   보이는 막대 자리(Thickness + 2 × Margin)만큼 보이는 칸이 줄어든다. 손잡이를 끌면 따라 넘어가고, 막대 빈 곳을 누르면 그쪽으로 보이는 칸의 7/8 만큼.
//   숨은 막대는 그리지도 판정하지도 않는다. 막대를 누르면 바깥 요소의 끌기(SetDraggable)는 시작하지 않는다.
// 휠: 관리자가 포인터 아래에서 가장 안쪽 칸부터 준다(그 방향 끝에 닿은 칸은 바깥 칸 · 휠을 켠 바깥 요소로). 한 칸 = 스타일 WheelStep.
//   세로로 넘길 것이 없고 가로로만 넘길 수 있는 칸은 세로 휠도 가로로 넘긴다. 꺼진(SetEnabled(false)) 칸은 휠 · 막대를 받지 않는다.
//   휠 받기(SetWheelInput, GG-21)가 기본으로 켜져 있다 — 끄면 이 칸은 휠을 바깥에 넘긴다. 휠은 넘기기에 쓰고 OnScrolled 로 알린다(OnWheel 은 오지 않는다).
// 부드럽게 넘기기: 스타일 SmoothSeconds > 0 이면 휠 · 막대 빈 곳 누르기가 넘긴 양 트윈(EGameUITweenProperty::ScrollOffset, OutCubic)으로
//   이 칸을 가진 위젯에서 돈다. 연달아 굴리면 목표가 이어 쌓인다. 게임도 같은 트윈을 위젯 PlayTween 으로 직접 재생할 수 있다.
//   손잡이 끌기 · ScrollTo · ScrollBy · ScrollIntoView 는 넘긴 양을 쓰던 트윈(그 묶음 전체)을 취소하고 바로 넘긴다.
// 열쇠(SetScrollKey): 칸째 다시 만들어도 같은 위젯 안 같은 열쇠의 새 칸은 마지막 넘긴 양에서 시작한다(위젯이 기억, OnShutdown 때 잊음).
//   첫 배치 전에 코드로 넘기면(ScrollTo 등) 그 자리가 기억보다 앞선다. 열쇠는 한 위젯 안에서 칸마다 다르게 준다.
// 입력 받기 · 자식 잘라내기가 기본으로 켜져 있다(칸 위 클릭 · 휠이 아래 · 월드로 가지 않는다). 이 요소의 자식(GetChildren)은 내부 요소다 — 직접 붙이거나 지우지 않는다.
// 그리기 변환(돌리기 · 배율)을 받은 칸: 내용은 돌린 칸을 감싸는 축 정렬 사각형으로 잘리고, 판정은 돌린 칸 안만이다.
class GAMEFRAMEWORKS_API PGameUIScrollBox : public PGameUIElement
{
	friend class PGameUITimeline;
	friend class PGameUIScrollBoxBar;
	friend class PGameUIScrollBoxThumb;

	// 지금 배치 값 · 스타일로 정해지는 칸 모양(논리 단위). 배치 · 입력 · 코드가 같은 계산을 쓴다.
	struct HScrollGeometry
	{
		HVector2 ContentSize;                 // 정했거나 자동으로 잰 내용 크기
		HVector2 ViewportSize;                // 막대를 뺀 보이는 칸
		HVector2 MaxScroll;
		bool     bBarVisible[2] = {};         // [0] 세로, [1] 가로
	};

	PSharedPtr<PGameUIElement> _viewport;      // 보이는 칸(자식 잘라내기)
	PSharedPtr<PGameUIElement> _content;       // 넘기는 내용(위치 = −넘긴 양)
	PSharedPtr<PGameUIElement> _bars[2];       // 막대 바탕 [0] 세로, [1] 가로
	PSharedPtr<PGameUIElement> _thumbs[2];     // 손잡이

	EGameUIScrollDirection      _direction = EGameUIScrollDirection::Vertical;
	HVector2                    _contentSize;  // 정한 값(축마다 0 = 자동)
	HVector2                    _offset;
	HVector2                    _smoothTarget; // 부드럽게 넘기는 중의 목표
	PSharedPtr<PGameUITimeline> _smoothTimeline;
	HGameUIScrollStyle          _style;
	bool                        _bHasStyle    = false;
	PName                       _scrollKey;
	bool                        _bKeyRestored = false;   // 열쇠 기억을 이미 봤거나 코드로 넘겼다
	PWeakPtr<JGGameWidget>      _ownerWidget;            // 마지막 배치의 위젯(부드럽게 넘기기 · 열쇠 · 기본 스타일)

public:
	HOnGameUIScrolled OnScrolled;

public:
	PGameUIScrollBox();
	virtual ~PGameUIScrollBox();

	// 내용 요소. 넘길 자식은 여기에 붙인다.
	PSharedPtr<PGameUIElement> GetContent() const;
	void SetScrollDirection(EGameUIScrollDirection direction);
	EGameUIScrollDirection GetScrollDirection() const;
	// 내용 크기(논리 단위). 축마다 0 이면 자동(보이는 자식의 사각형을 감싸는 크기).
	void SetContentSize(const HVector2& size);
	const HVector2& GetContentSize() const;

	// ---- 넘기기(논리 단위, 바로). 넘긴 양을 쓰던 트윈(부드럽게 넘기기 포함)은 취소한다. 바뀌면 OnScrolled.
	void ScrollTo(const HVector2& offset);
	void ScrollBy(const HVector2& delta);
	// descendant(내용 요소 아래 요소)가 보이는 칸 안에 들게 가장 적게 넘긴다. margin = 칸 가장자리와 띄울 거리.
	// 배치 값으로 계산하므로 방금 붙인 요소에도 된다(그리기 변환은 보지 않는다). 칸보다 크면 위 · 왼쪽을 맞춘다. 내용 요소 아래가 아니면 false.
	bool ScrollIntoView(const PSharedPtr<PGameUIElement>& descendant, float32 margin = 0.0f);
	const HVector2& GetScrollOffset() const;
	// 지금 배치 값 · 스타일로 계산한 값(배치 전에도 된다): 내용 크기, 막대를 뺀 보이는 칸, 넘길 수 있는 최대 양, 막대가 보이나.
	HVector2 GetResolvedContentSize() const;
	HVector2 GetViewportSize() const;
	HVector2 GetMaxScroll() const;
	bool IsScrollBarVisible(bool bVertical) const;

	// ---- 열쇠(게임이 정한 이름 — 같은 위젯 안에서 칸째 다시 만들어도 넘긴 양을 잇는다). 기본은 없음.
	void SetScrollKey(const PName& key);
	const PName& GetScrollKey() const;

	// ---- 스타일. 정하지 않으면 관리자 기본 스크롤 스타일을 통째로 쓴다.
	void SetStyle(const HGameUIScrollStyle& style);
	// 직접 정한 스타일을 지운다(관리자 기본을 따른다).
	void ClearStyle();
	bool HasStyle() const;
	// 쓰는 스타일: 직접 정한 것, 없으면 관리자 기본(manager 가 nullptr 이면 타입 기본값).
	HGameUIScrollStyle ResolveStyle(const PGameUIManager* manager) const;

protected:
	virtual void onLayout(JGGameWidget* ownerWidget) override;
	// 휠 = 넘기기(그 방향 끝이면 false — 바깥 요소가 받는다).
	virtual bool onWheel(float32 wheel, float32 wheelHorizontal, const HVector2& position) override;

private:
	HGameUIScrollStyle resolveCurrentStyle() const;
	HVector2           resolveContentSize() const;
	HScrollGeometry    computeGeometry(const HGameUIScrollStyle& style) const;
	HVector2           clampOffset(const HVector2& offset, const HScrollGeometry& geometry) const;
	// 내용 요소 크기: 넘기는 방향은 max(내용, 보이는 칸), 넘기지 않는 방향은 보이는 칸.
	HVector2           contentElementSize(const HScrollGeometry& geometry) const;
	void               layoutParts(const HGameUIScrollStyle& style, const HScrollGeometry& geometry);
	// 입력이 이어 쌓을 자리: 부드럽게 넘기는 중이면 목표, 아니면 지금 넘긴 양.
	HVector2           scrollBase() const;

	// 넘긴 양을 바꾸고(이미 잘린 값) 열쇠 기억에 쓰고 알린다.
	void applyOffset(const HVector2& offset);
	void rememberOffset();
	// 넘긴 양을 쓰던 트윈을 취소한다(부드럽게 넘기기 묶음은 바로 멈춘다).
	void cancelScrollTweens();
	// 입력으로 넘기기: 스타일이 부드럽게면 트윈, 아니면 바로(트윈 취소).
	void scrollToByInput(const HVector2& target, const HGameUIScrollStyle& style);
	// 휠(세로 · 가로 칸 수, onWheel 이 부른다). 그 방향으로 넘어갔으면 true(끝에 닿아 못 넘기면 false — 바깥 칸 · 요소가 받는다).
	bool handleWheel(float32 wheel, float32 wheelHorizontal);
	// 막대: 빈 곳 누르기(direction = −1 위 · 왼쪽, +1 아래 · 오른쪽), 손잡이 끌기(막대 안 손잡이 시작 자리).
	void pageScroll(bool bVertical, float32 direction);
	void dragThumbTo(bool bVertical, float32 thumbStart);
	// 넘긴 양 트윈이 쓴다(그 트윈을 취소하지 않는다).
	void setOffsetFromTween(const HVector2& offset);
};
