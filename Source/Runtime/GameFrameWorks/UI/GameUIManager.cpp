#include "PCH/PCH.h"
#include "UI/GameUIManager.h"
#include "UI/GameWidget.h"
#include "UI/GameUIElement.h"
#include "UI/GameUIFont.h"
#include "UI/GameUIRenderer.h"
#include "UI/GameUIFile.h"
#include "UI/GameUITextInput.h"
#include "JGGraphics.h"
#include "Classes/Texture.h"
#include <cmath>

// stb_image 구현을 이 DLL 안에만 둔다(STB_IMAGE_STATIC).
#pragma warning(push, 0)
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#pragma warning(pop)

namespace
{
	// 기본 글꼴 후보(ER-016): 게임 Content → 엔진 Content 의 이 이름, 그다음 OS 글꼴. OS 글꼴은 이 PC 에서 보기 위한 것이고 패키지에 실리지 않는다.
	const char* const GameUIDefaultFontInContent  = "Fonts/GameUIDefault.ttf";
	const char* const GameUISystemFontCandidates[] = { "C:/Windows/Fonts/malgun.ttf", "C:/Windows/Fonts/arial.ttf" };

	PSharedPtr<PJGGraphicsAPI> findGraphicsAPI()
	{
		HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>();
		return (graphicsModule != nullptr) ? graphicsModule->GetGraphicsAPI() : nullptr;
	}

	bool isValidLayer(EGameUILayer layer)
	{
		return (int32)layer >= 0 && (int32)layer < (int32)EGameUILayer::Count;
	}

	bool isValidButton(EGameUIPointerButton button)
	{
		return (int32)button >= 0 && (int32)button < (int32)EGameUIPointerButton::Count;
	}

	// element 아래(자기 포함)에 target 이 있으면 true. 그 길에서 target 에 가장 가까운 끌 수 있는 요소(target 포함)를 outSource 에 둔다.
	bool findDraggableOnPath(const PSharedPtr<PGameUIElement>& element, const PGameUIElement* target, PSharedPtr<PGameUIElement>& outSource)
	{
		if (element.IsValid() == false)
		{
			return false;
		}

		bool bOnPath = element.GetRawPointer() == target;
		if (bOnPath == false)
		{
			for (const PSharedPtr<PGameUIElement>& child : element->GetChildren())
			{
				if (findDraggableOnPath(child, target, outSource))
				{
					bOnPath = true;
					break;
				}
			}
		}

		// target 쪽부터 돌아 나오므로 처음 정한 것이 가장 가깝다.
		if (bOnPath && outSource.IsValid() == false && element->IsDraggable())
		{
			outSource = element;
		}
		return bOnPath;
	}

	// 누른 요소부터 위로 가장 가까운 끌 수 있는 요소. 꺼진 요소는 이벤트를 무시하므로 끌리지 않는다
	// (꺼진 조상 아래에서 맞으면 판정이 그 꺼진 조상이므로 누른 요소만 보면 된다).
	PSharedPtr<PGameUIElement> findDragSource(const HList<PSharedPtr<JGGameWidget>>& widgets, const PSharedPtr<PGameUIElement>& pressed)
	{
		if (pressed.IsValid() == false || pressed->IsEnabled() == false)
		{
			return nullptr;
		}
		for (const PSharedPtr<JGGameWidget>& widget : widgets)
		{
			PSharedPtr<PGameUIElement> source;
			if (findDraggableOnPath(widget->GetRoot(), pressed.GetRawPointer(), source))
			{
				return source;
			}
		}
		return nullptr;
	}

	// element 아래(자기 포함)에 target 이 있으면 true. 그 길의 요소(element · target 포함)를 바깥 → 안쪽 순서로 outPath 에 둔다.
	bool collectPath(const PSharedPtr<PGameUIElement>& element, const PGameUIElement* target, HList<PSharedPtr<PGameUIElement>>& outPath)
	{
		if (element.IsValid() == false)
		{
			return false;
		}

		outPath.push_back(element);
		if (element.GetRawPointer() == target)
		{
			return true;
		}
		for (const PSharedPtr<PGameUIElement>& child : element->GetChildren())
		{
			if (collectPath(child, target, outPath))
			{
				return true;
			}
		}
		outPath.pop_back();
		return false;
	}

	// 커서 깜빡임: 주기 1초 중 앞 0.6초 보인다. 입력 · 커서 이동 직후에는 보인다.
	constexpr float32 CaretBlinkPeriod  = 1.0f;
	constexpr float32 CaretBlinkVisible = 0.6f;

	// element 아래(자기 포함)에 target 이 보이고 켜진 채로 있나.
	bool containsUsable(const PSharedPtr<PGameUIElement>& element, const PGameUIElement* target)
	{
		if (element.IsValid() == false || element->IsVisible() == false || element->IsEnabled() == false)
		{
			return false;
		}
		if (element.GetRawPointer() == target)
		{
			return true;
		}
		for (const PSharedPtr<PGameUIElement>& child : element->GetChildren())
		{
			if (containsUsable(child, target))
			{
				return true;
			}
		}
		return false;
	}

	// 보이고 켜진 입력란을 그리는 순서대로 모은다(Tab 순서).
	void collectTextInputs(const PSharedPtr<PGameUIElement>& element, HList<PSharedPtr<PGameUITextInput>>& outInputs)
	{
		if (element.IsValid() == false || element->IsVisible() == false || element->IsEnabled() == false)
		{
			return;
		}

		// PSharedPtr 는 const 를 전파하므로 비 const 사본으로 바꾼다.
		PSharedPtr<PGameUIElement> candidate = element;
		PSharedPtr<PGameUITextInput> input = RawDynamicCast<PGameUITextInput>(candidate);
		if (input.IsValid())
		{
			outInputs.push_back(input);
		}
		for (const PSharedPtr<PGameUIElement>& child : element->GetChildren())
		{
			collectTextInputs(child, outInputs);
		}
	}
}

PGameUIManager::~PGameUIManager()
{
	// 훅은 부르지 않는다(종료 중일 수 있다). 남은 위젯 · 묶음이 사라진 관리자를 가리키지 않게만 끊는다. 정상 경로는 Shutdown 이 먼저 돈다.
	auto detach = [](PSharedPtr<JGGameWidget>& widget)
	{
		widget->_manager  = nullptr;
		widget->_bActive  = false;
		widget->_bClosing = false;
		for (PSharedPtr<PGameUITimeline>& timeline : widget->_timelines)
		{
			timeline->_manager = nullptr;
		}
	};
	for (int32 layerIndex = 0; layerIndex < (int32)EGameUILayer::Count; ++layerIndex)
	{
		for (PSharedPtr<JGGameWidget>& widget : _layers[layerIndex])
		{
			detach(widget);
		}
		for (PSharedPtr<JGGameWidget>& widget : _closing[layerIndex])
		{
			detach(widget);
		}
	}
}

