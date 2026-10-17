#include "PCH/PCH.h"
#include "Viewport/GameViewport.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Components/CameraComponent.h"
#include "Actors/GameplayControllerActor.h"
#include "UI/GameUIManager.h"
#include "UI/GameUIUtf8.h"
#include "JGGraphics.h"
#include "Classes/Scene.h"
#include "Classes/SceneRenderer.h"
#include "Math/Ray.h"

namespace
{
	bool hasGraphicsAPI()
	{
		HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>();
		return graphicsModule != nullptr && graphicsModule->GetGraphicsAPI() != nullptr;
	}

	// 엔진 포인터 버튼 → 게임 UI 버튼. 게임 UI 는 왼쪽 · 오른쪽 · 가운데만 받는다.
	bool toGameUIButton(EPointerButton button, EGameUIPointerButton& outButton)
	{
		switch (button)
		{
		case EPointerButton::Left:
			outButton = EGameUIPointerButton::Left;
			return true;
		case EPointerButton::Right:
			outButton = EGameUIPointerButton::Right;
			return true;
		case EPointerButton::Middle:
			outButton = EGameUIPointerButton::Middle;
			return true;
		default:
			return false;
		}
	}

	bool isValidButton(EPointerButton button)
	{
		return (int32)button >= 0 && (int32)button < (int32)EPointerButton::Count;
	}

	bool hasModifier(EInputModifier modifiers, EInputModifier modifier)
	{
		return (modifiers & modifier) != EInputModifier::None;
	}

	// 입력란 편집 키(엔진 키 → 게임 UI 키). bRepeat = 누르고 있으면 반복해서 넘긴다(지우기 · 좌우 이동).
	struct HTextEditKey
	{
		EKeyCode   Key;
		EGameUIKey GameKey;
		bool       bRepeat;
	};

	const HTextEditKey TextEditKeys[] =
	{
		{ EKeyCode::BackSpace, EGameUIKey::Backspace, true  },
		{ EKeyCode::Del,       EGameUIKey::Delete,    true  },
		{ EKeyCode::Left,      EGameUIKey::Left,      true  },
		{ EKeyCode::Right,     EGameUIKey::Right,     true  },
		{ EKeyCode::Home,      EGameUIKey::Home,      false },
		{ EKeyCode::End,       EGameUIKey::End,       false },
		{ EKeyCode::Tab,       EGameUIKey::Tab,       false },
		{ EKeyCode::Enter,     EGameUIKey::Enter,     false },
		{ EKeyCode::Esc,       EGameUIKey::Escape,    false },
	};
}

bool PGameViewport::Initialize(const PString& name, uint32 width, uint32 height)
{
	if (width == 0 || height == 0)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Error, "PGameViewport %s: size must not be zero (%dx%d)", name, (int32)width, (int32)height);
		return false;
	}

	_name     = name;
	_size     = HVector2Int((int32)width, (int32)height);
	_renderer = nullptr;
	_resizeFailedSize = HVector2Int();

	// 그래픽이 없으면(헤드리스) 렌더러 없이 입력 라우팅만 한다.
	if (hasGraphicsAPI() == false)
	{
		return true;
	}

	_renderer = Allocate<PSceneRenderer>();
	if (_renderer->Initialize(name, width, height) == false)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Error, "PGameViewport %s: fail to initialize the scene renderer (%dx%d)", name, (int32)width, (int32)height);
		_renderer = nullptr;
		return false;
	}
	return true;
}

void PGameViewport::Resize(uint32 width, uint32 height)
{
	if (width == 0 || height == 0)
	{
		return;
	}
	_size = HVector2Int((int32)width, (int32)height);
}

