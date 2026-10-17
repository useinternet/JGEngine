#pragma once
#include "Input/InputEvent.h"
#include "Platform/PlatformDefines.h"

// Win32 입력 메시지 → 엔진 입력 이벤트(GP-C1). 창 하나의 입력 상태(눌린 버튼 · UTF-16 앞쪽 서로게이트)를 든다.
// OS 를 바꾸는 일(포인터 잡기 · 나감 추적 · 휠 화면 좌표 → 클라이언트 좌표)은 하지 않는다 — 창 프로시저가 결과를 보고 한다.
// 그래서 창 없이 가짜 메시지로 시험할 수 있다(JGConsole input.selftest).
class HWindowsInputTranslator
{
	uint32 _pressedButtons       = 0;   // 비트 = 1 << EPointerButton
	uint16 _pendingHighSurrogate = 0;   // WM_CHAR 로 온 UTF-16 앞쪽 서로게이트(뒤쪽을 기다림)

public:
	// 입력 메시지면 이벤트를 outEvent 에 쓰고 true. 입력 메시지가 아니거나 낼 이벤트가 없으면 false
	// (앞쪽 서로게이트 · 제어 문자 · 버튼을 누른 채 나감 등). modifiers = 이 메시지 때 눌린 보조 키(창 프로시저가 채운다).
	// WM_MOUSEWHEEL · WM_MOUSEHWHEEL 의 Position 은 화면 좌표 그대로다(Win32 가 그렇게 준다) — 창 프로시저가 클라이언트 좌표로 바꾼다.
	bool Translate(uint32 msg, WPARAM wParam, LPARAM lParam, EInputModifier modifiers, HInputEvent& outEvent);
	bool IsAnyButtonDown() const;
	bool IsButtonDown(EPointerButton button) const;
	// 눌린 버튼 · 서로게이트를 잊는다(포커스를 잃었을 때 — FocusLost 를 낼 때 스스로 부른다).
	void Reset();

	// Win32 가상 키 → 엔진 키(Input/Key.h). 표에 없는 키는 Unknown. HPlatform::GetSystemKeyCode 의 거꾸로.
	static EKeyCode ToEngineKeyCode(uint32 virtualKey);
};
