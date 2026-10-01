#include "PCH/PCH.h"
#include "UI/GameUITextInput.h"
#include "UI/GameUIText.h"
#include "UI/GameUIFont.h"
#include "UI/GameUIDrawList.h"
#include "UI/GameUIManager.h"
#include "UI/GameUIUtf8.h"
#include "Classes/Texture.h"

namespace
{
	constexpr float32 CaretWidth = 2.0f;   // 논리 단위

	// 입력란에 넣지 않는 글자: C0 제어 문자(줄바꿈 · 탭 포함) · DEL · C1 제어 문자
	bool isControl(uint32 codepoint)
	{
		return codepoint < 0x20 || (codepoint >= 0x7F && codepoint < 0xA0);
	}

	HRect toPixels(const HRect& rect, float32 scale)
	{
		return HRect(rect.left * scale, rect.top * scale, rect.right * scale, rect.bottom * scale);
	}

	HRect intersect(const HRect& a, const HRect& b)
	{
		return HRect(HMath::Max(a.left, b.left), HMath::Max(a.top, b.top), HMath::Min(a.right, b.right), HMath::Min(a.bottom, b.bottom));
	}

	bool isEmptyRect(const HRect& rect)
	{
		return rect.right <= rect.left || rect.bottom <= rect.top;
	}
}

PGameUITextInput::PGameUITextInput()
{
	_bHitTestVisible = true;
}

void PGameUITextInput::SetText(const PString& text)
{
	HList<uint32> codepoints;
	filterInto(text, codepoints);
	if (_maxLength > 0 && (int32)codepoints.size() > _maxLength)
	{
		codepoints.resize((size_t)_maxLength);
	}

	_codepoints = codepoints;
	_caret      = GetLength();
	_anchor     = _caret;
	rebuildText();
}

const PString& PGameUITextInput::GetText() const
{
	return _text;
}

int32 PGameUITextInput::GetLength() const
{
	return (int32)_codepoints.size();
}

void PGameUITextInput::SetPlaceholder(const PString& placeholder)
{
	_placeholder = placeholder;
}

const PString& PGameUITextInput::GetPlaceholder() const
{
	return _placeholder;
}

void PGameUITextInput::SetMaxLength(int32 maxLength)
{
	_maxLength = HMath::Max(0, maxLength);
	if (_maxLength > 0 && GetLength() > _maxLength)
	{
		_codepoints.resize((size_t)_maxLength);
		_caret  = HMath::Min(_caret, _maxLength);
		_anchor = HMath::Min(_anchor, _maxLength);
		rebuildText();
	}
}

int32 PGameUITextInput::GetMaxLength() const
{
	return _maxLength;
}

void PGameUITextInput::SetAllowedCharacters(const PString& characters)
{
	_allowedCharacters = characters;
	_allowedCodepoints.clear();

	const char* cursor = characters.GetCStr();
	const char* end    = cursor + characters.Size();
	uint32 codepoint = 0;
	while (HGameUIUtf8::Next(cursor, end, codepoint))
	{
		if (isControl(codepoint) == false)
		{
			_allowedCodepoints.push_back(codepoint);
		}
	}

	// 지금 글에서 허용 안 되는 글자를 뺀다.
	HList<uint32> kept;
	for (uint32 existing : _codepoints)
	{
		if (isAllowed(existing))
		{
			kept.push_back(existing);
		}
	}
	if (kept.size() != _codepoints.size())
	{
		_codepoints = kept;
		_caret      = GetLength();
		_anchor     = _caret;
		rebuildText();
	}
}

const PString& PGameUITextInput::GetAllowedCharacters() const
{
	return _allowedCharacters;
}

void PGameUITextInput::SetStyle(const HGameUITextInputStyle& style)
{
	_style     = style;
	_bHasStyle = true;
}

void PGameUITextInput::ClearStyle()
{
	_style     = HGameUITextInputStyle();
	_bHasStyle = false;
}

bool PGameUITextInput::HasStyle() const
{
	return _bHasStyle;
}

HGameUITextInputStyle PGameUITextInput::ResolveStyle(const PGameUIManager* manager) const
{
	if (_bHasStyle || manager == nullptr)
	{
		return _style;
	}
	return manager->GetDefaultTextInputStyle();
}

