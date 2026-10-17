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
	Wheel,   // 휠 (Wheel · WheelHorizontal 값)
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
	Justify,   // 양쪽 맞춤(글자 요소): 자동으로 바뀐 줄을 폭에 맞춰 편다. 문단 마지막 줄 · 말줄임 줄은 왼쪽
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
// 휠은 한 칸 = 1.0(고해상도 휠은 소수). Wheel = 세로(앞으로 굴리면 + — 위로 넘긴다), WheelHorizontal = 가로(오른쪽으로 +).
struct HGameUIPointerEvent
{
	EGameUIPointerEventType Type            = EGameUIPointerEventType::Move;
	HVector2                Position;
	EGameUIPointerButton    Button          = EGameUIPointerButton::Left;
	float32                 Wheel           = 0.0f;
	float32                 WheelHorizontal = 0.0f;
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

// 요소 그리기 변환(ER-014). 배치는 그대로 두고 그릴 때와 입력 판정 때만 적용하며 자식에 곱해진다. 기준점 = 요소 배치 사각형 안 피벗 자리.
struct HGameUIRenderTransform
{
	float32  Angle = 0.0f;                     // 도. 화면에서 시계 방향이 +
	HVector2 Scale = HVector2(1.0f, 1.0f);     // 가로 · 세로 배율
	HVector2 Translation;                      // 배치 뒤 옮기기(논리 단위)
};

// 트윈이 움직이는 요소 속성(GG-16, UI/GameUITween.h). 값: 위치 · 크기 · 배율 · 옮기기 · 넘긴 양 = HVector2, 각도 · 불투명도 = 실수, 색 = HLinearColor.
enum class EGameUITweenProperty : int32
{
	Position,      // SetPosition(배치 — 자식도 따라 다시 배치된다)
	Size,          // SetSize
	Angle,         // 그리기 변환 각도(도)
	Scale,         // 그리기 변환 배율
	Translation,   // 그리기 변환 옮기기(논리 단위)
	Opacity,       // SetOpacity
	Color,         // 그림 · 글 요소의 색(그 밖의 요소는 움직이지 않는다)
	ScrollOffset,  // 스크롤 칸의 넘긴 양(GG-20, 그 밖의 요소는 움직이지 않는다 — 부드럽게 넘기기)
	Count,
};

// 2D 아핀 변환 p' = (A·x + C·y + X, B·x + D·y + Y). 게임 UI 그리기 · 판정용(기본값 = 단위 변환).
struct GAMEFRAMEWORKS_API HGameUIAffine
{
	float32 A = 1.0f;
	float32 B = 0.0f;
	float32 C = 0.0f;
	float32 D = 1.0f;
	float32 X = 0.0f;
	float32 Y = 0.0f;

	// pivot(논리) 을 기준으로 회전 · 배율을 적용하고 옮기는 변환.
	static HGameUIAffine FromRenderTransform(const HGameUIRenderTransform& transform, const HVector2& pivot);
	// (this · rhs)(p) = this(rhs(p)) — rhs 를 먼저 적용한다.
	HGameUIAffine operator*(const HGameUIAffine& rhs) const;
	HVector2 Apply(const HVector2& point) const;
	// 뒤집을 수 없으면(배율 0) 단위 변환.
	HGameUIAffine Inverse() const;
	// 같은 변환을 scale 배 한 좌표계에서(논리 → 렌더 타깃 픽셀): S · this · S⁻¹.
	HGameUIAffine Scaled(float32 scale) const;
	// 넓이 배율의 제곱근(가로 · 세로 배율의 기하 평균). 글자를 그 배율로 래스터화할 때 쓴다.
	float32 GetUniformScale() const;
	bool IsIdentity() const;
};
