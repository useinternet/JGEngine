#include "PCH/PCH.h"
#include "UI/GameUIElement.h"
#include "UI/GameUIDrawList.h"
#include <cmath>

HGameUIAffine HGameUIAffine::FromRenderTransform(const HGameUIRenderTransform& transform, const HVector2& pivot)
{
	// 피벗을 원점으로 옮기고 → 배율 → 회전 → 피벗 + 옮기기 자리로.
	const float32 radians = HMath::ConvertToRadians(transform.Angle);
	const float32 cosine  = std::cos(radians);
	const float32 sine    = std::sin(radians);

	HGameUIAffine result;
	result.A = cosine * transform.Scale.x;
	result.B = sine * transform.Scale.x;
	result.C = -sine * transform.Scale.y;
	result.D = cosine * transform.Scale.y;
	result.X = pivot.x + transform.Translation.x - (result.A * pivot.x + result.C * pivot.y);
	result.Y = pivot.y + transform.Translation.y - (result.B * pivot.x + result.D * pivot.y);
	return result;
}

HGameUIAffine HGameUIAffine::operator*(const HGameUIAffine& rhs) const
{
	HGameUIAffine result;
	result.A = A * rhs.A + C * rhs.B;
	result.B = B * rhs.A + D * rhs.B;
	result.C = A * rhs.C + C * rhs.D;
	result.D = B * rhs.C + D * rhs.D;
	result.X = A * rhs.X + C * rhs.Y + X;
	result.Y = B * rhs.X + D * rhs.Y + Y;
	return result;
}

HVector2 HGameUIAffine::Apply(const HVector2& point) const
{
	return HVector2(A * point.x + C * point.y + X, B * point.x + D * point.y + Y);
}

HGameUIAffine HGameUIAffine::Inverse() const
{
	const float32 determinant = A * D - B * C;
	if (HMath::Abs(determinant) < 1.0e-12f)
	{
		return HGameUIAffine();
	}

	HGameUIAffine result;
	result.A = D / determinant;
	result.B = -B / determinant;
	result.C = -C / determinant;
	result.D = A / determinant;
	result.X = -(result.A * X + result.C * Y);
	result.Y = -(result.B * X + result.D * Y);
	return result;
}

HGameUIAffine HGameUIAffine::Scaled(float32 scale) const
{
	// 선형 부분은 그대로, 옮기기만 배율을 따른다.
	HGameUIAffine result = *this;
	result.X = X * scale;
	result.Y = Y * scale;
	return result;
}

float32 HGameUIAffine::GetUniformScale() const
{
	return std::sqrt(HMath::Abs(A * D - B * C));
}

bool HGameUIAffine::IsIdentity() const
{
	return A == 1.0f && B == 0.0f && C == 0.0f && D == 1.0f && X == 0.0f && Y == 0.0f;
}

bool PGameUIElement::RemoveChild(const PSharedPtr<PGameUIElement>& child)
{
	for (auto it = _children.begin(); it != _children.end(); ++it)
	{
		if (it->GetRawPointer() == child.GetRawPointer())
		{
			_children.erase(it);
			return true;
		}
	}
	return false;
}

void PGameUIElement::ClearChildren()
{
	_children.clear();
}

const HList<PSharedPtr<PGameUIElement>>& PGameUIElement::GetChildren() const
{
	return _children;
}

PSharedPtr<PGameUIElement> PGameUIElement::FindChild(const PName& name, bool bRecursive) const
{
	for (const PSharedPtr<PGameUIElement>& child : _children)
	{
		if (child->_name == name)
		{
			return child;
		}
	}

	if (bRecursive)
	{
		for (const PSharedPtr<PGameUIElement>& child : _children)
		{
			PSharedPtr<PGameUIElement> found = child->FindChild(name, true);
			if (found.IsValid())
			{
				return found;
			}
		}
	}
	return nullptr;
}

const PName& PGameUIElement::GetName() const
{
	return _name;
}

void PGameUIElement::SetVisible(bool bVisible)
{
	_bVisible = bVisible;
}

bool PGameUIElement::IsVisible() const
{
	return _bVisible;
}

void PGameUIElement::SetHitTestVisible(bool bHitTestVisible)
{
	_bHitTestVisible = bHitTestVisible;
}

bool PGameUIElement::IsHitTestVisible() const
{
	return _bHitTestVisible;
}

void PGameUIElement::SetEnabled(bool bEnabled)
{
	_bEnabled = bEnabled;
}

bool PGameUIElement::IsEnabled() const
{
	return _bEnabled;
}

void PGameUIElement::SetDraggable(bool bDraggable)
{
	_bDraggable = bDraggable;
}