bool PGameViewport::Render(PString* outProblem)
{
	PSharedPtr<PWorld>            world;
	PSharedPtr<JGCameraComponent> camera;
	if (findView(world, camera, outProblem) == false)
	{
		return false;
	}

	if (_renderer == nullptr)
	{
		if (outProblem != nullptr)
		{
			*outProblem = PString::Format("Game viewport %s has no renderer (no graphics)", _name);
		}
		return false;
	}

	// 렌더 텍스처를 뷰포트 크기에 맞춘다(PSceneRenderer::Resize, GP-G2). 크기가 실제로 다를 때만 다시 만든다 — 셰이더 두 개를 다시 컴파일하므로
	// "크기 변경이 멈춘 뒤 한 번" Resize 하는 것은 호스트 몫이다. 옛 텍스처는 GPU 가 쓰던 프레임이 끝난 뒤 놓여서 Update 단계에서 해도 된다.
	// 실패하면 이전 크기로 그리고, 같은 크기로는 다시 시도하지 않는다(매 프레임 컴파일하지 않게).
	const bool bRenderSizeDiffers = _renderer->GetWidth() != (uint32)_size.x || _renderer->GetHeight() != (uint32)_size.y;
	if (bRenderSizeDiffers == true && _size != _resizeFailedSize)
	{
		if (_renderer->Resize((uint32)_size.x, (uint32)_size.y) == false)
		{
			_resizeFailedSize = _size;
			JG_LOG(GameFrameWorks, ELogLevel::Error, "PGameViewport %s: fail to resize the render texture to %dx%d (stays %dx%d)",
				_name, _size.x, _size.y, (int32)_renderer->GetWidth(), (int32)_renderer->GetHeight());
		}
	}

	_renderer->Render(*world->GetScene(), camera->GetSceneCameraID());

	// 게임 UI 를 월드 위에 그린다. UI 색은 sRGB 로 정한 값이라 sRGB 디스플레이 텍스처 위에 그린다(선형 출력 텍스처에 그리면 옅게 뜬다, Graphics 5-14).
	PSharedPtr<PGameUIManager> ui = getUI();
	if (ui != nullptr)
	{
		ui->Render(_renderer->GetDisplayTexture());
	}
	return true;
}

PSharedPtr<IRawTexture> PGameViewport::GetDisplayTexture() const
{
	if (_renderer == nullptr)
	{
		return nullptr;
	}
	return _renderer->GetDisplayTexture();
}

HVector2Int PGameViewport::GetSize() const
{
	return _size;
}

HVector2Int PGameViewport::GetRenderSize() const
{
	if (_renderer == nullptr)
	{
		return _size;
	}
	return HVector2Int((int32)_renderer->GetWidth(), (int32)_renderer->GetHeight());
}

bool PGameViewport::HandleInput(const HInputEvent& event, EGameViewportInputTarget* outTarget)
{
	EGameViewportInputTarget target = EGameViewportInputTarget::None;

	switch (event.Type)
	{
	case EInputEventType::PointerMove:
	case EInputEventType::PointerDown:
	case EInputEventType::PointerUp:
	case EInputEventType::PointerWheel:
	case EInputEventType::PointerLeave:
		target = handlePointer(event);
		break;
	case EInputEventType::KeyDown:
		target = handleKeyDown(event);
		break;
	case EInputEventType::KeyUp:
		// 입력란에 포커스가 있는 동안의 키는 게임 몫이다(호스트 단축키로 쓰지 않는다).
		target = WantsTextInput() ? EGameViewportInputTarget::GameUI : EGameViewportInputTarget::None;
		break;
	case EInputEventType::Char:
		target = handleChar(event);
		break;
	case EInputEventType::FocusLost:
		releaseAllButtons();
		break;
	case EInputEventType::FocusGained:
	default:
		break;
	}

	if (outTarget != nullptr)
	{
		*outTarget = target;
	}
	return target != EGameViewportInputTarget::None;
}

bool PGameViewport::WantsTextInput() const
{
	PSharedPtr<PGameUIManager> ui = getUI();
	return ui != nullptr && ui->WantsTextInput();
}

bool PGameViewport::GetTextInputCaretRect(HRect* outRect) const
{
	PSharedPtr<PGameUIManager> ui = getUI();
	if (ui == nullptr || ui->WantsTextInput() == false)
	{
		return false;
	}

	HRect caret;
	if (ui->GetTextInputCaretRect(caret) == false)
	{
		return false;
	}

	// 렌더 텍스처 픽셀(게임 UI 좌표) → 뷰포트 픽셀
	const HVector2 targetSize = getTargetSize();
	const float32  toViewportX = (targetSize.x > 0.0f) ? (float32)_size.x / targetSize.x : 1.0f;
	const float32  toViewportY = (targetSize.y > 0.0f) ? (float32)_size.y / targetSize.y : 1.0f;
	if (outRect != nullptr)
	{
		*outRect = HRect(caret.left * toViewportX, caret.top * toViewportY, caret.right * toViewportX, caret.bottom * toViewportY);
	}
	return true;
}

