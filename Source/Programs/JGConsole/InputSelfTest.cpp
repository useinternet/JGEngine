#include "PCH/PCH.h"

#include <iostream>
#include "Core.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "Input/InputEvent.h"
#include "Platform/Platform.h"
#include "Platform/JWindow.h"
#include "Math/Rect.h"
#ifdef _PLATFORM_WINDOWS
#include "Platform/Windows/WindowsInput.h"
#endif

// JGConsole.exe input.selftest [-window=<초>] [-clipboard] — 엔진 입력 계층(GP-C1) 자체 검사.
// 기본: 창 없이 가짜 Win32 메시지를 변환기(HWindowsInputTranslator)에 넣어 이벤트를 본다.
// -window=N: 진짜 창 "JGInputCheck" 를 N초 띄우고 OnInput 으로 받은 이벤트를 한 줄씩 출력한다(밖에서 PostMessage 로 입력을 넣어 본다. WM_CLOSE 로 일찍 끝남).
// -clipboard: 클립보드에 UTF-8 글을 넣고 다시 읽는다 — 사용자 클립보드를 바꾸므로 기본 검사에는 넣지 않는다.

namespace
{
	int32 GCheckCount   = 0;
	int32 GFailureCount = 0;

	void check(bool bCondition, const char* caseName)
	{
		++GCheckCount;
		if (bCondition == false)
		{
			++GFailureCount;
			std::cout << "input.selftest FAILED: " << caseName << std::endl;
		}
	}

	bool nearlyEqual(float32 a, float32 b)
	{
		return HMath::Abs(a - b) <= 0.001f;
	}

#ifdef _PLATFORM_WINDOWS
	LPARAM pointLParam(int32 x, int32 y)
	{
		return MAKELPARAM((WORD)(int16)x, (WORD)(int16)y);
	}

	// 변환 하나. 이벤트가 나오면 true.
	bool translate(HWindowsInputTranslator& translator, uint32 msg, WPARAM wParam, LPARAM lParam, HInputEvent& outEvent, EInputModifier modifiers = EInputModifier::None)
	{
		outEvent = HInputEvent();
		return translator.Translate(msg, wParam, lParam, modifiers, outEvent);
	}

