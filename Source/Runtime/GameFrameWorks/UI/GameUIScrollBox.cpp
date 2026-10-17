#include "PCH/PCH.h"
#include "UI/GameUIScrollBox.h"
#include "UI/GameUIDrawList.h"
#include "UI/GameUIManager.h"
#include "UI/GameUITween.h"
#include "UI/GameWidget.h"
#include "Classes/Texture.h"

namespace
{
	constexpr float32 ScrollEpsilon = 0.01f;    // 이보다 작은 넘침은 넘침이 아니다(실수 오차)
	constexpr float32 PageFraction  = 0.875f;   // 막대 빈 곳 누르기 한 번 = 보이는 칸의 7/8

	HRect toPixels(const HRect& rect, float32 scale)
	{
		return HRect(rect.left * scale, rect.top * scale, rect.right * scale, rect.bottom * scale);
	}

	float32 axisOf(const HVector2& value, bool bVertical)
	{
		return bVertical ? value.y : value.x;
	}

	// 내용 좌표(parentRect)에서 배치 식 그대로 내려가며 target 의 사각형을 찾는다(배치 전 요소도 된다).
	bool findRectBelow(const PSharedPtr<PGameUIElement>& parent, const HRect& parentRect, const PGameUIElement* target, HRect& outRect)
	{
		for (const PSharedPtr<PGameUIElement>& child : parent->GetChildren())
		{
			const HRect childRect = child->CalculateLayoutRect(parentRect);
			if (child.GetRawPointer() == target)
			{
				outRect = childRect;
				return true;
			}
			if (findRectBelow(child, childRect, target, outRect))
			{
				return true;
			}
		}
		return false;
	}
}

// 막대 바탕(내부). 빈 곳을 누르면 그쪽으로 보이는 칸의 7/8 만큼 넘긴다.
class PGameUIScrollBoxBar : public PGameUIElement
{
public:
	PGameUIScrollBox* Box       = nullptr;   // 이 막대를 가진 칸(칸이 막대를 가지므로 칸이 먼저 사라지지 않는다)
	bool              bVertical = true;

public:
	PGameUIScrollBoxBar()
	{
		_bHitTestVisible = true;
	}

protected:
	virtual void onBuildDraw(HGameUIDrawContext& context) override
	{
		const HGameUIScrollStyle style = Box->ResolveStyle(context.Manager);
		context.DrawList->AddQuad(toPixels(_layoutRect, context.Scale), HRect(0.0f, 0.0f, 1.0f, 1.0f), style.TrackColor, style.TrackTexture, nullptr, context.Clip);
	}

	virtual bool onPointerHeldBegin(EGameUIPointerButton button, const HVector2& position) override
	{
		if (button != EGameUIPointerButton::Left)
		{
			return false;
		}

		// 손잡이 앞을 눌렀으면 앞(위 · 왼쪽)으로, 뒤를 눌렀으면 뒤로. 누르고 있는 동안의 이동은 쓰지 않는다(바깥 끌기만 막는다).
		// 넘김 알림 처리기가 칸을 트리에서 빼도 이 함수가 끝날 때까지 칸을 잡아 둔다(아래도 같다).
		PSharedPtr<PGameUIScrollBox> box = SharedWrap(Box);
		const HRect& thumb = box->_thumbs[bVertical ? 0 : 1]->GetLayoutRect();
		const float32 pointer    = axisOf(CanvasToLocal(position), bVertical);
		const float32 thumbStart = bVertical ? thumb.top : thumb.left;
		const float32 thumbEnd   = bVertical ? thumb.bottom : thumb.right;
		if (pointer < thumbStart)
		{
			box->pageScroll(bVertical, -1.0f);
		}
		else if (pointer >= thumbEnd)
		{
			box->pageScroll(bVertical, 1.0f);
		}
		return true;
	}
};

// 손잡이(내부). 누르고 끌면 따라 넘어간다(문턱 없이 바로).
class PGameUIScrollBoxThumb : public PGameUIElement
{
public:
	PGameUIScrollBox* Box        = nullptr;
	bool              bVertical  = true;
	bool              bHovered   = false;
	bool              bPressed   = false;
	float32           GrabOffset = 0.0f;   // 누른 자리 − 손잡이 시작(축 방향)

public:
	PGameUIScrollBoxThumb()
	{
		_bHitTestVisible = true;
	}

protected:
	virtual void onBuildDraw(HGameUIDrawContext& context) override
	{
		const HGameUIScrollStyle style = Box->ResolveStyle(context.Manager);
		const HLinearColor& color = bPressed ? style.ThumbPressed : (bHovered ? style.ThumbHovered : style.ThumbNormal);
		context.DrawList->AddQuad(toPixels(_layoutRect, context.Scale), HRect(0.0f, 0.0f, 1.0f, 1.0f), color, style.ThumbTexture, nullptr, context.Clip);
	}