bool PGameViewport::ViewportPointToRay(const HVector2& viewportPoint, HRay* outRay) const
{
	PSharedPtr<PWorld>            world;
	PSharedPtr<JGCameraComponent> camera;
	if (_size.x <= 0 || _size.y <= 0 || findView(world, camera, nullptr) == false)
	{
		return false;
	}

	// 광선은 보이는 그림(렌더 텍스처)의 비율로 낸다. 뷰포트와 렌더 텍스처 크기가 같으면 뷰포트 비율이다.
	const HVector2    normalized(viewportPoint.x / (float32)_size.x, viewportPoint.y / (float32)_size.y);
	const HVector2Int renderSize = GetRenderSize();
	const float32     aspect     = (renderSize.y > 0) ? (float32)renderSize.x / (float32)renderSize.y : 1.0f;
	if (outRay != nullptr)
	{
		*outRay = camera->ViewportPointToRay(normalized, aspect);
	}
	return true;
}

void PGameViewport::SetSource(const PSharedPtr<PWorld>& world, const PSharedPtr<PGameUIManager>& ui)
{
	_bHasSource  = (world != nullptr || ui != nullptr);
	_sourceWorld = world;
	_sourceUI    = ui;
}

PSharedPtr<PWorld> PGameViewport::getWorld() const
{
	if (_bHasSource == true)
	{
		return _sourceWorld.Pin();
	}
	if (JGGameInstance::HasInstance() == false)
	{
		return nullptr;
	}
	return JGGameInstance::Get().GetWorld();
}

PSharedPtr<PGameUIManager> PGameViewport::getUI() const
{
	if (_bHasSource == true)
	{
		return _sourceUI.Pin();
	}
	if (JGGameInstance::HasInstance() == false)
	{
		return nullptr;
	}
	return JGGameInstance::Get().GetUI();
}

bool PGameViewport::findView(PSharedPtr<PWorld>& outWorld, PSharedPtr<JGCameraComponent>& outCamera, PString* outProblem) const
{
	if (_bHasSource == false && JGGameInstance::HasInstance() == false)
	{
		if (outProblem != nullptr)
		{
			*outProblem = "GameFrameWorks is not running";
		}
		return false;
	}

	outWorld = getWorld();
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
	return true;
}

HVector2 PGameViewport::toTarget(const HVector2& viewportPoint) const
{
	const HVector2 targetSize = getTargetSize();
	if (_size.x <= 0 || _size.y <= 0)
	{
		return viewportPoint;
	}
	return HVector2(viewportPoint.x * targetSize.x / (float32)_size.x, viewportPoint.y * targetSize.y / (float32)_size.y);
}

HVector2 PGameViewport::getTargetSize() const
{
	const HVector2Int renderSize = GetRenderSize();
	return HVector2((float32)renderSize.x, (float32)renderSize.y);
}

