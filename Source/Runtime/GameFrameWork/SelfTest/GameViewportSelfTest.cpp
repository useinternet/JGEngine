#include "PCH/PCH.h"
#include "SelfTest/GameViewportSelfTest.h"
#include "Viewport/GameViewport.h"
#include "Core/World.h"
#include "Actors/CameraActor.h"
#include "Actors/GameplayControllerActor.h"
#include "Components/CameraComponent.h"
#include "Components/PickShapeComponent.h"
#include "UI/GameUIManager.h"
#include "UI/GameWidget.h"
#include "UI/GameUIButton.h"
#include "UI/GameUIImage.h"
#include "UI/GameUITextInput.h"
#include "JGGraphics.h"
#include "Classes/Texture.h"
#include "Math/Ray.h"
#include <iostream>

namespace
{
	// 화면(뷰포트 = UI 기준 크기, 배율 1)과 배치. 버튼 [10, 70]x[10, 40], 입력란 [10, 130]x[60, 84], 가운데(160, 90)는 빈 곳 = 월드.
	// 끌 수 있는 요소 [200, 260]x[10, 50] 은 끌기 검사(GP-F1c)에서 더한다.
	constexpr uint32  ViewportTestWidth  = 320;
	constexpr uint32  ViewportTestHeight = 180;
	const HVector2    ViewportTestButtonPoint(40.0f, 25.0f);
	const HVector2    ViewportTestInputPoint(60.0f, 72.0f);
	const HVector2    ViewportTestEmptyPoint(160.0f, 90.0f);
	const HVector2    ViewportTestCardPoint(220.0f, 30.0f);

	struct HViewportCheck
	{
		int32 Failures = 0;
		int32 Passed   = 0;
		int32 Skipped  = 0;

		void operator()(bool bCondition, const PString& what)
		{
			if (bCondition == true)
			{
				++Passed;
				std::cout << "  [PASS] " << what.GetRawString() << std::endl;
				return;
			}
			++Failures;
			std::cout << "  [FAIL] " << what.GetRawString() << std::endl;
			JG_LOG(GameViewportSelfTest, ELogLevel::Error, "[FAIL] %s", what);
		}
	};

	// 클릭이 OnClick 까지 오는지 센다.
	class PViewportTestController : public JGGameplayControllerActor
	{
	public:
		int32               Clicks = 0;
		HGameplayPickResult Last;

	protected:
		virtual void OnClick(const HGameplayPickResult& pick) override
		{
			++Clicks;
			Last = pick;
		}
	};

	HInputEvent pointerEvent(EInputEventType type, const HVector2& position, EPointerButton button = EPointerButton::Left)
	{
		HInputEvent event;
		event.Type     = type;
		event.Position = position;
		event.Button   = button;
		return event;
	}

	HInputEvent wheelEvent(const HVector2& position, float32 steps)
	{
		HInputEvent event;
		event.Type       = EInputEventType::PointerWheel;
		event.Position   = position;
		event.WheelDelta = HVector2(0.0f, steps);
		return event;
	}

	HInputEvent keyEvent(EKeyCode key, EInputModifier modifiers = EInputModifier::None, bool bRepeat = false)
	{
		HInputEvent event;
		event.Type      = EInputEventType::KeyDown;
		event.Key       = key;
		event.Modifiers = modifiers;
		event.bRepeat   = bRepeat;
		return event;
	}

	HInputEvent charEvent(uint32 codepoint)
	{
		HInputEvent event;
		event.Type      = EInputEventType::Char;
		event.Codepoint = codepoint;
		return event;
	}

	EGameViewportInputTarget send(PGameViewport& viewport, const HInputEvent& event)
	{
		EGameViewportInputTarget target = EGameViewportInputTarget::None;
		viewport.HandleInput(event, &target);
		return target;
	}

	bool nearlyEqual(float32 a, float32 b, float32 tolerance = 0.001f)
	{
		return HMath::Abs(a - b) <= tolerance;
	}

	bool hasGraphicsAPI()
	{
		HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>();
		return graphicsModule != nullptr && graphicsModule->GetGraphicsAPI() != nullptr;
	}