PSharedPtr<JGGameWidget> PGameUIManager::PushWidgetByClass(PSharedPtr<JGClass> widgetClass, EGameUILayer layer)
{
	if (widgetClass == nullptr)
	{
		return nullptr;
	}
	PSharedPtr<JGGameWidget> widget = RawDynamicCast<JGGameWidget>(AllocateByClass(widgetClass));
	if (widget == nullptr)
	{
		JG_LOG(GameUI, ELogLevel::Error, "PushWidgetByClass: class is not a JGGameWidget");
		return nullptr;
	}
	if (PushWidgetInstance(widget, layer) == false)
	{
		return nullptr;
	}
	return widget;
}

bool PGameUIManager::PushWidgetInstance(PSharedPtr<JGGameWidget> widget, EGameUILayer layer)
{
	if (widget == nullptr)
	{
		return false;
	}
	if (isValidLayer(layer) == false)
	{
		JG_LOG(GameUI, ELogLevel::Error, "PushWidget: bad layer %d", (int32)layer);
		return false;
	}
	// 스택에 있으면 올리지 않는다. 이 관리자에서 닫힘 전환 중이면 다시 연다.
	if (widget->_manager != nullptr && (widget->_bClosing == false || widget->_manager != this))
	{
		JG_LOG(GameUI, ELogLevel::Warning, "PushWidget: %s is already in the %s stack", widget->GetType().GetName().ToString(), PString(GetLayerName(widget->_layer)));
		return false;
	}

	// 끌기 취소 처리기가 스택을 바꿀 수 있으므로 포인터 상태를 먼저 풀고 다시 본다.
	resetPointerState();
	if (widget->_manager != nullptr && (widget->_bClosing == false || widget->_manager != this))
	{
		return false;
	}
	++_stackVersion;

	// 닫힘 전환 중이었다: 목록에서 빼고 지금 모습에서 다시 연다(초기화된 그대로라 OnInitialize · OnShutdown 없음).
	const bool bReopen = widget->_bClosing;
	if (bReopen)
	{
		removeClosing(widget.GetRawPointer());
		widget->_bClosing = false;
	}

	widget->_manager = this;
	widget->_layer   = layer;
	if (widget->_bInitialized == false)
	{
		widget->_bInitialized = true;
		widget->OnInitialize();
	}

	HList<PSharedPtr<JGGameWidget>>& stack = _layers[(int32)layer];
	if (stack.empty() == false)
	{
		deactivate(stack.back());
	}
	stack.push_back(widget);
	JG_LOG(GameUI, ELogLevel::Info, "GameUI push %s on %s (stack %d)%s", widget->GetType().GetName().ToString(), PString(GetLayerName(layer)), (int32)stack.size(),
		PString(bReopen ? " - reopened while closing" : ""));
	// 열림 전환은 OnActivated 전에 건다(첫 그리기가 닫힌 모습에서 시작한다).
	widget->beginTransition(true, bReopen);
	activate(widget);
	return true;
}

bool PGameUIManager::RemoveWidget(PSharedPtr<JGGameWidget> widget)
{
	if (widget == nullptr || widget->_manager != this)
	{
		return false;
	}

	// 끌기 취소 처리기가 스택을 바꿀 수 있으므로 포인터 상태를 먼저 풀고 자리를 찾는다.
	resetPointerState();
	if (widget->_manager != this)
	{
		return false;
	}

	HList<PSharedPtr<JGGameWidget>>& stack = _layers[(int32)widget->_layer];
	auto found = stack.end();
	for (auto it = stack.begin(); it != stack.end(); ++it)
	{
		if (it->GetRawPointer() == widget.GetRawPointer())
		{
			found = it;
			break;
		}
	}
	if (found == stack.end())
	{
		return false;
	}

	const bool bWasTop = (found + 1) == stack.end();
	++_stackVersion;

	stack.erase(found);
	JG_LOG(GameUI, ELogLevel::Info, "GameUI remove %s from %s (stack %d)", widget->GetType().GetName().ToString(), PString(GetLayerName(widget->_layer)), (int32)stack.size());

	// 보이던 위젯(맨 위)이고 닫힘 전환이 있으면: 전환이 끝날 때까지 그 레이어 맨 위에 그린 뒤 내린다(입력 · 뒤로가기는 이미 받지 않는다).
	// 닫히는 중 표시는 OnDeactivated 전에 한다 — 훅 안에서 IsClosing() 이 true(닫는 모습 트윈을 걸면 닫히는 동안 돈다).
	const bool bCloseWithTransition = bWasTop && widget->_closeTransition.IsEnabled();
	if (bCloseWithTransition)
	{
		widget->_bClosing = true;
		_closing[(int32)widget->_layer].push_back(widget);
	}
	deactivate(widget);

	if (bCloseWithTransition == false)
	{
		retire(widget);
	}
	else if (widget->_bClosing && widget->_manager == this)
	{
		// 훅이 다시 열었거나(PushWidget) 비웠으면(ClearLayer) 여기서 할 일이 없다.
		widget->beginTransition(false, true);
		if (widget->_bClosing && widget->isTransitionPlaying() == false)
		{
			// 움직일 것이 없는 전환(닫힌 모습 = 열린 모습)은 바로 끝난다.
			removeClosing(widget.GetRawPointer());
			retire(widget);
		}
	}

	if (bWasTop && stack.empty() == false)
	{
		activate(stack.back());
	}
	return true;
}

void PGameUIManager::ClearLayer(EGameUILayer layer)
{
	if (isValidLayer(layer) == false)
	{
		return;
	}

	HList<PSharedPtr<JGGameWidget>>& stack   = _layers[(int32)layer];
	HList<PSharedPtr<JGGameWidget>>& closing = _closing[(int32)layer];
	if (stack.empty() && closing.empty())
	{
		return;
	}

	// 끌기 취소 처리기가 스택을 바꿀 수 있으므로 포인터 상태를 먼저 푼다.
	resetPointerState();

	// 아래 위젯이 잠깐 활성이 되지 않도록 한 번에 비우고 위에서부터 내린다. 닫힘 전환 중인 위젯(맨 위에 그려지던)은 전환 없이 먼저 내린다.
	HList<PSharedPtr<JGGameWidget>> closingWidgets = closing;
	closing.clear();
	HList<PSharedPtr<JGGameWidget>> widgets = stack;
	stack.clear();
	++_stackVersion;

	for (auto it = closingWidgets.rbegin(); it != closingWidgets.rend(); ++it)
	{
		retire(*it);
	}
	for (auto it = widgets.rbegin(); it != widgets.rend(); ++it)
	{
		deactivate(*it);
		retire(*it);
	}
}