EGameViewportInputTarget PGameViewport::handlePointer(const HInputEvent& event)
{
	PSharedPtr<PGameUIManager> ui = getUI();
	const HVector2 targetSize = getTargetSize();

	HGameUIPointerEvent uiEvent;
	uiEvent.Position = toTarget(event.Position);

	switch (event.Type)
	{
	case EInputEventType::PointerMove:
	{
		uiEvent.Type = EGameUIPointerEventType::Move;
		const bool bConsumed = ui != nullptr && ui->HandlePointer(uiEvent, targetSize);
		return bConsumed ? EGameViewportInputTarget::GameUI : EGameViewportInputTarget::None;
	}
	case EInputEventType::PointerLeave:
	{
		uiEvent.Type = EGameUIPointerEventType::Leave;
		if (ui != nullptr)
		{
			ui->HandlePointer(uiEvent, targetSize);
		}
		return EGameViewportInputTarget::None;
	}
	case EInputEventType::PointerWheel:
	{
		uiEvent.Type            = EGameUIPointerEventType::Wheel;
		uiEvent.Wheel           = event.WheelDelta.y;
		uiEvent.WheelHorizontal = event.WheelDelta.x;
		const bool bConsumed = (event.WheelDelta.y != 0.0f || event.WheelDelta.x != 0.0f) && ui != nullptr && ui->HandlePointer(uiEvent, targetSize);
		return bConsumed ? EGameViewportInputTarget::GameUI : EGameViewportInputTarget::None;
	}
	case EInputEventType::PointerDown:
	{
		if (isValidButton(event.Button) == false)
		{
			return EGameViewportInputTarget::None;
		}
		const int32 index = (int32)event.Button;

		// 게임 UI 먼저. UI 는 누름으로 입력란 포커스도 정한다(다른 곳 · 월드를 누르면 푼다).
		EGameViewportInputTarget target = EGameViewportInputTarget::None;
		if (toGameUIButton(event.Button, uiEvent.Button) == true && ui != nullptr)
		{
			uiEvent.Type = EGameUIPointerEventType::Down;
			if (ui->HandlePointer(uiEvent, targetSize) == true)
			{
				target = EGameViewportInputTarget::GameUI;
			}
		}

		// UI 가 받지 않은 왼쪽 누름은 월드로(활성 카메라 광선 → 컨트롤러).
		if (target == EGameViewportInputTarget::None && event.Button == EPointerButton::Left)
		{
			target = clickWorld(event.Position);
		}

		_bPressed[index]    = true;
		_pressTarget[index] = target;
		return target;
	}
	case EInputEventType::PointerUp:
	{
		if (isValidButton(event.Button) == false)
		{
			return EGameViewportInputTarget::None;
		}
		const int32 index = (int32)event.Button;

		// 뗌은 늘 게임 UI 에 보낸다(UI 가 잡은 누름은 밖에서 떼도 UI 가 받는다).
		bool bConsumedByUI = false;
		if (toGameUIButton(event.Button, uiEvent.Button) == true && ui != nullptr)
		{
			uiEvent.Type  = EGameUIPointerEventType::Up;
			bConsumedByUI = ui->HandlePointer(uiEvent, targetSize);
		}

		const EGameViewportInputTarget pressTarget = _bPressed[index] ? _pressTarget[index] : EGameViewportInputTarget::None;
		_bPressed[index]    = false;
		_pressTarget[index] = EGameViewportInputTarget::None;

		if (pressTarget != EGameViewportInputTarget::None)
		{
			return pressTarget;
		}
		return bConsumedByUI ? EGameViewportInputTarget::GameUI : EGameViewportInputTarget::None;
	}
	default:
		return EGameViewportInputTarget::None;
	}
}

EGameViewportInputTarget PGameViewport::handleKeyDown(const HInputEvent& event)
{
	PSharedPtr<PGameUIManager> ui = getUI();
	if (ui == nullptr)
	{
		return EGameViewportInputTarget::None;
	}

	// 입력란 포커스가 있으면 편집 키 · Ctrl 단축키를 그쪽으로. 이때 키는 모두 게임 몫이다(Esc 도 뒤로가기가 아니라 포커스 해제).
	if (ui->WantsTextInput() == true)
	{
		const bool bShift = hasModifier(event.Modifiers, EInputModifier::Shift);
		const bool bCtrl  = hasModifier(event.Modifiers, EInputModifier::Ctrl);
		const bool bAlt   = hasModifier(event.Modifiers, EInputModifier::Alt);

		for (const HTextEditKey& editKey : TextEditKeys)
		{
			if (editKey.Key != event.Key)
			{
				continue;
			}
			if (event.bRepeat == false || editKey.bRepeat == true)
			{
				ui->HandleKey(editKey.GameKey, bShift);
			}
			return EGameViewportInputTarget::GameUI;
		}

		// Ctrl 단축키: 전체 선택 · 복사 · 잘라내기 · 붙여넣기(OS 클립보드, UTF-8). AltGr(= Ctrl+Alt)는 글자 입력이라 뺀다.
		if (bCtrl == true && bAlt == false)
		{
			if (event.Key == EKeyCode::A && event.bRepeat == false)
			{
				ui->HandleKey(EGameUIKey::SelectAll);
			}
			else if ((event.Key == EKeyCode::C || event.Key == EKeyCode::X) && event.bRepeat == false)
			{
				const PString selection = ui->GetTextInputSelection();
				if (selection.Empty() == false && HPlatform::SetClipboardText(selection) == true && event.Key == EKeyCode::X)
				{
					ui->HandleKey(EGameUIKey::Delete);
				}
			}
			else if (event.Key == EKeyCode::V)
			{
				ui->HandleTextInput(HPlatform::GetClipboardText());
			}
		}
		return EGameViewportInputTarget::GameUI;
	}

	// Esc = 뒤로가기(CommonUI 의 Back). 맨 위 뒤로가기 처리 위젯이 받는다.
	if (event.Key == EKeyCode::Esc && event.bRepeat == false)
	{
		return ui->HandleBackAction() ? EGameViewportInputTarget::GameUI : EGameViewportInputTarget::None;
	}
	return EGameViewportInputTarget::None;
}