	// 그래픽이 있으면(에디터 안 gmtest) 진짜 렌더러로 그리고 크기를 바꿔 본다. 작은 크기라 G버퍼가 작고, 옛 텍스처는 GPU 가 다 쓴 뒤 놓인다(Graphics GP-G2).
	// 콘솔 명령(프레임 밖)에서 렌더러를 만들어 그리고 버리는 장면이라 프레임 밖 GPU 기록 방어(Graphics 5-33, 사용자 결정 D-31)의 회귀도 잡는다 —
	// 방어가 빠지면 제출 때 D3D12 [538] · [527](리소스 상태) 경고가 난다(에디터 jg_log 의 D3D12 줄로 본다). 헤드리스(JGConsole)에서는 건너뛴다.
	void runRendererChecks(HViewportCheck& check, const PSharedPtr<PWorld>& world, const PSharedPtr<PGameUIManager>& ui)
	{
		if (hasGraphicsAPI() == false)
		{
			std::cout << "  [SKIP] renderer section: no graphics (headless)" << std::endl;
			++check.Skipped;
			return;
		}

		PSharedPtr<PGameViewport> viewport = Allocate<PGameViewport>();
		const bool bInitialized = viewport->Initialize(PString("ViewportSelfTest"), 64, 36);
		viewport->SetSource(world, ui);

		PString problem;
		const bool bFirst = viewport->Render(&problem);
		PSharedPtr<IRawTexture> first = viewport->GetDisplayTexture();
		const uint32 firstWidth  = first != nullptr ? first->GetTextureInfo().Width : 0;
		const uint32 firstHeight = first != nullptr ? first->GetTextureInfo().Height : 0;
		check(bInitialized == true && bFirst == true && firstWidth == 64 && firstHeight == 36,
			PString::Format("renderer: Render draws into a %ux%u display texture == 64x36 (%s)", firstWidth, firstHeight, problem));

		viewport->Resize(32, 18);
		const bool bSecond = viewport->Render(&problem);
		PSharedPtr<IRawTexture> second = viewport->GetDisplayTexture();
		const uint32 secondWidth  = second != nullptr ? second->GetTextureInfo().Width : 0;
		const uint32 secondHeight = second != nullptr ? second->GetTextureInfo().Height : 0;
		check(bSecond == true && secondWidth == 32 && secondHeight == 18 && viewport->GetRenderSize().x == 32 && viewport->GetRenderSize().y == 18,
			PString::Format("renderer: after Resize the next Render uses a %ux%u display texture == 32x18 (PSceneRenderer::Resize)", secondWidth, secondHeight));
	}

	void runViewportChecks(HViewportCheck& check)
	{
		// 월드: 궤도 카메라가 원점을 보고, 원점에 판정 상자 하나, 컨트롤러 하나
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("ViewportTestWorld"));
		world->BeginPlay();
		PSharedPtr<JGCameraActor> cameraActor = world->SpawnActor<JGCameraActor>(PName("ViewportCamera"));
		cameraActor->SetOrbit(HVector3(0.0f, 0.0f, 0.0f), 0.0f, HMath::ConvertToRadians(60.0f), 8.0f);
		PSharedPtr<JGActor> target = world->SpawnActor<JGActor>(PName("ViewportTarget"));
		target->AddComponent<JGPickShapeComponent>()->SetLocalBox(HVector3(0.0f, 0.0f, 0.0f), HVector3(1.0f, 1.0f, 1.0f));
		PSharedPtr<PViewportTestController> controller = world->SpawnActor<PViewportTestController>(PName("ViewportController"));
		world->Tick(1.0f / 60.0f);

