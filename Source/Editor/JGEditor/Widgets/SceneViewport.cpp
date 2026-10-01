#include "PCH/PCH.h"
#include "Widgets/SceneViewport.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Components/CameraComponent.h"
#include "Components/PickShapeComponent.h"
#include "Actors/CameraActor.h"
#include "Actors/GameplayControllerActor.h"
#include "Classes/Scene.h"
#include "Classes/SceneRenderer.h"
#include "Classes/Texture.h"
#include "GUI.h"
#include "UI/GameUIManager.h"
#include "UI/GameUIUtf8.h"

namespace
{
	// 렌더 해상도. 창 크기가 바뀔 때마다 G버퍼를 다시 만들지 않도록 고정하고, 표시할 때 비율을 지켜 줄인다.
	constexpr uint32  SceneViewportTextureWidth  = 1280;
	constexpr uint32  SceneViewportTextureHeight = 720;

	// 창이 처음 뜰 때 ImGui 는 내용 크기에 창을 맞춘다. 내용(이미지) 크기를 창에서 구하므로 최소 크기가 없으면 창이 작은 채로 남는다.
	constexpr float32 SceneViewportMinImageWidth  = 640.0f;
	constexpr float32 SceneViewportMinImageHeight = 360.0f;

	constexpr float32 SceneViewportOrbitRadiansPerPixel = 0.01f;
	constexpr float32 SceneViewportZoomPerWheelStep     = 0.9f;
}

PString JGSceneViewport::GetTitleName() const
{
	return "Scene Viewport";
}

void JGSceneViewport::OnInitialize()
{
	_renderer = Allocate<PSceneRenderer>();
	if (_renderer->Initialize(PString("SceneViewport"), SceneViewportTextureWidth, SceneViewportTextureHeight) == false)
	{
		JG_LOG(JGEditor, ELogLevel::Error, "SceneViewport: fail to initialize the scene renderer");
		_renderer = nullptr;
	}
}

void JGSceneViewport::OnShutdown()
{
	_renderer = nullptr;
}

void JGSceneViewport::OnUpdate()
{
	// Update 단계는 GUI 생성(GraphicsBegin) 뒤 · GUI 드로우(GraphicsEnd) 앞이라, 여기서 기록한 드로우가 이번 프레임 GUI 가 텍스처를 그리기 전에 실행된다.
	PSharedPtr<PWorld>            world;
	PSharedPtr<JGCameraComponent> camera;
	if (findView(world, camera, nullptr) == false)
	{
		return;
	}
	_renderer->Render(*world->GetScene(), camera->GetSceneCameraID());

	// 게임 UI(게임 인스턴스의 JGGameWidget 레이어 스택)를 월드 위에 그린다. 올라간 위젯이 없으면 아무것도 하지 않는다.
	PSharedPtr<PGameUIManager> ui = JGGameInstance::Get().GetUI();
	if (ui != nullptr)
	{
		ui->Render(_renderer->GetOutputTexture());
	}
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

	if (_renderer == nullptr || _renderer->GetOutputTexture() == nullptr)
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

	PSharedPtr<PScene> scene = world->GetScene();

	// 비율을 지켜 창에 맞춘다. 아래 두 줄(상태 · 클릭 결과) 자리를 남긴다.
	const float32 aspect    = (float32)_renderer->GetWidth() / (float32)_renderer->GetHeight();
	const float32 textSpace = HGUI::GetFrameHeightWithSpacing() * 2.0f;
	HVector2 imageSize(_contentSize.x, _contentSize.y - textSpace);
	if (imageSize.x / aspect > imageSize.y)
	{
		imageSize.x = imageSize.y * aspect;
	}
	else
	{
		imageSize.y = imageSize.x / aspect;
	}
	if (imageSize.x < SceneViewportMinImageWidth || imageSize.y < SceneViewportMinImageHeight)
	{
		imageSize = HVector2(SceneViewportMinImageWidth, SceneViewportMinImageWidth / aspect);
	}

	const HGUIImageInput input = HGUI::InteractiveImage(PString("##SceneViewportImage"), _renderer->GetOutputTexture()->GetTextureID(), imageSize);
	handleInput(world, camera, input, imageSize);
	handleKeyboard(input, imageSize);

	HGUI::Text(PString::Format("World %s | actors %d | meshes %d | camera %s | LMB pick, RMB drag orbit, wheel zoom",
		world->GetName().ToString(), (int32)world->GetActors().size(), (int32)scene->GetMeshes().size(),
		camera->GetOwner() != nullptr ? camera->GetOwner()->GetName().ToString() : PString("?")));
	if (_lastClickText.Empty() == false)
	{
		HGUI::Text(_lastClickText);
	}
}

