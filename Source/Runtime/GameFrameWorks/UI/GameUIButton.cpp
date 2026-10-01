#include "PCH/PCH.h"
#include "UI/GameUIButton.h"
#include "UI/GameUIText.h"
#include "UI/GameUIFont.h"
#include "UI/GameUIDrawList.h"
#include "UI/GameUIManager.h"
#include "Classes/Texture.h"

PGameUIButton::PGameUIButton()
{
	_bHitTestVisible = true;
}

void PGameUIButton::SetStyle(const HGameUIButtonStyle& style)
{
	_style     = style;
	_bHasStyle = true;
}

void PGameUIButton::ClearStyle()
{
	_style     = HGameUIButtonStyle();
	_bHasStyle = false;
}

bool PGameUIButton::HasStyle() const
{
	return _bHasStyle;
}

void PGameUIButton::SetLabel(const PString& label)
{
	_label = label;
}

const PString& PGameUIButton::GetLabel() const
{
	return _label;
}

bool PGameUIButton::IsHovered() const
{
	return _bHovered;
}

bool PGameUIButton::IsPressed() const
{
	return _bPressed;
}

HGameUIButtonStyle PGameUIButton::ResolveStyle(const PGameUIManager* manager) const
{
	if (_bHasStyle || manager == nullptr)
	{
		return _style;
	}
	return manager->GetDefaultButtonStyle();
}

const HLinearColor& PGameUIButton::PickStateColor(const HGameUIButtonStyle& style, bool bEnabled, bool bHovered, bool bPressed)
{
	if (bEnabled == false)
	{
		return style.Disabled;
	}
	if (bPressed && bHovered)
	{
		return style.Pressed;
	}
	if (bHovered)
	{
		return style.Hovered;
	}
	return style.Normal;
}

void PGameUIButton::onBuildDraw(HGameUIDrawContext& context)
{
	const HGameUIButtonStyle style = ResolveStyle(context.Manager);
	const bool bEnabled = context.bEnabled;

	const float32 scale = context.Scale;
	const HRect pixelRect(_layoutRect.left * scale, _layoutRect.top * scale, _layoutRect.right * scale, _layoutRect.bottom * scale);
	context.DrawList->AddQuad(pixelRect, HRect(0.0f, 0.0f, 1.0f, 1.0f), PickStateColor(style, bEnabled, _bHovered, _bPressed), style.Texture, nullptr, context.Clip);

	if (_label.Empty())
	{
		return;
	}

	PSharedPtr<PGameUIFont> font = PGameUIText::ResolveFont(style.Label.Font, context.Manager);
	if (font.IsValid())
	{
		const HLinearColor& labelColor = bEnabled ? style.Label.Color : style.DisabledLabelColor;
		PGameUIText::AddTextDraw(context, *font, _label, style.Label.FontSize, _layoutRect, labelColor,
			EGameUIHorizontalAlign::Center, EGameUIVerticalAlign::Middle);
	}
}

void PGameUIButton::onPointerEnter()
{
	_bHovered = true;
}

void PGameUIButton::onPointerLeave()
{
	_bHovered = false;
}

void PGameUIButton::onPointerDown(EGameUIPointerButton button)
{
	// 꺼진 버튼은 누름 상태가 되지 않는다(입력은 관리자가 받아 아래로 넘기지 않는다).
	if (button == EGameUIPointerButton::Left && _bEnabled)
	{
		_bPressed = true;
	}
}

void PGameUIButton::onPointerUp(EGameUIPointerButton button, bool bInside)
{
	if (button != EGameUIPointerButton::Left)
	{
		return;
	}

	const bool bWasPressed = _bPressed;
	_bPressed = false;
	if (bWasPressed && bInside && _bEnabled)
	{
		OnClicked.BroadCast();
	}
}