bool PGameUIElement::IsDraggable() const
{
	return _bDraggable;
}

void PGameUIElement::SetClipChildren(bool bClipChildren)
{
	_bClipChildren = bClipChildren;
}

bool PGameUIElement::IsClipChildren() const
{
	return _bClipChildren;
}

void PGameUIElement::SetWheelInput(bool bWheelInput)
{
	_bWheelInput = bWheelInput;
}

bool PGameUIElement::IsWheelInput() const
{
	return _bWheelInput;
}

bool PGameUIElement::onWheel(float32 wheel, float32 wheelHorizontal, const HVector2& position)
{
	// 람다로 받는 요소는 늘 받는다(알림에는 돌려줄 값이 없다). 바깥에 넘기려면 파생해서 false.
	OnWheel.BroadCast(*this, wheel, wheelHorizontal, position);
	return true;
}

void PGameUIElement::SetLayout(const HVector2& anchor, const HVector2& pivot, const HVector2& position, const HVector2& size)
{
	_anchor   = anchor;
	_pivot    = pivot;
	_position = position;
	_size     = size;
}

void PGameUIElement::SetAnchor(const HVector2& anchor)
{
	_anchor = anchor;
}

void PGameUIElement::SetPivot(const HVector2& pivot)
{
	_pivot = pivot;
}

void PGameUIElement::SetPosition(const HVector2& position)
{
	_position = position;
}

void PGameUIElement::SetSize(const HVector2& size)
{
	_size = size;
}

const HVector2& PGameUIElement::GetAnchor() const
{
	return _anchor;
}

const HVector2& PGameUIElement::GetPivot() const
{
	return _pivot;
}

const HVector2& PGameUIElement::GetPosition() const
{
	return _position;
}

const HVector2& PGameUIElement::GetSize() const
{
	return _size;
}

const HRect& PGameUIElement::GetLayoutRect() const
{
	return _layoutRect;
}

HRect PGameUIElement::CalculateLayoutRect(const HRect& parentRect) const
{
	const float32 parentWidth  = parentRect.right - parentRect.left;
	const float32 parentHeight = parentRect.bottom - parentRect.top;

	const float32 left = parentRect.left + _anchor.x * parentWidth  + _position.x - _pivot.x * _size.x;
	const float32 top  = parentRect.top  + _anchor.y * parentHeight + _position.y - _pivot.y * _size.y;
	return HRect(left, top, left + _size.x, top + _size.y);
}

void PGameUIElement::SetRenderTransform(const HGameUIRenderTransform& transform)
{
	_renderTransform     = transform;
	_bHasRenderTransform = true;
}

const HGameUIRenderTransform& PGameUIElement::GetRenderTransform() const
{
	return _renderTransform;
}

void PGameUIElement::ClearRenderTransform()
{
	_renderTransform     = HGameUIRenderTransform();
	_bHasRenderTransform = false;
}

void PGameUIElement::SetOpacity(float32 opacity)
{
	_opacity = HMath::Clamp(opacity, 0.0f, 1.0f);
}

float32 PGameUIElement::GetOpacity() const
{
	return _opacity;
}

HVector2 PGameUIElement::CanvasToLocal(const HVector2& canvasPoint) const
{
	return _bWorldTransformed ? _worldInverse.Apply(canvasPoint) : canvasPoint;
}

HRect PGameUIElement::LocalToCanvasBounds(const HRect& localRect) const
{
	if (_bWorldTransformed == false)
	{
		return localRect;
	}

	const HVector2 corners[4] =
	{
		_worldTransform.Apply(HVector2(localRect.left, localRect.top)),
		_worldTransform.Apply(HVector2(localRect.right, localRect.top)),
		_worldTransform.Apply(HVector2(localRect.right, localRect.bottom)),
		_worldTransform.Apply(HVector2(localRect.left, localRect.bottom)),
	};
	HRect bounds(corners[0].x, corners[0].y, corners[0].x, corners[0].y);
	for (int32 i = 1; i < 4; ++i)
	{
		bounds.left   = HMath::Min(bounds.left, corners[i].x);
		bounds.top    = HMath::Min(bounds.top, corners[i].y);
		bounds.right  = HMath::Max(bounds.right, corners[i].x);
		bounds.bottom = HMath::Max(bounds.bottom, corners[i].y);
	}
	return bounds;
}

void PGameUIElement::adoptChild(PSharedPtr<PGameUIElement> child, const PName& name)
{
	child->_name = name;
	_children.push_back(child);
}