	virtual void onPointerEnter() override
	{
		bHovered = true;
	}

	virtual void onPointerLeave() override
	{
		bHovered = false;
	}

	virtual bool onPointerHeldBegin(EGameUIPointerButton button, const HVector2& position) override
	{
		if (button != EGameUIPointerButton::Left)
		{
			return false;
		}
		bPressed   = true;
		GrabOffset = axisOf(CanvasToLocal(position), bVertical) - (bVertical ? _layoutRect.top : _layoutRect.left);
		// 부드럽게 넘기던 것은 멈추고 손잡이를 따른다.
		PSharedPtr<PGameUIScrollBox> box = SharedWrap(Box);
		box->cancelScrollTweens();
		return true;
	}

	virtual void onPointerHeldMove(const HVector2& position) override
	{
		PSharedPtr<PGameUIScrollBox> box = SharedWrap(Box);
		const HRect& track = box->_bars[bVertical ? 0 : 1]->GetLayoutRect();
		const float32 thumbStart = axisOf(CanvasToLocal(position), bVertical) - GrabOffset - (bVertical ? track.top : track.left);
		box->dragThumbTo(bVertical, thumbStart);
	}

	virtual void onPointerUp(EGameUIPointerButton button, bool bInside) override
	{
		if (button == EGameUIPointerButton::Left)
		{
			bPressed = false;
		}
	}
};

PGameUIScrollBox::PGameUIScrollBox()
{
	_bHitTestVisible = true;
	_bClipChildren   = true;
	_bWheelInput     = true;

	_viewport = AddChild<PGameUIElement>(PName(PString("ScrollBox.Viewport")));
	_viewport->SetClipChildren(true);
	_content = _viewport->AddChild<PGameUIElement>(PName(PString("ScrollBox.Content")));

	// 막대는 보이는 칸 뒤에 붙인다(위에 그려지고 먼저 판정된다).
	const char* const barNames[2]   = { "ScrollBox.VerticalBar", "ScrollBox.HorizontalBar" };
	const char* const thumbNames[2] = { "ScrollBox.VerticalThumb", "ScrollBox.HorizontalThumb" };
	for (int32 axis = 0; axis < 2; ++axis)
	{
		PSharedPtr<PGameUIScrollBoxBar> bar = AddChild<PGameUIScrollBoxBar>(PName(PString(barNames[axis])));
		bar->Box       = this;
		bar->bVertical = axis == 0;
		bar->SetVisible(false);

		PSharedPtr<PGameUIScrollBoxThumb> thumb = bar->AddChild<PGameUIScrollBoxThumb>(PName(PString(thumbNames[axis])));
		thumb->Box       = this;
		thumb->bVertical = axis == 0;

		_bars[axis]   = bar;
		_thumbs[axis] = thumb;
	}
}

PGameUIScrollBox::~PGameUIScrollBox()
{
}

PSharedPtr<PGameUIElement> PGameUIScrollBox::GetContent() const
{
	return _content;
}

void PGameUIScrollBox::SetScrollDirection(EGameUIScrollDirection direction)
{
	_direction = direction;
}

EGameUIScrollDirection PGameUIScrollBox::GetScrollDirection() const
{
	return _direction;
}

void PGameUIScrollBox::SetContentSize(const HVector2& size)
{
	_contentSize = HVector2(HMath::Max(size.x, 0.0f), HMath::Max(size.y, 0.0f));
}

const HVector2& PGameUIScrollBox::GetContentSize() const
{
	return _contentSize;
}

void PGameUIScrollBox::ScrollTo(const HVector2& offset)
{
	// 코드로 넘긴 자리가 열쇠 기억보다 앞선다(첫 배치 전에 불렀을 때).
	_bKeyRestored = true;
	cancelScrollTweens();
	applyOffset(clampOffset(offset, computeGeometry(resolveCurrentStyle())));
}

void PGameUIScrollBox::ScrollBy(const HVector2& delta)
{
	ScrollTo(HVector2(_offset.x + delta.x, _offset.y + delta.y));
}