void PGameUIManager::ClearAllWidgets()
{
	for (int32 layerIndex = (int32)EGameUILayer::Count - 1; layerIndex >= 0; --layerIndex)
	{
		ClearLayer((EGameUILayer)layerIndex);
	}
}

PSharedPtr<JGGameWidget> PGameUIManager::GetTopWidget(EGameUILayer layer) const
{
	if (isValidLayer(layer) == false || _layers[(int32)layer].empty())
	{
		return nullptr;
	}
	return _layers[(int32)layer].back();
}

const HList<PSharedPtr<JGGameWidget>>& PGameUIManager::GetLayerStack(EGameUILayer layer) const
{
	JG_CHECK(isValidLayer(layer));
	return _layers[(int32)layer];
}

uint64 PGameUIManager::GetWidgetCount() const
{
	uint64 count = 0;
	for (const HList<PSharedPtr<JGGameWidget>>& stack : _layers)
	{
		count += (uint64)stack.size();
	}
	return count;
}

void PGameUIManager::SetReferenceSize(const HVector2& referenceSize)
{
	_referenceSize = referenceSize;
}

const HVector2& PGameUIManager::GetReferenceSize() const
{
	return _referenceSize;
}

float32 PGameUIManager::GetScale() const
{
	return _scale;
}

HVector2 PGameUIManager::GetLogicalSize() const
{
	if (_scale <= 0.0f)
	{
		return _targetSize;
	}
	return _targetSize / _scale;
}

HVector2 PGameUIManager::TargetToCanvas(const HVector2& targetPoint) const
{
	if (_scale <= 0.0f)
	{
		return targetPoint;
	}
	return targetPoint / _scale;
}

void PGameUIManager::UpdateLayout(const HVector2& targetSize)
{
	_targetSize = targetSize;
	_scale = 1.0f;
	if (_referenceSize.x > 0.0f && _referenceSize.y > 0.0f && targetSize.x > 0.0f && targetSize.y > 0.0f)
	{
		_scale = HMath::Min(targetSize.x / _referenceSize.x, targetSize.y / _referenceSize.y);
	}

	// 보이는 위젯(각 스택 맨 위 · 전환 중에 함께 그리는 위젯)만 배치한다. 가려진 위젯은 맨 위가 될 때 다시 배치된다.
	// 루트는 화면 전체이고 피벗은 가운데다(전환 배율이 화면 가운데를 기준으로 한다).
	const HVector2 logicalSize = GetLogicalSize();
	HList<PSharedPtr<JGGameWidget>> visibleWidgets;
	for (int32 layerIndex = 0; layerIndex < (int32)EGameUILayer::Count; ++layerIndex)
	{
		getVisibleWidgets((EGameUILayer)layerIndex, visibleWidgets);
	}
	for (PSharedPtr<JGGameWidget>& widget : visibleWidgets)
	{
		PSharedPtr<PGameUIElement> root = widget->_root;
		root->_anchor   = HVector2(0.5f, 0.5f);
		root->_pivot    = HVector2(0.5f, 0.5f);
		root->_position = HVector2(0.0f, 0.0f);
		root->_size     = logicalSize;
		root->layout(HRect(0.0f, 0.0f, logicalSize.x, logicalSize.y), widget.GetRawPointer());
	}
}

void PGameUIManager::SetTweenTimeScale(float32 timeScale)
{
	_tweenTimeScale = HMath::Max(timeScale, 0.0f);
}

float32 PGameUIManager::GetTweenTimeScale() const
{
	return _tweenTimeScale;
}

void PGameUIManager::Update(float32 deltaSeconds)
{
	// 입력을 받지 못하게 된 입력란(위젯이 빠지거나 덮임 · 안 보임 · 꺼짐)은 포커스를 푼다.
	_caretBlinkTime += deltaSeconds;
	if (_textInputFocus.Pin().IsValid() && getUsableTextInputFocus().IsValid() == false)
	{
		ClearTextInputFocus();
	}

	// 훅 · 묶음 함수 안에서 스택이 바뀔 수 있으므로 사본으로 돈다. 그사이 빠지거나 덮인 위젯은 건너뛴다.
	HList<PSharedPtr<JGGameWidget>> activeWidgets;
	for (HList<PSharedPtr<JGGameWidget>>& stack : _layers)
	{
		if (stack.empty() == false && stack.back()->_bActive)
		{
			activeWidgets.push_back(stack.back());
		}
	}

	// 묶음(트윈 · 전환)은 시간 배율을 곱한 dt 로 먼저 진행한다(OnUpdate 가 이번 프레임 값을 본다). OnUpdate 는 그대로의 dt.
	const float32 tweenSeconds = deltaSeconds * _tweenTimeScale;
	for (PSharedPtr<JGGameWidget>& widget : activeWidgets)
	{
		if (widget->_bActive && widget->_manager == this)
		{
			widget->tickTimelines(tweenSeconds);
		}
		if (widget->_bActive && widget->_manager == this)
		{
			widget->OnUpdate(deltaSeconds);
		}
	}

	// 닫힘 전환 중인 위젯: 묶음을 진행하고, 전환이 끝났으면 내린다(OnShutdown). 그사이 다시 열린 위젯은 건너뛴다.
	HList<PSharedPtr<JGGameWidget>> closingWidgets;
	for (HList<PSharedPtr<JGGameWidget>>& closing : _closing)
	{
		for (PSharedPtr<JGGameWidget>& widget : closing)
		{
			closingWidgets.push_back(widget);
		}
	}
	for (PSharedPtr<JGGameWidget>& widget : closingWidgets)
	{
		if (widget->_bClosing == false || widget->_manager != this)
		{
			continue;
		}
		widget->tickTimelines(tweenSeconds);
		if (widget->_bClosing && widget->_manager == this && widget->isTransitionPlaying() == false)
		{
			removeClosing(widget.GetRawPointer());
			JG_LOG(GameUI, ELogLevel::Info, "GameUI closed %s on %s", widget->GetType().GetName().ToString(), PString(GetLayerName(widget->_layer)));
			retire(widget);
		}
	}
}