void JGSceneViewport::handleInput(PSharedPtr<PWorld> world, PSharedPtr<JGCameraComponent> camera, const HGUIImageInput& input, const HVector2& imageSize)
{
	if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
	{
		return;
	}

	const float32 aspect = (float32)_renderer->GetWidth() / (float32)_renderer->GetHeight();

	// 게임 UI가 먼저 받는다. UI가 받은 누름 · 휠은 월드로 넘기지 않는다(메뉴 위젯이 떠 있으면 빈 곳도 UI 가 받는다).
	bool bClickConsumedByUI[(int32)EGUIMouseButton::Count];
	bool bWheelConsumedByUI = false;
	forwardPointerToGameUI(input, imageSize, bClickConsumedByUI, bWheelConsumedByUI);

	if (input.bClicked[(int32)EGUIMouseButton::Left] == true && bClickConsumedByUI[(int32)EGUIMouseButton::Left] == true)
	{
		const HVector2 viewportPoint(input.LocalPosition.x / imageSize.x, input.LocalPosition.y / imageSize.y);
		_lastClickText = PString::Format("Click (%.3f, %.3f): game UI (not picked)", viewportPoint.x, viewportPoint.y);
		JG_LOG(JGEditor, ELogLevel::Info, "SceneViewport %s", _lastClickText);
	}
	else if (input.bClicked[(int32)EGUIMouseButton::Left] == true)
	{
		const HVector2 viewportPoint(input.LocalPosition.x / imageSize.x, input.LocalPosition.y / imageSize.y);
		const HRay ray = camera->ViewportPointToRay(viewportPoint, aspect);

		HList<PSharedPtr<JGGameplayControllerActor>> controllers;
		world->FindActors<JGGameplayControllerActor>(controllers);
		for (PSharedPtr<JGGameplayControllerActor>& controller : controllers)
		{
			controller->HandleClick(ray);
		}

		// 표시용. 컨트롤러가 있으면 첫 컨트롤러의 결과, 없으면 월드 판정만.
		PString result;
		if (controllers.empty() == false)
		{
			result = controllers[0]->GetLastPick().ToString();
		}
		else
		{
			HWorldPickHit hit;
			if (world->PickActor(ray, &hit) == true && hit.Actor != nullptr)
			{
				result = PString::Format("actor %s distance %.2f (no controller in the world)", hit.Actor->GetName().ToString(), hit.Distance);
			}
			else
			{
				result = "nothing (no controller in the world)";
			}
		}
		_lastClickText = PString::Format("Click (%.3f, %.3f): %s", viewportPoint.x, viewportPoint.y, result);
		JG_LOG(JGEditor, ELogLevel::Info, "SceneViewport %s", _lastClickText);
	}

	PSharedPtr<JGCameraActor> cameraActor = RawDynamicCast<JGCameraActor>(camera->GetOwner());
	if (cameraActor == nullptr || cameraActor->GetMode() != ECameraActorMode::Orbit)
	{
		return;
	}

	const bool bRightDragOwnedByUI = _bPointerOwnedByUI[(int32)EGUIMouseButton::Right];
	if (input.bDown[(int32)EGUIMouseButton::Right] == true && bRightDragOwnedByUI == false && (input.MouseDelta.x != 0.0f || input.MouseDelta.y != 0.0f))
	{
		cameraActor->AddOrbitInput(input.MouseDelta.x * SceneViewportOrbitRadiansPerPixel, input.MouseDelta.y * SceneViewportOrbitRadiansPerPixel, 1.0f);
	}
	if (input.Wheel != 0.0f && bWheelConsumedByUI == false)
	{
		cameraActor->AddOrbitInput(0.0f, 0.0f, powf(SceneViewportZoomPerWheelStep, input.Wheel));
	}
}