		// 게임 UI: 기준 크기 = 뷰포트 크기(배율 1). 버튼 하나 · 입력란 하나
		PSharedPtr<PGameUIManager> ui = Allocate<PGameUIManager>();
		ui->SetReferenceSize(HVector2((float32)ViewportTestWidth, (float32)ViewportTestHeight));
		PSharedPtr<JGGameWidget>      hud    = ui->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIButton>     button = hud->AddChild<PGameUIButton>(PName("ViewportButton"));
		button->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(10.0f, 10.0f), HVector2(60.0f, 30.0f));
		PSharedPtr<PGameUITextInput>  input  = hud->AddChild<PGameUITextInput>(PName("ViewportInput"));
		input->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(10.0f, 60.0f), HVector2(120.0f, 24.0f));
		int32 buttonClicks = 0;
		button->OnClicked.AddLambda([&buttonClicks]()
		{
			++buttonClicks;
		});

		// 렌더러 없는 뷰포트: Initialize 대신 Resize 로 크기만 정한다(GPU 자원을 만들지 않는다)
		PSharedPtr<PGameViewport> viewport = Allocate<PGameViewport>();
		viewport->Resize(ViewportTestWidth, ViewportTestHeight);
		viewport->SetSource(world, ui);

		// 동작을 먼저 하고 검사한다(검사 메시지가 함수 인자라 평가 순서가 정해져 있지 않다).
		PString problem;
		const bool bRendered = viewport->Render(&problem);
		check(viewport->GetDisplayTexture() == nullptr && bRendered == false && problem.Empty() == false,
			PString::Format("viewport: without a renderer Render is false with a reason (%s)", problem));
		check(viewport->GetSize().x == (int32)ViewportTestWidth && viewport->GetSize().y == (int32)ViewportTestHeight
			&& viewport->GetRenderSize().x == (int32)ViewportTestWidth, "viewport: size = render size without a renderer");

		// 헤드리스 Initialize: 렌더러 없이 성공(그래픽이 있으면 렌더러를 만들므로 이 검사는 헤드리스에서만)
		if (hasGraphicsAPI() == false)
		{
			PSharedPtr<PGameViewport> headless = Allocate<PGameViewport>();
			check(headless->Initialize(PString("ViewportHeadless"), 64, 36) == true && headless->GetDisplayTexture() == nullptr
				&& headless->GetSize().x == 64 && headless->GetSize().y == 36, "viewport: headless Initialize succeeds without a renderer");
		}
		else
		{
			std::cout << "  [SKIP] headless Initialize: graphics is running" << std::endl;
			++check.Skipped;
		}

		// UI 먼저: 버튼 위 누름은 UI 가 받고 월드로 가지 않는다
		check(send(*viewport, pointerEvent(EInputEventType::PointerMove, ViewportTestButtonPoint)) == EGameViewportInputTarget::GameUI, "viewport: move over a button goes to the game UI");
		check(send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestButtonPoint)) == EGameViewportInputTarget::GameUI && controller->Clicks == 0,
			"viewport: down on a game UI button goes to the UI, not the world");
		const EGameViewportInputTarget buttonUp = send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestButtonPoint));
		check(buttonUp == EGameViewportInputTarget::GameUI && buttonClicks == 1, PString::Format("viewport: up completes the button click (clicks %d == 1)", buttonClicks));

		// 빈 곳 왼쪽 누름 → 활성 카메라 광선 → 컨트롤러 HandleClick → 원점 상자
		const EGameViewportInputTarget worldDown = send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestEmptyPoint));
		check(worldDown == EGameViewportInputTarget::World && controller->Clicks == 1 && controller->Last.bHitActor == true && controller->Last.Actor == target,
			PString::Format("viewport: down on empty space goes to the world through the camera ray (%s)", controller->Last.ToString()));
		check(send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestEmptyPoint)) == EGameViewportInputTarget::World && controller->Clicks == 1,
			"viewport: the up of a world press reports the world and does not click again");

		// 호스트 몫: 빈 곳 오른쪽 누름 · 휠(에디터 궤도 · 줌), X1 버튼
		check(send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestEmptyPoint, EPointerButton::Right)) == EGameViewportInputTarget::None && controller->Clicks == 1,
			"viewport: right press on empty space is left to the host");
		send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestEmptyPoint, EPointerButton::Right));
		check(send(*viewport, wheelEvent(ViewportTestEmptyPoint, 1.0f)) == EGameViewportInputTarget::None, "viewport: wheel on empty space is left to the host");
		check(send(*viewport, wheelEvent(ViewportTestButtonPoint, 1.0f)) == EGameViewportInputTarget::GameUI, "viewport: wheel over the game UI goes to the UI");
		check(send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestEmptyPoint, EPointerButton::X1)) == EGameViewportInputTarget::None && controller->Clicks == 1,
			"viewport: an X1 press is left to the host");
		send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestEmptyPoint, EPointerButton::X1));

		// Menu 위젯이 떠 있으면 빈 곳 누름도 UI 가 받아 월드를 막는다
		PSharedPtr<JGGameWidget> menu = ui->PushWidget<JGGameWidget>(EGameUILayer::Menu);
		menu->SetInputMode(EGameWidgetInputMode::Menu);
		check(send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestEmptyPoint)) == EGameViewportInputTarget::GameUI && controller->Clicks == 1,
			"viewport: a Menu widget blocks the world (press goes to the UI)");
		send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestEmptyPoint));

		// Esc = 뒤로가기: 뒤로가기 처리 위젯이 받는다(기본 처리 = 스택에서 빠짐). 처리 위젯이 없으면 호스트 몫
		menu->SetBackHandler(true);
		check(send(*viewport, keyEvent(EKeyCode::Esc)) == EGameViewportInputTarget::GameUI && menu->IsInStack() == false && hud->IsInStack() == true,
			"viewport: Esc is the back action (the menu closes)");
		check(send(*viewport, keyEvent(EKeyCode::Esc)) == EGameViewportInputTarget::None, "viewport: Esc without a back handler is left to the host");

		// 입력란: 누르면 포커스 → 확정 글자 · 편집 키 · Ctrl+A, Esc 는 포커스 해제(뒤로가기 아님)
		check(send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestInputPoint)) == EGameViewportInputTarget::GameUI && viewport->WantsTextInput() == true,
			"viewport: pressing a text input gives it the focus");
		send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestInputPoint));
		send(*viewport, charEvent(0x41));     // 'A'
		send(*viewport, charEvent(0xD55C));   // '한'
		const PString typed = input->GetText();
		check(typed == PString("A\xED\x95\x9C"), PString::Format("viewport: committed characters reach the focused input (%s)", typed));
		check(send(*viewport, keyEvent(EKeyCode::BackSpace)) == EGameViewportInputTarget::GameUI && input->GetText() == PString("A"), "viewport: Backspace edits the focused input");
		check(send(*viewport, keyEvent(EKeyCode::W)) == EGameViewportInputTarget::GameUI, "viewport: other keys while typing belong to the game (not the host)");
		send(*viewport, keyEvent(EKeyCode::A, EInputModifier::Ctrl));
		check(ui->GetTextInputSelection() == PString("A"), "viewport: Ctrl+A selects all in the focused input");

		// IME 조합 창 자리: 렌더러가 없으면 뷰포트 = 렌더 텍스처 크기라 게임 UI 의 커서 사각형 그대로다. 가로는 입력란 안
		HRect caret;
		HRect uiCaret;
		const bool bCaret   = viewport->GetTextInputCaretRect(&caret);
		const bool bUICaret = ui->GetTextInputCaretRect(uiCaret);
		check(bCaret == true && bUICaret == true && nearlyEqual(caret.left, uiCaret.left) && nearlyEqual(caret.top, uiCaret.top) && nearlyEqual(caret.bottom, uiCaret.bottom)
			&& caret.left >= 10.0f && caret.left <= 130.0f,
			PString::Format("viewport: the IME caret rect is the input caret in viewport pixels (%.1f, %.1f, %.1f, %.1f)", caret.left, caret.top, caret.right, caret.bottom));
		check(send(*viewport, keyEvent(EKeyCode::Esc)) == EGameViewportInputTarget::GameUI && viewport->WantsTextInput() == false && hud->IsInStack() == true,
			"viewport: Esc while typing releases the focus instead of going back");

		// 포커스를 잃으면 누르고 있던 누름을 취소한다: 버튼은 클릭하지 않고, 다음 빈 곳 누름은 다시 월드로 간다
		send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestButtonPoint));
		HInputEvent focusLost;
		focusLost.Type = EInputEventType::FocusLost;
		send(*viewport, focusLost);
		send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestButtonPoint));
		check(buttonClicks == 1, PString::Format("viewport: focus lost cancels a held press (clicks %d == 1)", buttonClicks));
		const EGameViewportInputTarget afterFocusLost = send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestEmptyPoint));
		send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestEmptyPoint));
		check(afterFocusLost == EGameViewportInputTarget::World && controller->Clicks == 2, "viewport: after focus lost the game UI holds no press (empty space goes to the world)");

		// 포커스를 잃으면 진행 중인 게임 UI 끌기는 "취소"로 끝난다(GP-F1c). 화면 밖 뗌이 먼저 가면 "빈 곳에 놓음"(취소 아님)이 된다
		PSharedPtr<PGameUIImage> card = hud->AddChild<PGameUIImage>(PName("ViewportDragCard"));
		card->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(200.0f, 10.0f), HVector2(60.0f, 40.0f));
		card->SetHitTestVisible(true);
		card->SetDraggable(true);
		int32 dragBegins    = 0;
		int32 dragEnds      = 0;
		bool  bDragCanceled = false;
		bool  bDragDropped  = false;
		ui->OnDragBegin.AddLambda([&dragBegins](const PSharedPtr<PGameUIElement>&, const HVector2&)
		{
			++dragBegins;
		});
		ui->OnDragEnd.AddLambda([&dragEnds, &bDragCanceled, &bDragDropped](const PSharedPtr<PGameUIElement>&, const HVector2&, const PSharedPtr<PGameUIElement>& dropTarget, bool bCanceled)
		{
			++dragEnds;
			bDragCanceled = bCanceled;
			bDragDropped  = dropTarget.IsValid();
		});
		send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestCardPoint));
		const EGameViewportInputTarget dragMove = send(*viewport, pointerEvent(EInputEventType::PointerMove, ViewportTestCardPoint + HVector2(20.0f, 0.0f)));
		const bool bDragging = ui->GetDraggedElement().GetRawPointer() == card.GetRawPointer();
		check(dragMove == EGameViewportInputTarget::GameUI && dragBegins == 1 && bDragging == true, "viewport: press and move on a draggable element starts a game UI drag");
		send(*viewport, focusLost);
		check(dragEnds == 1 && bDragCanceled == true && bDragDropped == false && ui->GetDraggedElement().IsValid() == false,
			PString::Format("viewport: focus lost ends the game UI drag as canceled, not as a drop on empty space (ends %d, canceled %d)", dragEnds, bDragCanceled ? 1 : 0));
		send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestCardPoint + HVector2(20.0f, 0.0f)));
		const EGameViewportInputTarget afterDragFocusLost = send(*viewport, pointerEvent(EInputEventType::PointerDown, ViewportTestEmptyPoint));
		send(*viewport, pointerEvent(EInputEventType::PointerUp, ViewportTestEmptyPoint));
		check(dragEnds == 1 && afterDragFocusLost == EGameViewportInputTarget::World && controller->Clicks == 3,
			"viewport: after the canceled drag a late release does not end it again and empty space goes to the world");

		// 크기 변경: UI 좌표 기준이 따라온다(배율 2 → 버튼 [20, 140]x[20, 80])
		viewport->Resize(ViewportTestWidth * 2, ViewportTestHeight * 2);
		check(send(*viewport, pointerEvent(EInputEventType::PointerDown, HVector2(100.0f, 50.0f))) == EGameViewportInputTarget::GameUI,
			"viewport: after Resize the game UI is hit at the scaled position");
		send(*viewport, pointerEvent(EInputEventType::PointerUp, HVector2(100.0f, 50.0f)));
		viewport->Resize(0, 0);
		check(viewport->GetSize().x == (int32)ViewportTestWidth * 2 && viewport->GetSize().y == (int32)ViewportTestHeight * 2, "viewport: Resize to zero (minimized) is ignored");
		viewport->Resize(ViewportTestWidth, ViewportTestHeight);

		// 뷰포트 가운데 광선은 궤도 대상(원점)을 향한다
		HRay ray;
		const bool bRay = viewport->ViewportPointToRay(HVector2((float32)ViewportTestWidth * 0.5f, (float32)ViewportTestHeight * 0.5f), &ray);
		const HVector3 toTarget = HVector3::Normalize(HVector3(0.0f, 0.0f, 0.0f) - ray.origin);
		check(bRay == true && nearlyEqual(HVector3::Dot(ray.dir, toTarget), 1.0f), "viewport: the center ray points at the orbit target");

		// 소스를 비우면 게임 인스턴스로 돌아간다(입력은 보내지 않는다 — 에디터의 게임 월드를 건드리지 않게)
		viewport->SetSource(nullptr, nullptr);
		check(viewport->Render(&problem) == false, "viewport: with the source cleared and no renderer Render is false");

		runRendererChecks(check, world, ui);

		ui->Shutdown();
		world->EndPlay();
	}
}

HSelfTestResult PGameViewportSelfTest::Run()
{
	HViewportCheck check;
	std::cout << "== GameViewport self test ==" << std::endl;

	runViewportChecks(check);

	std::cout << "== GameViewport self test: " << check.Passed << " passed, " << check.Failures << " failed ==" << std::endl;
	JG_LOG(GameViewportSelfTest, ELogLevel::Info, "GameViewport self test: %d passed, %d failed", check.Passed, check.Failures);
	return { check.Passed, check.Failures, check.Skipped };
}