void PGameUIManager::Render(const PSharedPtr<IRawTexture>& target)
{
	if ((GetWidgetCount() == 0 && hasClosingWidgets() == false) || target.IsValid() == false || target->IsValid() == false)
	{
		return;
	}
	if (_renderer.IsValid() == false)
	{
		_renderer = Allocate<PGameUIRenderer>();
	}

	const HTextureInfo& targetInfo = target->GetTextureInfo();
	BuildDrawList(HVector2((float32)targetInfo.Width, (float32)targetInfo.Height), _drawList);
	_renderer->Render(_drawList, target);
}

void PGameUIManager::BuildDrawList(const HVector2& targetSize, HGameUIDrawList& outDrawList)
{
	outDrawList.Clear();
	if (targetSize.x <= 0.0f || targetSize.y <= 0.0f)
	{
		return;
	}

	// 글꼴 아틀라스 프레임: 이 목록에 넣는 글자의 페이지는 목록을 다 만들 때까지 비우지 않는다.
	PGameUIFont::BeginFrame();
	UpdateLayout(targetSize);

	HGameUIDrawContext context;
	context.DrawList = &outDrawList;
	context.Scale    = _scale;
	context.Clip     = HRect(0.0f, 0.0f, targetSize.x, targetSize.y);
	context.Manager  = this;

	// 레이어 아래부터, 각 스택의 맨 위 위젯만(스택 아래 위젯은 가려져 보이지 않는다 — 맨 위가 열림 전환 중이면 그 바로 아래도), 그 위에 닫힘 전환 중인 위젯.
	HList<PSharedPtr<JGGameWidget>> visibleWidgets;
	for (int32 layerIndex = 0; layerIndex < (int32)EGameUILayer::Count; ++layerIndex)
	{
		getVisibleWidgets((EGameUILayer)layerIndex, visibleWidgets);
	}
	for (PSharedPtr<JGGameWidget>& widget : visibleWidgets)
	{
		widget->_root->buildDraw(context);
	}
}

bool PGameUIManager::HandlePointer(const HGameUIPointerEvent& event, const HVector2& targetSize)
{
	if (targetSize.x <= 0.0f || targetSize.y <= 0.0f)
	{
		return false;
	}
	if (event.Type == EGameUIPointerEventType::Leave)
	{
		// 호버만 푼다. 끄는 중이면 끌기는 이어진다(에디터 뷰포트는 누른 채 나가도 이동을 보내고, 밖에서 떼면 Leave 다음에 뗌을 보낸다).
		_bPointerInside = false;
		setHovered(nullptr);
		return false;
	}

	// 입력은 그리기보다 먼저 올 수 있다(방금 올린 위젯, 타깃 크기 변경). 사각형을 맞춘 뒤 판정한다.
	UpdateLayout(targetSize);
	const HVector2 point = TargetToCanvas(event.Position);
	const bool bLeftButton = event.Button == EGameUIPointerButton::Left;
	_pointerPosition = point;
	_bPointerInside  = true;
	if ((event.Type == EGameUIPointerEventType::Down || event.Type == EGameUIPointerEventType::Up) && isValidButton(event.Button))
	{
		_bPointerDown[(int32)event.Button] = event.Type == EGameUIPointerEventType::Down;
	}

	// 끄는 중에 왼쪽 누름이 또 왔다(호스트가 뗌을 놓쳤다): 이전 끌기는 취소하고 새 누름으로 다룬다.
	if (event.Type == EGameUIPointerEventType::Down && bLeftButton && _dragged.IsValid())
	{
		CancelDrag();
	}

	// 전하는 중에 훅이 스택을 바꿀 수 있으므로 판정 대상은 사본이다.
	const HList<PSharedPtr<JGGameWidget>> inputWidgets = getInputWidgets();
	const bool bBlocked = inputWidgets.empty() == false && inputWidgets.back()->_inputMode == EGameWidgetInputMode::Menu;
	const uint32 stackVersion = _stackVersion;

	// 누른 끌기 후보가 누른 자리에서 문턱을 넘게 움직이면 끌기를 시작한다.
	if (event.Type == EGameUIPointerEventType::Move && _dragged.IsValid() == false)
	{
		PSharedPtr<PGameUIElement> candidate = _dragCandidate.Pin();
		if (candidate.IsValid() && HVector2::Length(point - _dragStart) > _dragThreshold)
		{
			beginDrag(candidate);
			if (_stackVersion != stackVersion)
			{
				// 시작 처리기가 화면을 바꿨다(끌기는 그때 취소됐다). 호버는 다음 이동에서 다시 정한다.
				return true;
			}
		}
	}

	// 끄는 동안은 끄는 요소와 그 자손을 빼고 판정한다(그 아래 놓을 곳).
	PSharedPtr<PGameUIElement> hit = hitTestWidgets(inputWidgets, point, _dragged.GetRawPointer());
	if (_dragged.IsValid())
	{
		if (event.Type == EGameUIPointerEventType::Move)
		{
			setHovered(hit);
			moveDrag(point, hit);
			return true;
		}
		if (event.Type == EGameUIPointerEventType::Up && bLeftButton)
		{
			endDrag(point, hit, false);
			return true;
		}
	}

	switch (event.Type)
	{
	case EGameUIPointerEventType::Move:
	{
		// 누르는 동안 이동을 받는 요소(손잡이 · 슬라이더)를 끄는 동안은 호버를 바꾸지 않는다(지나가는 요소가 밝아지지 않게).
		PSharedPtr<PGameUIElement> captured = _captured.Pin();
		if (captured.IsValid() && _bCapturedHeld)
		{
			captured->onPointerHeldMove(point);
			return true;
		}
		setHovered(hit);
		return hit.IsValid() || captured.IsValid() || bBlocked;
	}
	case EGameUIPointerEventType::Down:
	{
		setHovered(hit);
		updateTextInputFocusOnDown(hit, event.Button, point);
		if (hit.IsValid() == false)
		{
			// Menu 위젯이 떠 있으면 빈 곳 클릭도 아래 · 월드로 보내지 않는다.
			return bBlocked;
		}
		const bool bNewCapture = _captured.Pin().IsValid() == false;
		if (bNewCapture)
		{
			_captured       = hit;
			_capturedButton = event.Button;
			_bCapturedHeld  = false;
			if (bLeftButton)
			{
				// 끌기 후보: 누른 요소부터 위로 가장 가까운 끌 수 있는 요소. 문턱을 넘게 움직이면 끌기가 된다.
				_dragCandidate = findDragSource(inputWidgets, hit);
				_dragStart     = point;
			}
		}
		hit->onPointerDown(event.Button);
		// 누른 요소가 누르는 동안 이동을 직접 받겠다면(손잡이 · 슬라이더) 조상 끌기 후보는 버린다.
		if (bNewCapture && _stackVersion == stackVersion && _captured.Pin().GetRawPointer() == hit.GetRawPointer()
			&& hit->onPointerHeldBegin(event.Button, point))
		{
			_bCapturedHeld = true;
			_dragCandidate = nullptr;
		}
		return true;
	}
	case EGameUIPointerEventType::Up:
	{
		if (bLeftButton)
		{
			_dragCandidate = nullptr;
		}

		PSharedPtr<PGameUIElement> captured = _captured.Pin();
		if (captured.IsValid() && _capturedButton == event.Button)
		{
			// 누른 요소가 뗌을 받는다. 포인터가 밖으로 나갔으면 bInside = false(버튼은 클릭하지 않는다).
			_captured      = nullptr;
			_bCapturedHeld = false;
			captured->onPointerUp(event.Button, hit.GetRawPointer() == captured.GetRawPointer());
			if (_stackVersion == stackVersion)
			{
				// 클릭으로 화면이 열리거나 닫혔으면 호버는 다음 이동에서 다시 정한다.
				setHovered(hit);
			}
			return true;
		}

		if (hit.IsValid())
		{
			hit->onPointerUp(event.Button, true);
			if (_stackVersion == stackVersion)
			{
				setHovered(hit);
			}
			return true;
		}
		setHovered(nullptr);
		return bBlocked;
	}
	case EGameUIPointerEventType::Wheel:
	{
		// 휠을 켠 요소가 받는다(GG-21): 끄는 중이면 휠을 켠 끄는 요소, 그다음 포인터 아래 길의 가장 안쪽부터(스크롤 칸은 끝이면 바깥).
		// 받을 요소가 없으면 UI 위에서만 소비한다(월드 줌을 막는다).
		if (routeWheel(inputWidgets, hit, event.Wheel, event.WheelHorizontal, point))
		{
			return true;
		}
		return hit.IsValid() || bBlocked;
	}
	default:
		break;
	}
	return false;
}

