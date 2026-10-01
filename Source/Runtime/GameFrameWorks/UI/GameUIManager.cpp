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
	// 개발용 기본 글꼴 후보. 시스템 글꼴은 이 PC 에서 보기 위한 것이고 게임에 넣어 배포하지 않는다.
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
	// 훅은 부르지 않는다(종료 중일 수 있다). 남은 위젯이 사라진 관리자를 가리키지 않게만 끊는다. 정상 경로는 Shutdown 이 먼저 돈다.
	for (HList<PSharedPtr<JGGameWidget>>& stack : _layers)
	{
		for (PSharedPtr<JGGameWidget>& widget : stack)
		{
			widget->_manager = nullptr;
			widget->_bActive = false;
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
	if (widget->_manager != nullptr)
	{
		JG_LOG(GameUI, ELogLevel::Warning, "PushWidget: %s is already in the %s stack", widget->GetType().GetName().ToString(), PString(GetLayerName(widget->_layer)));
		return false;
	}

	resetPointerState();
	++_stackVersion;

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
	JG_LOG(GameUI, ELogLevel::Info, "GameUI push %s on %s (stack %d)", widget->GetType().GetName().ToString(), PString(GetLayerName(layer)), (int32)stack.size());
	activate(widget);
	return true;
}

bool PGameUIManager::RemoveWidget(PSharedPtr<JGGameWidget> widget)
{
	if (widget == nullptr || widget->_manager != this)
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
	resetPointerState();
	++_stackVersion;

	stack.erase(found);
	JG_LOG(GameUI, ELogLevel::Info, "GameUI remove %s from %s (stack %d)", widget->GetType().GetName().ToString(), PString(GetLayerName(widget->_layer)), (int32)stack.size());
	deactivate(widget);
	retire(widget);

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

	HList<PSharedPtr<JGGameWidget>>& stack = _layers[(int32)layer];
	if (stack.empty())
	{
		return;
	}

	// 아래 위젯이 잠깐 활성이 되지 않도록 한 번에 비우고 위에서부터 내린다.
	HList<PSharedPtr<JGGameWidget>> widgets = stack;
	stack.clear();
	resetPointerState();
	++_stackVersion;

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

	// 보이는 위젯(각 스택 맨 위)만 배치한다. 가려진 위젯은 맨 위가 될 때 다시 배치된다.
	const HVector2 logicalSize = GetLogicalSize();
	for (HList<PSharedPtr<JGGameWidget>>& stack : _layers)
	{
		if (stack.empty())
		{
			continue;
		}
		PSharedPtr<PGameUIElement> root = stack.back()->_root;
		root->_anchor   = HVector2(0.0f, 0.0f);
		root->_pivot    = HVector2(0.0f, 0.0f);
		root->_position = HVector2(0.0f, 0.0f);
		root->_size     = logicalSize;
		root->layout(HRect(0.0f, 0.0f, logicalSize.x, logicalSize.y));
	}
}

void PGameUIManager::Update(float32 deltaSeconds)
{
	// 입력을 받지 못하게 된 입력란(위젯이 빠지거나 덮임 · 안 보임 · 꺼짐)은 포커스를 푼다.
	_caretBlinkTime += deltaSeconds;
	if (_textInputFocus.Pin().IsValid() && getUsableTextInputFocus().IsValid() == false)
	{
		ClearTextInputFocus();
	}

	// 훅 안에서 스택이 바뀔 수 있으므로 사본으로 돈다. 그사이 빠지거나 덮인 위젯은 건너뛴다.
	HList<PSharedPtr<JGGameWidget>> activeWidgets;
	for (HList<PSharedPtr<JGGameWidget>>& stack : _layers)
	{
		if (stack.empty() == false && stack.back()->_bActive)
		{
			activeWidgets.push_back(stack.back());
		}
	}

	for (PSharedPtr<JGGameWidget>& widget : activeWidgets)
	{
		if (widget->_bActive && widget->_manager == this)
		{
			widget->OnUpdate(deltaSeconds);
		}
	}
}

void PGameUIManager::Render(const PSharedPtr<IRawTexture>& target)
{
	if (GetWidgetCount() == 0 || target.IsValid() == false || target->IsValid() == false)
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

	UpdateLayout(targetSize);

	HGameUIDrawContext context;
	context.DrawList = &outDrawList;
	context.Scale    = _scale;
	context.Clip     = HRect(0.0f, 0.0f, targetSize.x, targetSize.y);
	context.Manager  = this;

	// 레이어 아래부터, 각 스택의 맨 위 위젯만(스택 아래 위젯은 가려져 보이지 않는다).
	for (HList<PSharedPtr<JGGameWidget>>& stack : _layers)
	{
		if (stack.empty() == false)
		{
			stack.back()->_root->buildDraw(context);
		}
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
		setHovered(nullptr);
		return false;
	}

	// 입력은 그리기보다 먼저 올 수 있다(방금 올린 위젯, 타깃 크기 변경). 사각형을 맞춘 뒤 판정한다.
	UpdateLayout(targetSize);
	const HVector2 point = TargetToCanvas(event.Position);

	// 전하는 중에 훅이 스택을 바꿀 수 있으므로 판정 대상은 사본이다.
	const HList<PSharedPtr<JGGameWidget>> inputWidgets = getInputWidgets();
	const bool bBlocked = inputWidgets.empty() == false && inputWidgets.back()->_inputMode == EGameWidgetInputMode::Menu;
	PSharedPtr<PGameUIElement> hit = hitTestWidgets(inputWidgets, point);
	const uint32 stackVersion = _stackVersion;

	switch (event.Type)
	{
	case EGameUIPointerEventType::Move:
	{
		setHovered(hit);
		return hit.IsValid() || _captured.Pin().IsValid() || bBlocked;
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
		if (_captured.Pin().IsValid() == false)
		{
			_captured       = hit;
			_capturedButton = event.Button;
		}
		hit->onPointerDown(event.Button);
		return true;
	}
	case EGameUIPointerEventType::Up:
	{
		PSharedPtr<PGameUIElement> captured = _captured.Pin();
		if (captured.IsValid() && _capturedButton == event.Button)
		{
			// 누른 요소가 뗌을 받는다. 포인터가 밖으로 나갔으면 bInside = false(버튼은 클릭하지 않는다).
			_captured = nullptr;
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
		// 휠은 UI 위에서만 소비한다(월드 줌을 막는다). 스크롤 요소는 아직 없다.
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
	outRect = HRect(caret.left * _scale, caret.top * _scale, caret.right * _scale, caret.bottom * _scale);
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

	HList<PString> candidates;
	{
		PString contentFontPath;
		HFileHelper::CombinePath(HFileHelper::EngineContentDirectory(), PString(GameUIDefaultFontInContent), &contentFontPath);
		candidates.push_back(contentFontPath);
	}
	for (const char* systemFont : GameUISystemFontCandidates)
	{
		candidates.push_back(PString(systemFont));
	}

	for (const PString& candidate : candidates)
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

	JG_LOG(GameUI, ELogLevel::Error, "No default font. Put a TTF at Content/%s or call LoadFont", PString(GameUIDefaultFontInContent));
	return nullptr;
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

PSharedPtr<PGameUIElement> PGameUIManager::hitTestWidgets(const HList<PSharedPtr<JGGameWidget>>& widgets, const HVector2& point) const
{
	for (const PSharedPtr<JGGameWidget>& widget : widgets)
	{
		PSharedPtr<PGameUIElement> hit = PGameUIElement::hitTest(widget->_root, point);
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
	widget->_manager = nullptr;
	widget->OnShutdown();
	widget->_root->ClearChildren();
	widget->_bInitialized = false;
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
	_captured = nullptr;
	if (captured.IsValid())
	{
		captured->onPointerUp(_capturedButton, false);
	}
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
			input->placeCaret(point.x, this);
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