EGameViewportInputTarget PGameViewport::handleChar(const HInputEvent& event)
{
	PSharedPtr<PGameUIManager> ui = getUI();
	if (ui == nullptr || ui->WantsTextInput() == false)
	{
		return EGameViewportInputTarget::None;
	}

	// 확정 글자 하나(IME 확정 포함). 제어 문자 · 허용 안 되는 글자는 관리자가 버린다.
	char utf8[5] = {};
	const int32 length = HGameUIUtf8::Encode(event.Codepoint, utf8);
	if (length > 0)
	{
		ui->HandleTextInput(PString(utf8));
	}
	return EGameViewportInputTarget::GameUI;
}

EGameViewportInputTarget PGameViewport::clickWorld(const HVector2& viewportPoint)
{
	// Menu 위젯이 월드를 막으면 게임 UI 가 이미 누름을 받았다(HandlePointer 가 true) — 여기까지 오지 않는다.
	PSharedPtr<PWorld> world = getWorld();
	HRay ray;
	if (world == nullptr || ViewportPointToRay(viewportPoint, &ray) == false)
	{
		return EGameViewportInputTarget::None;
	}

	HList<PSharedPtr<JGGameplayControllerActor>> controllers;
	world->FindActors<JGGameplayControllerActor>(controllers);
	for (PSharedPtr<JGGameplayControllerActor>& controller : controllers)
	{
		controller->HandleClick(ray);
	}
	return controllers.empty() ? EGameViewportInputTarget::None : EGameViewportInputTarget::World;
}

void PGameViewport::releaseAllButtons()
{
	PSharedPtr<PGameUIManager> ui = getUI();
	const HVector2 targetSize = getTargetSize();

	// 포커스를 잃으면 게임 UI 의 끌기를 먼저 취소한다(OnDragEnd, bCanceled = true). 아래 화면 밖 뗌을 먼저 보내면 "빈 곳에 놓음"으로 끝난다.
	if (ui != nullptr)
	{
		ui->CancelDrag();
	}

	// 그다음 누름을 취소한다: 화면 밖에서 뗀 것으로 보낸다(잡은 요소는 놓이고, 밖에서 뗐으니 버튼은 클릭하지 않는다).
	const HVector2 outside(-1.0f, -1.0f);
	for (int32 i = 0; i < (int32)EPointerButton::Count; ++i)
	{
		if (_bPressed[i] == false)
		{
			continue;
		}
		_bPressed[i]    = false;
		_pressTarget[i] = EGameViewportInputTarget::None;

		HGameUIPointerEvent uiEvent;
		if (ui != nullptr && toGameUIButton((EPointerButton)i, uiEvent.Button) == true)
		{
			uiEvent.Type     = EGameUIPointerEventType::Up;
			uiEvent.Position = outside;
			ui->HandlePointer(uiEvent, targetSize);
		}
	}

	if (ui != nullptr)
	{
		HGameUIPointerEvent leave;
		leave.Type     = EGameUIPointerEventType::Leave;
		leave.Position = outside;
		ui->HandlePointer(leave, targetSize);
	}
}
