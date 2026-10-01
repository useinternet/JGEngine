#include "PCH/PCH.h"
#include "UI/GameUI.h"
#include "UI/GameUIUtf8.h"
#include "UI/GameUIDrawList.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

// gameui.selftest — GPU 없이 도는 게임 UI 자체 검사. JGConsole(헤드리스)과 에디터 콘솔 양쪽에서 실행한다.
// 검사마다 자기 PGameUIManager 를 만든다(게임 인스턴스의 UI · 떠 있는 게임 화면은 건드리지 않는다).
// 결과는 JG_LOG(GameUI)로 남긴다: 실패마다 Error 한 줄, 끝에 "gameui.selftest: OK (통과/전체)".
namespace
{
	int32 GCheckCount   = 0;
	int32 GFailureCount = 0;
	int32 GSkipCount    = 0;

	const HVector2 GTarget(1000.0f, 1000.0f);

	void check(bool bCondition, const char* caseName)
	{
		++GCheckCount;
		if (bCondition == false)
		{
			++GFailureCount;
			JG_LOG(GameUI, ELogLevel::Error, "gameui.selftest FAILED: %s", PString(caseName));
		}
	}

	bool nearlyEqual(float32 a, float32 b, float32 tolerance = 0.01f)
	{
		return HMath::Abs(a - b) <= tolerance;
	}

	bool isRect(const HRect& rect, float32 left, float32 top, float32 right, float32 bottom)
	{
		return nearlyEqual(rect.left, left) && nearlyEqual(rect.top, top) && nearlyEqual(rect.right, right) && nearlyEqual(rect.bottom, bottom);
	}

	bool isColor(const HLinearColor& a, const HLinearColor& b)
	{
		return nearlyEqual(a.R, b.R, 0.001f) && nearlyEqual(a.G, b.G, 0.001f) && nearlyEqual(a.B, b.B, 0.001f) && nearlyEqual(a.A, b.A, 0.001f);
	}

	HGameUIPointerEvent pointer(EGameUIPointerEventType type, float32 x, float32 y, EGameUIPointerButton button = EGameUIPointerButton::Left)
	{
		HGameUIPointerEvent event;
		event.Type     = type;
		event.Position = HVector2(x, y);
		event.Button   = button;
		if (type == EGameUIPointerEventType::Wheel)
		{
			event.Wheel = 1.0f;
		}
		return event;
	}

