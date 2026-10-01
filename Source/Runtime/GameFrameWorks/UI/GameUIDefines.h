#pragma once
#include "Core/GameFrameWorksDefines.h"

// 게임 UI(플레이어에게 보이는 화면). ImGui(GUI 모듈)는 에디터 · 도구 전용이고, 게임 화면은 이것으로 만든다.
// 화면 하나 = JGGameWidget(언리얼 CommonUI 의 ActivatableWidget 자리). 레이어 스택 관리자 PGameUIManager 는 JGGameInstance 가 가진다.
// 호스트(에디터 Scene Viewport, 나중의 단독 실행 호스트)가 그리기 · 포인터 · 뒤로가기를 넘긴다. 그래픽 없이(헤드리스) 써도 된다.
// 설계: Document/Memory/GameFrameWorks/Files/GameUI_설계_2026-10-01.md

enum class EGameUIPointerEventType
{
	Move,    // 포인터 이동
	Down,    // 버튼 누름
	Up,      // 버튼 뗌
	Wheel,   // 휠 (Wheel 값)
	Leave,   // 포인터가 그리는 영역을 벗어남. 호버를 푼다
};

enum class EGameUIPointerButton
{
	Left = 0,
	Right,
	Middle,
	Count,
};

enum class EGameUIHorizontalAlign
{
	Left,
	Center,
	Right,
};

enum class EGameUIVerticalAlign
{
	Top,
	Middle,
	Bottom,
};

// 글자 요소의 자동 줄바꿈. '\n' 은 어느 방식에서나 줄을 바꾼다. 바꾼 자리의 공백은 버린다.
// 두 방식 모두 닫는 부호(, . ! ? ) ] 」 … 등)는 줄 첫머리에, 여는 부호(( [ 「 등)는 줄 끝에 두지 않는다.
enum class EGameUITextWrap
{
	None,        // 요소 폭을 넘어도 바꾸지 않는다
	Word,        // 공백에서 바꾼다. 한 단어가 줄보다 길면 그 안에서 바꾼다. 한자 · 가나는 글자 사이에서 바꾼다(한글은 어절 단위)
	Character,   // 아무 글자 사이에서나 바꾼다
};

// 레이어. 아래(Game)부터 그리고, 입력은 위(Modal)부터 받는다. 레이어마다 위젯 스택이 하나 있다(Lyra 의 PrimaryGameLayout 레이어).
enum class EGameUILayer : int32
{
	Game = 0,   // HUD
	GameMenu,   // 게임 위에 뜨는 게임 화면(손패 확대 · 목록처럼)
	Menu,       // 일시정지 · 설정
	Modal,      // 확인 대화상자
	Count,
};

// 위젯이 활성일 때 입력을 어떻게 나눌지(CommonUI 의 Input Config).
enum class EGameWidgetInputMode
{
	Game,   // UI 요소 위의 입력만 받고, 나머지는 아래 레이어 · 월드로 넘긴다 (HUD)
	Menu,   // 이 위젯이 활성이면 아래 레이어 · 월드는 입력을 받지 못한다 (메뉴 · 대화상자)
};

// 호스트가 넣는 포인터 이벤트. 좌표는 렌더 타깃 픽셀(왼쪽 위 원점, 아래로 +y).
struct HGameUIPointerEvent
{
	EGameUIPointerEventType Type   = EGameUIPointerEventType::Move;
	HVector2                Position;
	EGameUIPointerButton    Button = EGameUIPointerButton::Left;
	float32                 Wheel  = 0.0f;
};

// 호스트가 넣는 편집 키(글자 입력 포커스가 있을 때, PGameUIManager::HandleKey). 글자 자체는 HandleTextInput 으로 따로 온다.
enum class EGameUIKey
{
	Backspace,
	Delete,
	Enter,       // 확정(OnCommitted) 후 포커스 해제
	Escape,      // 포커스만 해제(뒤로가기로 쓰지 않는다)
	Left,
	Right,
	Home,
	End,
	Tab,         // 같은 위젯의 다음 입력란(Shift 면 앞)
	SelectAll,   // Ctrl+A
};
