#include "PCH/PCH.h"
#include "Widgets/SceneViewport.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Components/CameraComponent.h"
#include "Components/PickShapeComponent.h"
#include "Actors/CameraActor.h"
#include "Actors/GameplayControllerActor.h"
#include "Viewport/GameViewport.h"
#include "Classes/Scene.h"
#include "Classes/Texture.h"
#include "Math/Ray.h"
#include "GUI.h"
#include "UI/GameUIUtf8.h"

namespace
{
	// 처음 만드는 렌더 크기. 보이는 이미지 크기를 알게 되면(첫 프레임) 0.3초 뒤 그 크기로 바꾼다.
	constexpr uint32  SceneViewportInitialWidth  = 1280;
	constexpr uint32  SceneViewportInitialHeight = 720;

	// 보이는 이미지는 16:9(게임 UI 기준 해상도 1920x1080 과 같은 비율). 화면 검증 스크립트가 이미지 사각형을 이 비율로 역산한다.
	constexpr float32 SceneViewportAspect = 16.0f / 9.0f;

	// 창이 처음 뜰 때 ImGui 는 내용 크기에 창을 맞춘다. 내용(이미지) 크기를 창에서 구하므로 최소 크기가 없으면 창이 작은 채로 남는다.
	// 100%(96 DPI) 기준 크기다 — 화면 배율(HGUI::GetDpiScale)을 곱해 쓴다(D-22 ②).
	constexpr float32 SceneViewportMinImageWidth  = 640.0f;
	constexpr float32 SceneViewportMinImageHeight = 360.0f;

	// 보이는 크기가 이만큼 그대로이면 렌더 텍스처를 그 크기로 다시 만든다(셰이더 두 개를 다시 컴파일하므로 창을 끄는 동안에는 하지 않는다).
	constexpr float64 SceneViewportResizeSettleSeconds = 0.3;

	// 100% 기준 마우스 1픽셀당 궤도 회전. 실제 픽셀로 받으므로 화면 배율로 나눠 같은 손 움직임에 같은 각도를 준다.
	constexpr float32 SceneViewportOrbitRadiansPerPixel = 0.01f;
	constexpr float32 SceneViewportZoomPerWheelStep     = 0.9f;

	EPointerButton toPointerButton(int32 guiButton)
	{
		switch ((EGUIMouseButton)guiButton)
		{
		case EGUIMouseButton::Right:
			return EPointerButton::Right;
		case EGUIMouseButton::Middle:
			return EPointerButton::Middle;
		case EGUIMouseButton::Left:
		default:
			return EPointerButton::Left;
		}
	}

	EInputModifier currentModifiers()
	{
		EInputModifier modifiers = EInputModifier::None;
		if (HGUI::IsShiftDown() == true)
		{
			modifiers |= EInputModifier::Shift;
		}
		if (HGUI::IsCtrlDown() == true)
		{
			modifiers |= EInputModifier::Ctrl;
		}
		if (HGUI::IsAltDown() == true)
		{
			modifiers |= EInputModifier::Alt;
		}
		return modifiers;
	}

	// ImGui 키 → 엔진 키. bRepeat = 누르고 있으면 반복해서 넘긴다.
	struct HKeyMapping
	{
		EGUIKey  GUIKey;
		EKeyCode Key;
		bool     bRepeat;
	};

	// 게임 입력란 편집 키. 지우기 · 좌우 이동만 반복한다.
	const HKeyMapping TextEditKeys[] =
	{
		{ EGUIKey::Backspace,   EKeyCode::BackSpace, true  },
		{ EGUIKey::Delete,      EKeyCode::Del,       true  },
		{ EGUIKey::LeftArrow,   EKeyCode::Left,      true  },
		{ EGUIKey::RightArrow,  EKeyCode::Right,     true  },
		{ EGUIKey::Home,        EKeyCode::Home,      false },
		{ EGUIKey::End,         EKeyCode::End,       false },
		{ EGUIKey::Tab,         EKeyCode::Tab,       false },
		{ EGUIKey::Enter,       EKeyCode::Enter,     false },
		{ EGUIKey::KeypadEnter, EKeyCode::Enter,     false },
		{ EGUIKey::Escape,      EKeyCode::Esc,       false },
	};

	// 입력란 Ctrl 단축키: 전체 선택 · 복사 · 잘라내기 · 붙여넣기(붙여넣기만 반복). 클립보드는 게임 뷰포트가 OS 클립보드로 다룬다.
	const HKeyMapping TextShortcutKeys[] =
	{
		{ EGUIKey::A, EKeyCode::A, false },
		{ EGUIKey::C, EKeyCode::C, false },
		{ EGUIKey::X, EKeyCode::X, false },
		{ EGUIKey::V, EKeyCode::V, true  },
	};
}

