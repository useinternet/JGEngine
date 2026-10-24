#pragma once
#include "Core/GameFrameWorkDefines.h"
#include "Input/InputEvent.h"
#include "Math/Rect.h"

class HRay;
class IRawTexture;
class PSceneRenderer;
class PWorld;
class PGameUIManager;
class JGCameraComponent;

// 게임 뷰포트(D-23 GP-F1): 게임 화면 하나. 월드를 활성 카메라로 그리고 그 위에 게임 UI 를 얹은 텍스처를 내고, 그 화면으로 오는 입력을 게임에 나눠 준다.
// 에디터 씬 뷰포트(JGEditor — ImGui 창에 표시)와 게임 호스트(GameHost — 창 전체, 스왑체인에 제출)가 같이 쓴다. ImGui · 창 · OS 를 모른다.
//
//   보여 주는 것: 게임 인스턴스의 활성 월드와 게임 UI. SetSource 로 다른 월드 · UI 를 줄 수 있다(자체 테스트).
//   좌표: 뷰포트 픽셀(왼쪽 위 0, 0 · 아래로 +y). 범위는 GetSize(). 호스트는 표시 영역 좌표를 뷰포트 픽셀로 바꿔 넣는다
//         (GameHost 는 창 클라이언트 = 뷰포트라 그대로, 에디터는 줄여 보여 주는 이미지 좌표를 늘려서).
//   입력 순서: 게임 UI 먼저 — 입력 모드(Menu 위젯이면 아래 레이어 · 월드를 막음)와 레이어 규칙은 PGameUIManager 그대로.
//         UI 가 받지 않은 왼쪽 누름 → 활성 카메라 광선 → 월드의 모든 JGGameplayControllerActor::HandleClick.
//         Esc → 뒤로가기(HandleBackAction). 게임 입력란에 포커스가 있으면 확정 글자 · 편집 키가 그쪽으로 가고 그때 Esc 는 포커스 해제다.
//         궤도 · 줌 같은 호스트 조작은 여기 없다 — HandleInput 이 false(대상 None)를 돌려준 이벤트를 호스트가 쓴다.
//   그리기: Render = 월드 → 디스플레이 텍스처(sRGB) → 게임 UI. 결과는 GetDisplayTexture (호스트가 매 프레임 다시 받는다).
//   그래픽이 없으면(헤드리스, JGConsole) 렌더러를 만들지 않고 입력 라우팅만 한다.
// 메인 스레드 전용.
//
// 입력란의 Ctrl+C/X/V 는 OS 클립보드(HPlatform, GP-C1)를 쓴다. IME 조합 창 위치는 호스트가 GetTextInputCaretRect 로 HPlatform::SetImeCompositionRect 를 부른다(창을 아는 쪽).
// 렌더 텍스처 크기는 Render 가 뷰포트 크기에 맞춘다(PSceneRenderer::Resize, Graphics GP-G2). 바뀔 때마다 셰이더 두 개를 다시 컴파일하므로
//   호스트는 크기 변경이 멈춘 뒤 한 번 Resize 한다. 맞추기 전 · 실패했을 때는 게임 UI · 광선 좌표를 렌더 텍스처 크기로 바꿔 맞춘다.

// HandleInput 이 이벤트를 어디로 보냈나.
enum class EGameViewportInputTarget : uint8
{
	None,     // 게임이 쓰지 않았다 — 호스트가 자기 조작(에디터 궤도 · 줌 등)에 써도 된다
	GameUI,   // 게임 UI 가 받았다(요소 · 입력란 · 뒤로가기, 또는 Menu 위젯이 월드를 막음)
	World,    // 왼쪽 누름이 월드로 갔다(활성 카메라 광선 → 컨트롤러 HandleClick, 컨트롤러가 하나 이상)
};

class GAMEFRAMEWORK_API PGameViewport : public IMemoryObject
{
	PString                    _name;
	HVector2Int                _size;                  // 뷰포트 크기 = 입력 좌표 기준
	PSharedPtr<PSceneRenderer> _renderer;              // 그래픽이 없으면 nullptr
	HVector2Int                _resizeFailedSize;      // 렌더 텍스처 크기 변경이 실패한 크기(같은 크기로는 다시 시도하지 않는다)

	// SetSource 로 정한 월드 · UI. 정하지 않았으면 게임 인스턴스의 것을 쓴다.
	bool                       _bHasSource = false;
	PWeakPtr<PWorld>           _sourceWorld;
	PWeakPtr<PGameUIManager>   _sourceUI;