void JGSceneViewport::forwardPointerToGameUI(const HGUIImageInput& input, const HVector2& imageSize, bool outClickConsumed[(int32)EGUIMouseButton::Count], bool& outWheelConsumed)
{
	for (int32 i = 0; i < (int32)EGUIMouseButton::Count; ++i)
	{
		outClickConsumed[i] = false;
	}
	outWheelConsumed = false;

	PSharedPtr<PGameUIManager> ui = JGGameInstance::Get().GetUI();
	if (ui == nullptr)
	{
		return;
	}

	// 이미지 좌표 → 렌더 타깃 픽셀. 게임 UI 는 렌더 타깃 픽셀을 받는다(이미지는 창에 맞춰 줄여 보여 준다).
	const HVector2 targetSize((float32)_renderer->GetWidth(), (float32)_renderer->GetHeight());
	HGameUIPointerEvent event;
	event.Position = HVector2(input.LocalPosition.x * targetSize.x / imageSize.x, input.LocalPosition.y * targetSize.y / imageSize.y);

	bool bAnyDown = false;
	for (int32 i = 0; i < (int32)EGUIMouseButton::Count; ++i)
	{
		bAnyDown = bAnyDown || input.bDown[i];
	}

	if (input.bHovered || bAnyDown)
	{
		event.Type = EGameUIPointerEventType::Move;
		ui->HandlePointer(event, targetSize);
		_bPointerWasInside = true;
	}
	else if (_bPointerWasInside)
	{
		event.Type = EGameUIPointerEventType::Leave;
		ui->HandlePointer(event, targetSize);
		_bPointerWasInside = false;
	}

	for (int32 i = 0; i < (int32)EGUIMouseButton::Count; ++i)
	{
		event.Button = (EGameUIPointerButton)i;
		if (input.bClicked[i])
		{
			event.Type = EGameUIPointerEventType::Down;
			const bool bConsumed  = ui->HandlePointer(event, targetSize);
			outClickConsumed[i]   = bConsumed;
			_bPointerOwnedByUI[i] = bConsumed;
		}

		// 뗌: 직전 프레임에 누르고 있었는데 이제 아니다. 한 프레임 안에 누르고 뗀 경우도 여기서 보낸다.
		const bool bReleased = (_bPointerWasDown[i] || input.bClicked[i]) && input.bDown[i] == false;
		if (bReleased)
		{
			event.Type = EGameUIPointerEventType::Up;
			ui->HandlePointer(event, targetSize);
			_bPointerOwnedByUI[i] = false;
		}
		_bPointerWasDown[i] = input.bDown[i];
	}

	if (input.Wheel != 0.0f)
	{
		event.Type  = EGameUIPointerEventType::Wheel;
		event.Wheel = input.Wheel;
		outWheelConsumed = ui->HandlePointer(event, targetSize);
	}
}

void JGSceneViewport::handleKeyboard(const HGUIImageInput& input, const HVector2& imageSize)
{
	// 씬 뷰포트 창에 포커스가 있을 때만 키보드를 게임 UI 로 넘긴다.
	if (HGUI::IsWindowFocused() == false)
	{
		return;
	}

	PSharedPtr<PGameUIManager> ui = JGGameInstance::Get().GetUI();
	if (ui != nullptr && ui->WantsTextInput())
	{
		forwardTextInput(*ui, input, imageSize);
		return;
	}

	// Esc = 게임 UI 뒤로가기(CommonUI 의 Back). 맨 위 뒤로가기 처리 위젯이 받는다.
	if (HGUI::IsKeyPressed(EGUIKey::Escape) == false)
	{
		return;
	}
	const bool bHandled = ui != nullptr && ui->HandleBackAction();
	_lastClickText = bHandled ? PString("Back (Esc): handled by game UI") : PString("Back (Esc): no game UI back handler");
	JG_LOG(JGEditor, ELogLevel::Info, "SceneViewport %s", _lastClickText);
}