bool PGameUIScrollBox::ScrollIntoView(const PSharedPtr<PGameUIElement>& descendant, float32 margin)
{
	if (descendant.IsValid() == false)
	{
		return false;
	}

	const HScrollGeometry geometry = computeGeometry(resolveCurrentStyle());
	const HVector2 contentSize = contentElementSize(geometry);
	HRect rect;
	if (findRectBelow(_content, HRect(0.0f, 0.0f, contentSize.x, contentSize.y), descendant.GetRawPointer(), rect) == false)
	{
		return false;
	}

	// 보이는 창 = [넘긴 양, 넘긴 양 + 보이는 칸]. 앞으로 벗어났거나 칸보다 크면 앞을 맞추고, 뒤로 벗어났으면 뒤를 맞춘다.
	HVector2 target = _offset;
	const bool bAxes[2] = { _direction != EGameUIScrollDirection::Horizontal, _direction != EGameUIScrollDirection::Vertical };
	for (int32 axis = 0; axis < 2; ++axis)
	{
		if (bAxes[axis] == false)
		{
			continue;
		}
		const bool    bVertical = axis == 0;
		const float32 start     = (bVertical ? rect.top : rect.left) - margin;
		const float32 end       = (bVertical ? rect.bottom : rect.right) + margin;
		const float32 view      = axisOf(geometry.ViewportSize, bVertical);
		float32&      value     = bVertical ? target.y : target.x;
		if (start < value || end - start > view)
		{
			value = start;
		}
		else if (end > value + view)
		{
			value = end - view;
		}
	}
	ScrollTo(target);
	return true;
}

const HVector2& PGameUIScrollBox::GetScrollOffset() const
{
	return _offset;
}

HVector2 PGameUIScrollBox::GetResolvedContentSize() const
{
	return resolveContentSize();
}

HVector2 PGameUIScrollBox::GetViewportSize() const
{
	return computeGeometry(resolveCurrentStyle()).ViewportSize;
}

HVector2 PGameUIScrollBox::GetMaxScroll() const
{
	return computeGeometry(resolveCurrentStyle()).MaxScroll;
}

bool PGameUIScrollBox::IsScrollBarVisible(bool bVertical) const
{
	return computeGeometry(resolveCurrentStyle()).bBarVisible[bVertical ? 0 : 1];
}

void PGameUIScrollBox::SetScrollKey(const PName& key)
{
	_scrollKey    = key;
	_bKeyRestored = false;
}

const PName& PGameUIScrollBox::GetScrollKey() const
{
	return _scrollKey;
}

void PGameUIScrollBox::SetStyle(const HGameUIScrollStyle& style)
{
	_style     = style;
	_bHasStyle = true;
}

void PGameUIScrollBox::ClearStyle()
{
	_style     = HGameUIScrollStyle();
	_bHasStyle = false;
}

bool PGameUIScrollBox::HasStyle() const
{
	return _bHasStyle;
}

HGameUIScrollStyle PGameUIScrollBox::ResolveStyle(const PGameUIManager* manager) const
{
	if (_bHasStyle || manager == nullptr)
	{
		return _style;
	}
	return manager->GetDefaultScrollStyle();
}

void PGameUIScrollBox::onLayout(JGGameWidget* ownerWidget)
{
	// 이 칸을 가진 위젯을 기억한다(부드럽게 넘기기 · 열쇠 · 기본 스타일). 다른 위젯으로 옮겨 붙었으면(처음 배치는 아님) 그 위젯의 기억에서 다시 시작한다.
	PSharedPtr<JGGameWidget> widget = _ownerWidget.Pin();
	if (widget.GetRawPointer() != ownerWidget)
	{
		if (widget.IsValid())
		{
			_bKeyRestored = false;
		}
		widget = nullptr;
		if (ownerWidget != nullptr)
		{
			widget = SharedWrap(ownerWidget);
		}
		_ownerWidget = widget;
	}

	const HGameUIScrollStyle style = ResolveStyle(widget.IsValid() ? widget->GetManager() : nullptr);
	const HScrollGeometry geometry = computeGeometry(style);

	// 열쇠: 처음 배치될 때 위젯이 기억한 넘긴 양에서 시작한다.
	if (_bKeyRestored == false && _scrollKey != NAME_NONE && widget.IsValid())
	{
		_bKeyRestored = true;
		auto found = widget->_scrollMemory.find(_scrollKey);
		if (found != widget->_scrollMemory.end())
		{
			_offset = found->second;
		}
	}

	// 내용이 줄었으면 넘긴 양도 따라 줄인다(배치 중이라 알리지 않는다).
	_offset = clampOffset(_offset, geometry);
	rememberOffset();
	layoutParts(style, geometry);
}