	// 포인터 상태. 누름마다 누가 받았는지 기억해 뗌의 대상을 같게 돌려준다.
	EGameViewportInputTarget   _pressTarget[(int32)EPointerButton::Count] = {};
	bool                       _bPressed[(int32)EPointerButton::Count]    = {};

public:
	PGameViewport() = default;
	virtual ~PGameViewport() = default;

	// name = 렌더 리소스 이름 앞부분. 크기가 0 이면 false. 그래픽이 있는데 렌더러를 만들지 못하면 false(오류 로그). 헤드리스면 렌더러 없이 true.
	bool Initialize(const PString& name, uint32 width, uint32 height);
	// 뷰포트 크기를 바꾼다. "크기 변경이 멈춘 뒤 한 번" 은 호출하는 호스트 몫(렌더 텍스처를 다시 만들 때 셰이더를 다시 컴파일한다). 0 이면 무시한다(최소화).
	// 입력 좌표 기준은 바로 바뀌고, 렌더 텍스처는 다음 Render 에서 맞춘다(그 뒤 GetDisplayTexture 는 새 객체).
	void Resize(uint32 width, uint32 height);
	// 월드(활성 카메라) → 디스플레이 텍스처 → 게임 UI. 그릴 월드 · 카메라 · 렌더러가 없으면 false(outProblem 에 이유, 텍스처는 지난 그림 그대로).
	bool Render(PString* outProblem = nullptr);
	// 그린 결과(sRGB 디스플레이 텍스처). 렌더러가 텍스처를 다시 만들 수 있으니 보관하지 말고 매 프레임 다시 받는다. 헤드리스면 nullptr.
	PSharedPtr<IRawTexture> GetDisplayTexture() const;
	// 뷰포트 크기(픽셀).
	HVector2Int GetSize() const;
	// 렌더 텍스처 크기(픽셀). Resize 뒤 다음 Render 전까지(또는 크기 변경이 실패하면) 뷰포트 크기와 다를 수 있다. 렌더러가 없으면 뷰포트 크기.
	HVector2Int GetRenderSize() const;

	// 입력 하나. 포인터 좌표는 뷰포트 픽셀. 게임(UI · 월드)이 썼으면 true. outTarget 에 누가 받았는지(nullptr 이면 돌려주지 않는다).
	// 뗌(PointerUp)은 그 버튼의 누름을 받은 쪽을 돌려준다 — 호스트가 "UI 가 잡은 끌기" 를 알 수 있다.
	// FocusLost 는 게임 UI 의 끌기를 취소로 끝내고(OnDragEnd, bCanceled = true), 누르고 있던 버튼의 누름을 취소한다(게임 UI 에 화면 밖 뗌 · 벗어남 — 버튼은 클릭하지 않는다).
	bool HandleInput(const HInputEvent& event, EGameViewportInputTarget* outTarget = nullptr);
	// 게임 입력란에 글자 입력 포커스가 있나(호스트: 이때 키보드 · IME 를 게임으로 보낸다).
	bool WantsTextInput() const;
	// IME 조합 창 자리 = 포커스 입력란의 커서 사각형(뷰포트 픽셀). 포커스가 없으면 false.
	bool GetTextInputCaretRect(HRect* outRect) const;
	// 뷰포트 픽셀 → 활성 카메라 광선(호스트 디버그 · 피킹 표시용). 월드 · 카메라가 없으면 false.
	bool ViewportPointToRay(const HVector2& viewportPoint, HRay* outRay) const;

	// 보여 줄 월드 · UI 를 따로 정한다(자체 테스트 · 특수 호스트). 둘 다 nullptr 이면 게임 인스턴스로 돌아간다.
	void SetSource(const PSharedPtr<PWorld>& world, const PSharedPtr<PGameUIManager>& ui);

private:
	PSharedPtr<PWorld>         getWorld() const;
	PSharedPtr<PGameUIManager> getUI() const;
	// 그릴 · 광선을 낼 활성 카메라. 없으면 false 이고 outProblem 에 이유.
	bool findView(PSharedPtr<PWorld>& outWorld, PSharedPtr<JGCameraComponent>& outCamera, PString* outProblem) const;
	// 뷰포트 픽셀 → 렌더 텍스처 픽셀(게임 UI 좌표). 두 크기가 같으면 그대로다.
	HVector2 toTarget(const HVector2& viewportPoint) const;
	HVector2 getTargetSize() const;

	EGameViewportInputTarget handlePointer(const HInputEvent& event);
	EGameViewportInputTarget handleKeyDown(const HInputEvent& event);
	EGameViewportInputTarget handleChar(const HInputEvent& event);
	EGameViewportInputTarget clickWorld(const HVector2& viewportPoint);
	void releaseAllButtons();
};