bool PGameUITextInput::IsFocused() const
{
	return _bFocused;
}

bool PGameUITextInput::IsHovered() const
{
	return _bHovered;
}

int32 PGameUITextInput::GetCaretPosition() const
{
	return _caret;
}

void PGameUITextInput::GetSelection(int32& outStart, int32& outEnd) const
{
	outStart = HMath::Min(_caret, _anchor);
	outEnd   = HMath::Max(_caret, _anchor);
}

PString PGameUITextInput::GetSelectedText() const
{
	int32 start = 0;
	int32 end   = 0;
	GetSelection(start, end);
	if (start == end)
	{
		return PString();
	}

	HList<uint32> selected(_codepoints.begin() + start, _codepoints.begin() + end);
	return HGameUIUtf8::Encode(selected);
}

void PGameUITextInput::onBuildDraw(HGameUIDrawContext& context)
{
	const HGameUITextInputStyle style = ResolveStyle(context.Manager);
	const float32 scale     = context.Scale;
	const HRect   pixelRect = toPixels(_layoutRect, scale);

	// 배경: 꺼짐 > 포커스 > 호버 > 보통
	const HLinearColor* background = &style.Normal;
	if (context.bEnabled == false)
	{
		background = &style.Disabled;
	}
	else if (_bFocused)
	{
		background = &style.Focused;
	}
	else if (_bHovered)
	{
		background = &style.Hovered;
	}
	context.DrawList->AddQuad(pixelRect, HRect(0.0f, 0.0f, 1.0f, 1.0f), *background, style.Texture, nullptr, context.Clip);

	PSharedPtr<PGameUIFont> font = PGameUIText::ResolveFont(style.Text.Font, context.Manager);
	if (font.IsValid() == false)
	{
		return;
	}

	const float32 fontSize     = style.Text.FontSize;
	const float32 contentLeft  = _layoutRect.left + style.Padding;
	const float32 contentRight = _layoutRect.right - style.Padding;
	HList<float32> boundaries;
	measureBoundaries(*font, fontSize, boundaries);
	updateScroll(boundaries, HMath::Max(0.0f, contentRight - contentLeft));

	const float32 lineHeight = font->MeasureLineHeight(fontSize);
	const float32 lineTop    = _layoutRect.top + ((_layoutRect.bottom - _layoutRect.top) - lineHeight) * 0.5f;
	const float32 originX    = contentLeft - _scroll;

	// 선택 · 글은 좌우 여백 안쪽만 보인다.
	HGameUIDrawContext inner = context;
	inner.Clip = intersect(context.Clip, toPixels(HRect(contentLeft, _layoutRect.top, contentRight, _layoutRect.bottom), scale));
	if (isEmptyRect(inner.Clip) == false)
	{
		int32 start = 0;
		int32 end   = 0;
		GetSelection(start, end);
		if (_bFocused && start != end)
		{
			const HRect selection(originX + boundaries[(size_t)start], lineTop, originX + boundaries[(size_t)end], lineTop + lineHeight);
			inner.DrawList->AddQuad(toPixels(selection, scale), HRect(0.0f, 0.0f, 1.0f, 1.0f), style.SelectionColor, nullptr, nullptr, inner.Clip);
		}

		const bool     bPlaceholder = _codepoints.empty();
		const PString& shown        = bPlaceholder ? _placeholder : _text;
		if (shown.Empty() == false)
		{
			HLinearColor color = style.Text.Color;
			if (bPlaceholder)
			{
				color = style.PlaceholderColor;
			}
			else if (context.bEnabled == false)
			{
				color = style.DisabledTextColor;
			}

			HGameUITextLayout layout;
			font->LayoutText(shown, fontSize, 0.0f, EGameUITextWrap::None, 0, false, layout);
			const float32 textLeft = bPlaceholder ? contentLeft : originX;
			PGameUIText::AddTextDraw(inner, *font, shown, layout, fontSize, HRect(textLeft, _layoutRect.top, textLeft + layout.Size.x, _layoutRect.bottom), color,
				EGameUIHorizontalAlign::Left, EGameUIVerticalAlign::Middle);
		}
	}

	// 커서(깜빡임). 끝에 있을 때 잘리지 않게 여백까지는 그린다.
	if (_bFocused && context.bEnabled && context.Manager != nullptr && context.Manager->isCaretBlinkOn())
	{
		const float32 caretX = originX + boundaries[(size_t)_caret];
		const HRect caret(caretX - CaretWidth * 0.5f, lineTop, caretX + CaretWidth * 0.5f, lineTop + lineHeight);
		context.DrawList->AddQuad(toPixels(caret, scale), HRect(0.0f, 0.0f, 1.0f, 1.0f), style.CaretColor, nullptr, nullptr, intersect(context.Clip, pixelRect));
	}
}