bool PGameUIManager::HandleBackAction()
{
	const HList<PSharedPtr<JGGameWidget>> inputWidgets = getInputWidgets();
	for (const PSharedPtr<JGGameWidget>& candidate : inputWidgets)
	{
		// PSharedPtr 는 const 를 전파하므로 비 const 사본으로 훅을 부른다.
		PSharedPtr<JGGameWidget> widget = candidate;
		if (widget->_bBackHandler == false || widget->_bActive == false)
		{
			continue;
		}
		if (widget->OnBackAction())
		{
			JG_LOG(GameUI, ELogLevel::Info, "GameUI back handled by %s", widget->GetType().GetName().ToString());
			return true;
		}
	}
	return false;
}

bool PGameUIManager::IsWorldInputBlocked() const
{
	const HList<PSharedPtr<JGGameWidget>> inputWidgets = getInputWidgets();
	return inputWidgets.empty() == false && inputWidgets.back()->_inputMode == EGameWidgetInputMode::Menu;
}

PSharedPtr<PGameUIElement> PGameUIManager::GetHoveredElement() const
{
	return _hovered.Pin();
}

const HVector2& PGameUIManager::GetPointerPosition() const
{
	return _pointerPosition;
}

bool PGameUIManager::IsPointerDown(EGameUIPointerButton button) const
{
	return isValidButton(button) && _bPointerDown[(int32)button];
}

bool PGameUIManager::IsPointerInside() const
{
	return _bPointerInside;
}

void PGameUIManager::SetDragThreshold(float32 threshold)
{
	_dragThreshold = HMath::Max(threshold, 0.0f);
}

float32 PGameUIManager::GetDragThreshold() const
{
	return _dragThreshold;
}

PSharedPtr<PGameUIElement> PGameUIManager::GetDraggedElement() const
{
	return _dragged;
}

void PGameUIManager::CancelDrag()
{
	_dragCandidate = nullptr;
	endDrag(_pointerPosition, nullptr, true);
}

bool PGameUIManager::WantsTextInput() const
{
	return getUsableTextInputFocus().IsValid();
}

PSharedPtr<PGameUITextInput> PGameUIManager::GetFocusedTextInput() const
{
	return getUsableTextInputFocus();
}

bool PGameUIManager::SetTextInputFocus(const PSharedPtr<PGameUITextInput>& input)
{
	if (input.IsValid() == false || isTextInputUsable(input.GetRawPointer()) == false)
	{
		return false;
	}
	setTextInputFocus(input);
	return true;
}

void PGameUIManager::ClearTextInputFocus()
{
	setTextInputFocus(nullptr);
}

bool PGameUIManager::HandleTextInput(const PString& text)
{
	PSharedPtr<PGameUITextInput> input = getUsableTextInputFocus();
	if (input.IsValid() == false)
	{
		return false;
	}
	_caretBlinkTime = 0.0f;
	input->insertText(text);
	return true;
}

bool PGameUIManager::HandleKey(EGameUIKey key, bool bShift)
{
	PSharedPtr<PGameUITextInput> input = getUsableTextInputFocus();
	if (input.IsValid() == false)
	{
		return false;
	}
	_caretBlinkTime = 0.0f;

	switch (key)
	{
	case EGameUIKey::Escape:
		ClearTextInputFocus();
		break;
	case EGameUIKey::Enter:
		// 먼저 풀고 알린다(처리기가 다른 입력란에 포커스를 주거나 화면을 닫아도 된다).
		ClearTextInputFocus();
		input->commit();
		break;
	case EGameUIKey::Tab:
		focusNextTextInput(input, bShift);
		break;
	default:
		input->handleEditKey(key, bShift);
		break;
	}
	return true;
}

bool PGameUIManager::GetTextInputCaretRect(HRect& outRect)
{
	PSharedPtr<PGameUITextInput> input = getUsableTextInputFocus();
	HRect caret;
	if (input.IsValid() == false || input->getCaretRect(this, caret) == false)
	{
		return false;
	}
	// 그리기 변환이 있으면 변환한 커서 사각형을 감싸는 사각형(IME 창 자리).
	const HRect bounds = input->LocalToCanvasBounds(caret);
	outRect = HRect(bounds.left * _scale, bounds.top * _scale, bounds.right * _scale, bounds.bottom * _scale);
	return true;
}

