#pragma once
#include "JGEditorDefine.h"
#include "Widget.h"
#include "SceneViewport.generation.h"

class PGameViewport;
class PWorld;
class JGCameraComponent;
struct HGUIImageInput;
enum class EGameViewportInputTarget : uint8;

// 씬 뷰포트. 게임 화면(GameFrameWork PGameViewport — 활성 월드 + 게임 UI)을 ImGui 창에 보여 주는 에디터 호스트다(GP-E1, D-23).
// 게임 모듈을 만들면서 게임 월드를 보는 창이다. GameFrameWork 가 돌지 않으면(프로젝트 없이 띄운 에디터) 안내 문구만 그린다.
//   OnUpdate(열려 있는 동안 매 프레임, GUI 생성 뒤 · GUI 드로우 앞) → PGameViewport::Render (월드 → 디스플레이 텍스처 → 게임 UI)
//   OnGenerateGUI → 텍스처를 16:9 로 창에 맞춰 보여 주고, ImGui 입력을 엔진 입력 이벤트(HInputEvent, 뷰포트 픽셀)로 바꿔 PGameViewport::HandleInput 에 넘긴다.
//     무엇을 게임이 받는지는 게임 뷰포트가 정한다: 게임 UI 먼저(입력 모드 Menu 위젯이면 빈 곳도 UI), 남은 왼쪽 누름 → 활성 카메라 광선 → 컨트롤러 HandleClick,
//     창에 포커스가 있을 때 Esc → 뒤로가기, 게임 입력란 포커스가 있으면 확정 글자 · 편집 키 · Ctrl+A/C/X/V(그때 Esc 는 입력란 포커스 해제).
//   에디터 조작은 여기 남는다: 게임이 쓰지 않은(대상 None) 오른쪽 끌기 · 휠 → 궤도 모드 JGCameraActor 의 궤도 · 줌, 클릭 결과 글(피킹 디버그),
//     ImGui 키보드 내비게이션에서 편집 키 가져오기, OS IME 조합 창을 게임 커서 옆에 두기.
// 렌더 해상도 = 보이는 이미지 크기(D-22 ①): 보이는 크기(16:9, 정수 픽셀)가 바뀌면 0.3초 그대로일 때 한 번 PGameViewport::Resize 한다
//   (렌더 텍스처를 다시 만들 때 셰이더 두 개를 다시 컴파일하므로 창을 끄는 동안에는 하지 않는다). 그때까지는 지난 크기 그림을 늘려 보여 준다.
JGCLASS()
class JGEDITOR_API JGSceneViewport : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

private:
	PSharedPtr<PGameViewport> _viewport;
	HVector2                  _contentSize;
	PString                   _lastClickText;

	// 렌더 크기 맞추기(D-22 ①). 원하는 크기(보이는 이미지 크기)가 바뀐 시각부터 그대로인 시간을 잰다.
	HVector2Int _desiredSize;
	float64     _desiredSizeSince = 0.0;

	// ImGui 입력 → 엔진 이벤트. InteractiveImage 는 누른 프레임(bClicked)과 누르는 중(bDown)만 주므로 뗌은 직전 프레임과 비교해 만든다.
	// 게임(UI · 월드)이 받은 누름은 뗄 때까지 에디터 조작(궤도)에 쓰지 않는다.
	bool _bPointerWasDown[(int32)EGUIMouseButton::Count] = {};
	bool _bPressUsedByGame[(int32)EGUIMouseButton::Count] = {};
	bool _bPointerWasInside = false;

	// 포커스를 잃은 뒤 버튼이 다 풀릴 때까지 포인터를 넘기지 않는다(FocusLost 로 누름을 취소했으므로 그 뒤 풀림은 뗌이 아니다).
	bool _bWasFocused     = false;
	bool _bPointerCanceled = false;

protected:
	virtual PString GetTitleName() const override;
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnUpdate() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnGenerateGUI() override;

private:
	// 그릴 월드와 그 활성 카메라(상태 줄 · 궤도용). 없으면 false 이고 outProblem 에 이유.
	bool findView(PSharedPtr<PWorld>& outWorld, PSharedPtr<JGCameraComponent>& outCamera, PString* outProblem) const;
	// 내용 영역에 16:9 로 맞춘 이미지 크기(정수 픽셀, 최소 640x360). 아래 두 줄(상태 · 클릭 결과) 자리를 남긴다.
	HVector2Int computeImageSize() const;
	// 원하는 크기가 0.3초 그대로면 게임 뷰포트 크기를 바꾼다(렌더 텍스처는 다음 Render 에서 맞춰진다).
	void updateViewportSize(const HVector2Int& desiredSize);
	// 이미지 좌표(이미지 왼쪽 위 기준 픽셀) → 뷰포트 픽셀.
	HVector2 toViewport(const HVector2& imagePoint, const HVector2& imageSize) const;

	// 창이 ImGui 포커스를 잃었거나 앱이 OS 포커스를 잃은 프레임에 FocusLost 를 넘긴다(게임 뷰포트가 누름 · 게임 UI 끌기를 취소).
	void handleFocusLoss();
	void handlePointer(PSharedPtr<PWorld> world, PSharedPtr<JGCameraComponent> camera, const HGUIImageInput& input, const HVector2& imageSize);
	// 창에 포커스가 있을 때 키보드: 게임 입력란 포커스가 있으면 글자 입력, 없으면 Esc = 뒤로가기.
	void handleKeyboard(const HGUIImageInput& input, const HVector2& imageSize);
	void forwardTextInput(const HGUIImageInput& input, const HVector2& imageSize);
	// 왼쪽 누름을 누가 받았는지 클릭 결과 글로 남긴다(게임 UI · 컨트롤러 피킹 · 컨트롤러가 없을 때 월드 판정).
	void describeLeftClick(PSharedPtr<PWorld> world, const HVector2& normalizedPoint, const HVector2& viewportPoint, EGameViewportInputTarget target);
};