PString JGSceneViewport::GetTitleName() const
{
	return "Scene Viewport";
}

void JGSceneViewport::OnInitialize()
{
	_viewport = Allocate<PGameViewport>();
	if (_viewport->Initialize(PString("SceneViewport"), SceneViewportInitialWidth, SceneViewportInitialHeight) == false)
	{
		JG_LOG(JGEditor, ELogLevel::Error, "SceneViewport: fail to initialize the game viewport");
		_viewport = nullptr;
	}
}

void JGSceneViewport::OnShutdown()
{
	_viewport = nullptr;
}

void JGSceneViewport::OnUpdate()
{
	if (_viewport == nullptr)
	{
		return;
	}

	// Update 단계는 GUI 생성(GraphicsBegin) 뒤 · GUI 드로우(GraphicsEnd) 앞이라, 여기서 기록한 드로우가 이번 프레임 GUI 가 텍스처를 그리기 전에 실행된다.
	// 그릴 월드 · 카메라가 없으면 그리지 않는다(이유는 OnGenerateGUI 가 보여 준다).
	_viewport->Render();
}

void JGSceneViewport::OnLayout(const HWidgetLayout& InLayout)
{
	_contentSize = InLayout.ContentSize;
}

bool JGSceneViewport::findView(PSharedPtr<PWorld>& outWorld, PSharedPtr<JGCameraComponent>& outCamera, PString* outProblem) const
{
	if (JGGameInstance::HasInstance() == false)
	{
		if (outProblem != nullptr)
		{
			*outProblem = "GameFrameWorks is not running";
		}
		return false;
	}

	outWorld = JGGameInstance::Get().GetWorld();
	if (outWorld == nullptr)
	{
		if (outProblem != nullptr)
		{
			*outProblem = "No world is loaded";
		}
		return false;
	}

	outCamera = outWorld->GetActiveCamera();
	if (outCamera == nullptr || outCamera->GetSceneCameraID().IsValid() == false)
	{
		if (outProblem != nullptr)
		{
			*outProblem = PString::Format("World %s: no active camera", outWorld->GetName().ToString());
		}
		return false;
	}

	if (_viewport == nullptr || _viewport->GetDisplayTexture() == nullptr)
	{
		if (outProblem != nullptr)
		{
			*outProblem = "Scene viewport renderer is not ready";
		}
		return false;
	}
	return true;
}

void JGSceneViewport::OnGenerateGUI()
{
	PSharedPtr<PWorld>            world;
	PSharedPtr<JGCameraComponent> camera;
	PString                       problem;
	if (findView(world, camera, &problem) == false)
	{
		HGUI::Text(problem);
		return;
	}

	const HVector2Int imagePixels = computeImageSize();
	updateViewportSize(imagePixels);

	// 렌더 텍스처는 크기를 바꾸면 새 객체가 된다 — 보관하지 않고 매 프레임 다시 받는다.
	const HVector2 imageSize((float32)imagePixels.x, (float32)imagePixels.y);
	const HGUIImageInput input = HGUI::InteractiveImage(PString("##SceneViewportImage"), _viewport->GetDisplayTexture()->GetTextureID(), imageSize);
	handleFocusLoss();
	handlePointer(world, camera, input, imageSize);
	handleKeyboard(input, imageSize);

	const HVector2Int renderSize = _viewport->GetRenderSize();
	PSharedPtr<PScene> scene = world->GetScene();
	HGUI::Text(PString::Format("World %s | actors %d | meshes %d | camera %s | render %dx%d | LMB pick, RMB drag orbit, wheel zoom",
		world->GetName().ToString(), (int32)world->GetActors().size(), (int32)scene->GetMeshes().size(),
		camera->GetOwner() != nullptr ? camera->GetOwner()->GetName().ToString() : PString("?"), renderSize.x, renderSize.y));
	if (_lastClickText.Empty() == false)
	{
		HGUI::Text(_lastClickText);
	}
}