	// 왼쪽 누르고 떼기. 누름을 UI 가 받았는지 돌려준다.
	bool click(PSharedPtr<PGameUIManager>& manager, float32 x, float32 y, const HVector2& target = GTarget)
	{
		const bool bConsumed = manager->HandlePointer(pointer(EGameUIPointerEventType::Down, x, y), target);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, x, y), target);
		return bConsumed;
	}

	HList<uint32> decodeAll(const char* text, uint64 length)
	{
		HList<uint32> codepoints;
		const char* cursor = text;
		const char* end    = text + length;
		uint32 codepoint = 0;
		while (HGameUIUtf8::Next(cursor, end, codepoint))
		{
			codepoints.push_back(codepoint);
		}
		return codepoints;
	}

	PString joinEvents(const HList<PString>& events)
	{
		PString joined;
		for (const PString& event : events)
		{
			joined = joined.Empty() ? event : PString::Format("%s %s", joined, event);
		}
		return joined;
	}

	// 훅 순서를 기록하는 검사용 위젯(리플렉션 없는 파생 — GFW 의 다른 자체 검사와 같은 방식).
	class PUITestWidget : public JGGameWidget
	{
	public:
		HList<PString>* Events      = nullptr;   // 공유 기록. 훅마다 "태그:훅"
		PString         Tag;
		int32           Updates     = 0;
		bool            bCustomBack = false;     // true 면 기본(닫기) 대신 bBackResult 만 돌려준다
		bool            bBackResult = false;

	protected:
		virtual void OnInitialize() override
		{
			record("init");
		}
		virtual void OnShutdown() override
		{
			record("shutdown");
		}
		virtual void OnActivated() override
		{
			record("activate");
		}
		virtual void OnDeactivated() override
		{
			record("deactivate");
		}
		virtual void OnUpdate(float32 deltaSeconds) override
		{
			++Updates;
		}
		virtual bool OnBackAction() override
		{
			record("back");
			if (bCustomBack)
			{
				return bBackResult;
			}
			return JGGameWidget::OnBackAction();
		}

	private:
		void record(const char* hook)
		{
			if (Events != nullptr)
			{
				Events->push_back(PString::Format("%s:%s", Tag, PString(hook)));
			}
		}
	};

	// OnInitialize 에서 버튼 하나를 만드는 검사용 위젯.
	class PUITestButtonWidget : public PUITestWidget
	{
	public:
		HRect                     ButtonRect;            // 논리 단위
		int32                     Clicks        = 0;
		bool                      bCloseOnClick = false; // 클릭 처리기 안에서 자기를 닫는다(입력 전달 중 스택 변경)
		PSharedPtr<PGameUIButton> Button;

	protected:
		virtual void OnInitialize() override
		{
			PUITestWidget::OnInitialize();
			Button = AddChild<PGameUIButton>(PName("Button"));
			Button->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(ButtonRect.left, ButtonRect.top),
				HVector2(ButtonRect.right - ButtonRect.left, ButtonRect.bottom - ButtonRect.top));
			Button->OnClicked.AddLambda([this]()
				{
					++Clicks;
					if (bCloseOnClick)
					{
						DeactivateWidget();
					}
				});
		}
	};

	PSharedPtr<PUITestWidget> makeTestWidget(HList<PString>& events, const char* tag)
	{
		PSharedPtr<PUITestWidget> widget = Allocate<PUITestWidget>();
		widget->Events = &events;
		widget->Tag    = PString(tag);
		return widget;
	}

	PSharedPtr<PUITestButtonWidget> makeButtonWidget(const HRect& buttonRect)
	{
		PSharedPtr<PUITestButtonWidget> widget = Allocate<PUITestButtonWidget>();
		widget->ButtonRect = buttonRect;
		return widget;
	}

	PSharedPtr<PGameUIManager> makeManager()
	{
		PSharedPtr<PGameUIManager> manager = Allocate<PGameUIManager>();
		manager->SetReferenceSize(GTarget);
		return manager;
	}

	void testUtf8()
	{
		// A 한 € 😀
		const char mixed[] = "A\xED\x95\x9C\xE2\x82\xAC\xF0\x9F\x98\x80";
		HList<uint32> decoded = decodeAll(mixed, sizeof(mixed) - 1);
		check(decoded.size() == 4 && decoded[0] == 0x41 && decoded[1] == 0xD55C && decoded[2] == 0x20AC && decoded[3] == 0x1F600, "utf8: 1/2/3/4-byte sequences");

		const char badLead[] = "\xFF" "B";
		decoded = decodeAll(badLead, sizeof(badLead) - 1);
		check(decoded.size() == 2 && decoded[0] == HGameUIUtf8::ReplacementCharacter && decoded[1] == 'B', "utf8: invalid lead byte -> U+FFFD, next char kept");

		const char truncated[] = "\xED\x95";
		decoded = decodeAll(truncated, sizeof(truncated) - 1);
		check(decoded.size() == 2 && decoded[0] == HGameUIUtf8::ReplacementCharacter && decoded[1] == HGameUIUtf8::ReplacementCharacter, "utf8: truncated sequence");

		const char overlong[] = "\xC0\xAF";
		decoded = decodeAll(overlong, sizeof(overlong) - 1);
		check(decoded.empty() == false && decoded[0] == HGameUIUtf8::ReplacementCharacter, "utf8: overlong encoding rejected");

		const char surrogate[] = "\xED\xA0\x80";
		decoded = decodeAll(surrogate, sizeof(surrogate) - 1);
		check(decoded.empty() == false && decoded[0] == HGameUIUtf8::ReplacementCharacter, "utf8: surrogate rejected");
	}

	void testLayout()
	{
		PSharedPtr<PGameUIManager> manager = Allocate<PGameUIManager>();
		manager->SetReferenceSize(HVector2(1920.0f, 1080.0f));

		PSharedPtr<JGGameWidget> widget = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		check(widget.IsValid() && widget->IsActive() && widget->IsInStack() && widget->GetLayer() == EGameUILayer::Game, "stack: PushWidget<T> creates an active widget");

		PSharedPtr<PGameUIButton> corner = widget->AddChild<PGameUIButton>(PName("Corner"));
		corner->SetLayout(HVector2(1.0f, 1.0f), HVector2(1.0f, 1.0f), HVector2(-20.0f, -20.0f), HVector2(200.0f, 80.0f));

		PSharedPtr<PGameUIImage> centered = widget->AddChild<PGameUIImage>(PName("Centered"));
		centered->SetLayout(HVector2(0.5f, 0.5f), HVector2(0.5f, 0.5f), HVector2(0.0f, 0.0f), HVector2(100.0f, 50.0f));

		PSharedPtr<PGameUIImage> panel = widget->AddChild<PGameUIImage>(PName("Panel"));
		panel->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 100.0f), HVector2(400.0f, 300.0f));
		PSharedPtr<PGameUIImage> nested = panel->AddChild<PGameUIImage>(PName("Nested"));
		nested->SetLayout(HVector2(1.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(-10.0f, 10.0f), HVector2(50.0f, 50.0f));

		// 16:9 타깃 1280x720: 배율 2/3, 논리 크기 = 기준 크기
		manager->UpdateLayout(HVector2(1280.0f, 720.0f));
		check(nearlyEqual(manager->GetScale(), 1280.0f / 1920.0f, 0.0001f), "layout: fit scale on 16:9 target");
		check(nearlyEqual(manager->GetLogicalSize().x, 1920.0f) && nearlyEqual(manager->GetLogicalSize().y, 1080.0f), "layout: logical size equals reference on same aspect");
		check(isRect(corner->GetLayoutRect(), 1700.0f, 980.0f, 1900.0f, 1060.0f), "layout: bottom-right anchor + pivot + offset");
		check(isRect(centered->GetLayoutRect(), 910.0f, 515.0f, 1010.0f, 565.0f), "layout: center anchor + center pivot");
		check(isRect(nested->GetLayoutRect(), 440.0f, 110.0f, 490.0f, 160.0f), "layout: nested element uses parent rect");

		// 더 높은 타깃 1280x1024: 폭이 제한, 논리 높이가 늘어나 아래쪽 앵커가 따라 내려간다
		manager->UpdateLayout(HVector2(1280.0f, 1024.0f));
		check(nearlyEqual(manager->GetScale(), 1280.0f / 1920.0f, 0.0001f), "layout: fit scale limited by width");
		check(nearlyEqual(manager->GetLogicalSize().y, 1536.0f), "layout: logical height grows on taller target");
		check(isRect(corner->GetLayoutRect(), 1700.0f, 1436.0f, 1900.0f, 1516.0f), "layout: bottom anchor follows logical height");

		// 좌표 변환
		const HVector2 logical = manager->TargetToCanvas(HVector2(640.0f, 360.0f));
		check(nearlyEqual(logical.x, 960.0f) && nearlyEqual(logical.y, 540.0f), "layout: target -> canvas point");

		check(widget->FindElement(PName("Nested")).GetRawPointer() == nested.GetRawPointer(), "tree: FindElement searches descendants");
		check(widget->GetRoot()->FindChild(PName("Nested"), false).IsValid() == false, "tree: FindChild non-recursive");
		check(panel->RemoveChild(nested) && panel->GetChildren().empty(), "tree: RemoveChild");

		manager->Shutdown();
	}

	void testPointerAndButton()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);

		PSharedPtr<PGameUIButton> buttonA = widget->AddChild<PGameUIButton>(PName("A"));
		buttonA->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 100.0f), HVector2(200.0f, 100.0f));
		PSharedPtr<PGameUIButton> buttonB = widget->AddChild<PGameUIButton>(PName("B"));   // 나중에 추가 → 위
		buttonB->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(150.0f, 120.0f), HVector2(200.0f, 100.0f));
		PSharedPtr<PGameUIImage> decoration = widget->AddChild<PGameUIImage>(PName("Decoration"));
		decoration->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(700.0f, 700.0f), HVector2(100.0f, 100.0f));

		int32 clicksA = 0;
		int32 clicksB = 0;
		buttonA->OnClicked.AddLambda([&clicksA]()
			{
				++clicksA;
			});
		buttonB->OnClicked.AddLambda([&clicksB]()
			{
				++clicksB;
			});

		// 겹친 곳은 위(B)가 받는다
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 160.0f, 130.0f), GTarget), "pointer: move over UI is consumed");
		check(buttonB->IsHovered() && buttonA->IsHovered() == false, "pointer: topmost element is hovered");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 160.0f, 130.0f), GTarget), "pointer: down on button is consumed");
		check(buttonB->IsPressed(), "button: pressed after down");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 160.0f, 130.0f), GTarget), "pointer: up on captured button is consumed");
		check(clicksB == 1 && clicksA == 0 && buttonB->IsPressed() == false, "button: down + up inside = one click on the top button");

		// 누른 채 밖으로 나가서 떼면 클릭이 아니다(캡처된 버튼이 뗌을 받는다)
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 110.0f, 110.0f), GTarget);
		check(buttonA->IsPressed(), "button: A pressed");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 500.0f, 500.0f), GTarget);
		check(buttonA->IsHovered() == false, "button: hover leaves when pointer moves out");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 500.0f, 500.0f), GTarget), "pointer: up outside is still consumed by the captured button");
		check(clicksA == 0 && buttonA->IsPressed() == false, "button: release outside is not a click");

		// 빈 곳 · 입력을 받지 않는 이미지는 월드로 넘긴다
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 800.0f, 50.0f), GTarget) == false, "pointer: down on empty area is not consumed");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 750.0f, 750.0f), GTarget) == false, "pointer: image without hit test passes through");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 750.0f, 750.0f), GTarget);
		decoration->SetHitTestVisible(true);
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 750.0f, 750.0f), GTarget), "pointer: image with hit test blocks the world");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 750.0f, 750.0f), GTarget);

		// 휠 · 오른쪽 버튼 · 숨김 · 벗어남
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Wheel, 160.0f, 130.0f), GTarget), "pointer: wheel over UI is consumed");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Wheel, 900.0f, 50.0f), GTarget) == false, "pointer: wheel over empty area is not consumed");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 160.0f, 130.0f, EGameUIPointerButton::Right), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 160.0f, 130.0f, EGameUIPointerButton::Right), GTarget);
		check(clicksB == 1, "button: right button does not click");

		manager->HandlePointer(pointer(EGameUIPointerEventType::Leave, 0.0f, 0.0f), GTarget);
		check(buttonB->IsHovered() == false, "pointer: leave clears hover");

		buttonB->SetVisible(false);
		click(manager, 160.0f, 130.0f);
		check(clicksA == 1 && clicksB == 1, "pointer: hidden element is skipped, the one below receives");

		// 배율이 있는 타깃: 타깃 픽셀 → 논리 좌표로 판정
		click(manager, 55.0f, 55.0f, HVector2(500.0f, 500.0f));
		check(clicksA == 2, "pointer: scaled target maps to canvas units");

		manager->Shutdown();
	}

	void testDrawList()
	{
		HGameUIDrawList list;
		const HRect clip(0.0f, 0.0f, 100.0f, 100.0f);
		const HRect uv(0.0f, 0.0f, 1.0f, 1.0f);
		const HLinearColor white(1.0f, 1.0f, 1.0f, 1.0f);
		PSharedPtr<PGameUIFont> fontKey = Allocate<PGameUIFont>();   // 배치 구분용(로드하지 않음)

		list.AddQuad(HRect(0.0f, 0.0f, 10.0f, 10.0f), uv, white, nullptr, nullptr, clip);
		list.AddQuad(HRect(10.0f, 0.0f, 20.0f, 10.0f), uv, white, nullptr, nullptr, clip);
		check(list.GetQuadCount() == 2 && list.Commands.size() == 1 && list.Commands[0].IndexCount == 12, "drawlist: same texture + clip merge into one batch");
		check(list.Vertices.size() == 8 && list.Indices.size() == 12 && list.Indices[6] == 4, "drawlist: 4 vertices / 6 indices per quad");

		list.AddQuad(HRect(20.0f, 0.0f, 30.0f, 10.0f), uv, white, nullptr, fontKey.GetRawPointer(), clip);
		check(list.Commands.size() == 2 && list.Commands[1].Font == fontKey.GetRawPointer() && list.Commands[1].IndexOffset == 12, "drawlist: font atlas starts a new batch");

		list.AddQuad(HRect(30.0f, 0.0f, 40.0f, 10.0f), uv, white, nullptr, fontKey.GetRawPointer(), HRect(0.0f, 0.0f, 50.0f, 50.0f));
		check(list.Commands.size() == 3, "drawlist: different clip starts a new batch");

		const uint64 quadsBefore = list.GetQuadCount();
		list.AddQuad(HRect(200.0f, 200.0f, 210.0f, 210.0f), uv, white, nullptr, nullptr, clip);
		list.AddQuad(HRect(5.0f, 5.0f, 5.0f, 9.0f), uv, white, nullptr, nullptr, clip);
		check(list.GetQuadCount() == quadsBefore, "drawlist: fully clipped and empty quads are skipped");

		list.Clear();
		check(list.GetQuadCount() == 0 && list.Commands.empty(), "drawlist: clear");

		// 관리자 → 그리기 목록: 보이는 요소만, 배율 적용
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIImage> image = widget->AddChild<PGameUIImage>(PName("Image"));
		image->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 100.0f), HVector2(200.0f, 200.0f));
		PSharedPtr<PGameUIImage> hidden = widget->AddChild<PGameUIImage>(PName("Hidden"));
		hidden->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(50.0f, 50.0f));
		hidden->SetVisible(false);
		PSharedPtr<PGameUIButton> button = widget->AddChild<PGameUIButton>(PName("Button"));
		button->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(400.0f, 100.0f), HVector2(100.0f, 40.0f));

		manager->BuildDrawList(HVector2(500.0f, 500.0f), list);
		check(list.GetQuadCount() == 2, "draw: hidden element is not drawn");
		check(list.Vertices.empty() == false && nearlyEqual(list.Vertices[0].Position.x, 50.0f) && nearlyEqual(list.Vertices[2].Position.y, 150.0f), "draw: draw list is in target pixels (scale 0.5)");

		// 같은 스택에서 가려진 위젯은 그리지 않는다
		PSharedPtr<JGGameWidget> cover = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		manager->BuildDrawList(HVector2(500.0f, 500.0f), list);
		check(list.GetQuadCount() == 0, "draw: widget under the top of its stack is not drawn");
		cover->DeactivateWidget();

		// 다른 레이어는 함께 그린다(아래 레이어 먼저)
		PSharedPtr<JGGameWidget> menu = manager->PushWidget<JGGameWidget>(EGameUILayer::Menu);
		PSharedPtr<PGameUIImage> menuImage = menu->AddChild<PGameUIImage>(PName("MenuImage"));
		menuImage->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(600.0f, 600.0f), HVector2(100.0f, 100.0f));
		manager->BuildDrawList(HVector2(1000.0f, 1000.0f), list);
		check(list.GetQuadCount() == 3 && nearlyEqual(list.Vertices[8].Position.x, 600.0f), "draw: layers draw bottom to top");

		manager->Shutdown();
		check(widget->IsInStack() == false && widget->GetRoot()->GetChildren().empty() && menu->IsInStack() == false, "stack: Shutdown retires widgets and clears their trees");
	}

	void testFont()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<PGameUIFont> font = manager->GetDefaultFont();
		if (font.IsValid() == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no default font on this machine. Font checks skipped");
			manager->Shutdown();
			return;
		}
		JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: font %s", font->GetPath());

		const PGameUIFont::HGlyph& glyphA = font->GetGlyph('A', 32);
		const uint64 glyphsAfterFirst = font->GetGlyphCount();
		check(glyphA.bInAtlas && glyphA.Width > 0 && glyphA.Height > 0 && glyphA.Advance > 0.0f, "font: 'A' rasterized into the atlas");
		check(glyphA.OffsetY < 0, "font: glyph sits above the baseline");
		const HRect firstUV = glyphA.UV;
		const PGameUIFont::HGlyph& glyphAAgain = font->GetGlyph('A', 32);
		check(isRect(glyphAAgain.UV, firstUV.left, firstUV.top, firstUV.right, firstUV.bottom) && font->GetGlyphCount() == glyphsAfterFirst, "font: glyph cache reuses the same atlas slot");

		const PGameUIFont::HGlyph& space = font->GetGlyph(' ', 32);
		check(space.bInAtlas == false && space.Advance > 0.0f, "font: space has advance but no bitmap");

		const PGameUIFont::HGlyph& smallA = font->GetGlyph('A', 16);
		check(smallA.Height > 0 && smallA.Height < glyphA.Height, "font: smaller pixel size gives a smaller glyph");

		if (font->HasGlyph(0xD55C))
		{
			const PGameUIFont::HGlyph& hangul = font->GetGlyph(0xD55C, 32);
			check(hangul.bInAtlas && hangul.Width > 0 && hangul.Height > 0, "font: Hangul U+D55C rasterized");
		}
		else
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: default font has no Hangul glyphs. Hangul check skipped");
		}

		const float32 ascent     = font->GetAscent(32);
		const float32 lineHeight = font->GetLineHeight(32);
		check(ascent > 0.0f && lineHeight > ascent, "font: line metrics");

		const HVector2 sizeAB = font->MeasureText(PString("AB"), 32);
		check(sizeAB.x > glyphA.Advance && nearlyEqual(sizeAB.y, lineHeight, 0.5f), "font: measure one line");
		const HVector2 sizeTwoLines = font->MeasureText(PString("A\nBB"), 32);
		const float32  longestLine  = HMath::Max(font->MeasureText(PString("A"), 32).x, font->MeasureText(PString("BB"), 32).x);
		check(nearlyEqual(sizeTwoLines.y, lineHeight * 2.0f, 0.5f), "font: measure two lines (height = 2 lines)");
		check(nearlyEqual(sizeTwoLines.x, longestLine, 0.01f), "font: measure two lines (width = longest line)");

		bool bHasCoverage = false;
		const std::vector<uint8>& atlas = font->GetAtlasPixels();
		for (size_t i = 3; i < atlas.size(); i += 4)
		{
			if (atlas[i] != 0)
			{
				bHasCoverage = true;
				break;
			}
		}
		check(bHasCoverage, "font: atlas has glyph coverage");

		// 글자 요소 → 글리프 사각형(아틀라스 배치). 화면 배율만큼 작은 픽셀 크기로 그린다.
		PSharedPtr<JGGameWidget> widget = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIText> text = widget->AddChild<PGameUIText>(PName("Text"));
		text->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(400.0f, 100.0f));
		text->SetText(PString("Hi \xED\x95\x9C"));   // "Hi 한"
		text->SetFontSize(32.0f);

		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		const uint64 expectedQuads = font->HasGlyph(0xD55C) ? 3 : 2;
		check(list.GetQuadCount() >= expectedQuads && list.Commands.empty() == false && list.Commands[0].Font == font.GetRawPointer(), "text: glyph quads use the default font atlas");

		manager->BuildDrawList(HVector2(500.0f, 500.0f), list);
		check(list.Vertices.size() >= 4 && (list.Vertices[2].Position.y - list.Vertices[0].Position.y) <= (float32)font->GetGlyph('H', 16).Height + 0.5f, "text: rasterized at screen scale (32 x 0.5 = 16px)");

		manager->Shutdown();
	}

	// 배치한 줄 하나의 글(바이트 범위)
	PString lineText(const PString& text, const HGameUITextLine& line)
	{
		PString result;
		text.SubString(&result, line.Begin, line.End - line.Begin);
		return result;
	}

	PString joinLines(const PString& text, const HGameUITextLayout& layout)
	{
		PString joined;
		for (const HGameUITextLine& line : layout.Lines)
		{
			joined += lineText(text, line);
		}
		return joined;
	}

	float32 measureWidth(const PGameUIFont& font, const PString& text, float32 fontSize)
	{
		HGameUITextLayout layout;
		font.LayoutText(text, fontSize, 0.0f, EGameUITextWrap::None, 0, false, layout);
		return layout.Size.x;
	}

	bool linesFit(const HGameUITextLayout& layout, float32 width)
	{
		for (const HGameUITextLine& line : layout.Lines)
		{
			if (line.Width > width + 0.02f)
			{
				return false;
			}
		}
		return true;
	}

	void testTextWrap()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<PGameUIFont> font = manager->GetDefaultFont();
		if (font.IsValid() == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no default font on this machine. Text wrap checks skipped");
			manager->Shutdown();
			return;
		}

		const float32 size = 20.0f;
		HGameUITextLayout layout;

		// 래스터화 없는 수치 = 같은 정수 크기의 글리프 수치
		check(nearlyEqual(font->MeasureAdvance('A', 32.0f), font->GetGlyph('A', 32).Advance, 0.001f) && nearlyEqual(font->MeasureLineHeight(32.0f), font->GetLineHeight(32), 0.001f), "wrap: logical metrics match the pixel metrics at the same size");

		// 없음 · 빈 글 · '\n'
		const PString words("word word word word word");
		const float32 twoWords = measureWidth(*font, PString("word word"), size);
		font->LayoutText(words, size, twoWords, EGameUITextWrap::None, 0, false, layout);
		check(layout.Lines.size() == 1 && layout.Size.x > twoWords, "wrap: None keeps one line wider than the width");
		font->LayoutText(PString(""), size, 100.0f, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.empty() && nearlyEqual(layout.Size.x, 0.0f) && nearlyEqual(layout.Size.y, 0.0f), "wrap: empty text has no lines");
		font->LayoutText(PString("ab\ncd"), size, 1000.0f, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.size() == 2 && lineText(PString("ab\ncd"), layout.Lines[1]) == PString("cd"), "wrap: '\\n' breaks a line in Word mode");
		font->LayoutText(PString("ab\n"), size, 1000.0f, EGameUITextWrap::Character, 0, false, layout);
		check(layout.Lines.size() == 2 && layout.Lines[1].Begin == layout.Lines[1].End, "wrap: a trailing '\\n' leaves an empty last line");

		// 단어: 맞는 만큼 넣고 공백에서 바꾼다. 딱 맞는 폭은 바꾸지 않는다.
		font->LayoutText(words, size, twoWords, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.size() == 3 && lineText(words, layout.Lines[0]) == PString("word word") && lineText(words, layout.Lines[1]) == PString("word word") && lineText(words, layout.Lines[2]) == PString("word"), "wrap: Word fills lines up to the width (an exact fit stays on the line)");
		check(linesFit(layout, twoWords), "wrap: Word lines are not wider than the width");
		check(nearlyEqual(layout.Size.y, layout.LineHeight * 3.0f) && nearlyEqual(layout.LineHeight, font->MeasureLineHeight(size)), "wrap: height = lines x line height");

		const PString spaced("word    word");
		font->LayoutText(spaced, size, measureWidth(*font, PString("word"), size) + 1.0f, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.size() == 2 && lineText(spaced, layout.Lines[0]) == PString("word") && lineText(spaced, layout.Lines[1]) == PString("word"), "wrap: spaces at a break are dropped (no trailing or leading spaces)");
		check(nearlyEqual(layout.Lines[0].Width, measureWidth(*font, PString("word"), size)), "wrap: a wrapped line width does not count trailing spaces");

		// 줄보다 긴 단어는 그 안에서 바꾼다
		const PString longWord("abcdefghijklmnopqrstuvwxyz");
		const float32 tenLetters = measureWidth(*font, PString("abcdefghij"), size);
		font->LayoutText(longWord, size, tenLetters, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.size() >= 3 && linesFit(layout, tenLetters) && joinLines(longWord, layout) == longWord, "wrap: a word longer than the line breaks inside the word");

		// 한글은 어절(공백) 단위, 한자 · 가나는 글자 사이에서 바꾼다(단어 방식)
		const PString hangulWords("\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4 \xEB\x9D\xBC\xEB\xA7\x88\xEB\xB0\x94 \xEC\x82\xAC\xEC\x95\x84\xEC\x9E\x90");   // "가나다 라마바 사아자"
		font->LayoutText(hangulWords, size, measureWidth(*font, PString("\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4 \xEB\x9D\xBC\xEB\xA7\x88\xEB\xB0\x94"), size) + 0.5f, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.size() == 2 && lineText(hangulWords, layout.Lines[1]) == PString("\xEC\x82\xAC\xEC\x95\x84\xEC\x9E\x90"), "wrap: Hangul wraps at word spaces in Word mode");

		const PString mixedIdeograph("aaaa\xE6\xBC\xA2" "bbbb");   // "aaaa漢bbbb"
		const float32 mixedWidth = measureWidth(*font, PString("aaaa\xE6\xBC\xA2" "b"), size) + 0.5f;
		font->LayoutText(mixedIdeograph, size, mixedWidth, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.size() == 2 && lineText(mixedIdeograph, layout.Lines[0]) == PString("aaaa\xE6\xBC\xA2"), "wrap: Word mode may break next to an ideograph");
		const PString mixedHangul("aaaa\xED\x95\x9C" "bbbb");   // "aaaa한bbbb"
		font->LayoutText(mixedHangul, size, measureWidth(*font, PString("aaaa\xED\x95\x9C" "b"), size) + 0.5f, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.size() == 2 && lineText(mixedHangul, layout.Lines[0]) == PString("aaaa\xED\x95\x9C" "b"), "wrap: Word mode keeps Hangul inside a word (breaks only when the word overflows)");

		// 글자: 아무 글자 사이에서나
		const PString hangulRun("\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4\xEB\x9D\xBC\xEB\xA7\x88\xEB\xB0\x94\xEC\x82\xAC\xEC\x95\x84\xEC\x9E\x90\xEC\xB0\xA8");   // "가나다라마바사아자차"
		const PString threeSyllables("\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4");   // "가나다"
		const float32 threeWidth = measureWidth(*font, threeSyllables, size) + 0.5f;
		font->LayoutText(hangulRun, size, threeWidth, EGameUITextWrap::Character, 0, false, layout);
		check(layout.Lines.size() == 4 && lineText(hangulRun, layout.Lines[0]) == threeSyllables && joinLines(hangulRun, layout) == hangulRun && linesFit(layout, threeWidth), "wrap: Character mode breaks between any glyphs");

		// 닫는 부호는 줄 첫머리에 오지 않는다(앞 글자와 함께 넘어간다). 여는 부호는 줄 끝에 남지 않는다.
		const PString closing("\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4.");   // "가나다."
		font->LayoutText(closing, size, threeWidth, EGameUITextWrap::Character, 0, false, layout);
		check(layout.Lines.size() == 2 && lineText(closing, layout.Lines[1]) == PString("\xEB\x8B\xA4."), "wrap: a closing mark does not start a line (Character)");
		font->LayoutText(closing, size, threeWidth, EGameUITextWrap::Word, 0, false, layout);
		check(layout.Lines.size() == 2 && lineText(closing, layout.Lines[1]) == PString("\xEB\x8B\xA4."), "wrap: a closing mark does not start a line (Word, inside a long word)");
		const PString opening("\xEA\xB0\x80\xEB\x82\x98(\xEB\x8B\xA4");   // "가나(다"
		font->LayoutText(opening, size, measureWidth(*font, PString("\xEA\xB0\x80\xEB\x82\x98("), size) + 0.5f, EGameUITextWrap::Character, 0, false, layout);
		check(layout.Lines.size() == 2 && lineText(opening, layout.Lines[1]) == PString("(\xEB\x8B\xA4"), "wrap: an opening mark does not end a line");

		// 최대 줄 수 · 말줄임
		const uint32 expectedEllipsis = font->HasGlyph(0x2026) ? 0x2026u : (uint32)'.';
		font->LayoutText(words, size, twoWords, EGameUITextWrap::Word, 2, false, layout);
		check(layout.Lines.size() == 2 && layout.bTruncated && layout.Lines[1].bEllipsis == false && nearlyEqual(layout.Size.y, layout.LineHeight * 2.0f), "wrap: MaxLines cuts the extra lines");
		font->LayoutText(words, size, twoWords, EGameUITextWrap::Word, 2, true, layout);
		check(layout.Lines.size() == 2 && layout.Lines[1].bEllipsis && linesFit(layout, twoWords) && layout.EllipsisCodepoint == expectedEllipsis, "wrap: the last kept line ends with an ellipsis inside the width");
		check(layout.Lines[1].End < layout.Lines[1].Begin + 9 && lineText(words, layout.Lines[0]) == PString("word word"), "wrap: the ellipsis shortens only the last kept line");
		font->LayoutText(words, size, twoWords, EGameUITextWrap::Word, 3, true, layout);
		check(layout.Lines.size() == 3 && layout.bTruncated == false && layout.Lines[2].bEllipsis == false, "wrap: no ellipsis when every line fits in MaxLines");

		font->LayoutText(longWord, size, tenLetters, EGameUITextWrap::None, 0, true, layout);
		check(layout.Lines.size() == 1 && layout.Lines[0].bEllipsis && linesFit(layout, tenLetters) && layout.Lines[0].End < longWord.Size(), "wrap: None + ellipsis shortens a line wider than the width");
		font->LayoutText(PString("abc"), size, tenLetters, EGameUITextWrap::None, 0, true, layout);
		check(layout.Lines.size() == 1 && layout.Lines[0].bEllipsis == false && lineText(PString("abc"), layout.Lines[0]) == PString("abc"), "wrap: None + ellipsis leaves a fitting line alone");

		// 글자 요소: 크기 측정 · 그리기 · 잘라내기. 기준 해상도 1920x1080 → 아래 타깃의 배율은 1 · 0.667 · 0.5
		manager->SetReferenceSize(HVector2(1920.0f, 1080.0f));
		PSharedPtr<JGGameWidget> widget = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIText> text = widget->AddChild<PGameUIText>(PName("Paragraph"));
		const PString paragraph("The quick brown fox jumps over the lazy dog. \xEB\x8B\xA4\xEB\x9E\x8C\xEC\xA5\x90 \xED\x97\x8C \xEC\xB3\x87\xEB\xB0\x94\xED\x80\xB4\xEC\x97\x90 \xED\x83\x80\xEA\xB3\xA0\xED\x8C\x8C. Pack my box with five dozen liquor jugs.");
		text->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 50.0f), HVector2(300.0f, 400.0f));
		text->SetText(paragraph);
		text->SetFontSize(size);
		check(text->GetWrap() == EGameUITextWrap::None && text->GetMaxLines() == 0 && text->IsEllipsis() == false && text->IsClip() == false, "text: wrap is off by default (None, no max lines, no ellipsis, no clip)");
		text->SetWrap(EGameUITextWrap::Word);

		font->LayoutText(paragraph, size, 300.0f, EGameUITextWrap::Word, 0, false, layout);
		const HVector2 measured = text->MeasureContent(300.0f, manager.GetRawPointer());
		check(layout.Lines.size() >= 3 && nearlyEqual(measured.y, layout.LineHeight * (float32)layout.Lines.size()) && measured.x <= 300.02f, "text: MeasureContent = wrapped box (height = lines x line height)");

		// 배율이 달라도 같은 글리프 수, 모두 요소 폭 안(배치 × 배율로 그린다)
		uint64 glyphQuads = 0;
		const float32 targets[] = { 1920.0f, 1280.0f, 960.0f };
		for (float32 targetWidth : targets)
		{
			HGameUIDrawList list;
			const HVector2 target(targetWidth, targetWidth * 1080.0f / 1920.0f);
			manager->BuildDrawList(target, list);
			const float32 scale = manager->GetScale();
			float32 right = 0.0f;
			for (const H2DVertex& vertex : list.Vertices)
			{
				right = HMath::Max(right, vertex.Position.x);
			}
			if (glyphQuads == 0)
			{
				glyphQuads = list.GetQuadCount();
			}
			check(list.GetQuadCount() == glyphQuads && list.GetQuadCount() > 0, "text: wrapped text draws the same glyphs at every scale");
			check(right <= (100.0f + 300.0f) * scale + 1.5f, "text: wrapped glyphs stay inside the element width at every scale");
		}

		// 잘라내기: 그 글자의 클립 = 요소 사각형(픽셀) ∩ 부모 클립
		HGameUIDrawList list;
		manager->BuildDrawList(HVector2(1920.0f, 1080.0f), list);
		check(list.Commands.empty() == false && isRect(list.Commands[0].Clip, 0.0f, 0.0f, 1920.0f, 1080.0f), "text: without clip the text uses the parent clip");
		text->SetClip(true);
		manager->BuildDrawList(HVector2(1920.0f, 1080.0f), list);
		check(list.Commands.empty() == false && isRect(list.Commands[0].Clip, 100.0f, 50.0f, 400.0f, 450.0f), "text: clip = the element rect");
		manager->BuildDrawList(HVector2(960.0f, 540.0f), list);
		check(list.Commands.empty() == false && isRect(list.Commands[0].Clip, 50.0f, 25.0f, 200.0f, 225.0f), "text: clip follows the screen scale");
		text->SetPosition(HVector2(1800.0f, 50.0f));
		manager->BuildDrawList(HVector2(1920.0f, 1080.0f), list);
		check(list.Commands.empty() == false && isRect(list.Commands[0].Clip, 1800.0f, 50.0f, 1920.0f, 450.0f), "text: clip is cut by the parent clip");

		// 최대 줄 수 + 말줄임 → 측정 높이
		text->SetPosition(HVector2(100.0f, 50.0f));
		text->SetMaxLines(2);
		text->SetEllipsis(true);
		check(nearlyEqual(text->MeasureContent(300.0f, manager.GetRawPointer()).y, layout.LineHeight * 2.0f), "text: MeasureContent follows MaxLines");
		text->SetMaxLines(-3);
		check(text->GetMaxLines() == 0, "text: a negative MaxLines means no limit");

		manager->Shutdown();
	}

	bool hasVertexColor(const HGameUIDrawList& list, const HLinearColor& color)
	{
		for (const H2DVertex& vertex : list.Vertices)
		{
			if (isColor(vertex.Color, color))
			{
				return true;
			}
		}
		return false;
	}

	void testTextInput()
	{
		const PString hangul("\xED\x95\x9C\xEA\xB8\x80");   // "한글"

		// UTF-8 쓰기(입력란 글 · 호스트 글자 전달)
		HList<uint32> roundTrip;
		roundTrip.push_back('A');
		roundTrip.push_back(0xD55C);
		roundTrip.push_back(0x1F600);
		check(HGameUIUtf8::Encode(roundTrip) == PString("A\xED\x95\x9C\xF0\x9F\x98\x80"), "input: UTF-8 encode (1 / 3 / 4 bytes)");

		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUITextInput> name = widget->AddChild<PGameUITextInput>(PName("Name"));
		name->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 100.0f), HVector2(400.0f, 60.0f));
		PSharedPtr<PGameUITextInput> address = widget->AddChild<PGameUITextInput>(PName("Address"));
		address->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 200.0f), HVector2(400.0f, 60.0f));
		address->SetAllowedCharacters(PString("0123456789.:"));
		address->SetMaxLength(21);

		HList<PString> changes;
		int32   commits = 0;
		PString committed;
		name->OnTextChanged.AddLambda([&changes](const PString& value)
			{
				changes.push_back(value);
			});
		name->OnCommitted.AddLambda([&commits, &committed](const PString& value)
			{
				++commits;
				committed = value;
			});

		// 포커스
		check(manager->WantsTextInput() == false && manager->HandleTextInput(PString("a")) == false && manager->HandleKey(EGameUIKey::Backspace) == false, "input: without focus the keyboard is not taken");
		check(click(manager, 120.0f, 130.0f) && manager->WantsTextInput() && name->IsFocused() && manager->GetFocusedTextInput().GetRawPointer() == name.GetRawPointer(), "input: clicking an input takes the text focus");

		// 글자 · 한글 · 제어 문자
		check(manager->HandleTextInput(PString("Hi")) && name->GetText() == PString("Hi") && name->GetCaretPosition() == 2 && changes.size() == 1 && changes[0] == PString("Hi"), "input: typed text goes in at the caret (one change event)");
		manager->HandleTextInput(hangul);
		check(name->GetText() == PString("Hi\xED\x95\x9C\xEA\xB8\x80") && name->GetLength() == 4 && name->GetCaretPosition() == 4, "input: Hangul counts one per character");
		check(manager->HandleTextInput(PString("\n\t\x01\x7F")) && name->GetLength() == 4 && changes.size() == 2, "input: control characters are dropped (no change event)");

		// 편집 키
		manager->HandleKey(EGameUIKey::Backspace);
		check(name->GetText() == PString("Hi\xED\x95\x9C") && name->GetCaretPosition() == 3 && changes.size() == 3, "input: Backspace deletes before the caret");
		manager->HandleKey(EGameUIKey::Left);
		manager->HandleKey(EGameUIKey::Left);
		manager->HandleKey(EGameUIKey::Delete);
		check(name->GetText() == PString("H\xED\x95\x9C") && name->GetCaretPosition() == 1, "input: Left moves the caret, Delete deletes after it");
		manager->HandleKey(EGameUIKey::Home);
		const int32 homeCaret = name->GetCaretPosition();
		manager->HandleKey(EGameUIKey::End);
		check(homeCaret == 0 && name->GetCaretPosition() == 2, "input: Home / End");
		manager->HandleKey(EGameUIKey::Left);
		manager->HandleKey(EGameUIKey::Left);
		manager->HandleKey(EGameUIKey::Left);
		check(name->GetCaretPosition() == 0, "input: the caret stops at the start");

		// 선택
		manager->HandleKey(EGameUIKey::Right, true);
		check(manager->GetTextInputSelection() == PString("H"), "input: Shift + Right selects");
		manager->HandleTextInput(PString("J"));
		check(name->GetText() == PString("J\xED\x95\x9C") && name->GetCaretPosition() == 1 && manager->GetTextInputSelection().Empty(), "input: typing replaces the selection");
		manager->HandleKey(EGameUIKey::SelectAll);
		check(manager->GetTextInputSelection() == PString("J\xED\x95\x9C"), "input: select all");
		manager->HandleKey(EGameUIKey::Backspace);
		check(name->GetText().Empty() && name->GetLength() == 0 && changes.back().Empty(), "input: Backspace deletes the selection");

		const uint64 changesBeforeSet = changes.size();
		name->SetText(PString("abc"));
		check(name->GetText() == PString("abc") && name->GetCaretPosition() == 3 && changes.size() == changesBeforeSet, "input: SetText puts the caret at the end without a change event");
		manager->HandleKey(EGameUIKey::SelectAll);
		manager->HandleKey(EGameUIKey::Left);
		int32 selectionStart = 0;
		int32 selectionEnd   = 0;
		name->GetSelection(selectionStart, selectionEnd);
		check(name->GetCaretPosition() == 0 && selectionStart == selectionEnd, "input: Left without Shift collapses a selection to its start");
		manager->HandleKey(EGameUIKey::End, true);
		check(manager->GetTextInputSelection() == PString("abc"), "input: Shift + End selects to the end");
		manager->HandleKey(EGameUIKey::Home, true);
		name->GetSelection(selectionStart, selectionEnd);
		check(selectionStart == 0 && selectionEnd == 0 && name->GetCaretPosition() == 0, "input: Shift + Home goes back to the anchor (empty selection)");

		// 허용 문자 · 최대 길이 (주소 칸)
		click(manager, 120.0f, 230.0f);
		check(address->IsFocused() && name->IsFocused() == false, "input: clicking another input moves the focus");
		manager->HandleTextInput(PString("192.168.0.10:47771x"));
		check(address->GetText() == PString("192.168.0.10:47771"), "input: characters outside the allowed set are dropped");
		manager->HandleTextInput(PString("12345"));
		check(address->GetLength() == 21 && address->GetText() == PString("192.168.0.10:47771123"), "input: MaxLength keeps what fits");
		manager->HandleKey(EGameUIKey::SelectAll);
		manager->HandleTextInput(PString("9"));
		check(address->GetText() == PString("9"), "input: a selection can be replaced even when the input is full");
		address->SetMaxLength(0);
		address->SetText(PString("12.34abc"));
		check(address->GetText() == PString("12.34"), "input: SetText applies the allowed characters");

		// Tab
		manager->HandleKey(EGameUIKey::Tab);
		check(name->IsFocused() && manager->GetTextInputSelection() == PString("abc"), "input: Tab moves to the next input and selects its text");
		manager->HandleKey(EGameUIKey::Tab, true);
		check(address->IsFocused(), "input: Shift + Tab moves back");

		// Enter · Esc
		click(manager, 120.0f, 130.0f);
		name->SetText(PString("Player"));
		check(manager->HandleKey(EGameUIKey::Enter) && commits == 1 && committed == PString("Player") && manager->WantsTextInput() == false && name->IsFocused() == false, "input: Enter commits and releases the focus");
		widget->SetBackHandler(true);
		click(manager, 120.0f, 130.0f);
		check(manager->HandleKey(EGameUIKey::Escape) && manager->WantsTextInput() == false && widget->IsInStack() && commits == 1, "input: Esc only releases the focus (no commit, the widget stays)");
		check(manager->HandleKey(EGameUIKey::Escape) == false, "input: the next Esc is not taken by the input (the host uses it for back)");
		widget->SetBackHandler(false);

		// 다른 곳 누름 · 꺼짐 · 가려짐 · 숨김
		click(manager, 120.0f, 130.0f);
		click(manager, 800.0f, 800.0f);
		check(manager->WantsTextInput() == false && name->IsFocused() == false, "input: clicking elsewhere releases the focus");
		name->SetEnabled(false);
		click(manager, 120.0f, 130.0f);
		check(manager->WantsTextInput() == false, "input: a disabled input does not take the focus");
		name->SetEnabled(true);

		click(manager, 120.0f, 130.0f);
		PSharedPtr<JGGameWidget> overlay = manager->PushWidget<JGGameWidget>(EGameUILayer::GameMenu);
		check(manager->WantsTextInput(), "input: a Game-mode widget on top keeps the focus");
		overlay->DeactivateWidget();
		PSharedPtr<JGGameWidget> menu = manager->PushWidget<JGGameWidget>(EGameUILayer::Menu);
		menu->SetInputMode(EGameWidgetInputMode::Menu);
		check(manager->WantsTextInput() == false && manager->HandleTextInput(PString("x")) == false, "input: an input under a Menu-mode widget gets no keyboard");
		manager->Update(0.016f);
		check(name->IsFocused() == false, "input: Update releases the focus of an input that cannot get input");
		menu->DeactivateWidget();

		click(manager, 120.0f, 130.0f);
		name->SetVisible(false);
		check(manager->WantsTextInput() == false, "input: a hidden input gets no keyboard");
		name->SetVisible(true);
		check(manager->SetTextInputFocus(address) && address->IsFocused(), "input: SetTextInputFocus focuses a usable input");
		name->SetVisible(false);
		check(manager->SetTextInputFocus(name) == false && address->IsFocused(), "input: SetTextInputFocus refuses a hidden input");
		name->SetVisible(true);

		// 커서 자리 · 누른 자리 · 가로 밀기 · 그리기 (글꼴 필요)
		PSharedPtr<PGameUIFont> font = manager->GetDefaultFont();
		if (font.IsValid() == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no default font on this machine. Text input caret checks skipped");
			manager->Shutdown();
			return;
		}

		const HGameUITextInputStyle defaultStyle;
		const float32 fontSize    = defaultStyle.Text.FontSize;
		const float32 contentLeft = 100.0f + defaultStyle.Padding;
		click(manager, 120.0f, 130.0f);
		name->SetText(PString("abcd"));
		HRect caret;
		check(manager->GetTextInputCaretRect(caret) && nearlyEqual((caret.left + caret.right) * 0.5f, contentLeft + measureWidth(*font, PString("abcd"), fontSize), 0.05f) && caret.top >= 100.0f && caret.bottom <= 160.0f, "input: caret rect follows the text (render target pixels)");

		const float32 twoLetters = measureWidth(*font, PString("ab"), fontSize);
		click(manager, contentLeft + twoLetters + 0.3f, 130.0f);
		check(name->GetCaretPosition() == 2, "input: clicking places the caret at the nearest character boundary");

		name->SetText(PString("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"));
		check(manager->GetTextInputCaretRect(caret) && caret.right <= 500.0f + 0.5f && caret.left >= contentLeft - 2.0f, "input: a long text scrolls so the caret stays inside the input");
		manager->HandleKey(EGameUIKey::Home);
		check(manager->GetTextInputCaretRect(caret) && nearlyEqual((caret.left + caret.right) * 0.5f, contentLeft, 0.05f), "input: Home scrolls back to the start");

		HGameUITextInputStyle style;
		style.CaretColor       = HLinearColor(1.0f, 0.0f, 1.0f, 1.0f);
		style.SelectionColor   = HLinearColor(0.0f, 1.0f, 0.0f, 0.5f);
		style.PlaceholderColor = HLinearColor(0.0f, 0.5f, 1.0f, 1.0f);
		name->SetStyle(style);
		name->SetText(PString("ab"));
		manager->HandleKey(EGameUIKey::SelectAll);
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		check(hasVertexColor(list, style.SelectionColor) && hasVertexColor(list, style.CaretColor), "input: draws the selection and the caret");
		bool bContentClip = false;
		for (const HGameUIDrawCommand& command : list.Commands)
		{
			bContentClip = bContentClip || (command.Font == font.GetRawPointer() && isRect(command.Clip, contentLeft, 100.0f, 500.0f - defaultStyle.Padding, 160.0f));
		}
		check(bContentClip, "input: text is clipped to the inside of the padding");
		manager->Update(0.7f);   // 깜빡임 주기 1초 중 0.6초 뒤는 숨는다
		manager->BuildDrawList(GTarget, list);
		check(hasVertexColor(list, style.CaretColor) == false, "input: the caret blinks");

		name->SetText(PString(""));
		name->SetPlaceholder(PString("Name"));
		manager->BuildDrawList(GTarget, list);
		check(hasVertexColor(list, style.PlaceholderColor), "input: an empty input draws the placeholder");

		manager->Shutdown();
	}

	void testStackLifecycle()
	{
		HList<PString> events;
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<PUITestWidget>  a       = makeTestWidget(events, "A");
		PSharedPtr<PUITestWidget>  b       = makeTestWidget(events, "B");

		check(manager->PushWidgetInstance(a, EGameUILayer::Game) && joinEvents(events) == PString("A:init A:activate"), "stack: first push = OnInitialize + OnActivated");
		events.clear();
		manager->PushWidgetInstance(b, EGameUILayer::Game);
		check(joinEvents(events) == PString("B:init A:deactivate B:activate"), "stack: push on the same layer deactivates the previous top");
		check(a->IsActive() == false && b->IsActive() && manager->GetTopWidget(EGameUILayer::Game).GetRawPointer() == b.GetRawPointer(), "stack: only the top of a stack is active");
		check(manager->PushWidgetInstance(b, EGameUILayer::Menu) == false, "stack: a widget is in one stack at a time");

		manager->Update(0.016f);
		check(b->Updates == 1 && a->Updates == 0, "stack: OnUpdate only for active widgets");

		events.clear();
		b->DeactivateWidget();
		check(joinEvents(events) == PString("B:deactivate B:shutdown A:activate"), "stack: DeactivateWidget pops and reactivates the widget below");
		check(b->IsInStack() == false && b->IsActive() == false && a->IsActive(), "stack: removed widget leaves the stack");

		PSharedPtr<PUITestWidget> m = makeTestWidget(events, "M");
		manager->PushWidgetInstance(m, EGameUILayer::Menu);
		check(a->IsActive() && m->IsActive(), "stack: tops of different layers are both active");
		manager->Update(0.016f);
		check(a->Updates == 1 && m->Updates == 1, "stack: every active top gets OnUpdate");

		events.clear();
		manager->PushWidgetInstance(b, EGameUILayer::Game);
		check(joinEvents(events) == PString("B:init A:deactivate B:activate"), "stack: a re-pushed widget starts again from OnInitialize");

		events.clear();
		manager->RemoveWidget(a);
		check(joinEvents(events) == PString("A:shutdown") && b->IsActive(), "stack: removing a covered widget only shuts it down");

		check(manager->FindWidget<PUITestWidget>().GetRawPointer() == m.GetRawPointer(), "stack: FindWidget searches from the top layer");
		check(manager->PushWidgetByClass(nullptr, EGameUILayer::Game) == nullptr, "stack: PushWidgetByClass(null) fails");
		PSharedPtr<JGGameWidget> byClass = manager->PushWidgetByClass(StaticClass<JGGameWidget>(), EGameUILayer::Modal);
		check(byClass.IsValid() && manager->GetTopWidget(EGameUILayer::Modal).GetRawPointer() == byClass.GetRawPointer(), "stack: PushWidgetByClass creates a widget from its reflection class");
		check(manager->GetWidgetCount() == 3, "stack: widget count");

		events.clear();
		manager->ClearAllWidgets();
		check(manager->GetWidgetCount() == 0 && b->IsInStack() == false && m->IsInStack() == false && byClass->IsInStack() == false, "stack: ClearAllWidgets");
		check(joinEvents(events) == PString("M:deactivate M:shutdown B:deactivate B:shutdown"), "stack: ClearAllWidgets retires from the top layer down");

		manager->Shutdown();
	}

	void testRouting()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();

		PSharedPtr<PUITestButtonWidget> hud = makeButtonWidget(HRect(100.0f, 100.0f, 300.0f, 200.0f));
		manager->PushWidgetInstance(hud, EGameUILayer::Game);
		check(click(manager, 150.0f, 150.0f) && hud->Clicks == 1, "route: HUD button receives clicks");
		check(click(manager, 800.0f, 800.0f) == false && manager->IsWorldInputBlocked() == false, "route: a Game-mode widget passes empty clicks to the world");

		// 위 레이어(Game 모드)가 먼저 받고, 맞지 않으면 아래 레이어로 내려간다
		PSharedPtr<PUITestButtonWidget> overlay = makeButtonWidget(HRect(150.0f, 150.0f, 350.0f, 250.0f));
		manager->PushWidgetInstance(overlay, EGameUILayer::GameMenu);
		click(manager, 200.0f, 180.0f);
		check(overlay->Clicks == 1 && hud->Clicks == 1, "route: a higher layer receives first");
		click(manager, 120.0f, 120.0f);
		check(hud->Clicks == 2, "route: input falls through a Game-mode layer to the layer below");
		overlay->DeactivateWidget();

		// Menu 모드 위젯: 아래 레이어 · 월드 차단
		PSharedPtr<PUITestButtonWidget> menu = makeButtonWidget(HRect(600.0f, 600.0f, 800.0f, 700.0f));
		menu->SetInputMode(EGameWidgetInputMode::Menu);
		manager->PushWidgetInstance(menu, EGameUILayer::Menu);
		check(manager->IsWorldInputBlocked(), "route: a Menu-mode widget blocks the world");
		check(click(manager, 150.0f, 150.0f) && hud->Clicks == 2, "route: a Menu-mode widget blocks buttons on lower layers");
		check(click(manager, 900.0f, 50.0f), "route: an empty click is consumed while a menu is up");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Wheel, 900.0f, 50.0f), GTarget), "route: wheel is consumed while a menu is up");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 150.0f, 150.0f), GTarget);
		check(hud->Button->IsHovered() == false, "route: a covered HUD button is not hovered while a menu is up");
		check(click(manager, 650.0f, 650.0f) && menu->Clicks == 1, "route: the menu button receives clicks");

		// 자기 클릭 처리기 안에서 닫기(입력을 전하는 중 스택 변경)
		menu->bCloseOnClick = true;
		click(manager, 650.0f, 650.0f);
		check(menu->Clicks == 2 && menu->IsInStack() == false && hud->IsActive(), "route: closing a widget from its own click handler");
		check(manager->IsWorldInputBlocked() == false && click(manager, 900.0f, 50.0f) == false, "route: world input comes back after the menu closes");

		manager->Shutdown();
	}

	void testBackAction()
	{
		HList<PString> events;
		PSharedPtr<PGameUIManager> manager = makeManager();

		PSharedPtr<PUITestWidget> hud = makeTestWidget(events, "H");
		manager->PushWidgetInstance(hud, EGameUILayer::Game);
		check(manager->HandleBackAction() == false, "back: nothing handles back on a plain HUD");

		PSharedPtr<PUITestWidget> menu = makeTestWidget(events, "M");
		menu->SetBackHandler(true);
		menu->SetInputMode(EGameWidgetInputMode::Menu);
		manager->PushWidgetInstance(menu, EGameUILayer::Menu);
		check(manager->HandleBackAction() && menu->IsInStack() == false && hud->IsActive(), "back: the default back action closes the top back handler");

		// 뒤로가기를 처리하지 않는 Menu 모드 위젯(꼭 답해야 하는 대화상자)이 막는다
		PSharedPtr<PUITestWidget> menu2 = makeTestWidget(events, "M2");
		menu2->SetBackHandler(true);
		menu2->SetInputMode(EGameWidgetInputMode::Menu);
		manager->PushWidgetInstance(menu2, EGameUILayer::Menu);
		PSharedPtr<PUITestWidget> question = makeTestWidget(events, "Q");
		question->SetInputMode(EGameWidgetInputMode::Menu);
		manager->PushWidgetInstance(question, EGameUILayer::Modal);
		check(manager->HandleBackAction() == false && menu2->IsInStack(), "back: a Menu-mode widget without back handling stops it");
		question->DeactivateWidget();
		menu2->DeactivateWidget();

		// 처리하지 않겠다고 돌려주면(false) 아래 뒤로가기 처리 위젯으로 간다
		PSharedPtr<PUITestWidget> popup = makeTestWidget(events, "P");
		popup->SetBackHandler(true);
		popup->bCustomBack = true;
		popup->bBackResult = false;
		manager->PushWidgetInstance(popup, EGameUILayer::GameMenu);
		hud->SetBackHandler(true);
		events.clear();
		check(manager->HandleBackAction() && hud->IsInStack() == false && popup->IsInStack(), "back: a declined back action goes to the next handler below");
		check(joinEvents(events) == PString("P:back H:back H:deactivate H:shutdown"), "back: back handlers are asked from the top layer down");

		manager->Shutdown();
	}

	void testDisabledAndStyle()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);

		PSharedPtr<PGameUIButton> button = widget->AddChild<PGameUIButton>(PName("Button"));
		button->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 100.0f), HVector2(200.0f, 100.0f));
		int32 clicks = 0;
		button->OnClicked.AddLambda([&clicks]()
			{
				++clicks;
			});

		PSharedPtr<PGameUIImage> panel = widget->AddChild<PGameUIImage>(PName("Panel"));
		panel->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(500.0f, 500.0f), HVector2(300.0f, 300.0f));
		PSharedPtr<PGameUIButton> inner = panel->AddChild<PGameUIButton>(PName("Inner"));
		inner->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(10.0f, 10.0f), HVector2(100.0f, 50.0f));
		int32 innerClicks = 0;
		inner->OnClicked.AddLambda([&innerClicks]()
			{
				++innerClicks;
			});

		// 비활성
		button->SetEnabled(false);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		check(button->IsPressed() == false, "disabled: a disabled button does not enter the pressed state");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 150.0f, 150.0f), GTarget);
		check(click(manager, 150.0f, 150.0f) && clicks == 0, "disabled: a disabled button blocks the world but does not click");
		button->SetEnabled(true);
		click(manager, 150.0f, 150.0f);
		check(clicks == 1, "disabled: enabled again, the button clicks");

		panel->SetEnabled(false);
		check(click(manager, 520.0f, 520.0f) && innerClicks == 0, "disabled: a disabled parent blocks its children");
		panel->SetEnabled(true);
		click(manager, 520.0f, 520.0f);
		check(innerClicks == 1, "disabled: an enabled parent lets its children click");

		// 버튼 스타일 → 그리기 목록의 색. 사각형 순서: 버튼(0) · 패널(1) · 안쪽 버튼(2), 하나당 정점 4
		manager->HandlePointer(pointer(EGameUIPointerEventType::Leave, 0.0f, 0.0f), GTarget);
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		check(list.Vertices.size() >= 12 && isColor(list.Vertices[0].Color, HGameUIButtonStyle().Normal), "style: an unstyled button uses the default button style");

		HGameUIButtonStyle custom;
		custom.Normal = HLinearColor(0.9f, 0.1f, 0.1f, 1.0f);
		button->SetStyle(custom);
		manager->BuildDrawList(GTarget, list);
		check(button->HasStyle() && isColor(list.Vertices[0].Color, custom.Normal), "style: SetStyle overrides the default");

		HGameUIButtonStyle newDefault;
		newDefault.Normal = HLinearColor(0.1f, 0.8f, 0.2f, 1.0f);
		manager->SetDefaultButtonStyle(newDefault);
		button->ClearStyle();
		manager->BuildDrawList(GTarget, list);
		check(button->HasStyle() == false && isColor(list.Vertices[0].Color, newDefault.Normal), "style: the manager default applies to unstyled buttons");

		button->SetEnabled(false);
		manager->BuildDrawList(GTarget, list);
		check(isColor(list.Vertices[0].Color, newDefault.Disabled), "style: a disabled button draws the disabled color");
		button->SetEnabled(true);

		panel->SetEnabled(false);
		manager->BuildDrawList(GTarget, list);
		check(list.Vertices.size() >= 12 && isColor(list.Vertices[8].Color, newDefault.Disabled), "style: a button under a disabled parent draws disabled");
		panel->SetEnabled(true);

		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 150.0f, 150.0f), GTarget);
		manager->BuildDrawList(GTarget, list);
		check(isColor(list.Vertices[0].Color, newDefault.Hovered), "style: a hovered button draws the hovered color");

		// 글자 스타일: 필드별 덮어쓰기, 나머지는 관리자 기본
		PSharedPtr<PGameUIText> text = widget->AddChild<PGameUIText>(PName("Text"));
		HGameUITextStyle defaultText;
		defaultText.FontSize = 30.0f;
		defaultText.Color    = HLinearColor(0.5f, 0.6f, 0.7f, 1.0f);
		manager->SetDefaultTextStyle(defaultText);

		HGameUITextStyle resolved = text->ResolveStyle(manager.GetRawPointer());
		check(nearlyEqual(resolved.FontSize, 30.0f) && isColor(resolved.Color, defaultText.Color), "style: text without overrides uses the default text style");
		text->SetFontSize(44.0f);
		resolved = text->ResolveStyle(manager.GetRawPointer());
		check(nearlyEqual(resolved.FontSize, 44.0f) && isColor(resolved.Color, defaultText.Color), "style: text overrides one field and keeps the rest");
		HGameUITextStyle explicitStyle;
		explicitStyle.FontSize = 12.0f;
		explicitStyle.Color    = HLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
		text->SetStyle(explicitStyle);
		resolved = text->ResolveStyle(manager.GetRawPointer());
		check(nearlyEqual(resolved.FontSize, 12.0f) && isColor(resolved.Color, explicitStyle.Color), "style: SetStyle overrides every field");
		text->ClearStyle();
		resolved = text->ResolveStyle(manager.GetRawPointer());
		check(nearlyEqual(resolved.FontSize, 30.0f) && isColor(resolved.Color, defaultText.Color), "style: ClearStyle returns to the default");

		manager->Shutdown();
	}

	void testGameInstanceUI()
	{
		if (JGGameInstance::HasInstance() == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no game instance. Instance check skipped");
			return;
		}
		check(JGGameInstance::Get().GetUI().IsValid(), "instance: JGGameInstance owns a UI manager");
	}

	bool executeSelfTest(const HConsoleCommandArgs& args)
	{
		GCheckCount   = 0;
		GFailureCount = 0;
		GSkipCount    = 0;

		testUtf8();
		testLayout();
		testPointerAndButton();
		testDrawList();
		testFont();
		testTextWrap();
		testTextInput();
		testStackLifecycle();
		testRouting();
		testBackAction();
		testDisabledAndStyle();
		testGameInstanceUI();

		if (GFailureCount == 0)
		{
			JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: OK (%d/%d), skipped groups %d", GCheckCount, GCheckCount, GSkipCount);
			return true;
		}
		JG_LOG(GameUI, ELogLevel::Error, "gameui.selftest: FAILED (%d/%d), skipped groups %d", GCheckCount - GFailureCount, GCheckCount, GSkipCount);
		return false;
	}

	HAutoConsoleCommand GameUISelfTestCommand(
		"gameui.selftest",
		"gameui.selftest",
		"Run the game UI self tests (widget stacks, input routing, back, styles, layout, font, text wrap, text input). No GPU needed",
		&executeSelfTest);
}