PString PGameUIManager::GetTextInputSelection() const
{
	PSharedPtr<PGameUITextInput> input = getUsableTextInputFocus();
	if (input.IsValid() == false)
	{
		return PString();
	}
	return input->GetSelectedText();
}

PSharedPtr<PGameUIFont> PGameUIManager::LoadFont(const PString& path)
{
	const PName key(path);
	auto found = _fonts.find(key);
	if (found != _fonts.end())
	{
		return found->second;
	}

	PSharedPtr<PGameUIFont> font = Allocate<PGameUIFont>();
	if (font->LoadFromFile(path) == false)
	{
		return nullptr;
	}

	_fonts.emplace(key, font);
	JG_LOG(GameUI, ELogLevel::Info, "Font loaded : %s", path);
	return font;
}

PSharedPtr<PGameUIFont> PGameUIManager::GetDefaultFont()
{
	if (_bDefaultFontTried)
	{
		return _defaultFont;
	}
	_bDefaultFontTried = true;

	// Content 글꼴(패키지에 실린다): 게임 → 엔진. 엔진 단독 실행이면 게임 Content 가 없다(빈 경로).
	HList<PString> contentCandidates;
	for (const PString* contentDirectory : { &HFileHelper::GameContentDirectory(), &HFileHelper::EngineContentDirectory() })
	{
		if (contentDirectory->Empty() == false)
		{
			PString contentFontPath;
			HFileHelper::CombinePath(*contentDirectory, PString(GameUIDefaultFontInContent), &contentFontPath);
			contentCandidates.push_back(contentFontPath);
		}
	}
	for (const PString& candidate : contentCandidates)
	{
		if (HFileHelper::Exists(candidate) == false)
		{
			continue;
		}
		_defaultFont = LoadFont(candidate);
		if (_defaultFont.IsValid())
		{
			return _defaultFont;
		}
	}

	// OS 글꼴: 이 PC 에서 보기 위한 것. 패키지에는 없으므로 패키지 빌드면 경고한다(다른 PC 에는 없을 수 있다 — 맑은 고딕이 없으면 한글이 빈 칸).
	for (const char* systemFont : GameUISystemFontCandidates)
	{
		const PString candidate(systemFont);
		if (HFileHelper::Exists(candidate) == false)
		{
			continue;
		}
		_defaultFont = LoadFont(candidate);
		if (_defaultFont.IsValid())
		{
#if JG_WITH_EDITOR == 0
			JG_LOG(GameUI, ELogLevel::Warning, "Game UI default font is the OS font %s - put %s in the game Content or call SetDefaultFont", candidate, PString(GameUIDefaultFontInContent));
#endif
			return _defaultFont;
		}
	}

	JG_LOG(GameUI, ELogLevel::Error, "No default font. Put a TTF at Content/%s or call SetDefaultFont", PString(GameUIDefaultFontInContent));
	return nullptr;
}

void PGameUIManager::SetDefaultFont(const PSharedPtr<PGameUIFont>& font)
{
	_defaultFont       = font;
	_bDefaultFontTried = font.IsValid();
}

PSharedPtr<IRawTexture> PGameUIManager::LoadTexture(const PString& path)
{
	const PName key(path);
	auto found = _textures.find(key);
	if (found != _textures.end())
	{
		return found->second;
	}

	PSharedPtr<PJGGraphicsAPI> graphicsAPI = findGraphicsAPI();
	if (graphicsAPI.IsValid() == false)
	{
		JG_LOG(GameUI, ELogLevel::Warning, "LoadTexture needs the graphics module : %s", path);
		return nullptr;
	}

	std::vector<uint8> fileData;
	if (HGameUIFile::ReadAllBytes(path, fileData) == false)
	{
		JG_LOG(GameUI, ELogLevel::Error, "Image file is not readable : %s", path);
		return nullptr;
	}

	int32 width    = 0;
	int32 height   = 0;
	int32 channels = 0;
	stbi_uc* pixels = stbi_load_from_memory(fileData.data(), (int32)fileData.size(), &width, &height, &channels, 4);
	if (pixels == nullptr)
	{
		JG_LOG(GameUI, ELogLevel::Error, "Fail to decode image %s : %s", path, PString(stbi_failure_reason()));
		return nullptr;
	}

	PString fileName;
	HFileHelper::FileNameOnly(path, &fileName);

	HTextureInfo textureInfo;
	textureInfo.Name       = PString::Format("GameUI_%s", fileName);
	textureInfo.Width      = (uint32)width;
	textureInfo.Height     = (uint32)height;
	textureInfo.Format     = ETextureFormat::R8G8B8A8_Unorm;
	textureInfo.Flags      = ETextureFlags::None;
	textureInfo.FilterMode = ETextureFilterMode::Linear;
	textureInfo.WrapMode   = ETextureWrapMode::Clamp;
	textureInfo.MipLevel   = 1;
	textureInfo.ArraySize  = 1;

	// 픽셀은 요청 시점에 업로드 스테이징으로 복사되므로 바로 놓아도 된다.
	PSharedPtr<IRawTexture> texture = graphicsAPI->CreateRawTexture(pixels, textureInfo);
	stbi_image_free(pixels);
	if (texture.IsValid() == false || texture->IsValid() == false)
	{
		JG_LOG(GameUI, ELogLevel::Error, "Fail to create texture : %s", path);
		return nullptr;
	}

	_textures.emplace(key, texture);
	JG_LOG(GameUI, ELogLevel::Info, "Texture loaded : %s (%dx%d)", path, width, height);
	return texture;
}

void PGameUIManager::SetDefaultTextStyle(const HGameUITextStyle& style)
{
	_defaultTextStyle = style;
}

const HGameUITextStyle& PGameUIManager::GetDefaultTextStyle() const
{
	return _defaultTextStyle;
}

void PGameUIManager::SetDefaultButtonStyle(const HGameUIButtonStyle& style)
{
	_defaultButtonStyle = style;
}

const HGameUIButtonStyle& PGameUIManager::GetDefaultButtonStyle() const
{
	return _defaultButtonStyle;
}

void PGameUIManager::SetDefaultTextInputStyle(const HGameUITextInputStyle& style)
{
	_defaultTextInputStyle = style;
}

const HGameUITextInputStyle& PGameUIManager::GetDefaultTextInputStyle() const
{
	return _defaultTextInputStyle;
}

void PGameUIManager::SetDefaultScrollStyle(const HGameUIScrollStyle& style)
{
	_defaultScrollStyle = style;
}