HVector2Int JGSceneViewport::computeImageSize() const
{
	const float32 textSpace       = HGUI::GetFrameHeightWithSpacing() * 2.0f;
	const float32 availableWidth  = _contentSize.x;
	const float32 availableHeight = _contentSize.y - textSpace;

	int32 width  = (int32)floorf(availableWidth);
	int32 height = (int32)floorf((float32)width / SceneViewportAspect + 0.5f);
	if ((float32)height > availableHeight)
	{
		height = (int32)floorf(availableHeight);
		width  = (int32)floorf((float32)height * SceneViewportAspect + 0.5f);
	}
	const float32 dpiScale  = HGUI::GetDpiScale();
	const int32   minWidth  = (int32)(SceneViewportMinImageWidth * dpiScale + 0.5f);
	const int32   minHeight = (int32)(SceneViewportMinImageHeight * dpiScale + 0.5f);
	if (width < minWidth || height < minHeight)
	{
		width  = minWidth;
		height = minHeight;
	}
	return HVector2Int(width, height);
}

void JGSceneViewport::updateViewportSize(const HVector2Int& desiredSize)
{
	const float64 now = HGUI::GetTime();
	if (desiredSize != _desiredSize)
	{
		_desiredSize      = desiredSize;
		_desiredSizeSince = now;
		return;
	}

	const HVector2Int viewportSize = _viewport->GetSize();
	if (viewportSize == desiredSize || now - _desiredSizeSince < SceneViewportResizeSettleSeconds)
	{
		return;
	}

	// 게임 UI 배율은 렌더 크기 / 1920x1080 의 작은 쪽이다(PGameUIManager). 보이는 크기로 그리므로 글자를 늘려 보이지 않는다.
	const float32 uiScale = HMath::Min((float32)desiredSize.x / 1920.0f, (float32)desiredSize.y / 1080.0f);
	JG_LOG(JGEditor, ELogLevel::Info, "SceneViewport render size %dx%d -> %dx%d (game UI scale %.3f)",
		viewportSize.x, viewportSize.y, desiredSize.x, desiredSize.y, uiScale);
	_viewport->Resize((uint32)desiredSize.x, (uint32)desiredSize.y);
}

HVector2 JGSceneViewport::toViewport(const HVector2& imagePoint, const HVector2& imageSize) const
{
	// 렌더 크기를 맞추기 전(0.3초 동안)에는 뷰포트 크기와 보이는 크기가 다르다 — 비율로 바꾼다.
	const HVector2Int viewportSize = _viewport->GetSize();
	return HVector2(imagePoint.x * (float32)viewportSize.x / imageSize.x, imagePoint.y * (float32)viewportSize.y / imageSize.y);
}

void JGSceneViewport::handlePointer(PSharedPtr<PWorld> world, PSharedPtr<JGCameraComponent> camera, const HGUIImageInput& input, const HVector2& imageSize)
{
	if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
	{
		return;
	}

	HInputEvent event;
	event.Position  = toViewport(input.LocalPosition, imageSize);
	event.Modifiers = currentModifiers();

	bool bAnyDown = false;
	for (int32 i = 0; i < (int32)EGUIMouseButton::Count; ++i)
	{
		bAnyDown = bAnyDown || input.bDown[i];
	}

	// 포커스를 잃어 누름을 취소했다 — 버튼이 다 풀릴 때까지 아무것도 넘기지 않는다(풀림을 뗌으로 보내지 않는다).
	if (_bPointerCanceled == true)
	{
		if (bAnyDown == true)
		{
			return;
		}
		_bPointerCanceled = false;
	}

	// 이동 · 벗어남: 이미지 위에 있거나 누르는 중이면 매 프레임 위치를 보낸다(게임 UI 호버 · 끌기).
	if (input.bHovered || bAnyDown)
	{
		event.Type = EInputEventType::PointerMove;
		_viewport->HandleInput(event);
		_bPointerWasInside = true;
	}
	else if (_bPointerWasInside)
	{
		event.Type = EInputEventType::PointerLeave;
		_viewport->HandleInput(event);
		_bPointerWasInside = false;
	}

	for (int32 i = 0; i < (int32)EGUIMouseButton::Count; ++i)
	{
		event.Button = toPointerButton(i);
		if (input.bClicked[i])
		{
			event.Type = EInputEventType::PointerDown;
			EGameViewportInputTarget target = EGameViewportInputTarget::None;
			_viewport->HandleInput(event, &target);
			_bPressUsedByGame[i] = target != EGameViewportInputTarget::None;

			if (i == (int32)EGUIMouseButton::Left)
			{
				const HVector2 normalizedPoint(input.LocalPosition.x / imageSize.x, input.LocalPosition.y / imageSize.y);
				describeLeftClick(world, normalizedPoint, event.Position, target);
			}
		}

		// 뗌: 직전 프레임에 누르고 있었는데 이제 아니다. 한 프레임 안에 누르고 뗀 경우도 여기서 보낸다.
		const bool bReleased = (_bPointerWasDown[i] || input.bClicked[i]) && input.bDown[i] == false;
		if (bReleased)
		{
			event.Type = EInputEventType::PointerUp;
			_viewport->HandleInput(event);
			_bPressUsedByGame[i] = false;
		}
		_bPointerWasDown[i] = input.bDown[i];
	}

	bool bWheelUsedByGame = false;
	if (input.Wheel != 0.0f || input.WheelHorizontal != 0.0f)
	{
		event.Type       = EInputEventType::PointerWheel;
		event.WheelDelta = HVector2(input.WheelHorizontal, input.Wheel);
		bWheelUsedByGame = _viewport->HandleInput(event);
	}

	// 에디터 조작: 게임이 쓰지 않은 오른쪽 끌기 · 휠 → 궤도 카메라의 궤도 회전 · 거리.
	PSharedPtr<JGCameraActor> cameraActor = RawDynamicCast<JGCameraActor>(camera->GetOwner());
	if (cameraActor == nullptr || cameraActor->GetMode() != ECameraActorMode::Orbit)
	{
		return;
	}

	const int32 right = (int32)EGUIMouseButton::Right;
	if (input.bDown[right] == true && _bPressUsedByGame[right] == false && (input.MouseDelta.x != 0.0f || input.MouseDelta.y != 0.0f))
	{
		const float32 radiansPerPixel = SceneViewportOrbitRadiansPerPixel / HGUI::GetDpiScale();
		cameraActor->AddOrbitInput(input.MouseDelta.x * radiansPerPixel, input.MouseDelta.y * radiansPerPixel, 1.0f);
	}
	if (input.Wheel != 0.0f && bWheelUsedByGame == false)
	{
		cameraActor->AddOrbitInput(0.0f, 0.0f, powf(SceneViewportZoomPerWheelStep, input.Wheel));
	}
}

