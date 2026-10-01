#pragma once
#include "JGEditorDefine.h"
#include "Widget.h"
#include "SceneViewport.generation.h"

class PSceneRenderer;
class PWorld;
class JGCameraComponent;
class PGameUIManager;
struct HGUIImageInput;

// 씬 뷰포트. 게임 인스턴스의 활성 월드를 그 월드의 활성 카메라로 그려 보여 주고, 마우스 입력을 월드에 넘긴다.
// 게임 모듈을 만들면서 게임 월드를 보는 창이다. GameFrameWorks 가 돌지 않으면(프로젝트 없이 띄운 에디터) 안내 문구만 그린다.
//   OnUpdate(열려 있는 동안 매 프레임, GUI 생성 뒤 · GUI 드로우 앞) → 장면을 출력 텍스처에 그린다
//   OnGenerateGUI → 텍스처를 보여 주고 입력을 받는다
//   왼쪽 클릭 → 활성 카메라의 광선 → 월드의 모든 JGGameplayControllerActor::HandleClick (피킹 결과는 컨트롤러가 받는다)
//   오른쪽 끌기 · 휠 → 활성 카메라가 궤도 모드 JGCameraActor 면 궤도 회전 · 거리
// 게임 UI(게임 인스턴스의 JGGameWidget 레이어 스택, GameFrameWorks UI/)의 호스트: 월드 위에 그리고, 포인터를 월드보다 먼저 넘긴다.
//   UI가 받은 누름 · 휠은 월드(피킹 · 궤도 · 줌)로 넘기지 않는다. 입력 모드 Menu 위젯이 떠 있으면 빈 곳 입력도 UI 가 받는다.
//   창에 포커스가 있을 때 Esc → 게임 UI 뒤로가기(HandleBackAction).
//   게임 입력란에 글자 입력 포커스가 있으면(WantsTextInput) 키보드를 그쪽으로: 확정 글자(IME 확정 포함) · 편집 키 · Ctrl+A/C/X/V,
//   편집 키는 ImGui 내비게이션에서 가져오고, OS IME 조합 창을 게임 커서 옆에 둔다. 그때 Esc 는 입력란 포커스 해제로 끝난다.
JGCLASS()
class JGEDITOR_API JGSceneViewport : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

private:
	PSharedPtr<PSceneRenderer> _renderer;
	HVector2                   _contentSize;
	PString                    _lastClickText;

	// 게임 UI 로 넘기는 포인터 상태. InteractiveImage 는 누른 프레임(bClicked)과 누르는 중(bDown)만 주므로
	// 뗌은 직전 프레임의 bDown 과 비교해 만든다. UI가 받은 누름은 뗄 때까지 월드 입력에 넘기지 않는다.
	bool _bPointerWasDown[(int32)EGUIMouseButton::Count]   = {};
	bool _bPointerOwnedByUI[(int32)EGUIMouseButton::Count] = {};
	bool _bPointerWasInside = false;

protected:
	virtual PString GetTitleName() const override;
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnUpdate() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnGenerateGUI() override;

private:
	// 그릴 월드와 그 활성 카메라. 없으면 false 이고 outProblem 에 이유.
	bool findView(PSharedPtr<PWorld>& outWorld, PSharedPtr<JGCameraComponent>& outCamera, PString* outProblem) const;
	void handleInput(PSharedPtr<PWorld> world, PSharedPtr<JGCameraComponent> camera, const HGUIImageInput& input, const HVector2& imageSize);
	// 포인터를 게임 UI 에 먼저 넘긴다. 이번 프레임 UI가 받은 누름 · 휠을 outClickConsumed · outWheelConsumed 에 적는다.
	void forwardPointerToGameUI(const HGUIImageInput& input, const HVector2& imageSize, bool outClickConsumed[(int32)EGUIMouseButton::Count], bool& outWheelConsumed);
	// 창에 포커스가 있을 때 키보드를 게임 UI 로 넘긴다: 입력란 포커스가 있으면 글자 입력, 없으면 Esc = 뒤로가기.
	void handleKeyboard(const HGUIImageInput& input, const HVector2& imageSize);
	// 글자 · 편집 키 · 클립보드를 포커스 입력란으로, IME 조합 창을 게임 커서 옆에.
	void forwardTextInput(PGameUIManager& ui, const HGUIImageInput& input, const HVector2& imageSize);
};