void PGameUITextInput::onPointerEnter()
{
	_bHovered = true;
}

void PGameUITextInput::onPointerLeave()
{
	_bHovered = false;
}

void PGameUITextInput::setFocused(bool bFocused)
{
	_bFocused = bFocused;
	if (bFocused == false)
	{
		_anchor = _caret;
	}
}

void PGameUITextInput::insertText(const PString& text)
{
	HList<uint32> incoming;
	filterInto(text, incoming);
	if (incoming.empty())
	{
		return;
	}

	// 선택이 있으면 바꾼다. 넣을 수 있는 만큼만 넣는다(최대 길이).
	int32 start = 0;
	int32 end   = 0;
	GetSelection(start, end);
	const bool bReplaced = start != end;
	if (bReplaced)
	{
		_codepoints.erase(_codepoints.begin() + start, _codepoints.begin() + end);
		_caret  = start;
		_anchor = start;
	}

	int32 count = (int32)incoming.size();
	if (_maxLength > 0)
	{
		count = HMath::Min(count, _maxLength - GetLength());
	}
	if (count > 0)
	{
		_codepoints.insert(_codepoints.begin() + _caret, incoming.begin(), incoming.begin() + count);
		_caret += count;
		_anchor = _caret;
	}

	if (count > 0 || bReplaced)
	{
		rebuildText();
		notifyChanged();
	}
}

void PGameUITextInput::handleEditKey(EGameUIKey key, bool bShift)
{
	int32 start = 0;
	int32 end   = 0;
	GetSelection(start, end);
	const bool bSelection = start != end;

	switch (key)
	{
	case EGameUIKey::Backspace:
		if (bSelection)
		{
			deleteRange(start, end);
		}
		else if (_caret > 0)
		{
			deleteRange(_caret - 1, _caret);
		}
		break;
	case EGameUIKey::Delete:
		if (bSelection)
		{
			deleteRange(start, end);
		}
		else if (_caret < GetLength())
		{
			deleteRange(_caret, _caret + 1);
		}
		break;
	case EGameUIKey::Left:
		// 선택이 있을 때 Shift 없이 누르면 선택의 앞 끝으로 간다.
		if (bSelection && bShift == false)
		{
			moveCaret(start, false);
		}
		else
		{
			moveCaret(_caret - 1, bShift);
		}
		break;
	case EGameUIKey::Right:
		if (bSelection && bShift == false)
		{
			moveCaret(end, false);
		}
		else
		{
			moveCaret(_caret + 1, bShift);
		}
		break;
	case EGameUIKey::Home:
		moveCaret(0, bShift);
		break;
	case EGameUIKey::End:
		moveCaret(GetLength(), bShift);
		break;
	case EGameUIKey::SelectAll:
		selectAll();
		break;
	default:
		break;
	}
}

void PGameUITextInput::selectAll()
{
	_anchor = 0;
	_caret  = GetLength();
}

void PGameUITextInput::commit()
{
	// 처리기가 SetText 를 불러도 인자가 바뀌지 않게 사본을 넘긴다.
	const PString text = _text;
	OnCommitted.BroadCast(text);
}

void PGameUITextInput::placeCaret(float32 canvasX, PGameUIManager* manager)
{
	const HGameUITextInputStyle style = ResolveStyle(manager);
	PSharedPtr<PGameUIFont> font = PGameUIText::ResolveFont(style.Text.Font, manager);
	if (font.IsValid() == false)
	{
		moveCaret(GetLength(), false);
		return;
	}

	HList<float32> boundaries;
	measureBoundaries(*font, style.Text.FontSize, boundaries);
	const float32 localX = canvasX - (_layoutRect.left + style.Padding - _scroll);

	int32 nearest = 0;
	for (int32 i = 1; i < (int32)boundaries.size(); ++i)
	{
		if (HMath::Abs(boundaries[(size_t)i] - localX) < HMath::Abs(boundaries[(size_t)nearest] - localX))
		{
			nearest = i;
		}
	}
	moveCaret(nearest, false);
}