void PGameUIElement::layout(const HRect& parentRect, JGGameWidget* ownerWidget, const HGameUIAffine& parentWorld, float32 parentOpacity)
{
	_layoutRect = CalculateLayoutRect(parentRect);

	// 그리기 변환은 배치를 바꾸지 않는다: 자식 배치는 변환 전 사각형으로, 그릴 때 · 판정 때 조상 변환과 곱한 것을 쓴다.
	_worldTransform = parentWorld;
	if (_bHasRenderTransform)
	{
		const HVector2 pivotPoint(_layoutRect.left + _pivot.x * _size.x, _layoutRect.top + _pivot.y * _size.y);
		_worldTransform = parentWorld * HGameUIAffine::FromRenderTransform(_renderTransform, pivotPoint);
	}
	_bWorldTransformed = _worldTransform.IsIdentity() == false;
	_worldInverse      = _bWorldTransformed ? _worldTransform.Inverse() : HGameUIAffine();
	_worldOpacity      = parentOpacity * _opacity;

	// 자식 자리를 이 요소가 정하는 경우(스크롤 칸)는 여기서 정한다.
	onLayout(ownerWidget);

	// PSharedPtr 는 const 를 전파한다(const 참조의 -> 는 const T*). 자식을 고치므로 비 const 로 돈다.
	for (PSharedPtr<PGameUIElement>& child : _children)
	{
		child->layout(_layoutRect, ownerWidget, _worldTransform, _worldOpacity);
	}
}

void PGameUIElement::buildDraw(HGameUIDrawContext& context)
{
	if (_bVisible == false)
	{
		return;
	}

	const bool bParentEnabled = context.bEnabled;
	context.bEnabled = bParentEnabled && _bEnabled;

	// 이 요소가 넣는 사각형은 모두 이 요소의 변환(렌더 타깃 픽셀) · 불투명도를 거친다. 자식은 자기 것을 다시 정한다.
	context.DrawList->Transform = _bWorldTransformed ? _worldTransform.Scaled(context.Scale) : HGameUIAffine();
	context.DrawList->Opacity   = _worldOpacity;

	onBuildDraw(context);
	if (_bClipChildren == false)
	{
		for (PSharedPtr<PGameUIElement>& child : _children)
		{
			child->buildDraw(context);
		}
	}
	else
	{
		// 자식 잘라내기: 받은 클립 ∩ 자기 사각형(픽셀, 변환이 있으면 감싸는 축 정렬 사각형)으로 자손을 그리고 되돌린다. 겹치면 안쪽이 더 좁아진다.
		const HRect bounds     = LocalToCanvasBounds(_layoutRect);
		const HRect parentClip = context.Clip;
		const HRect clip(HMath::Max(parentClip.left, bounds.left * context.Scale), HMath::Max(parentClip.top, bounds.top * context.Scale),
			HMath::Min(parentClip.right, bounds.right * context.Scale), HMath::Min(parentClip.bottom, bounds.bottom * context.Scale));
		if (clip.right > clip.left && clip.bottom > clip.top)
		{
			context.Clip = clip;
			for (PSharedPtr<PGameUIElement>& child : _children)
			{
				child->buildDraw(context);
			}
			context.Clip = parentClip;
		}
	}

	context.bEnabled = bParentEnabled;
}

PSharedPtr<PGameUIElement> PGameUIElement::hitTest(const PSharedPtr<PGameUIElement>& element, const HVector2& point, const PGameUIElement* excluded)
{
	if (element.IsValid() == false || element->_bVisible == false || element.GetRawPointer() == excluded)
	{
		return nullptr;
	}

	// 변환된 요소는 포인터를 변환 전 자리로 되돌려 사각형 안인지 본다(돌린 모양 안에서만 맞는다).
	const HRect&   rect  = element->_layoutRect;
	const HVector2 local = element->CanvasToLocal(point);
	const bool bInside = local.x >= rect.left && local.x < rect.right && local.y >= rect.top && local.y < rect.bottom;

	// 뒤에 추가한 자식이 위에 있으므로 뒤에서부터. 꺼진 요소 아래에서 맞으면 꺼진 요소가 받는다(막기만 하고 반응하지 않는다).
	// 자식 잘라내기를 켠 요소는 자기 사각형 밖에서 자손을 보지 않는다(잘려 안 보이는 자손은 맞지 않는다).
	if (element->_bClipChildren == false || bInside)
	{
		const HList<PSharedPtr<PGameUIElement>>& children = element->_children;
		for (auto it = children.rbegin(); it != children.rend(); ++it)
		{
			PSharedPtr<PGameUIElement> hit = hitTest(*it, point, excluded);
			if (hit.IsValid())
			{
				return element->_bEnabled ? hit : element;
			}
		}
	}

	if (element->_bHitTestVisible && bInside)
	{
		return element;
	}
	return nullptr;
}