void JGSceneViewport::forwardTextInput(PGameUIManager& ui, const HGUIImageInput& input, const HVector2& imageSize)
{
	// 편집 키를 ImGui 키보드 내비게이션에서 가져온다(방향키 · Tab · Enter · Space 가 창 안 항목 이동 · 실행에 쓰이지 않게).
	HGUI::ClaimTextEditKeys();

	// 1) 확정 글자(IME 확정 포함)를 먼저. 제어 문자(Backspace · Enter 의 글자 등)는 관리자가 버린다. BMP 밖 글자(U+FFFD)는 아직 받지 않는다.
	HList<uint32> codepoints;
	HGUI::GetInputCharacters(codepoints);
	HList<uint32> accepted;
	for (uint32 codepoint : codepoints)
	{
		if (codepoint != HGameUIUtf8::ReplacementCharacter)
		{
			accepted.push_back(codepoint);
		}
	}
	if (accepted.empty() == false)
	{
		ui.HandleTextInput(HGameUIUtf8::Encode(accepted));
	}

	// 2) 편집 키. 지우기 · 방향은 누르고 있으면 반복한다.
	struct HKeyMapping
	{
		EGUIKey    GUIKey;
		EGameUIKey GameKey;
		bool       bRepeat;
	};
	static const HKeyMapping KeyMappings[] =
	{
		{ EGUIKey::Backspace,   EGameUIKey::Backspace, true  },
		{ EGUIKey::Delete,      EGameUIKey::Delete,    true  },
		{ EGUIKey::LeftArrow,   EGameUIKey::Left,      true  },
		{ EGUIKey::RightArrow,  EGameUIKey::Right,     true  },
		{ EGUIKey::Home,        EGameUIKey::Home,      false },
		{ EGUIKey::End,         EGameUIKey::End,       false },
		{ EGUIKey::Tab,         EGameUIKey::Tab,       false },
		{ EGUIKey::Enter,       EGameUIKey::Enter,     false },
		{ EGUIKey::KeypadEnter, EGameUIKey::Enter,     false },
		{ EGUIKey::Escape,      EGameUIKey::Escape,    false },
	};
	const bool bShift = HGUI::IsShiftDown();
	for (const HKeyMapping& mapping : KeyMappings)
	{
		// 앞 키가 포커스를 풀었으면(Enter · Esc) 나머지는 넘기지 않는다.
		if (ui.WantsTextInput() == false)
		{
			break;
		}
		if (HGUI::IsKeyPressed(mapping.GUIKey, mapping.bRepeat))
		{
			ui.HandleKey(mapping.GameKey, bShift);
		}
	}

	// 3) Ctrl 단축키: 전체 선택 · 복사 · 잘라내기 · 붙여넣기. AltGr(= Ctrl+Alt)는 글자 입력이라 뺀다.
	if (ui.WantsTextInput() && HGUI::IsCtrlDown() && HGUI::IsAltDown() == false)
	{
		if (HGUI::IsKeyPressed(EGUIKey::A))
		{
			ui.HandleKey(EGameUIKey::SelectAll);
		}

		const bool bCut = HGUI::IsKeyPressed(EGUIKey::X);
		if (bCut || HGUI::IsKeyPressed(EGUIKey::C))
		{
			const PString selection = ui.GetTextInputSelection();
			if (selection.Empty() == false)
			{
				HGUI::SetClipboardText(selection);
				if (bCut)
				{
					ui.HandleKey(EGameUIKey::Delete);
				}
			}
		}

		if (HGUI::IsKeyPressed(EGUIKey::V, true))
		{
			ui.HandleTextInput(HGUI::GetClipboardText());
		}
	}

	// 4) OS IME 조합 · 후보 창을 게임 커서 옆에(렌더 타깃 픽셀 → 이미지 → ImGui 화면 좌표).
	HRect caret;
	if (ui.WantsTextInput() && ui.GetTextInputCaretRect(caret) && imageSize.x > 0.0f && imageSize.y > 0.0f)
	{
		const float32 toImageX = imageSize.x / (float32)_renderer->GetWidth();
		const float32 toImageY = imageSize.y / (float32)_renderer->GetHeight();
		const HVector2 screenPosition(input.ScreenPosition.x + caret.left * toImageX, input.ScreenPosition.y + caret.top * toImageY);
		HGUI::SetTextInputPosition(screenPosition, (caret.bottom - caret.top) * toImageY);
	}
}