HGameUIScrollStyle PGameUIScrollBox::resolveCurrentStyle() const
{
	PSharedPtr<JGGameWidget> widget = _ownerWidget.Pin();
	return ResolveStyle(widget.IsValid() ? widget->GetManager() : nullptr);
}

HVector2 PGameUIScrollBox::resolveContentSize() const
{
	HVector2 size = _contentSize;
	if (size.x > 0.0f && size.y > 0.0f)
	{
		return size;
	}

	// 자동: 보이는 자식의 사각형을 감싸는 크기(앵커는 위 · 왼쪽으로 본다).
	HVector2 extent(0.0f, 0.0f);
	for (const PSharedPtr<PGameUIElement>& child : _content->GetChildren())
	{
		if (child->IsVisible() == false)
		{
			continue;
		}
		const HVector2& position = child->GetPosition();
		const HVector2& pivot    = child->GetPivot();
		const HVector2& childSize = child->GetSize();
		extent.x = HMath::Max(extent.x, position.x + (1.0f - pivot.x) * childSize.x);
		extent.y = HMath::Max(extent.y, position.y + (1.0f - pivot.y) * childSize.y);
	}
	if (size.x <= 0.0f)
	{
		size.x = extent.x;
	}
	if (size.y <= 0.0f)
	{
		size.y = extent.y;
	}
	return size;
}

PGameUIScrollBox::HScrollGeometry PGameUIScrollBox::computeGeometry(const HGameUIScrollStyle& style) const
{
	HScrollGeometry geometry;
	geometry.ContentSize = resolveContentSize();

	const HVector2 boxSize(HMath::Max(_size.x, 0.0f), HMath::Max(_size.y, 0.0f));
	const float32  barSpace       = HMath::Max(style.Thickness, 0.0f) + HMath::Max(style.Margin, 0.0f) * 2.0f;
	const bool     bCanVertical   = _direction != EGameUIScrollDirection::Horizontal;
	const bool     bCanHorizontal = _direction != EGameUIScrollDirection::Vertical;

	auto needsBar = [&style](bool bCanScroll, float32 content, float32 view)
	{
		if (bCanScroll == false || style.Visibility == EGameUIScrollBarVisibility::Never)
		{
			return false;
		}
		return style.Visibility == EGameUIScrollBarVisibility::Always || content > view + ScrollEpsilon;
	};

	// 한쪽 막대가 보이면 보이는 칸이 줄어 다른 쪽도 넘칠 수 있다 — 한 번 더 본다.
	bool bVerticalBar   = needsBar(bCanVertical, geometry.ContentSize.y, boxSize.y);
	bool bHorizontalBar = needsBar(bCanHorizontal, geometry.ContentSize.x, boxSize.x - (bVerticalBar ? barSpace : 0.0f));
	if (bVerticalBar == false)
	{
		bVerticalBar = needsBar(bCanVertical, geometry.ContentSize.y, boxSize.y - (bHorizontalBar ? barSpace : 0.0f));
		if (bVerticalBar && bHorizontalBar == false)
		{
			bHorizontalBar = needsBar(bCanHorizontal, geometry.ContentSize.x, boxSize.x - barSpace);
		}
	}

	geometry.ViewportSize = HVector2(HMath::Max(boxSize.x - (bVerticalBar ? barSpace : 0.0f), 0.0f), HMath::Max(boxSize.y - (bHorizontalBar ? barSpace : 0.0f), 0.0f));
	geometry.MaxScroll    = HVector2(bCanHorizontal ? HMath::Max(geometry.ContentSize.x - geometry.ViewportSize.x, 0.0f) : 0.0f,
		bCanVertical ? HMath::Max(geometry.ContentSize.y - geometry.ViewportSize.y, 0.0f) : 0.0f);
	geometry.bBarVisible[0] = bVerticalBar;
	geometry.bBarVisible[1] = bHorizontalBar;
	return geometry;
}