	void testPointer()
	{
		HWindowsInputTranslator translator;
		HInputEvent event;

		check(translate(translator, WM_MOUSEMOVE, MK_SHIFT, pointLParam(10, 20), event, EInputModifier::Shift)
			&& event.Type == EInputEventType::PointerMove && nearlyEqual(event.Position.x, 10.0f) && nearlyEqual(event.Position.y, 20.0f)
			&& event.Modifiers == EInputModifier::Shift, "pointer: move gives client pixels and modifiers");
		check(translate(translator, WM_MOUSEMOVE, 0, pointLParam(-5, 30), event) && nearlyEqual(event.Position.x, -5.0f),
			"pointer: negative coordinates (captured outside the window) stay signed");

		check(translate(translator, WM_LBUTTONDOWN, MK_LBUTTON, pointLParam(1, 2), event) && event.Type == EInputEventType::PointerDown
			&& event.Button == EPointerButton::Left && translator.IsButtonDown(EPointerButton::Left), "pointer: left down");
		check(translate(translator, WM_RBUTTONDOWN, MK_LBUTTON | MK_RBUTTON, pointLParam(1, 2), event) && event.Button == EPointerButton::Right
			&& translator.IsButtonDown(EPointerButton::Left) && translator.IsButtonDown(EPointerButton::Right), "pointer: right down while left is held");
		check(translate(translator, WM_LBUTTONUP, MK_RBUTTON, pointLParam(3, 4), event) && event.Type == EInputEventType::PointerUp
			&& event.Button == EPointerButton::Left && translator.IsButtonDown(EPointerButton::Left) == false && translator.IsAnyButtonDown(), "pointer: left up keeps right");
		check(translate(translator, WM_MOUSELEAVE, 0, 0, event) == false, "pointer: no leave while a button is held (the window keeps the pointer)");
		check(translate(translator, WM_RBUTTONUP, 0, pointLParam(3, 4), event) && translator.IsAnyButtonDown() == false, "pointer: right up");
		check(translate(translator, WM_MOUSELEAVE, 0, 0, event) && event.Type == EInputEventType::PointerLeave, "pointer: leave");

		check(translate(translator, WM_MBUTTONDOWN, MK_MBUTTON, pointLParam(0, 0), event) && event.Button == EPointerButton::Middle, "pointer: middle down");
		check(translate(translator, WM_MBUTTONUP, 0, pointLParam(0, 0), event) && event.Button == EPointerButton::Middle, "pointer: middle up");
		check(translate(translator, WM_XBUTTONDOWN, MAKEWPARAM(0, XBUTTON2), pointLParam(0, 0), event) && event.Button == EPointerButton::X2, "pointer: X2 down");
		check(translate(translator, WM_XBUTTONUP, MAKEWPARAM(0, XBUTTON1), pointLParam(0, 0), event) && event.Button == EPointerButton::X1, "pointer: X1 up");
		check(translate(translator, WM_XBUTTONUP, MAKEWPARAM(0, XBUTTON2), pointLParam(0, 0), event) && translator.IsAnyButtonDown() == false, "pointer: X2 up");
		check(translate(translator, WM_LBUTTONDBLCLK, MK_LBUTTON, pointLParam(5, 5), event) && event.Type == EInputEventType::PointerDown, "pointer: double click is a down");
		translate(translator, WM_LBUTTONUP, 0, pointLParam(5, 5), event);

		check(translate(translator, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)(int16)(-2 * WHEEL_DELTA)), pointLParam(300, 400), event)
			&& event.Type == EInputEventType::PointerWheel && nearlyEqual(event.WheelDelta.y, -2.0f) && nearlyEqual(event.WheelDelta.x, 0.0f)
			&& nearlyEqual(event.Position.x, 300.0f), "pointer: wheel = steps (y), position left in screen pixels for the window");
		check(translate(translator, WM_MOUSEHWHEEL, MAKEWPARAM(0, (WORD)WHEEL_DELTA), pointLParam(0, 0), event)
			&& nearlyEqual(event.WheelDelta.x, 1.0f) && nearlyEqual(event.WheelDelta.y, 0.0f), "pointer: horizontal wheel = steps (x)");
	}

	void testKeys()
	{
		HWindowsInputTranslator translator;
		HInputEvent event;

		check(translate(translator, WM_KEYDOWN, 'A', 0x001E0001, event, EInputModifier::Ctrl) && event.Type == EInputEventType::KeyDown
			&& event.Key == EKeyCode::A && event.bRepeat == false && event.Modifiers == EInputModifier::Ctrl, "keys: key down with modifiers");
		check(translate(translator, WM_KEYDOWN, 'A', 0x401E0001, event) && event.bRepeat, "keys: held key repeats (bit 30)");
		check(translate(translator, WM_KEYUP, VK_RETURN, (LPARAM)0xC01C0001, event) && event.Type == EInputEventType::KeyUp && event.Key == EKeyCode::Enter, "keys: key up");
		check(translate(translator, WM_SYSKEYDOWN, VK_F4, 0x203E0001, event) && event.Type == EInputEventType::KeyDown && event.Key == EKeyCode::F4, "keys: system key (Alt+F4) is a key down");
		check(translate(translator, WM_KEYDOWN, VK_SHIFT, 0, event) && event.Key == EKeyCode::Unknown, "keys: a key outside the engine table is Unknown");

		const EKeyCode keys[] = { EKeyCode::Esc, EKeyCode::Tab, EKeyCode::BackSpace, EKeyCode::Space, EKeyCode::Left, EKeyCode::Right,
			EKeyCode::Up, EKeyCode::Down, EKeyCode::Home, EKeyCode::End, EKeyCode::Del, EKeyCode::Num0, EKeyCode::NumPad9, EKeyCode::F12, EKeyCode::Z };
		bool bRoundTrip = true;
		for (EKeyCode key : keys)
		{
			if (HWindowsInputTranslator::ToEngineKeyCode((uint32)HPlatform::GetSystemKeyCode(key)) != key)
			{
				bRoundTrip = false;
			}
		}
		check(bRoundTrip, "keys: virtual key -> engine key is the reverse of HPlatform::GetSystemKeyCode");
		check(HWindowsInputTranslator::ToEngineKeyCode(0x1FF) == EKeyCode::Unknown, "keys: out of range virtual key is Unknown");
	}

	void testCharacters()
	{
		HWindowsInputTranslator translator;
		HInputEvent event;

		check(translate(translator, WM_CHAR, 'a', 0, event) && event.Type == EInputEventType::Char && event.Codepoint == 'a', "char: ASCII");
		check(translate(translator, WM_CHAR, 0xD55C, 0, event) && event.Codepoint == 0xD55C, "char: Hangul U+D55C (IME committed text arrives as WM_CHAR)");
		check(translate(translator, WM_CHAR, 0xD83D, 0, event) == false, "char: a high surrogate waits for the low one");
		check(translate(translator, WM_CHAR, 0xDE00, 0, event) && event.Codepoint == 0x1F600, "char: a surrogate pair becomes one codepoint (U+1F600)");
		check(translate(translator, WM_CHAR, 0xDE00, 0, event) == false, "char: a lone low surrogate is dropped");
		translate(translator, WM_CHAR, 0xD83D, 0, event);
		check(translate(translator, WM_CHAR, 'b', 0, event) && event.Codepoint == 'b', "char: a broken pair drops the high surrogate, keeps the next character");
		check(translate(translator, WM_CHAR, 0x08, 0, event) == false && translate(translator, WM_CHAR, 0x0D, 0, event) == false
			&& translate(translator, WM_CHAR, 0x1B, 0, event) == false && translate(translator, WM_CHAR, 0x7F, 0, event) == false,
			"char: control characters are not sent (they come as key down)");
	}

	void testFocusAndOthers()
	{
		HWindowsInputTranslator translator;
		HInputEvent event;

		check(translate(translator, WM_SETFOCUS, 0, 0, event) && event.Type == EInputEventType::FocusGained, "focus: gained");
		translate(translator, WM_LBUTTONDOWN, MK_LBUTTON, pointLParam(0, 0), event);
		translate(translator, WM_CHAR, 0xD83D, 0, event);
		check(translate(translator, WM_KILLFOCUS, 0, 0, event) && event.Type == EInputEventType::FocusLost && translator.IsAnyButtonDown() == false,
			"focus: lost forgets held buttons");
		check(translate(translator, WM_CHAR, 0xDE00, 0, event) == false, "focus: lost forgets a pending surrogate");
		check(translate(translator, WM_PAINT, 0, 0, event) == false && translate(translator, WM_SIZE, 0, 0, event) == false, "others: non-input messages give no event");
	}

	// -window: 진짜 창으로 받은 이벤트를 출력한다.
	void onWindowInput(const HInputEvent& event)
	{
		static const char* const TypeNames[] = { "PointerMove", "PointerDown", "PointerUp", "PointerWheel", "PointerLeave", "KeyDown", "KeyUp", "Char", "FocusGained", "FocusLost" };
		std::cout << "input event: " << TypeNames[(int32)event.Type]
			<< " pos=(" << event.Position.x << "," << event.Position.y << ")"
			<< " button=" << (int32)event.Button
			<< " wheel=(" << event.WheelDelta.x << "," << event.WheelDelta.y << ")"
			<< " key=" << (int32)event.Key << " repeat=" << (event.bRepeat ? 1 : 0)
			<< " mods=" << (int32)event.Modifiers
			<< " codepoint=U+" << std::hex << std::uppercase << event.Codepoint << std::dec << std::nouppercase
			<< std::endl;
	}

	bool runWindow(int32 seconds, const PString& mode)
	{
		HJWindowArguments args;
		args.Title      = "JGInputCheck";
		args.Size       = HVector2Int(800, 600);
		args.Mode       = (mode == PString("borderless")) ? EWindowMode::BorderlessFullscreen : EWindowMode::Windowed;
		args.bResizable = (mode == PString("fixed")) == false;
		PSharedPtr<PJWindow> window = HPlatform::CreateJWindow(args);
		if (window.IsValid() == false)
		{
			std::cout << "input.selftest: fail to create the window" << std::endl;
			return false;
		}

		HWND handle = (HWND)window->GetHandle();
		const LONG style = ::GetWindowLongW(handle, GWL_STYLE);
		RECT rect = {};
		::GetWindowRect(handle, &rect);
		std::cout << "input window: mode=" << (int32)args.Mode << " resizable=" << (args.bResizable ? 1 : 0)
			<< " popup=" << ((style & WS_POPUP) != 0 ? 1 : 0) << " thickframe=" << ((style & WS_THICKFRAME) != 0 ? 1 : 0)
			<< " maximizebox=" << ((style & WS_MAXIMIZEBOX) != 0 ? 1 : 0)
			<< " rect=(" << rect.left << "," << rect.top << ")-(" << rect.right << "," << rect.bottom << ")" << std::endl;

		HWindowCallBacks* callbacks = GCoreSystem::GetGlobalValues().WindowCallBacks.get();
		HDelegateHandle   handler   = callbacks->OnInput.AddStatic(&onWindowInput);

		const ULONGLONG start = ::GetTickCount64();
		bool bClosed = false;
		while (bClosed == false && ::GetTickCount64() - start < (ULONGLONG)seconds * 1000)
		{
			MSG msg;
			while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					bClosed = true;
					break;
				}
				::TranslateMessage(&msg);
				::DispatchMessageW(&msg);
			}
			::Sleep(5);
		}

		callbacks->OnInput.Remove(handler);
		if (::IsWindow(handle))
		{
			::DestroyWindow(handle);
		}
		window = nullptr;
		std::cout << "input.selftest: window closed (" << (bClosed ? "WM_CLOSE" : "timeout") << ")" << std::endl;
		return true;
	}

	void testClipboard()
	{
		const PString text("JGEngine \xED\x81\xB4\xEB\xA6\xBD\xEB\xB3\xB4\xEB\x93\x9C \xF0\x9F\x98\x80");   // "JGEngine 클립보드 😀"
		check(HPlatform::SetClipboardText(text), "clipboard: set UTF-8 text");
		check(HPlatform::GetClipboardText() == text, "clipboard: read back the same UTF-8 text (Hangul, emoji)");
	}
#endif

	bool executeSelfTest(const HConsoleCommandArgs& args)
	{
		GCheckCount   = 0;
		GFailureCount = 0;

#ifdef _PLATFORM_WINDOWS
		int32 seconds = 0;
		if (args.TryGetInt("window", seconds) && seconds > 0)
		{
			PString mode;
			args.TryGetString("mode", mode);
			return runWindow(seconds, mode);
		}

		testPointer();
		testKeys();
		testCharacters();
		testFocusAndOthers();
		if (args.Has(PString("clipboard")))
		{
			testClipboard();
		}
#endif

		std::cout << "input.selftest: " << (GFailureCount == 0 ? "OK" : "FAILED")
			<< " (" << (GCheckCount - GFailureCount) << "/" << GCheckCount << ")" << std::endl;
		return GFailureCount == 0;
	}

	HAutoConsoleCommand InputSelfTestCommand(
		"input.selftest",
		"input.selftest [-window=<seconds> [-mode=windowed|fixed|borderless]] [-clipboard]",
		"Run the engine input layer self tests (Win32 messages -> input events). -window opens a real window and prints events",
		&executeSelfTest);
}