bool PGameUITextInput::getCaretRect(PGameUIManager* manager, HRect& outRect)
{
	const HGameUITextInputStyle style = ResolveStyle(manager);
	PSharedPtr<PGameUIFont> font = PGameUIText::ResolveFont(style.Text.Font, manager);
	if (font.IsValid() == false)
	{
		return false;
	}

	const float32 fontSize     = style.Text.FontSize;
	const float32 contentLeft  = _layoutRect.left + style.Padding;
	const float32 contentRight = _layoutRect.right - style.Padding;
	HList<float32> boundaries;
	measureBoundaries(*font, fontSize, boundaries);
	updateScroll(boundaries, HMath::Max(0.0f, contentRight - contentLeft));

	const float32 lineHeight = font->MeasureLineHeight(fontSize);
	const float32 lineTop    = _layoutRect.top + ((_layoutRect.bottom - _layoutRect.top) - lineHeight) * 0.5f;
	const float32 caretX     = contentLeft - _scroll + boundaries[(size_t)_caret];
	outRect = HRect(caretX - CaretWidth * 0.5f, lineTop, caretX + CaretWidth * 0.5f, lineTop + lineHeight);
	return true;
}

void PGameUITextInput::moveCaret(int32 position, bool bExtend)
{
	_caret = HMath::Clamp(position, 0, GetLength());
	if (bExtend == false)
	{
		_anchor = _caret;
	}
}

void PGameUITextInput::deleteRange(int32 start, int32 end)
{
	_codepoints.erase(_codepoints.begin() + start, _codepoints.begin() + end);
	_caret  = start;
	_anchor = start;
	rebuildText();
	notifyChanged();
}

bool PGameUITextInput::isAllowed(uint32 codepoint) const
{
	if (_allowedCodepoints.empty())
	{
		return true;
	}
	for (uint32 allowed : _allowedCodepoints)
	{
		if (allowed == codepoint)
		{
			return true;
		}
	}
	return false;
}

void PGameUITextInput::filterInto(const PString& text, HList<uint32>& outCodepoints) const
{
	const char* cursor = text.GetCStr();
	const char* end    = cursor + text.Size();
	uint32 codepoint = 0;
	while (HGameUIUtf8::Next(cursor, end, codepoint))
	{
		if (isControl(codepoint) || isAllowed(codepoint) == false)
		{
			continue;
		}
		outCodepoints.push_back(codepoint);
	}
}

void PGameUITextInput::rebuildText()
{
	_text = HGameUIUtf8::Encode(_codepoints);
}

void PGameUITextInput::notifyChanged()
{
	// 처리기가 SetText 를 불러도 인자가 바뀌지 않게 사본을 넘긴다.
	const PString text = _text;
	OnTextChanged.BroadCast(text);
}

void PGameUITextInput::measureBoundaries(const PGameUIFont& font, float32 fontSize, HList<float32>& outBoundaries) const
{
	outBoundaries.clear();
	outBoundaries.reserve(_codepoints.size() + 1);

	float32 x = 0.0f;
	for (size_t i = 0; i < _codepoints.size(); ++i)
	{
		if (i > 0)
		{
			x += font.MeasureKerning(_codepoints[i - 1], _codepoints[i], fontSize);
		}
		outBoundaries.push_back(x);
		x += font.MeasureAdvance(_codepoints[i], fontSize);
	}
	outBoundaries.push_back(x);
}

void PGameUITextInput::updateScroll(const HList<float32>& boundaries, float32 visibleWidth)
{
	const float32 textWidth = boundaries.back();
	if (textWidth <= visibleWidth)
	{
		_scroll = 0.0f;
		return;
	}

	const float32 caretX = boundaries[(size_t)_caret];
	if (caretX - _scroll > visibleWidth)
	{
		_scroll = caretX - visibleWidth;
	}
	if (caretX - _scroll < 0.0f)
	{
		_scroll = caretX;
	}
	_scroll = HMath::Clamp(_scroll, 0.0f, textWidth - visibleWidth);
}