HVector2 PGameUIScrollBox::clampOffset(const HVector2& offset, const HScrollGeometry& geometry) const
{
	return HVector2(HMath::Clamp(offset.x, 0.0f, geometry.MaxScroll.x), HMath::Clamp(offset.y, 0.0f, geometry.MaxScroll.y));
}

HVector2 PGameUIScrollBox::contentElementSize(const HScrollGeometry& geometry) const
{
	const bool bCanVertical   = _direction != EGameUIScrollDirection::Horizontal;
	const bool bCanHorizontal = _direction != EGameUIScrollDirection::Vertical;
	return HVector2(bCanHorizontal ? HMath::Max(geometry.ContentSize.x, geometry.ViewportSize.x) : geometry.ViewportSize.x,
		bCanVertical ? HMath::Max(geometry.ContentSize.y, geometry.ViewportSize.y) : geometry.ViewportSize.y);
}

void PGameUIScrollBox::layoutParts(const HGameUIScrollStyle& style, const HScrollGeometry& geometry)
{
	const HVector2 zero(0.0f, 0.0f);
	_viewport->SetLayout(zero, zero, zero, geometry.ViewportSize);
	_content->SetLayout(zero, zero, HVector2(-_offset.x, -_offset.y), contentElementSize(geometry));

	const float32 thickness = HMath::Max(style.Thickness, 0.0f);
	const float32 margin    = HMath::Max(style.Margin, 0.0f);
	for (int32 axis = 0; axis < 2; ++axis)
	{
		const bool bVertical = axis == 0;
		_bars[axis]->SetVisible(geometry.bBarVisible[axis]);
		if (geometry.bBarVisible[axis] == false)
		{
			continue;
		}

		// 막대: 칸 안 오른쪽(세로) · 아래(가로), 길이는 보이는 칸 쪽(두 막대가 다 보이면 모서리는 비운다).
		const float32 trackLength = HMath::Max(axisOf(geometry.ViewportSize, bVertical) - margin * 2.0f, 0.0f);
		const HVector2 barPosition = bVertical ? HVector2(_size.x - margin - thickness, margin) : HVector2(margin, _size.y - margin - thickness);
		_bars[axis]->SetLayout(zero, zero, barPosition, bVertical ? HVector2(thickness, trackLength) : HVector2(trackLength, thickness));

		// 손잡이: 길이 = 보이는 칸 / 내용 비율(최소 길이), 자리 = 넘긴 양 비율.
		const float32 content   = axisOf(geometry.ContentSize, bVertical);
		const float32 view      = axisOf(geometry.ViewportSize, bVertical);
		const float32 maxScroll = axisOf(geometry.MaxScroll, bVertical);
		float32 thumbLength = (content > view && content > 0.0f) ? trackLength * view / content : trackLength;
		thumbLength = HMath::Min(HMath::Max(thumbLength, style.MinThumbLength), trackLength);
		const float32 thumbStart = (maxScroll > 0.0f) ? (trackLength - thumbLength) * axisOf(_offset, bVertical) / maxScroll : 0.0f;
		_thumbs[axis]->SetLayout(zero, zero, bVertical ? HVector2(0.0f, thumbStart) : HVector2(thumbStart, 0.0f),
			bVertical ? HVector2(thickness, thumbLength) : HVector2(thumbLength, thickness));
	}
}

HVector2 PGameUIScrollBox::scrollBase() const
{
	if (_smoothTimeline.IsValid() && _smoothTimeline->IsPlaying())
	{
		return _smoothTarget;
	}
	return _offset;
}

void PGameUIScrollBox::applyOffset(const HVector2& offset)
{
	if (offset.x == _offset.x && offset.y == _offset.y)
	{
		return;
	}
	_offset = offset;
	rememberOffset();

	// 처리기 안에서 이 칸이 풀려도 알림을 끝까지 보내도록 잡아 둔다.
	PSharedPtr<PGameUIScrollBox> keepAlive = SharedWrap(this);
	OnScrolled.BroadCast(_offset);
}

void PGameUIScrollBox::rememberOffset()
{
	if (_scrollKey == NAME_NONE)
	{
		return;
	}
	PSharedPtr<JGGameWidget> widget = _ownerWidget.Pin();
	if (widget.IsValid())
	{
		widget->_scrollMemory[_scrollKey] = _offset;
	}
}