const HGameUIScrollStyle& PGameUIManager::GetDefaultScrollStyle() const
{
	return _defaultScrollStyle;
}

void PGameUIManager::Shutdown()
{
	ClearAllWidgets();
	resetPointerState();
	ClearTextInputFocus();

	// GPU 텍스처는 여기서 놓아도 GPU 가 끝낸 뒤 해제된다(Graphics DeferRelease).
	_drawList.Clear();
	_renderer           = nullptr;
	_defaultFont        = nullptr;
	_bDefaultFontTried  = false;
	_defaultTextStyle      = HGameUITextStyle();
	_defaultButtonStyle    = HGameUIButtonStyle();
	_defaultTextInputStyle = HGameUITextInputStyle();
	_defaultScrollStyle    = HGameUIScrollStyle();
	_tweenTimeScale        = 1.0f;
	_fonts.clear();
	_textures.clear();
}

const char* PGameUIManager::GetLayerName(EGameUILayer layer)
{
	switch (layer)
	{
	case EGameUILayer::Game:
		return "Game";
	case EGameUILayer::GameMenu:
		return "GameMenu";
	case EGameUILayer::Menu:
		return "Menu";
	case EGameUILayer::Modal:
		return "Modal";
	default:
		break;
	}
	return "?";
}

HList<PSharedPtr<JGGameWidget>> PGameUIManager::getInputWidgets() const
{
	HList<PSharedPtr<JGGameWidget>> widgets;
	for (int32 layerIndex = (int32)EGameUILayer::Count - 1; layerIndex >= 0; --layerIndex)
	{
		const HList<PSharedPtr<JGGameWidget>>& stack = _layers[layerIndex];
		if (stack.empty() || stack.back()->_bActive == false)
		{
			continue;
		}
		widgets.push_back(stack.back());
		if (stack.back()->_inputMode == EGameWidgetInputMode::Menu)
		{
			break;
		}
	}
	return widgets;
}

PSharedPtr<PGameUIElement> PGameUIManager::hitTestWidgets(const HList<PSharedPtr<JGGameWidget>>& widgets, const HVector2& point, const PGameUIElement* excluded) const
{
	for (const PSharedPtr<JGGameWidget>& widget : widgets)
	{
		PSharedPtr<PGameUIElement> hit = PGameUIElement::hitTest(widget->_root, point, excluded);
		if (hit.IsValid())
		{
			return hit;
		}
	}
	return nullptr;
}

void PGameUIManager::activate(PSharedPtr<JGGameWidget> widget)
{
	if (widget->_bActive)
	{
		return;
	}
	widget->_bActive = true;
	widget->OnActivated();
}

void PGameUIManager::deactivate(PSharedPtr<JGGameWidget> widget)
{
	if (widget->_bActive == false)
	{
		return;
	}
	widget->_bActive = false;
	widget->OnDeactivated();
}

void PGameUIManager::retire(PSharedPtr<JGGameWidget> widget)
{
	// 묶음(전환 포함)은 OnShutdown 전에 취소한다(취소 알림 처리기가 아직 관리자를 쓸 수 있다). 닫힘 표시는 먼저 지운다 —
	// 처리기가 이 위젯을 다시 Push 해도 "닫히는 중 다시 열기"로 받지 않는다.
	widget->_bClosing = false;
	widget->cancelTimelines();
	widget->_manager = nullptr;
	widget->OnShutdown();
	widget->_root->ClearChildren();
	widget->_scrollMemory.clear();
	// 전환이 남긴 루트 모습을 처음으로(다시 Push 하면 처음부터).
	widget->_root->SetOpacity(1.0f);
	widget->_root->ClearRenderTransform();
	widget->_bInitialized = false;
}

void PGameUIManager::getVisibleWidgets(EGameUILayer layer, HList<PSharedPtr<JGGameWidget>>& outWidgets) const
{
	const HList<PSharedPtr<JGGameWidget>>& stack = _layers[(int32)layer];
	if (stack.empty() == false)
	{
		// 맨 위가 열림 전환 중이면 바로 아래 위젯도 그린다(새 화면이 그 위로 나타난다. 아래 위젯은 덮여 멈춘 모습).
		const PSharedPtr<JGGameWidget>& top = stack.back();
		if (stack.size() >= 2 && top->isTransitionPlaying())
		{
			outWidgets.push_back(stack[stack.size() - 2]);
		}
		outWidgets.push_back(top);
	}
	for (const PSharedPtr<JGGameWidget>& widget : _closing[(int32)layer])
	{
		outWidgets.push_back(widget);
	}
}

void PGameUIManager::removeClosing(const JGGameWidget* widget)
{
	HList<PSharedPtr<JGGameWidget>>& closing = _closing[(int32)widget->_layer];
	for (auto it = closing.begin(); it != closing.end(); ++it)
	{
		if (it->GetRawPointer() == widget)
		{
			closing.erase(it);
			return;
		}
	}
}

bool PGameUIManager::hasClosingWidgets() const
{
	for (const HList<PSharedPtr<JGGameWidget>>& closing : _closing)
	{
		if (closing.empty() == false)
		{
			return true;
		}
	}
	return false;
}

void PGameUIManager::setHovered(const PSharedPtr<PGameUIElement>& element)
{
	PSharedPtr<PGameUIElement> previous = _hovered.Pin();
	if (previous.GetRawPointer() == element.GetRawPointer())
	{
		return;
	}

	if (previous.IsValid())
	{
		previous->onPointerLeave();
	}
	_hovered = element;

	// PSharedPtr 는 const 를 전파하므로 비 const 사본으로 부른다.
	PSharedPtr<PGameUIElement> next = element;
	if (next.IsValid())
	{
		next->onPointerEnter();
	}
}

void PGameUIManager::resetPointerState()
{
	setHovered(nullptr);

	PSharedPtr<PGameUIElement> captured = _captured.Pin();
	_captured      = nullptr;
	_bCapturedHeld = false;
	if (captured.IsValid())
	{
		captured->onPointerUp(_capturedButton, false);
	}

	// 게임 처리기가 도는 일은 맨 뒤에(취소 처리기가 스택을 바꿔도 위 상태는 이미 풀려 있다).
	CancelDrag();
}

