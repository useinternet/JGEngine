#include "PCH/PCH.h"
#include "UI/GameUIElement.h"

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

void PGameUIElement::adoptChild(PSharedPtr<PGameUIElement> child, const PName& name)
{
	child->_name = name;
	_children.push_back(child);
}

void PGameUIElement::layout(const HRect& parentRect)
{
	const float32 parentWidth  = parentRect.right - parentRect.left;
	const float32 parentHeight = parentRect.bottom - parentRect.top;

	const float32 left = parentRect.left + _anchor.x * parentWidth  + _position.x - _pivot.x * _size.x;
	const float32 top  = parentRect.top  + _anchor.y * parentHeight + _position.y - _pivot.y * _size.y;
	_layoutRect = HRect(left, top, left + _size.x, top + _size.y);

	// PSharedPtr 는 const 를 전파한다(const 참조의 -> 는 const T*). 자식을 고치므로 비 const 로 돈다.
	for (PSharedPtr<PGameUIElement>& child : _children)
	{
		child->layout(_layoutRect);
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

	onBuildDraw(context);
	for (PSharedPtr<PGameUIElement>& child : _children)
	{
		child->buildDraw(context);
	}

	context.bEnabled = bParentEnabled;
}

PSharedPtr<PGameUIElement> PGameUIElement::hitTest(const PSharedPtr<PGameUIElement>& element, const HVector2& point)
{
	if (element.IsValid() == false || element->_bVisible == false)
	{
		return nullptr;
	}

	// 뒤에 추가한 자식이 위에 있으므로 뒤에서부터. 꺼진 요소 아래에서 맞으면 꺼진 요소가 받는다(막기만 하고 반응하지 않는다).
	const HList<PSharedPtr<PGameUIElement>>& children = element->_children;
	for (auto it = children.rbegin(); it != children.rend(); ++it)
	{
		PSharedPtr<PGameUIElement> hit = hitTest(*it, point);
		if (hit.IsValid())
		{
			return element->_bEnabled ? hit : element;
		}
	}

	const HRect& rect = element->_layoutRect;
	const bool bInside = point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
	if (element->_bHitTestVisible && bInside)
	{
		return element;
	}
	return nullptr;
}