void PGameUIScrollBox::cancelScrollTweens()
{
	// 넘긴 양을 쓰던 묶음은 다음 진행에서 취소로 끝난다(같은 속성을 다른 쪽이 가져간 것과 같다). 부드럽게 넘기기 묶음은 바로 멈춘다.
	_tweenWriters[(int32)EGameUITweenProperty::ScrollOffset] = 0;
	if (_smoothTimeline.IsValid())
	{
		PSharedPtr<PGameUITimeline> timeline = _smoothTimeline;
		_smoothTimeline = nullptr;
		timeline->Stop();
	}
}

void PGameUIScrollBox::scrollToByInput(const HVector2& target, const HGameUIScrollStyle& style)
{
	PSharedPtr<JGGameWidget> widget = _ownerWidget.Pin();
	if (style.SmoothSeconds > 0.0f && widget.IsValid())
	{
		// 부드럽게: 같은 속성 겹치기 규칙으로 앞 트윈은 다음 진행에서 끝나고, 새 트윈이 지금 자리에서 새 목표로 이어 간다.
		_smoothTarget = target;
		PSharedPtr<PGameUIScrollBox> self = SharedWrap(this);
		_smoothTimeline = widget->PlayTween(HGameUITween::To(self, EGameUITweenProperty::ScrollOffset, HGameUITweenValue(target), style.SmoothSeconds, EGameUIEase::OutCubic));
		return;
	}
	cancelScrollTweens();
	applyOffset(target);
}

bool PGameUIScrollBox::onWheel(float32 wheel, float32 wheelHorizontal, const HVector2& position)
{
	return handleWheel(wheel, wheelHorizontal);
}

bool PGameUIScrollBox::handleWheel(float32 wheel, float32 wheelHorizontal)
{
	const HGameUIScrollStyle style = resolveCurrentStyle();
	const HScrollGeometry geometry = computeGeometry(style);

	// 세로 휠은 앞으로 굴리면(+) 위로. 세로로 넘길 것이 없으면 가로로 넘긴다. 가로 휠은 오른쪽(+)으로.
	HVector2 delta(0.0f, 0.0f);
	if (geometry.MaxScroll.y > 0.0f)
	{
		delta.y -= wheel * style.WheelStep;
	}
	else
	{
		delta.x -= wheel * style.WheelStep;
	}
	delta.x += wheelHorizontal * style.WheelStep;

	// 이미 그 방향 끝이면 받지 않는다(바깥 칸으로).
	const HVector2 base   = scrollBase();
	const HVector2 target = clampOffset(HVector2(base.x + delta.x, base.y + delta.y), geometry);
	if (HMath::Abs(target.x - base.x) < ScrollEpsilon && HMath::Abs(target.y - base.y) < ScrollEpsilon)
	{
		return false;
	}
	scrollToByInput(target, style);
	return true;
}

void PGameUIScrollBox::pageScroll(bool bVertical, float32 direction)
{
	const HGameUIScrollStyle style = resolveCurrentStyle();
	const HScrollGeometry geometry = computeGeometry(style);
	HVector2 target = scrollBase();
	float32& value  = bVertical ? target.y : target.x;
	value += direction * axisOf(geometry.ViewportSize, bVertical) * PageFraction;
	scrollToByInput(clampOffset(target, geometry), style);
}

void PGameUIScrollBox::dragThumbTo(bool bVertical, float32 thumbStart)
{
	// 손잡이가 움직일 수 있는 거리(막대 − 손잡이)에서의 비율 = 넘긴 양 비율.
	const int32   axis        = bVertical ? 0 : 1;
	const HRect&  track       = _bars[axis]->GetLayoutRect();
	const HRect&  thumb       = _thumbs[axis]->GetLayoutRect();
	const float32 trackLength = bVertical ? track.bottom - track.top : track.right - track.left;
	const float32 thumbLength = bVertical ? thumb.bottom - thumb.top : thumb.right - thumb.left;
	const float32 travel      = trackLength - thumbLength;

	const HScrollGeometry geometry = computeGeometry(resolveCurrentStyle());
	const float32 ratio = (travel > 0.0f) ? HMath::Clamp(thumbStart / travel, 0.0f, 1.0f) : 0.0f;
	HVector2 target = _offset;
	float32& value  = bVertical ? target.y : target.x;
	value = ratio * axisOf(geometry.MaxScroll, bVertical);

	cancelScrollTweens();
	applyOffset(clampOffset(target, geometry));
}

void PGameUIScrollBox::setOffsetFromTween(const HVector2& offset)
{
	applyOffset(clampOffset(offset, computeGeometry(resolveCurrentStyle())));
}