void PGameUIManager::beginDrag(PSharedPtr<PGameUIElement> element)
{
	_dragCandidate = nullptr;

	// 끌기가 되면 그 누름은 클릭이 아니다: 누른 요소에 밖에서 뗀 것으로 알린다(버튼 누름 모습도 풀린다).
	PSharedPtr<PGameUIElement> captured = _captured.Pin();
	_captured      = nullptr;
	_bCapturedHeld = false;
	if (captured.IsValid())
	{
		captured->onPointerUp(_capturedButton, false);
	}

	_dragged = element;
	element->onDragBegin(_dragStart);
	OnDragBegin.BroadCast(element, _dragStart);
}

bool PGameUIManager::routeWheel(const HList<PSharedPtr<JGGameWidget>>& widgets, const PSharedPtr<PGameUIElement>& hit, float32 wheel, float32 wheelHorizontal, const HVector2& position)
{
	if (wheel == 0.0f && wheelHorizontal == 0.0f)
	{
		return false;
	}

	// 끄는 중: 끄는 동안의 판정은 끄는 요소와 그 자손을 뺀다(놓을 곳 찾기) — 휠을 켠 끄는 요소에 먼저 준다(지도를 끌면서 확대).
	// 안 켰으면 아래로 이어 간다(카드를 끌면서 그 아래 목록 넘기기).
	PSharedPtr<PGameUIElement> dragged = _dragged;
	if (dragged.IsValid() && dragged->_bWheelInput && dragged->_bEnabled && dragged->onWheel(wheel, wheelHorizontal, position))
	{
		return true;
	}
	if (hit.IsValid() == false)
	{
		return false;
	}

	for (const PSharedPtr<JGGameWidget>& widget : widgets)
	{
		// 길은 강참조로 든다(처리기가 트리를 다시 짜도 도는 동안 요소가 남는다).
		HList<PSharedPtr<PGameUIElement>> path;
		if (collectPath(widget->GetRoot(), hit.GetRawPointer(), path) == false)
		{
			continue;
		}
		// 가장 안쪽부터. 받지 않은 요소(그 방향 끝에 닿은 스크롤 칸 · false 를 돌려준 파생 요소)는 바깥에 넘긴다. 꺼진 요소는 받지 않는다.
		for (auto it = path.rbegin(); it != path.rend(); ++it)
		{
			PSharedPtr<PGameUIElement> element = *it;
			if (element->_bWheelInput && element->_bEnabled && element->onWheel(wheel, wheelHorizontal, position))
			{
				return true;
			}
		}
		return false;
	}
	return false;
}

void PGameUIManager::moveDrag(const HVector2& position, const PSharedPtr<PGameUIElement>& target)
{
	PSharedPtr<PGameUIElement> dragged = _dragged;
	dragged->onDragMove(position, target);
	OnDragMove.BroadCast(dragged, position, target);
}

void PGameUIManager::endDrag(const HVector2& position, const PSharedPtr<PGameUIElement>& dropTarget, bool bCanceled)
{
	// 먼저 비우고 알린다(처리기가 다시 누르거나 스택을 바꿔도 된다).
	PSharedPtr<PGameUIElement> dragged = _dragged;
	_dragged = nullptr;
	if (dragged.IsValid() == false)
	{
		return;
	}
	dragged->onDragEnd(position, dropTarget, bCanceled);
	OnDragEnd.BroadCast(dragged, position, dropTarget, bCanceled);
}

PSharedPtr<PGameUITextInput> PGameUIManager::getUsableTextInputFocus() const
{
	PSharedPtr<PGameUITextInput> input = _textInputFocus.Pin();
	if (input.IsValid() == false || isTextInputUsable(input.GetRawPointer()) == false)
	{
		return nullptr;
	}
	return input;
}

bool PGameUIManager::isTextInputUsable(const PGameUITextInput* input) const
{
	const HList<PSharedPtr<JGGameWidget>> inputWidgets = getInputWidgets();
	for (const PSharedPtr<JGGameWidget>& widget : inputWidgets)
	{
		if (containsUsable(widget->GetRoot(), input))
		{
			return true;
		}
	}
	return false;
}

void PGameUIManager::setTextInputFocus(const PSharedPtr<PGameUITextInput>& input)
{
	PSharedPtr<PGameUITextInput> previous = _textInputFocus.Pin();
	if (previous.GetRawPointer() == input.GetRawPointer())
	{
		return;
	}

	_textInputFocus = input;
	_caretBlinkTime = 0.0f;
	if (previous.IsValid())
	{
		previous->setFocused(false);
	}

	// PSharedPtr 는 const 를 전파하므로 비 const 사본으로 부른다.
	PSharedPtr<PGameUITextInput> next = input;
	if (next.IsValid())
	{
		next->setFocused(true);
	}
}

void PGameUIManager::updateTextInputFocusOnDown(const PSharedPtr<PGameUIElement>& hit, EGameUIPointerButton button, const HVector2& point)
{
	PSharedPtr<PGameUIElement> element = hit;
	PSharedPtr<PGameUITextInput> input = RawDynamicCast<PGameUITextInput>(element);
	if (input.IsValid() && input->IsEnabled())
	{
		// 입력란: 왼쪽 버튼이면 포커스 + 누른 자리에 커서. 다른 버튼은 포커스를 그대로 둔다.
		if (button == EGameUIPointerButton::Left)
		{
			setTextInputFocus(input);
			// 그리기 변환이 있으면 누른 자리를 변환 전 자리로 되돌려 커서를 놓는다(ER-014).
			input->placeCaret(input->CanvasToLocal(point).x, this);
			_caretBlinkTime = 0.0f;
		}
		return;
	}
	ClearTextInputFocus();
}

void PGameUIManager::focusNextTextInput(const PSharedPtr<PGameUITextInput>& current, bool bBackward)
{
	const HList<PSharedPtr<JGGameWidget>> inputWidgets = getInputWidgets();
	for (const PSharedPtr<JGGameWidget>& widget : inputWidgets)
	{
		HList<PSharedPtr<PGameUITextInput>> inputs;
		collectTextInputs(widget->GetRoot(), inputs);
		for (size_t i = 0; i < inputs.size(); ++i)
		{
			if (inputs[i].GetRawPointer() != current.GetRawPointer())
			{
				continue;
			}
			if (inputs.size() > 1)
			{
				const size_t count = inputs.size();
				PSharedPtr<PGameUITextInput> next = inputs[bBackward ? (i + count - 1) % count : (i + 1) % count];
				setTextInputFocus(next);
				next->selectAll();
			}
			return;
		}
	}
}

bool PGameUIManager::isCaretBlinkOn() const
{
	return fmodf(_caretBlinkTime, CaretBlinkPeriod) < CaretBlinkVisible;
}