void JGSceneViewport::handleFocusLoss()
{
	// 앱이 OS 포커스를 잃은 프레임(ImGui 가 그 프레임 끝에 키 · 마우스 버튼을 지운다) 또는 이 창이 ImGui 포커스를 잃은 프레임.
	const bool bFocused         = HGUI::IsWindowFocused();
	const bool bAppFocusLost    = HGUI::IsAppFocusLost();
	const bool bWindowFocusLost = _bWasFocused == true && bFocused == false;
	_bWasFocused = bFocused;
	if (bAppFocusLost == false && bWindowFocusLost == false)
	{
		return;
	}

	// 게임 뷰포트가 누르고 있던 버튼의 누름을 취소한다(게임 UI 끌기는 "취소"로 끝난다 — GFW GP-F1c).
	HInputEvent event;
	event.Type      = EInputEventType::FocusLost;
	event.Modifiers = currentModifiers();
	_viewport->HandleInput(event);

	bool bAnyPressed = false;
	for (int32 i = 0; i < (int32)EGUIMouseButton::Count; ++i)
	{
		bAnyPressed = bAnyPressed || _bPointerWasDown[i];
		_bPointerWasDown[i]  = false;
		_bPressUsedByGame[i] = false;
	}
	_bPointerWasInside = false;
	_bPointerCanceled  = true;
	if (bAnyPressed == true)
	{
		JG_LOG(JGEditor, ELogLevel::Info, "SceneViewport focus lost (%s): press canceled", bAppFocusLost ? "app" : "window");
	}
}

void JGSceneViewport::describeLeftClick(PSharedPtr<PWorld> world, const HVector2& normalizedPoint, const HVector2& viewportPoint, EGameViewportInputTarget target)
{
	PString result;
	if (target == EGameViewportInputTarget::GameUI)
	{
		result = "game UI (not picked)";
	}
	else if (target == EGameViewportInputTarget::World)
	{
		// 월드로 간 누름은 모든 컨트롤러가 받았다. 표시는 첫 컨트롤러의 피킹 결과.
		HList<PSharedPtr<JGGameplayControllerActor>> controllers;
		world->FindActors<JGGameplayControllerActor>(controllers);
		result = controllers.empty() ? PString("?") : controllers[0]->GetLastPick().ToString();
	}
	else
	{
		// 받을 컨트롤러가 없다 — 월드 판정만 보여 준다.
		HRay          ray;
		HWorldPickHit hit;
		if (_viewport->ViewportPointToRay(viewportPoint, &ray) == true && world->PickActor(ray, &hit) == true && hit.Actor != nullptr)
		{
			result = PString::Format("actor %s distance %.2f (no controller in the world)", hit.Actor->GetName().ToString(), hit.Distance);
		}
		else
		{
			result = "nothing (no controller in the world)";
		}
	}

	_lastClickText = PString::Format("Click (%.3f, %.3f): %s", normalizedPoint.x, normalizedPoint.y, result);
	JG_LOG(JGEditor, ELogLevel::Info, "SceneViewport %s", _lastClickText);
}

