#pragma once
#include "CoreDefines.h"
#include "Math/Vector.h"
#include "Input/Key.h"

// 엔진 입력 이벤트(GP-C1). 플랫폼 창이 OS 입력 메시지를 이것으로 바꿔 HWindowCallBacks::OnInput 으로 보낸다.
// 받는 쪽은 게임 호스트(게임 뷰포트 → 게임 UI · 월드)다. 에디터(ImGui)는 원시 WndProc 를 그대로 쓴다 — 구독자가 없으면 아무 일도 없다.
// 포인터 좌표는 창 클라이언트 픽셀(왼쪽 위가 0, 0). 버튼을 누르고 있는 동안 창이 포인터를 잡아 두므로 창 밖에서도 이동 · 뗌이 온다.
enum class EInputEventType : uint8
{
	PointerMove,
	PointerDown,    // Button
	PointerUp,      // Button
	PointerWheel,   // WheelDelta
	PointerLeave,   // 포인터가 창 클라이언트 밖으로 나감(버튼을 누르고 있지 않을 때)
	KeyDown,        // Key · bRepeat
	KeyUp,          // Key
	Char,           // 확정 글자 하나(IME 확정 글자 포함) — Codepoint. 제어 문자(U+0000~U+001F, U+007F)는 보내지 않는다
	FocusGained,    // 창이 키보드 포커스를 얻음
	FocusLost,      // 포커스를 잃음. 받는 쪽은 누르고 있던 키 · 버튼을 뗀 것으로 친다
};

enum class EPointerButton : uint8
{
	Left,
	Right,
	Middle,
	X1,
	X2,
	Count,
};

// 이벤트가 생길 때 눌려 있던 보조 키(모든 이벤트에 채운다).
JG_ENUM_FLAG(EInputModifier)
enum class EInputModifier
{
	None  = 0,
	Shift = 1 << 0,
	Ctrl  = 1 << 1,
	Alt   = 1 << 2,
};

struct HInputEvent
{
	EInputEventType Type       = EInputEventType::PointerMove;
	HVector2        Position;                              // 포인터 이벤트: 창 클라이언트 픽셀
	EPointerButton  Button     = EPointerButton::Left;     // PointerDown · PointerUp
	HVector2        WheelDelta;                            // PointerWheel: 한 칸 = 1.0. y = 세로(앞으로 굴리면 +), x = 가로(오른쪽으로 +)
	EKeyCode        Key        = EKeyCode::Unknown;        // KeyDown · KeyUp. 엔진 키 표(Input/Key.h)에 없는 키는 Unknown
	bool            bRepeat    = false;                    // KeyDown: 누르고 있어 반복된 것
	EInputModifier  Modifiers  = EInputModifier::None;
	uint32          Codepoint  = 0;                        // Char: UTF-32(UTF-16 서로게이트 쌍을 합친 값)
};
