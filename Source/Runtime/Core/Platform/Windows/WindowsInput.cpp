#include "PCH/PCH.h"
#include "WindowsInput.h"
#include "Platform/Platform.h"
#include <windowsx.h>
#include <array>

namespace
{
	HVector2 pointFromLParam(LPARAM lParam)
	{
		return HVector2((float32)GET_X_LPARAM(lParam), (float32)GET_Y_LPARAM(lParam));
	}

	uint32 buttonBit(EPointerButton button)
	{
		return 1u << (uint32)button;
	}

	// 글자로 보내지 않는 제어 문자(Backspace · Tab · Enter · Esc 등은 KeyDown 으로 온다)
	bool isControlCharacter(uint32 codepoint)
	{
		return codepoint < 0x20 || codepoint == 0x7F;
	}

	bool isHighSurrogate(uint32 unit)
	{
		return unit >= 0xD800 && unit <= 0xDBFF;
	}

	bool isLowSurrogate(uint32 unit)
	{
		return unit >= 0xDC00 && unit <= 0xDFFF;
	}
}

bool HWindowsInputTranslator::Translate(uint32 msg, WPARAM wParam, LPARAM lParam, EInputModifier modifiers, HInputEvent& outEvent)
{
	HInputEvent event;
	event.Modifiers = modifiers;

	switch (msg)
	{
	case WM_MOUSEMOVE:
		event.Type     = EInputEventType::PointerMove;
		event.Position = pointFromLParam(lParam);
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
		event.Type     = EInputEventType::PointerDown;
		event.Position = pointFromLParam(lParam);
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
		{
			event.Button = EPointerButton::Left;
		}
		else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK)
		{
			event.Button = EPointerButton::Right;
		}
		else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK)
		{
			event.Button = EPointerButton::Middle;
		}
		else
		{
			event.Button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? EPointerButton::X1 : EPointerButton::X2;
		}
		_pressedButtons |= buttonBit(event.Button);
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		event.Type     = EInputEventType::PointerUp;
		event.Position = pointFromLParam(lParam);
		if (msg == WM_LBUTTONUP)
		{
			event.Button = EPointerButton::Left;
		}
		else if (msg == WM_RBUTTONUP)
		{
			event.Button = EPointerButton::Right;
		}
		else if (msg == WM_MBUTTONUP)
		{
			event.Button = EPointerButton::Middle;
		}
		else
		{
			event.Button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? EPointerButton::X1 : EPointerButton::X2;
		}
		_pressedButtons &= ~buttonBit(event.Button);
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	{
		const float32 steps = (float32)GET_WHEEL_DELTA_WPARAM(wParam) / (float32)WHEEL_DELTA;
		event.Type       = EInputEventType::PointerWheel;
		event.Position   = pointFromLParam(lParam);   // 화면 좌표
		event.WheelDelta = (msg == WM_MOUSEWHEEL) ? HVector2(0.0f, steps) : HVector2(steps, 0.0f);
		break;
	}

	case WM_MOUSELEAVE:
		// 버튼을 누르고 있으면 창이 포인터를 잡고 있으므로 나감으로 치지 않는다(뗄 때까지 이동이 온다).
		if (_pressedButtons != 0)
		{
			return false;
		}
		event.Type = EInputEventType::PointerLeave;
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		event.Type    = EInputEventType::KeyDown;
		event.Key     = ToEngineKeyCode((uint32)wParam);
		event.bRepeat = (lParam & (1 << 30)) != 0;   // 30번 비트 = 이미 눌려 있었음
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		event.Type = EInputEventType::KeyUp;
		event.Key  = ToEngineKeyCode((uint32)wParam);
		break;

	case WM_CHAR:
	{
		// 유니코드 창의 WM_CHAR = UTF-16 한 단위. BMP 밖 글자는 앞쪽 · 뒤쪽 서로게이트 두 번으로 온다.
		const uint32 unit = (uint32)(wParam & 0xFFFF);
		if (isHighSurrogate(unit))
		{
			_pendingHighSurrogate = (uint16)unit;
			return false;
		}

		uint32 codepoint = unit;
		if (isLowSurrogate(unit))
		{
			if (_pendingHighSurrogate == 0)
			{
				return false;   // 짝 없는 뒤쪽 서로게이트
			}
			codepoint = 0x10000 + (((uint32)_pendingHighSurrogate - 0xD800) << 10) + (unit - 0xDC00);
		}
		_pendingHighSurrogate = 0;
		if (isControlCharacter(codepoint))
		{
			return false;
		}
		event.Type      = EInputEventType::Char;
		event.Codepoint = codepoint;
		break;
	}

	case WM_SETFOCUS:
		event.Type = EInputEventType::FocusGained;
		break;

	case WM_KILLFOCUS:
		event.Type = EInputEventType::FocusLost;
		Reset();
		break;

	default:
		return false;
	}

	outEvent = event;
	return true;
}

bool HWindowsInputTranslator::IsAnyButtonDown() const
{
	return _pressedButtons != 0;
}

bool HWindowsInputTranslator::IsButtonDown(EPointerButton button) const
{
	return (_pressedButtons & buttonBit(button)) != 0;
}

void HWindowsInputTranslator::Reset()
{
	_pressedButtons       = 0;
	_pendingHighSurrogate = 0;
}

EKeyCode HWindowsInputTranslator::ToEngineKeyCode(uint32 virtualKey)
{
	// HPlatform::GetSystemKeyCode(엔진 키 → 가상 키)를 한 번 거꾸로 펼친 표.
	static const std::array<EKeyCode, 256> table = []()
	{
		std::array<EKeyCode, 256> result;
		result.fill(EKeyCode::Unknown);
		for (int32 key = (int32)EKeyCode::Unknown + 1; key <= (int32)EKeyCode::MouseMidButton; ++key)
		{
			const int32 virtualCode = HPlatform::GetSystemKeyCode((EKeyCode)key);
			if (virtualCode > 0 && virtualCode < 256 && result[virtualCode] == EKeyCode::Unknown)
			{
				result[virtualCode] = (EKeyCode)key;
			}
		}
		return result;
	}();

	if (virtualKey >= 256)
	{
		return EKeyCode::Unknown;
	}
	return table[virtualKey];
}