void JGSceneViewport::handleKeyboard(const HGUIImageInput& input, const HVector2& imageSize)
{
	// 씬 뷰포트 창에 포커스가 있을 때만 키보드를 게임으로 넘긴다.
	if (HGUI::IsWindowFocused() == false)
	{
		return;
	}

	if (_viewport->WantsTextInput() == true)
	{
		forwardTextInput(input, imageSize);
		return;
	}

	// Esc = 게임 UI 뒤로가기(CommonUI 의 Back). 맨 위 뒤로가기 처리 위젯이 받는다.
	if (HGUI::IsKeyPressed(EGUIKey::Escape) == false)
	{
		return;
	}

	HInputEvent event;
	event.Type      = EInputEventType::KeyDown;
	event.Key       = EKeyCode::Esc;
	event.Modifiers = currentModifiers();
	const bool bHandled = _viewport->HandleInput(event);
	_lastClickText = bHandled ? PString("Back (Esc): handled by game UI") : PString("Back (Esc): no game UI back handler");
	JG_LOG(JGEditor, ELogLevel::Info, "SceneViewport %s", _lastClickText);
}

void JGSceneViewport::forwardTextInput(const HGUIImageInput& input, const HVector2& imageSize)
{
	// 편집 키를 ImGui 키보드 내비게이션에서 가져온다(방향키 · Tab · Enter · Space 가 창 안 항목 이동 · 실행에 쓰이지 않게).
	HGUI::ClaimTextEditKeys();

	HInputEvent event;
	event.Modifiers = currentModifiers();

	// 1) 확정 글자(IME 확정 포함)를 먼저. 제어 문자는 보내지 않는다(엔진 입력 규칙). BMP 밖 글자(ImGui 가 U+FFFD 로 준다)는 아직 받지 않는다.
	HList<uint32> codepoints;
	HGUI::GetInputCharacters(codepoints);
	event.Type = EInputEventType::Char;
	for (uint32 codepoint : codepoints)
	{
		if (codepoint < 0x20 || codepoint == 0x7F || codepoint == HGameUIUtf8::ReplacementCharacter)
		{
			continue;
		}
		event.Codepoint = codepoint;
		_viewport->HandleInput(event);
	}

	// 2) 편집 키. 앞 키가 포커스를 풀었으면(Enter · Esc) 나머지는 넘기지 않는다.
	event.Type = EInputEventType::KeyDown;
	for (const HKeyMapping& mapping : TextEditKeys)
	{
		if (_viewport->WantsTextInput() == false)
		{
			break;
		}
		if (HGUI::IsKeyPressed(mapping.GUIKey, mapping.bRepeat))
		{
			event.Key = mapping.Key;
			_viewport->HandleInput(event);
		}
	}

	// 3) Ctrl 단축키. AltGr(= Ctrl+Alt)는 글자 입력이라 뺀다.
	if (_viewport->WantsTextInput() && HGUI::IsCtrlDown() && HGUI::IsAltDown() == false)
	{
		for (const HKeyMapping& mapping : TextShortcutKeys)
		{
			if (HGUI::IsKeyPressed(mapping.GUIKey, mapping.bRepeat))
			{
				event.Key = mapping.Key;
				_viewport->HandleInput(event);
			}
		}
	}

	// 4) OS IME 조합 · 후보 창을 게임 커서 옆에(뷰포트 픽셀 → 이미지 → ImGui 화면 좌표).
	HRect caret;
	const HVector2Int viewportSize = _viewport->GetSize();
	if (_viewport->GetTextInputCaretRect(&caret) == true && viewportSize.x > 0 && viewportSize.y > 0)
	{
		const float32  toImageX = imageSize.x / (float32)viewportSize.x;
		const float32  toImageY = imageSize.y / (float32)viewportSize.y;
		const HVector2 screenPosition(input.ScreenPosition.x + caret.left * toImageX, input.ScreenPosition.y + caret.top * toImageY);
		HGUI::SetTextInputPosition(screenPosition, (caret.bottom - caret.top) * toImageY);
	}
}
