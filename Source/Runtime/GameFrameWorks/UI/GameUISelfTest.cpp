#include "PCH/PCH.h"
#include "UI/GameUI.h"
#include "UI/GameUIUtf8.h"
#include "UI/GameUIDrawList.h"
#include <chrono>
#include <cmath>
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
		bool            bClosingAtDeactivate = false;   // 마지막 OnDeactivated 때 IsClosing()

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
			bClosingAtDeactivate = IsClosing();
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

	// 묶음 끝 알림 기록(GG-16).
	struct HTimelineLog
	{
		int32 Finished = 0;
		int32 Canceled = 0;
	};

	void watch(PSharedPtr<PGameUITimeline> timeline, HTimelineLog& log)
	{
		timeline->OnFinished.AddLambda([&log](bool bCanceled)
			{
				++log.Finished;
				if (bCanceled)
				{
					++log.Canceled;
				}
			});
	}

	// 위젯 루트에 색 사각형을 붙인다(그리기 목록에서 색으로 찾는다).
	PSharedPtr<PGameUIImage> addPanel(PSharedPtr<JGGameWidget> widget, const char* name, const HLinearColor& color, const HRect& rect)
	{
		PSharedPtr<PGameUIImage> panel = widget->AddChild<PGameUIImage>(PName(name));
		panel->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(rect.left, rect.top), HVector2(rect.right - rect.left, rect.bottom - rect.top));
		panel->SetColor(color);
		return panel;
	}

	// 그 색(RGB)으로 그린 첫 사각형의 첫 정점 번호. 없으면 -1.
	int32 findQuad(const HGameUIDrawList& list, const HLinearColor& color)
	{
		for (size_t index = 0; index + 3 < list.Vertices.size(); index += 4)
		{
			const HLinearColor& vertexColor = list.Vertices[index].Color;
			if (nearlyEqual(vertexColor.R, color.R, 0.001f) && nearlyEqual(vertexColor.G, color.G, 0.001f) && nearlyEqual(vertexColor.B, color.B, 0.001f))
			{
				return (int32)index;
			}
		}
		return -1;
	}

	// 지금 그리면 그 색 사각형의 알파(불투명도가 곱해진 값). 그리지 않으면 -1.
	float32 drawnAlpha(PSharedPtr<PGameUIManager>& manager, const HLinearColor& color)
	{
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		const int32 index = findQuad(list, color);
		if (index < 0)
		{
			return -1.0f;
		}
		return list.Vertices[(size_t)index].Color.A;
	}

	// 검사용 게임 곡선: 끝값을 넘었다가 돌아온다(0 → 0, 0.5 → 1, 0.75 → 1.125, 1 → 1).
	float32 overshootCurve(float32 t)
	{
		return 3.0f * t - 2.0f * t * t;
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
		const std::vector<uint8>& atlas = font->GetAtlasPixels(0);
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

	// 기본 글꼴(ER-016): 게임 Content 의 Fonts/GameUIDefault.ttf 가 먼저, 게임이 정한 글꼴이 그보다 먼저.
	void testDefaultFont()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<PGameUIFont> searched = manager->GetDefaultFont();
		PSharedPtr<PGameUIFont> custom   = Allocate<PGameUIFont>();
		const bool bCustomLoaded = custom->LoadFromFile(PString("C:/Windows/Fonts/arial.ttf")) || custom->LoadFromFile(PString("C:/Windows/Fonts/malgun.ttf"));
		if (searched.IsValid() == false || bCustomLoaded == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no font to load. Default font check skipped");
			manager->Shutdown();
			return;
		}

		// 게임 프로젝트로 실행했고(JGConsole -game) 게임 Content 에 글꼴이 있으면 그것이 기본이다.
		if (HFileHelper::GameContentDirectory().Empty() == false)
		{
			PString gameFontPath;
			HFileHelper::CombinePath(HFileHelper::GameContentDirectory(), PString("Fonts/GameUIDefault.ttf"), &gameFontPath);
			if (HFileHelper::Exists(gameFontPath))
			{
				check(searched.GetRawPointer() == manager->LoadFont(gameFontPath).GetRawPointer(), "default font: the game Content font comes first");
			}
		}

		manager->SetDefaultFont(custom);
		check(manager->GetDefaultFont().GetRawPointer() == custom.GetRawPointer(), "default font: SetDefaultFont replaces the default font");

		// 글꼴을 정하지 않은 글은 게임이 정한 기본 글꼴로 그린다
		PSharedPtr<JGGameWidget> widget = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIText>  text   = widget->AddChild<PGameUIText>(PName("Text"));
		text->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 100.0f), HVector2(400.0f, 100.0f));
		text->SetText(PString("Abc"));
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		check(list.Commands.empty() == false && list.Commands[0].Font == custom.GetRawPointer(), "default font: text without a font draws with the game's default font");

		manager->SetDefaultFont(nullptr);
		check(manager->GetDefaultFont().GetRawPointer() == searched.GetRawPointer(), "default font: SetDefaultFont(nullptr) searches the candidates again");
		manager->BuildDrawList(GTarget, list);
		check(list.Commands.empty() == false && list.Commands[0].Font == searched.GetRawPointer(), "default font: text follows the default font again");

		manager->Shutdown();
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

	// 줄 마지막 글자의 끝(X + 서식 넣은 폭)
	float32 lineEndX(const PGameUIFont& font, const HGameUITextLayout& layout, const HGameUITextLine& line, float32 fontSize)
	{
		if (line.GlyphCount == 0)
		{
			return 0.0f;
		}
		const HGameUITextGlyph& last = layout.Glyphs[line.GlyphBegin + line.GlyphCount - 1];
		return last.X + font.MeasureStyledAdvance(last.Codepoint, fontSize, layout.Runs[last.Run]);
	}

	// 줄 안 i 번째와 그 앞 글자 사이(X 차이)
	float32 glyphGap(const HGameUITextLayout& layout, const HGameUITextLine& line, uint32 i)
	{
		return layout.Glyphs[line.GlyphBegin + i].X - layout.Glyphs[line.GlyphBegin + i - 1].X;
	}

	void testTextFormat()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<PGameUIFont> font = manager->GetDefaultFont();
		if (font.IsValid() == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no default font on this machine. Text format checks skipped");
			manager->Shutdown();
			return;
		}

		const float32 size = 20.0f;
		HGameUITextLayout        layout;
		HGameUITextLayoutOptions options;
		options.FontSize = size;

		// 서식 없는 배치: 글자 자리 = 폭(자간 보정 포함) 누적
		font->LayoutText(PString("ab"), options, layout);
		const float32 plainWidth = layout.Size.x;
		check(layout.Glyphs.size() == 2 && nearlyEqual(layout.Glyphs[0].X, 0.0f) && nearlyEqual(lineEndX(*font, layout, layout.Lines[0], size), plainWidth, 0.001f), "format: plain layout places glyphs at the advance sums");
		check(nearlyEqual(layout.LineHeight, font->MeasureLineHeight(size)) && nearlyEqual(layout.BaselineOffset, font->MeasureAscent(size)) && layout.Runs.size() == 1, "format: plain layout keeps the font line height and baseline");

		// 줄 간격 · 기준선
		options.LineHeight = 45.0f;
		font->LayoutText(PString("a\nb\nc"), options, layout);
		check(layout.Lines.size() == 3 && nearlyEqual(layout.Size.y, 135.0f) && nearlyEqual(layout.Lines[1].Top, 45.0f) && nearlyEqual(layout.Lines[2].Top, 90.0f), "format: line height sets the line pitch and the height");
		check(nearlyEqual(layout.BaselineOffset, (45.0f - font->MeasureLineHeight(size)) * 0.5f + font->MeasureAscent(size)), "format: the baseline sits in the middle of the extra line height");
		options.LineHeight = 0.0f;

		// 문단 간격: '\n' 다음 문단 앞에만(자동으로 바뀐 줄 앞에는 없다)
		const float32 fontLine = font->MeasureLineHeight(size);
		options.ParagraphSpacing = 10.0f;
		options.Wrap             = EGameUITextWrap::Word;
		options.WrapWidth        = measureWidth(*font, PString("word word"), size);
		font->LayoutText(PString("word word word\nx"), options, layout);
		check(layout.Lines.size() == 3 && nearlyEqual(layout.Lines[1].Top, fontLine) && nearlyEqual(layout.Lines[2].Top, fontLine * 2.0f + 10.0f) && nearlyEqual(layout.Size.y, fontLine * 3.0f + 10.0f), "format: paragraph spacing goes before each new paragraph only");
		check(layout.Lines[0].bParagraphStart && layout.Lines[1].bParagraphStart == false && layout.Lines[2].bParagraphStart && layout.Lines[0].bWrapped && layout.Lines[1].bWrapped == false, "format: lines know paragraph starts and wraps");
		options.ParagraphSpacing = 0.0f;
		options.Wrap             = EGameUITextWrap::None;
		options.WrapWidth        = 0.0f;

		// 자간: 글자 사이에만, 1/1000 em
		options.LetterSpacing = 100.0f;
		font->LayoutText(PString("ab"), options, layout);
		check(layout.Lines.size() == 1 && nearlyEqual(layout.Size.x, plainWidth + size * 0.1f, 0.001f) && nearlyEqual(lineEndX(*font, layout, layout.Lines[0], size), layout.Size.x, 0.001f), "format: letter spacing adds 1/1000 em between glyphs (not after the last)");
		options.LetterSpacing = -100.0f;
		font->LayoutText(PString("ab"), options, layout);
		check(nearlyEqual(layout.Size.x, plainWidth - size * 0.1f, 0.001f), "format: negative letter spacing tightens");
		options.LetterSpacing = 0.0f;

		// 장평 · 합성 굵게
		options.BaseStyle.HorizontalScale = 0.5f;
		font->LayoutText(PString("ab"), options, layout);
		check(nearlyEqual(layout.Size.x, plainWidth * 0.5f, 0.001f), "format: horizontal scale scales the width (kerning too)");
		options.BaseStyle.HorizontalScale = 1.0f;
		options.BaseStyle.bBold = true;
		font->LayoutText(PString("ab"), options, layout);
		check(nearlyEqual(layout.Size.x, plainWidth + 2.0f * size * PGameUIFont::BoldStrength, 0.001f), "format: synthetic bold widens each glyph by em x BoldStrength");
		options.BaseStyle.bBold = false;

		// 양쪽 맞춤: 자동으로 바뀐 줄은 폭에 맞고, 문단 마지막 줄은 왼쪽 그대로. 띄어쓰기 뒤 사이가 4배를 받는다.
		const PString words("word word word word word");
		options.Wrap      = EGameUITextWrap::Word;
		options.WrapWidth = measureWidth(*font, PString("word word"), size) + 30.0f;
		HGameUITextLayout natural;
		font->LayoutText(words, options, natural);
		options.bJustify = true;
		font->LayoutText(words, options, layout);
		check(layout.Lines.size() == 3 && layout.Lines[0].bJustified && layout.Lines[1].bJustified && layout.Lines[2].bJustified == false, "format: justify stretches wrapped lines, not the paragraph's last line");
		check(nearlyEqual(lineEndX(*font, layout, layout.Lines[0], size), options.WrapWidth, 0.01f) && nearlyEqual(layout.Lines[0].Width, options.WrapWidth), "format: a justified line ends exactly at the width");
		check(layout.Lines[2].Width < options.WrapWidth - 1.0f && nearlyEqual(layout.Glyphs[layout.Lines[2].GlyphBegin].X, 0.0f), "format: the last line stays left");
		const float32 extra = options.WrapWidth - natural.Lines[0].Width;
		check(layout.Lines[0].GlyphCount == 9
			&& nearlyEqual(glyphGap(layout, layout.Lines[0], 5) - glyphGap(natural, natural.Lines[0], 5), extra * 4.0f / 11.0f, 0.01f)
			&& nearlyEqual(glyphGap(layout, layout.Lines[0], 1) - glyphGap(natural, natural.Lines[0], 1), extra / 11.0f, 0.01f), "format: justify gives the gap after a space four shares");

		const PString hangulRun("\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4\xEB\x9D\xBC\xEB\xA7\x88\xEB\xB0\x94\xEC\x82\xAC\xEC\x95\x84\xEC\x9E\x90\xEC\xB0\xA8");   // "가나다라마바사아자차"
		options.Wrap      = EGameUITextWrap::Character;
		options.WrapWidth = measureWidth(*font, PString("\xEA\xB0\x80\xEB\x82\x98\xEB\x8B\xA4"), size) + 6.0f;   // "가나다" + 6
		options.bJustify  = false;
		font->LayoutText(hangulRun, options, natural);
		options.bJustify  = true;
		font->LayoutText(hangulRun, options, layout);
		check(layout.Lines.size() == 4 && layout.Lines[0].GlyphCount == 3 && layout.Lines[0].bJustified
			&& nearlyEqual(glyphGap(layout, layout.Lines[0], 1) - glyphGap(natural, natural.Lines[0], 1), 3.0f, 0.01f)
			&& nearlyEqual(glyphGap(layout, layout.Lines[0], 2) - glyphGap(natural, natural.Lines[0], 2), 3.0f, 0.01f), "format: a Hangul line without spaces is stretched evenly between glyphs");
		options.bJustify  = false;

		// 자간 + 말줄임도 폭 안
		options.Wrap          = EGameUITextWrap::None;
		options.WrapWidth     = measureWidth(*font, PString("abcdefgh"), size);
		options.bEllipsis     = true;
		options.LetterSpacing = 80.0f;
		font->LayoutText(PString("abcdefghijklmnop"), options, layout);
		check(layout.Lines.size() == 1 && layout.Lines[0].bEllipsis && layout.Lines[0].Width <= options.WrapWidth + 0.02f && nearlyEqual(lineEndX(*font, layout, layout.Lines[0], size), layout.Lines[0].Width, 0.001f), "format: ellipsis with letter spacing stays inside the width");
		options.bEllipsis     = false;
		options.LetterSpacing = 0.0f;

		// 마크업
		PSharedPtr<JGGameWidget> widget = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIText>  text   = widget->AddChild<PGameUIText>(PName("Rich"));
		text->SetFontSize(size);
		text->SetRichText(true);
		text->SetText(PString("<b>\xEA\xB5\xB5\xEA\xB2\x8C</b> \xEB\xB3\xB4\xED\x86\xB5 <i>\xEA\xB8\xB0\xEC\x9A\xB8</i> <color=#FF0000>\xEB\xB9\xA8\xEA\xB0\x95</color> <hscale=140>\xEB\x84\x93\xEA\xB2\x8C</hscale>"));   // "<b>굵게</b> 보통 <i>기울</i> <color=#FF0000>빨강</color> <hscale=140>넓게</hscale>"
		check(text->GetDisplayText() == PString("\xEA\xB5\xB5\xEA\xB2\x8C \xEB\xB3\xB4\xED\x86\xB5 \xEA\xB8\xB0\xEC\x9A\xB8 \xEB\xB9\xA8\xEA\xB0\x95 \xEB\x84\x93\xEA\xB2\x8C"), "markup: tags are removed from the display text");
		{
			const HList<HGameUITextSpan>& spans = text->GetSpans();
			check(spans.size() == 4 && spans[0].Begin == 0 && spans[0].End == 6 && spans[0].Style.bBold && spans[0].Style.bItalic == false
				&& spans[1].Begin == 14 && spans[1].End == 20 && spans[1].Style.bItalic
				&& spans[2].Style.bHasColor && isColor(spans[2].Style.Color, HLinearColor(1.0f, 0.0f, 0.0f, 1.0f))
				&& nearlyEqual(spans[3].Style.HorizontalScale, 1.4f), "markup: spans carry bold / italic / color / horizontal scale on display bytes");
		}
		text->SetText(PString("<b>a<i>b</i></b>c"));
		check(text->GetDisplayText() == PString("abc") && text->GetSpans().size() == 2 && text->GetSpans()[0].End == 1
			&& text->GetSpans()[1].Begin == 1 && text->GetSpans()[1].End == 2 && text->GetSpans()[1].Style.bBold && text->GetSpans()[1].Style.bItalic, "markup: nested tags combine");
		text->SetText(PString("&lt;b&gt; &amp; <foo> </b>x"));
		check(text->GetDisplayText() == PString("<b> & <foo> </b>x") && text->GetSpans().empty(), "markup: entities, unknown tags and unmatched closing tags stay as text");
		text->SetText(PString("<color=#12345>x</color>"));
		check(text->GetDisplayText() == PString("<color=#12345>x</color>"), "markup: a malformed color tag stays as text");
		text->SetRichText(false);
		check(text->GetDisplayText() == text->GetText() && text->GetSpans().empty(), "markup: rich text off keeps the markup as plain text");
		text->SetRichText(true);

		// 구간 굵게가 폭을 늘려 줄바꿈이 바뀐다
		text->SetWrap(EGameUITextWrap::Word);
		text->SetText(PString("aaaa aaaa"));
		const float32 fitWidth = measureWidth(*font, PString("aaaa aaaa"), size) + 0.5f;
		HGameUITextLayout richLayout;
		text->LayoutContent(fitWidth, manager.GetRawPointer(), richLayout);
		const uint64 plainLines = richLayout.Lines.size();
		text->SetText(PString("<b>aaaa aaaa</b>"));
		text->LayoutContent(fitWidth, manager.GetRawPointer(), richLayout);
		check(plainLines == 1 && richLayout.Lines.size() == 2 && richLayout.Runs.size() == 2 && richLayout.Runs[1].bBold, "markup: a bold span widens the text and changes the wrap");

		// 문단 서식 → MeasureContent · 요소 양쪽 맞춤
		HGameUIParagraphStyle paragraph;
		paragraph.LineHeight = 40.0f;
		text->SetParagraphStyle(paragraph);
		check(nearlyEqual(text->MeasureContent(fitWidth, manager.GetRawPointer()).y, 80.0f), "format: MeasureContent follows the paragraph style (2 lines x 40)");
		text->SetParagraphStyle(HGameUIParagraphStyle());
		text->SetText(words);
		text->SetAlignment(EGameUIHorizontalAlign::Justify, EGameUIVerticalAlign::Top);
		text->LayoutContent(measureWidth(*font, PString("word word"), size) + 30.0f, manager.GetRawPointer(), richLayout);
		check(richLayout.Lines.size() == 3 && richLayout.Lines[0].bJustified && richLayout.Lines[2].bJustified == false, "format: Justify alignment on the element stretches its wrapped lines");
		text->SetAlignment(EGameUIHorizontalAlign::Left, EGameUIVerticalAlign::Top);

		// 글리프 변형: 굵게 · 장평은 따로 래스터화
		const PGameUIFont::HGlyph normalGlyph = font->GetGlyph('H', 32);
		HGameUIGlyphVariant boldVariant;
		boldVariant.bBold = true;
		const PGameUIFont::HGlyph boldGlyph = font->GetGlyph('H', 32, boldVariant);
		HGameUIGlyphVariant wideVariant;
		wideVariant.HorizontalScale = 2.0f;
		const PGameUIFont::HGlyph wideGlyph = font->GetGlyph('H', 32, wideVariant);
		check(boldGlyph.bInAtlas && boldGlyph.Width > normalGlyph.Width && boldGlyph.Height > normalGlyph.Height
			&& (boldGlyph.UV.left != normalGlyph.UV.left || boldGlyph.UV.top != normalGlyph.UV.top), "format: synthetic bold rasterizes a wider glyph of its own");
		check(wideGlyph.bInAtlas && wideGlyph.Width >= normalGlyph.Width * 2 - 2 && wideGlyph.Width <= normalGlyph.Width * 2 + 2 && wideGlyph.Height == normalGlyph.Height, "format: horizontal scale rasterizes the glyph at that width");

		// 그리기: 기울임은 비튼 사각형, 구간 색은 그 글자의 정점 색
		text->SetWrap(EGameUITextWrap::None);
		text->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 100.0f), HVector2(600.0f, 100.0f));
		text->SetText(PString("<i>I</i><color=#00FF00>g</color>"));
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		check(list.Vertices.size() >= 8 && list.Vertices[0].Position.x > list.Vertices[3].Position.x + 0.5f && nearlyEqual(list.Vertices[0].Position.y, list.Vertices[1].Position.y), "format: an italic glyph is drawn sheared (top shifted right)");
		check(hasVertexColor(list, HLinearColor(0.0f, 1.0f, 0.0f, 1.0f)), "format: a span color goes to its glyph vertices");

		manager->Shutdown();
	}

	// 그리기 목록 사각형 quad 의 윗변 폭 · 비틀렸나(윗변이 오른쪽으로 밀림)
	float32 quadTopWidth(const HGameUIDrawList& list, uint64 quad)
	{
		return list.Vertices[quad * 4 + 1].Position.x - list.Vertices[quad * 4].Position.x;
	}

	bool isShearedQuad(const HGameUIDrawList& list, uint64 quad)
	{
		return list.Vertices[quad * 4].Position.x > list.Vertices[quad * 4 + 3].Position.x + 0.5f;
	}

	// 요청서(ER-010) 완료 기준 (a) ~ (d), 35px 본문
	void testTextFormatAcceptance()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<PGameUIFont>    font    = manager->GetDefaultFont();
		if (font.IsValid() == false || font->HasGlyph(0xD3ED) == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no Hangul default font on this machine. Text format acceptance checks skipped");
			manager->Shutdown();
			return;
		}

		const float32 size = 35.0f;
		PSharedPtr<JGGameWidget> widget = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIText>  text   = widget->AddChild<PGameUIText>(PName("Accept"));
		text->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(900.0f, 100.0f));
		text->SetFontSize(size);

		// (a) 줄 간격 45 · 자간 -100 · 양쪽 맞춤: 자동으로 바뀐 줄 끝이 폭에 맞고 문단 마지막 줄은 왼쪽, 높이 = 줄 수 x 45 + 문단 간격
		const float32 width = 400.0f;
		HGameUIParagraphStyle style;
		style.LineHeight       = 45.0f;
		style.LetterSpacing    = -100.0f;
		style.ParagraphSpacing = 8.0f;
		text->SetParagraphStyle(style);
		text->SetWrap(EGameUITextWrap::Word);
		text->SetAlignment(EGameUIHorizontalAlign::Justify, EGameUIVerticalAlign::Top);
		text->SetText(PString("Card body text wraps into several lines, both edges line up with the width and the last line stays left.\nSecond paragraph"));
		HGameUITextLayout layout;
		text->LayoutContent(width, manager.GetRawPointer(), layout);
		bool bEdges = layout.Lines.size() >= 4 && layout.Lines[0].bWrapped && layout.Lines.back().bWrapped == false;
		for (const HGameUITextLine& line : layout.Lines)
		{
			const float32 endX = lineEndX(*font, layout, line, size);
			if ((line.bWrapped && nearlyEqual(endX, width, 0.01f) == false) || (line.bWrapped == false && (line.bJustified || endX > width + 0.02f)))
			{
				bEdges = false;
			}
		}
		check(bEdges && nearlyEqual(text->MeasureContent(width, manager.GetRawPointer()).y, (float32)layout.Lines.size() * 45.0f + 8.0f),
			"accept (a): 35px, line height 45, letter spacing -100, justify: wrapped lines end at the width, last lines stay left, height = lines x 45 + paragraph spacing");

		// (b) 굵은 면 없는 TTF 한 요소 안에서 구간만 굵게 · 굵게 + 기울임(합성)
		text->SetParagraphStyle(HGameUIParagraphStyle());
		text->SetWrap(EGameUITextWrap::None);
		text->SetAlignment(EGameUIHorizontalAlign::Left, EGameUIVerticalAlign::Top);
		text->SetRichText(true);
		text->SetText(PString("\xED\x8F\xAD\xEB\xA1\x9C <b>\xED\x8F\xAD\xEB\xA1\x9C</b> <b><i>\xED\x8A\xB9\xEC\x84\xB1 \xEC\xB0\xB8\xEC\xA1\xB0</i></b>"));   // "폭로 <b>폭로</b> <b><i>특성 참조</i></b>"
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		check(list.GetQuadCount() == 8 && quadTopWidth(list, 2) > quadTopWidth(list, 0) && quadTopWidth(list, 3) > quadTopWidth(list, 1)
			&& isShearedQuad(list, 0) == false && isShearedQuad(list, 2) == false && isShearedQuad(list, 4) && isShearedQuad(list, 7),
			"accept (b): one element draws a plain word, a bold span and a bold italic span");

		// (c) 장평 93% → 폭 93%, 자간 +40 → 글자 사이 0.04 em
		text->SetRichText(false);
		text->SetText(PString("\xEC\x9D\xB4\xEB\xA6\x84 Name"));   // "이름 Name"
		const float32 plainWidth = text->MeasureContent(0.0f, manager.GetRawPointer()).x;
		HGameUIParagraphStyle narrow;
		narrow.HorizontalScale = 0.93f;
		text->SetParagraphStyle(narrow);
		const float32 narrowWidth = text->MeasureContent(0.0f, manager.GetRawPointer()).x;
		HGameUIParagraphStyle spaced;
		spaced.LetterSpacing = 40.0f;
		text->SetParagraphStyle(spaced);
		text->LayoutContent(0.0f, manager.GetRawPointer(), layout);
		check(nearlyEqual(narrowWidth, plainWidth * 0.93f, 0.01f) && layout.Lines.size() == 1 && layout.Lines[0].GlyphCount == 7
			&& nearlyEqual(layout.Size.x, plainWidth + 0.04f * size * 6.0f, 0.01f), "accept (c): horizontal scale 93% gives 93% width, letter spacing +40 adds 0.04 em per gap");

		// (d) 구간 하나만 장평 140% 인 글자가 뒤 글자를 민다
		text->SetParagraphStyle(HGameUIParagraphStyle());
		text->SetRichText(true);
		text->SetText(PString("aXb"));
		HGameUITextLayout plain;
		text->LayoutContent(0.0f, manager.GetRawPointer(), plain);
		text->SetText(PString("a<hscale=140>X</hscale>b"));
		text->LayoutContent(0.0f, manager.GetRawPointer(), layout);
		const float32 push = 0.4f * (font->MeasureAdvance('X', size) + font->MeasureKerning('a', 'X', size));
		check(plain.Glyphs.size() == 3 && layout.Glyphs.size() == 3 && nearlyEqual(layout.Glyphs[2].X - plain.Glyphs[2].X, push, 0.01f)
			&& nearlyEqual(layout.Size.x - plain.Size.x, push, 0.01f) && push > 1.0f, "accept (d): a 140% span pushes the following glyphs by its extra width");

		manager->Shutdown();
	}

	// 글리프 비트맵 커버리지(아틀라스에서 꺼냄, 행 우선 Width x Height)
	bool readGlyphCoverage(const PGameUIFont::HGlyph& glyph, std::vector<uint8>& outCoverage)
	{
		if (glyph.bInAtlas == false || glyph.Font == nullptr)
		{
			return false;
		}
		const std::vector<uint8>& pixels = glyph.Font->GetAtlasPixels(glyph.Page);
		const int32 left = (int32)(glyph.UV.left * (float32)PGameUIFont::AtlasSize + 0.5f);
		const int32 top  = (int32)(glyph.UV.top * (float32)PGameUIFont::AtlasSize + 0.5f);
		outCoverage.resize((size_t)glyph.Width * glyph.Height);
		for (int32 y = 0; y < glyph.Height; ++y)
		{
			for (int32 x = 0; x < glyph.Width; ++x)
			{
				outCoverage[(size_t)y * glyph.Width + x] = pixels[((size_t)(top + y) * PGameUIFont::AtlasSize + left + x) * 4 + 3];
			}
		}
		return true;
	}

	// 세로 획 글자의 가운데 줄 잉크 폭(커버리지 합 / 255)이 굵게로 는 양(픽셀). 글리프를 못 읽으면 -1.
	float32 boldInkIncrease(PGameUIFont& font, uint32 codepoint, int32 size, const HGameUIGlyphVariant& bold)
	{
		const PGameUIFont::HGlyph normal = font.GetGlyph(codepoint, size);
		const PGameUIFont::HGlyph thick  = font.GetGlyph(codepoint, size, bold);
		std::vector<uint8> normalCoverage;
		std::vector<uint8> thickCoverage;
		if (readGlyphCoverage(normal, normalCoverage) == false || readGlyphCoverage(thick, thickCoverage) == false)
		{
			return -1.0f;
		}
		// 굵은 상자는 사방으로 같게 크지 않을 수 있다(선명한 굵게, GG-19) — 같은 줄은 오프셋 차이로 찾는다.
		const int32 row      = normal.Height / 2;
		const int32 thickRow = row + normal.OffsetY - thick.OffsetY;
		if (thickRow < 0 || thickRow >= thick.Height)
		{
			return -1.0f;
		}
		float32 normalInk = 0.0f;
		float32 thickInk  = 0.0f;
		for (int32 x = 0; x < normal.Width; ++x)
		{
			normalInk += (float32)normalCoverage[(size_t)row * normal.Width + x] / 255.0f;
		}
		for (int32 x = 0; x < thick.Width; ++x)
		{
			thickInk += (float32)thickCoverage[(size_t)thickRow * thick.Width + x] / 255.0f;
		}
		return thickInk - normalInk;
	}

	// 보통 글리프에서 비어 있던 칸(커버리지 < 25%) 수와, 그중 굵게 뒤 75% 이상으로 막힌 칸 수(ER-012 (a) 판정). 못 읽은 글자가 있으면 false.
	bool countFilledHoles(PGameUIFont& font, const uint32* codepoints, int32 count, int32 size, const HGameUIGlyphVariant& bold, int32& outHoles, int32& outFilled)
	{
		outHoles  = 0;
		outFilled = 0;
		bool bRead = true;
		for (int32 index = 0; index < count; ++index)
		{
			const PGameUIFont::HGlyph normal = font.GetGlyph(codepoints[index], size);
			const PGameUIFont::HGlyph thick  = font.GetGlyph(codepoints[index], size, bold);
			std::vector<uint8> normalCoverage;
			std::vector<uint8> thickCoverage;
			if (readGlyphCoverage(normal, normalCoverage) == false || readGlyphCoverage(thick, thickCoverage) == false)
			{
				bRead = false;
				continue;
			}
			// 같은 자리는 오프셋 차이로(굵은 상자가 사방으로 같게 크지 않을 수 있다 — GG-19)
			const int32 shiftX = normal.OffsetX - thick.OffsetX;
			const int32 shiftY = normal.OffsetY - thick.OffsetY;
			for (int32 y = 0; y < normal.Height; ++y)
			{
				for (int32 x = 0; x < normal.Width; ++x)
				{
					if (normalCoverage[(size_t)y * normal.Width + x] < 64)
					{
						++outHoles;
						const int32 thickX = x + shiftX;
						const int32 thickY = y + shiftY;
						if (thickX >= 0 && thickY >= 0 && thickX < thick.Width && thickY < thick.Height && thickCoverage[(size_t)thickY * thick.Width + thickX] >= 192)
						{
							++outFilled;
						}
					}
				}
			}
		}
		return bRead;
	}

	// 획 사이 빈 곳: 보통 글리프에서 가로 또는 세로로 양쪽 끝이 잉크(≥ 50%)인 빈 칸(< 25%) 연속 구간. 그 수와,
	// 굵게 뒤 구간 전체가 75% 이상으로 진해진(막힌) 수, 그중 길이 3칸 이상인 구간이 막힌 수.
	void countClosedGaps(PGameUIFont& font, const uint32* codepoints, int32 count, int32 size, const HGameUIGlyphVariant& bold,
		int32& outGaps, int32& outClosed, int32& outWideClosed)
	{
		outGaps       = 0;
		outClosed     = 0;
		outWideClosed = 0;
		for (int32 index = 0; index < count; ++index)
		{
			const PGameUIFont::HGlyph normal = font.GetGlyph(codepoints[index], size);
			const PGameUIFont::HGlyph thick  = font.GetGlyph(codepoints[index], size, bold);
			std::vector<uint8> normalCoverage;
			std::vector<uint8> thickCoverage;
			if (readGlyphCoverage(normal, normalCoverage) == false || readGlyphCoverage(thick, thickCoverage) == false)
			{
				continue;
			}
			// 같은 자리는 오프셋 차이로(굵은 상자가 사방으로 같게 크지 않을 수 있다 — GG-19). 굵은 상자 밖은 빈 칸
			const int32 shiftX = normal.OffsetX - thick.OffsetX;
			const int32 shiftY = normal.OffsetY - thick.OffsetY;
			auto normalAt = [&](int32 x, int32 y)
			{
				return normalCoverage[(size_t)y * normal.Width + x];
			};
			auto thickAt = [&](int32 x, int32 y)
			{
				const int32 thickX = x + shiftX;
				const int32 thickY = y + shiftY;
				if (thickX < 0 || thickY < 0 || thickX >= thick.Width || thickY >= thick.Height)
				{
					return (uint8)0;
				}
				return thickCoverage[(size_t)thickY * thick.Width + thickX];
			};
			// bHorizontal: 줄(y 고정)을 따라, 아니면 열(x 고정)을 따라
			for (int32 pass = 0; pass < 2; ++pass)
			{
				const bool  bHorizontal = pass == 0;
				const int32 lines       = bHorizontal ? normal.Height : normal.Width;
				const int32 length      = bHorizontal ? normal.Width : normal.Height;
				for (int32 line = 0; line < lines; ++line)
				{
					int32 position = 0;
					while (position < length)
					{
						const int32 x = bHorizontal ? position : line;
						const int32 y = bHorizontal ? line : position;
						if (normalAt(x, y) >= 64)
						{
							++position;
							continue;
						}
						const int32 runBegin = position;
						while (position < length && normalAt(bHorizontal ? position : line, bHorizontal ? line : position) < 64)
						{
							++position;
						}
						const int32 runEnd = position;   // [runBegin, runEnd)
						if (runBegin == 0 || runEnd >= length)
						{
							continue;
						}
						const bool bInkBefore = normalAt(bHorizontal ? runBegin - 1 : line, bHorizontal ? line : runBegin - 1) >= 128;
						const bool bInkAfter  = normalAt(bHorizontal ? runEnd : line, bHorizontal ? line : runEnd) >= 128;
						if (bInkBefore == false || bInkAfter == false)
						{
							continue;
						}
						++outGaps;
						bool bClosed = true;
						for (int32 at = runBegin; at < runEnd; ++at)
						{
							if (thickAt(bHorizontal ? at : line, bHorizontal ? line : at) < 192)
							{
								bClosed = false;
								break;
							}
						}
						if (bClosed)
						{
							++outClosed;
							if (runEnd - runBegin >= 3)
							{
								++outWideClosed;
							}
						}
					}
				}
			}
		}
	}

	// 요청서(ER-012) 완료 기준 (a) (b): 합성 굵게가 글자 크기에 비례하고, 1픽셀 미만은 부분 픽셀로
	void testBoldAcceptance()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<PGameUIFont>    font    = manager->GetDefaultFont();
		if (font.IsValid() == false || font->HasGlyph(0xD3ED) == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no Hangul default font on this machine. Bold acceptance checks skipped");
			manager->Shutdown();
			return;
		}

		HGameUIGlyphVariant boldVariant;
		boldVariant.bBold = true;

		// (a) 11 · 14 · 16px 굵은 "폭로 강제 전투 협상": 보통 글리프에서 비어 있던 칸(커버리지 < 25%) 가운데 굵게 뒤 75% 이상으로 막힌 칸이 5% 미만
		const uint32 hangul[] = { 0xD3ED, 0xB85C, 0xAC15, 0xC81C, 0xC804, 0xD22C, 0xD611, 0xC0C1 };   // 폭 로 강 제 전 투 협 상
		const int32  smallSizes[] = { 11, 14, 16 };
		bool bOpen = true;
		for (int32 size : smallSizes)
		{
			int32 holes  = 0;
			int32 filled = 0;
			for (uint32 codepoint : hangul)
			{
				const PGameUIFont::HGlyph normal = font->GetGlyph(codepoint, size);
				const PGameUIFont::HGlyph bold   = font->GetGlyph(codepoint, size, boldVariant);
				std::vector<uint8> normalCoverage;
				std::vector<uint8> boldCoverage;
				if (readGlyphCoverage(normal, normalCoverage) == false || readGlyphCoverage(bold, boldCoverage) == false)
				{
					bOpen = false;
					continue;
				}
				const int32 padding = (bold.Width - normal.Width) / 2;
				for (int32 y = 0; y < normal.Height; ++y)
				{
					for (int32 x = 0; x < normal.Width; ++x)
					{
						if (normalCoverage[(size_t)y * normal.Width + x] < 64)
						{
							++holes;
							if (boldCoverage[(size_t)(y + padding) * bold.Width + x + padding] >= 192)
							{
								++filled;
							}
						}
					}
				}
			}
			JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: bold %dpx — %d of %d empty pixels filled", size, filled, holes);
			if (holes == 0 || filled * 20 >= holes)
			{
				bOpen = false;
			}
		}
		check(bOpen, "accept ER-012 (a): small synthetic bold Hangul (11 / 14 / 16px) keeps its empty pixels open");

		// (b) 11 · 24 · 48px 세로 획('I')의 잉크 폭(가운데 줄 커버리지 합 / 255) 증가 = em / 24 ± 0.5px
		const int32 inkSizes[] = { 11, 24, 48 };
		bool bInkWidth = true;
		for (int32 size : inkSizes)
		{
			const PGameUIFont::HGlyph normal = font->GetGlyph('I', size);
			const PGameUIFont::HGlyph bold   = font->GetGlyph('I', size, boldVariant);
			std::vector<uint8> normalCoverage;
			std::vector<uint8> boldCoverage;
			if (readGlyphCoverage(normal, normalCoverage) == false || readGlyphCoverage(bold, boldCoverage) == false)
			{
				bInkWidth = false;
				continue;
			}
			const int32 padding = (bold.Height - normal.Height) / 2;
			const int32 row     = normal.Height / 2;
			float32 normalInk = 0.0f;
			float32 boldInk   = 0.0f;
			for (int32 x = 0; x < normal.Width; ++x)
			{
				normalInk += (float32)normalCoverage[(size_t)row * normal.Width + x] / 255.0f;
			}
			for (int32 x = 0; x < bold.Width; ++x)
			{
				boldInk += (float32)boldCoverage[(size_t)(row + padding) * bold.Width + x] / 255.0f;
			}
			const float32 expected = (float32)size * PGameUIFont::BoldStrength;
			JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: bold %dpx ink width %.2f -> %.2f (+%.2f, expected +%.2f)", size, normalInk, boldInk, boldInk - normalInk, expected);
			if (HMath::Abs((boldInk - normalInk) - expected) > 0.5f)
			{
				bInkWidth = false;
			}
		}
		check(bInkWidth, "accept ER-012 (b): synthetic bold ink width grows by em / 24 (+-0.5px) at 11 / 24 / 48px");

		manager->Shutdown();
	}

	// 요청서(ER-018) 완료 기준: 글 요소마다 굵게 세기(GG-18, D-33). 세기 0 = 지금 그대로(em/24 — 위 ER-012 기준이 그대로 통과)
	void testBoldStrength()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();   // 기준 1000x1000 = GTarget → 배율 1
		PSharedPtr<PGameUIFont>    font    = manager->GetDefaultFont();
		if (font.IsValid() == false || font->HasGlyph(0xD3ED) == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no Hangul default font on this machine. Bold strength checks skipped");
			manager->Shutdown();
			return;
		}

		check(PGameUIFont::ResolveBoldStrength(0.0f) == PGameUIFont::BoldStrength && PGameUIFont::ResolveBoldStrength(-1.0f) == PGameUIFont::BoldStrength
			&& PGameUIFont::ResolveBoldStrength(1.0f / 12.0f) == 1.0f / 12.0f && PGameUIFont::ResolveBoldStrength(1.0f) == PGameUIFont::MaxBoldStrength,
			"bold strength: 0 or less is the engine default, above the maximum is the maximum");

		HGameUIGlyphVariant defaultBold;
		defaultBold.bBold = true;
		HGameUIGlyphVariant explicitDefault = defaultBold;
		explicitDefault.BoldStrength = PGameUIFont::BoldStrength;
		HGameUIGlyphVariant strongBold = defaultBold;
		strongBold.BoldStrength = 1.0f / 12.0f;

		// (b) 세기 0 = 지금 그대로(폭 + em/24, 지금 넓힘). 기본 세기를 직접 주면 폭은 같고, 선명한 굵게(GG-19)로 따로 그린다
		const PGameUIFont::HGlyph plain    = font->GetGlyph(0xD3ED, 19);
		const PGameUIFont::HGlyph byZero   = font->GetGlyph(0xD3ED, 19, defaultBold);
		const PGameUIFont::HGlyph byValue  = font->GetGlyph(0xD3ED, 19, explicitDefault);
		const PGameUIFont::HGlyph byStrong = font->GetGlyph(0xD3ED, 19, strongBold);
		check(byZero.bInAtlas && byValue.bInAtlas && byZero.Advance == plain.Advance + 19.0f * PGameUIFont::BoldStrength && byValue.Advance == byZero.Advance
			&& (byZero.Page != byValue.Page || byZero.UV.left != byValue.UV.left || byZero.UV.top != byValue.UV.top),
			"accept ER-018 (b): bold strength 0 keeps the default widening (width + em / 24); an explicit em / 24 has the same width but its own sharp glyph (GG-19)");

		// (c) 세기가 다른 같은 글자는 캐시에서 섞이지 않는다(따로 래스터화 · 폭)
		check(byStrong.bInAtlas && (byStrong.Page != byZero.Page || byStrong.UV.left != byZero.UV.left || byStrong.UV.top != byZero.UV.top)
			&& nearlyEqual(byStrong.Advance - plain.Advance, 19.0f / 12.0f, 0.001f), "accept ER-018 (c): glyphs with different bold strengths are cached and sized apart");

		// (a) 세기를 올리면 넓힘이 그만큼 커진다: 세로 획('I') 잉크 폭 증가가 세기 순서대로 늘고(em/24 < em/16 < em/12 < em/8), em/12 는 기본의 1.5배 이상.
		//     한글 "폭로 강제 전투 협상"의 3칸 이상 빈 곳은 em/12 에서도 막히지 않는다(1px 미만 넓힘은 획 바로 옆 한 겹만 진해진다).
		//     세기 · 크기마다 잉크 증가 · ER-012 빈 칸 판정 · 획 사이 빈 곳 막힘을 로그로 남긴다(게임이 세기를 고를 때 근거).
		const uint32  hangul[]      = { 0xD3ED, 0xB85C, 0xAC15, 0xC81C, 0xC804, 0xD22C, 0xD611, 0xC0C1 };   // 폭 로 강 제 전 투 협 상
		const int32   sizes[]       = { 14, 19, 24, 35 };
		const int32   denominators[] = { 24, 16, 12, 8 };   // 세기 = em / 분모
		bool bGrows       = true;
		bool bWideOpen    = true;
		for (int32 size : sizes)
		{
			float32 previousInk = 0.0f;
			float32 defaultInk  = 0.0f;
			for (int32 denominator : denominators)
			{
				HGameUIGlyphVariant variant = defaultBold;
				variant.BoldStrength = 1.0f / (float32)denominator;
				const float32 ink = boldInkIncrease(*font, 'I', size, variant);
				int32 holes      = 0;
				int32 filled     = 0;
				int32 gaps       = 0;
				int32 closed     = 0;
				int32 wideClosed = 0;
				countFilledHoles(*font, hangul, 8, size, variant, holes, filled);
				countClosedGaps(*font, hangul, 8, size, variant, gaps, closed, wideClosed);
				JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: bold em/%d %dpx — ink +%.2f (nominal +%.2f), empty pixels to 75%%+ %d of %d, gaps closed %d of %d (3px+ %d)",
					denominator, size, ink, (float32)size / (float32)denominator, filled, holes, closed, gaps, wideClosed);
				if (ink < 0.0f || ink <= previousInk)
				{
					bGrows = false;
				}
				if (denominator == 24)
				{
					defaultInk = ink;
				}
				if (denominator == 12 && (size == 14 || size == 19) && ink < defaultInk * 1.5f)
				{
					bGrows = false;
				}
				// 정한 세기는 GG-19 부터 선명한 굵게(넓힌 양이 정한 값 그대로 — 예전 넓힘보다 굵다)라 3px 틈은 em/16 에서 본다
				if (denominator == 16 && (size == 14 || size == 19) && wideClosed > 0)
				{
					bWideOpen = false;
				}
				previousInk = ink;
			}
		}
		check(bGrows, "accept ER-018 (a): the bold ink grows with the strength (em/24 < em/16 < em/12 < em/8, em/12 at least 1.5x em/24 at 14 / 19px)");
		check(bWideOpen, "accept ER-018 (a): at bold strength em/16 (14 / 19px, sharp since GG-19) Hangul gaps of 3px or more stay open");

		// 글 요소: 문단 서식의 세기가 요소 전체 굵게와 <b> 구간 모두에 든다. 폭은 그림과 같이 는다(줄 나누기 · 측정)
		PSharedPtr<JGGameWidget> widget = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		auto makeBoldText = [&widget](const char* name, const char* text, bool bRich, bool bWholeBold, float32 strength)
		{
			PSharedPtr<PGameUIText> element = widget->AddChild<PGameUIText>(PName(name));
			element->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(400.0f, 60.0f));
			element->SetFontSize(30.0f);
			element->SetRichText(bRich);
			element->SetText(PString(text));
			HGameUIParagraphStyle paragraph;
			paragraph.bBold        = bWholeBold;
			paragraph.BoldStrength = strength;
			element->SetParagraphStyle(paragraph);
			return element;
		};
		const PString pok("\xED\x8F\xAD");   // 폭
		const PString ro("\xEB\xA1\x9C");    // 로
		const PString pokRo   = pok + ro;
		const PString boldPok = PString("<b>") + pok + PString("</b>") + ro;
		PSharedPtr<PGameUIText> wholeDefault = makeBoldText("WholeDefault", pokRo.GetCStr(), false, true, 0.0f);
		PSharedPtr<PGameUIText> wholeStrong  = makeBoldText("WholeStrong", pokRo.GetCStr(), false, true, 1.0f / 12.0f);
		PSharedPtr<PGameUIText> spanDefault  = makeBoldText("SpanDefault", boldPok.GetCStr(), true, false, 0.0f);
		PSharedPtr<PGameUIText> spanStrong   = makeBoldText("SpanStrong", boldPok.GetCStr(), true, false, 1.0f / 12.0f);
		const float32 perGlyph = 30.0f * (1.0f / 12.0f - 1.0f / 24.0f);
		check(nearlyEqual(wholeStrong->MeasureContent(0.0f, manager.GetRawPointer()).x - wholeDefault->MeasureContent(0.0f, manager.GetRawPointer()).x, 2.0f * perGlyph, 0.01f),
			"bold strength: the paragraph strength widens every glyph of an element-wide bold text");
		check(nearlyEqual(spanStrong->MeasureContent(0.0f, manager.GetRawPointer()).x - spanDefault->MeasureContent(0.0f, manager.GetRawPointer()).x, perGlyph, 0.01f),
			"bold strength: <b> spans use the element's bold strength (plain glyphs keep their width)");

		// (c) 한 화면: 세기가 다른 두 요소의 같은 글자는 각자 세기로 그린다(다른 아틀라스 자리, 센 쪽이 더 넓은 그림)
		widget->GetRoot()->RemoveChild(spanDefault);
		widget->GetRoot()->RemoveChild(spanStrong);
		wholeDefault->SetText(pok);
		wholeStrong->SetText(pok);
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		const bool bTwoQuads = list.GetQuadCount() == 2;
		check(bTwoQuads && (list.Vertices[0].Texcoord.x != list.Vertices[4].Texcoord.x || list.Vertices[0].Texcoord.y != list.Vertices[4].Texcoord.y)
			&& (list.Vertices[5].Position.x - list.Vertices[4].Position.x) >= (list.Vertices[1].Position.x - list.Vertices[0].Position.x),
			"accept ER-018 (c): two elements with different bold strengths draw the same glyph each with its own strength");

		manager->Shutdown();
	}

	// 선명함 판정(ER-019): 잉크 칸(커버리지 ≥ 80 — 흰 바탕 루마 < 175) 수, 그중 아주 진한 칸(≥ 196 — 루마 < 60) 수, 가장 진한 커버리지.
	struct HInkStats
	{
		int32 Ink      = 0;
		int32 VeryDark = 0;
		int32 Darkest  = 0;
	};

	HInkStats measureInk(PGameUIFont& font, const uint32* codepoints, int32 count, int32 size, const HGameUIGlyphVariant* bold)
	{
		HInkStats stats;
		for (int32 index = 0; index < count; ++index)
		{
			const PGameUIFont::HGlyph glyph = (bold != nullptr) ? font.GetGlyph(codepoints[index], size, *bold) : font.GetGlyph(codepoints[index], size);
			std::vector<uint8> coverage;
			if (readGlyphCoverage(glyph, coverage) == false)
			{
				continue;
			}
			for (uint8 value : coverage)
			{
				stats.Darkest = HMath::Max(stats.Darkest, (int32)value);
				if (value >= 80)
				{
					++stats.Ink;
					if (value >= 196)
					{
						++stats.VeryDark;
					}
				}
			}
		}
		return stats;
	}

	float32 veryDarkShare(const HInkStats& stats)
	{
		return (stats.Ink > 0) ? (float32)stats.VeryDark / (float32)stats.Ink : 0.0f;
	}

	// 요청서(ER-019) 완료 기준: 세기를 정한 굵게는 8배로 그려 넓힌 뒤 줄여 선명하다(GG-19, D-34). 세기 0 은 지금 그대로(위 ER-012 · ER-018 기준)
	void testSharpBold()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<PGameUIFont>    font    = manager->GetDefaultFont();
		if (font.IsValid() == false || font->HasGlyph(0xD3ED) == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no Hangul default font on this machine. Sharp bold checks skipped");
			manager->Shutdown();
			return;
		}

		const uint32 hangul[] = { 0xD3ED, 0xB85C, 0xAC15, 0xC81C, 0xC804, 0xD22C, 0xD611, 0xC0C1 };   // 폭 로 강 제 전 투 협 상
		HGameUIGlyphVariant defaultBold;   // 세기 0 — 지금 넓힘
		defaultBold.bBold = true;
		HGameUIGlyphVariant sharp16 = defaultBold;
		sharp16.BoldStrength = 1.0f / 16.0f;
		HGameUIGlyphVariant sharp28 = defaultBold;
		sharp28.BoldStrength = 1.0f / 28.0f;   // 지금 넓힘으로 em/16 이던 굵기

		const int32 sizes[] = { 14, 16, 20 };
		bool bDark    = true;
		bool bGapOpen = true;
		bool bNominal = true;
		bool bBoxSize = true;
		for (int32 size : sizes)
		{
			// (a) 굵은 획 가운데가 보통 글만큼 진하다: 가장 진한 값이 보통 글 − 5 안, 아주 진한 칸 비율 ≥ 보통 글(em/16 · em/28). 세기 0(지금 넓힘)은 참고로 남긴다
			const HInkStats regular = measureInk(*font, hangul, 8, size, nullptr);
			const HInkStats current = measureInk(*font, hangul, 8, size, &defaultBold);
			const HInkStats strong  = measureInk(*font, hangul, 8, size, &sharp16);
			const HInkStats same    = measureInk(*font, hangul, 8, size, &sharp28);
			JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: sharp bold %dpx — very dark %.1f%% regular, %.1f%% default bold, %.1f%% em/16, %.1f%% em/28 (darkest %d / %d / %d / %d)",
				size, veryDarkShare(regular) * 100.0f, veryDarkShare(current) * 100.0f, veryDarkShare(strong) * 100.0f, veryDarkShare(same) * 100.0f,
				regular.Darkest, current.Darkest, strong.Darkest, same.Darkest);
			if (strong.Darkest < regular.Darkest - 5 || same.Darkest < regular.Darkest - 5
				|| veryDarkShare(strong) < veryDarkShare(regular) || veryDarkShare(same) < veryDarkShare(regular))
			{
				bDark = false;
			}

			// (b) 지금과 같은 굵기(em/28)에서 획 사이 1~2px 틈은 막히지 않는다: 막힘 2% 이하, 3px 이상 0
			int32 gaps       = 0;
			int32 closed     = 0;
			int32 wideClosed = 0;
			countClosedGaps(*font, hangul, 8, size, sharp28, gaps, closed, wideClosed);
			JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: sharp bold em/28 %dpx — gaps closed %d of %d (3px+ %d)", size, closed, gaps, wideClosed);
			if (gaps == 0 || closed * 50 > gaps || wideClosed > 0)
			{
				bGapOpen = false;
			}

			// 넓힌 양 = 정한 세기 그대로: 세로 획('I') 잉크 폭 증가 = em/16 ± 0.35px(지금 넓힘은 1픽셀 미만에서 명목값의 55~76%)
			const float32 ink     = boldInkIncrease(*font, 'I', size, sharp16);
			const float32 nominal = (float32)size / 16.0f;
			JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: sharp bold %dpx ink width +%.2f at em/16 (nominal +%.2f)", size, ink, nominal);
			if (ink < 0.0f || HMath::Abs(ink - nominal) > 0.35f)
			{
				bNominal = false;
			}

			// 아틀라스: 선명한 굵게 상자는 지금 넓힘 상자와 가로 · 세로 2px 안에서 같다
			for (uint32 codepoint : hangul)
			{
				const PGameUIFont::HGlyph before = font->GetGlyph(codepoint, size, defaultBold);
				const PGameUIFont::HGlyph after  = font->GetGlyph(codepoint, size, sharp28);
				if (HMath::Abs(after.Width - before.Width) > 2 || HMath::Abs(after.Height - before.Height) > 2)
				{
					bBoxSize = false;
				}
			}
		}
		check(bDark, "accept ER-019 (a): sharp bold strokes are as dark as regular strokes at 14 / 16 / 20px (darkest within 5, very dark share at least regular)");
		check(bGapOpen, "accept ER-019 (b): at the current weight (em/28) sharp bold keeps 1-2px gaps open (2% or less closed, none of 3px+)");
		check(bNominal, "sharp bold: the ink grows by the strength as set (em / 16 +-0.35px at 14 / 16 / 20px)");
		check(bBoxSize, "sharp bold: glyph boxes stay within 2px of the default bold boxes (same atlas use)");

		// 래스터 시간(참고): 처음 쓰는 한글 64자를 20 · 35px 로 — 지금 넓힘(세기 0)과 선명한 굵게(em/28)
		const int32 timedSizes[] = { 20, 35 };
		for (int32 size : timedSizes)
		{
			HGameUIGlyphVariant timedSharp = sharp28;
			timedSharp.HorizontalScale = 0.99f;   // 앞 검사에서 캐시된 글자와 겹치지 않게
			HGameUIGlyphVariant timedDefault = defaultBold;
			timedDefault.HorizontalScale = 0.99f;
			const auto start = std::chrono::steady_clock::now();
			for (uint32 index = 0; index < 64; ++index)
			{
				font->GetGlyph(0xB000 + index * 7, size, timedDefault);
			}
			const auto middle = std::chrono::steady_clock::now();
			for (uint32 index = 0; index < 64; ++index)
			{
				font->GetGlyph(0xB000 + index * 7, size, timedSharp);
			}
			const auto end = std::chrono::steady_clock::now();
			JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: sharp bold raster time %dpx — 64 glyphs %.2f ms default bold, %.2f ms sharp em/28", size,
				std::chrono::duration<double, std::milli>(middle - start).count(), std::chrono::duration<double, std::milli>(end - middle).count());
		}

		manager->Shutdown();
	}

	bool isPoint(const HVector2& a, const HVector2& b, float32 tolerance = 0.01f)
	{
		return nearlyEqual(a.x, b.x, tolerance) && nearlyEqual(a.y, b.y, tolerance);
	}

	// 사각형 quad 의 네 꼭짓점이 transform(rect 의 네 모서리)인가(왼쪽 위 · 오른쪽 위 · 오른쪽 아래 · 왼쪽 아래)
	bool isTransformedQuad(const HGameUIDrawList& list, uint64 quad, const HGameUIAffine& transform, const HRect& rect, float32 tolerance = 0.01f)
	{
		if (list.Vertices.size() < (quad + 1) * 4)
		{
			return false;
		}
		const HVector2 corners[4] =
		{
			HVector2(rect.left, rect.top), HVector2(rect.right, rect.top), HVector2(rect.right, rect.bottom), HVector2(rect.left, rect.bottom),
		};
		for (int32 i = 0; i < 4; ++i)
		{
			if (isPoint(list.Vertices[quad * 4 + i].Position, transform.Apply(corners[i]), tolerance) == false)
			{
				return false;
			}
		}
		return true;
	}

	bool isSameDrawList(const HGameUIDrawList& a, const HGameUIDrawList& b)
	{
		if (a.Vertices.size() != b.Vertices.size() || a.Indices != b.Indices || a.Commands.size() != b.Commands.size())
		{
			return false;
		}
		for (size_t i = 0; i < a.Vertices.size(); ++i)
		{
			if (isPoint(a.Vertices[i].Position, b.Vertices[i].Position, 0.0001f) == false || isColor(a.Vertices[i].Color, b.Vertices[i].Color) == false
				|| isPoint(a.Vertices[i].Texcoord, b.Vertices[i].Texcoord, 0.0001f) == false)
			{
				return false;
			}
		}
		return true;
	}

	float32 edgeLength(const HVector2& a, const HVector2& b)
	{
		const float32 dx = b.x - a.x;
		const float32 dy = b.y - a.y;
		return std::sqrt(dx * dx + dy * dy);
	}

	// 요청서(ER-014) 완료 기준: 그리기 변환(회전 · 배율 · 옮기기) · 불투명도
	void testRenderTransform()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();   // 기준 1000x1000 = GTarget → 배율 1
		PSharedPtr<PGameUIFont>    font    = manager->GetDefaultFont();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);

		// 카드: (300, 300)-(500, 600), 피벗 가운데 (400, 450). 자식 20개 = 그림 19 + 글자 1
		PSharedPtr<PGameUIImage> card = widget->AddChild<PGameUIImage>(PName("Card"));
		card->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.5f, 0.5f), HVector2(400.0f, 450.0f), HVector2(200.0f, 300.0f));
		card->SetHitTestVisible(true);
		PSharedPtr<PGameUIImage> firstChild;
		for (int32 i = 0; i < 19; ++i)
		{
			PSharedPtr<PGameUIImage> part = card->AddChild<PGameUIImage>(PName(PString::Format("Part%d", i)));
			part->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2((float32)(i % 5) * 40.0f, (float32)(i / 5) * 40.0f), HVector2(20.0f, 20.0f));
			if (i == 0)
			{
				firstChild = part;
			}
		}
		PSharedPtr<PGameUIText> label = card->AddChild<PGameUIText>(PName("Label"));
		label->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(20.0f, 200.0f), HVector2(100.0f, 40.0f));
		label->SetFontSize(20.0f);
		label->SetText(PString("H"));

		HGameUIDrawList plain;
		manager->BuildDrawList(GTarget, plain);
		check(plain.GetQuadCount() == (font.IsValid() ? 21u : 20u), "transform: the card draws itself, 19 parts and a label");

		// (a) 15° · 1.3배: 카드와 자식이 피벗을 기준으로 함께 돌고 커진다(배치는 그대로)
		HGameUIRenderTransform transform;
		transform.Angle = 15.0f;
		transform.Scale = HVector2(1.3f, 1.3f);
		card->SetRenderTransform(transform);
		const HGameUIAffine expected = HGameUIAffine::FromRenderTransform(transform, HVector2(400.0f, 450.0f));
		HGameUIDrawList turned;
		manager->BuildDrawList(GTarget, turned);
		check(turned.GetQuadCount() == plain.GetQuadCount() && isTransformedQuad(turned, 0, expected, HRect(300.0f, 300.0f, 500.0f, 600.0f))
			&& isTransformedQuad(turned, 1, expected, HRect(300.0f, 300.0f, 320.0f, 320.0f)) && isRect(card->GetLayoutRect(), 300.0f, 300.0f, 500.0f, 600.0f),
			"accept ER-014 (a): the element and its children turn and grow around the pivot, layout stays");
		check(nearlyEqual(edgeLength(turned.Vertices[0].Position, turned.Vertices[1].Position), 200.0f * 1.3f, 0.01f)
			&& turned.Vertices[1].Position.y > turned.Vertices[0].Position.y + 1.0f, "transform: 15 degrees clockwise on screen, 1.3x width");

		// 판정: 돌린 모양 안에서만 맞는다
		const HVector2 insideTurned = expected.Apply(HVector2(310.0f, 590.0f));   // 원래 사각형 밖(x < 300)이지만 돌린 모양 안
		const HRect    turnedBounds = card->LocalToCanvasBounds(card->GetLayoutRect());
		const HVector2 outsideTurned(turnedBounds.left + 3.0f, turnedBounds.top + 3.0f);   // 감싸는 사각형 모서리 = 돌린 모양 밖
		HGameUIPointerEvent move;
		move.Type     = EGameUIPointerEventType::Move;
		move.Position = insideTurned;
		manager->HandlePointer(move, GTarget);
		const bool bHitInside = manager->GetHoveredElement().GetRawPointer() == card.GetRawPointer();
		move.Position = outsideTurned;
		manager->HandlePointer(move, GTarget);
		const bool bMissOutside = manager->GetHoveredElement().IsValid() == false;
		check(insideTurned.x < 300.0f && bHitInside && bMissOutside, "accept ER-014 (a): hit testing follows the turned shape");
		check(isPoint(card->CanvasToLocal(insideTurned), HVector2(310.0f, 590.0f), 0.01f), "transform: CanvasToLocal undoes the transform");

		// 글자는 변환 배율의 정수 픽셀로 래스터화(20 x 1.3 = 26px)하고 화면에서 1:1
		if (font.IsValid())
		{
			const PGameUIFont::HGlyph glyph26 = font->GetGlyph('H', 26);
			const uint64 labelQuad = turned.GetQuadCount() - 1;
			const float32 width  = edgeLength(turned.Vertices[labelQuad * 4].Position, turned.Vertices[labelQuad * 4 + 1].Position);
			const float32 height = edgeLength(turned.Vertices[labelQuad * 4].Position, turned.Vertices[labelQuad * 4 + 3].Position);
			check(nearlyEqual(width, (float32)glyph26.Width, 0.05f) && nearlyEqual(height, (float32)glyph26.Height, 0.05f),
				"accept ER-014: glyphs are rasterized at the transform scale (20px x 1.3 = 26px) and drawn texel to pixel");
		}

		// 화면 배율 0.5: 같은 변환이 렌더 타깃 픽셀에서도 맞다
		HGameUIDrawList half;
		manager->BuildDrawList(HVector2(500.0f, 500.0f), half);
		check(isTransformedQuad(half, 0, expected.Scaled(0.5f), HRect(150.0f, 150.0f, 250.0f, 300.0f)), "transform: works at screen scale 0.5 (pixel space)");

		// 옮기기
		HGameUIRenderTransform moved;
		moved.Translation = HVector2(50.0f, -20.0f);
		card->SetRenderTransform(moved);
		HGameUIDrawList shifted;
		manager->BuildDrawList(GTarget, shifted);
		move.Position = HVector2(545.0f, 400.0f);   // 원래 사각형 밖, 옮긴 사각형 안
		manager->HandlePointer(move, GTarget);
		check(isPoint(shifted.Vertices[0].Position, HVector2(350.0f, 280.0f)) && manager->GetHoveredElement().GetRawPointer() == card.GetRawPointer(),
			"transform: translation moves the drawing and the hit area");

		// (b) 불투명도: 부모 0.5 · 자식 0.5 → 정점 알파 0.25
		card->ClearRenderTransform();
		card->SetOpacity(0.5f);
		firstChild->SetOpacity(0.5f);
		HGameUIDrawList faded;
		manager->BuildDrawList(GTarget, faded);
		check(nearlyEqual(faded.Vertices[0].Color.A, card->GetColor().A * 0.5f, 0.001f) && nearlyEqual(faded.Vertices[4].Color.A, firstChild->GetColor().A * 0.25f, 0.001f)
			&& nearlyEqual(faded.Vertices[8].Color.A, 0.5f, 0.001f), "accept ER-014 (b): opacity multiplies down to the children");
		move.Position = HVector2(400.0f, 450.0f);
		manager->HandlePointer(move, GTarget);
		check(manager->GetHoveredElement().GetRawPointer() == card.GetRawPointer(), "opacity: hit testing ignores opacity");

		// (c) 변환 · 불투명도를 지우면 지금까지와 똑같이 그린다
		card->SetOpacity(1.0f);
		firstChild->SetOpacity(1.0f);
		HGameUIDrawList again;
		manager->BuildDrawList(GTarget, again);
		check(isSameDrawList(plain, again), "accept ER-014 (c): without transform or opacity the draw list is unchanged");

		manager->Shutdown();
	}

	bool isSameGlyphSlot(const PGameUIFont::HGlyph& a, const PGameUIFont::HGlyph& b)
	{
		return a.bInAtlas && b.bInAtlas && a.Page == b.Page && isRect(a.UV, b.UV.left, b.UV.top, b.UV.right, b.UV.bottom);
	}

	void testFontAtlas()
	{
		PSharedPtr<PGameUIManager> manager     = makeManager();
		PSharedPtr<PGameUIFont>    defaultFont = manager->GetDefaultFont();
		if (defaultFont.IsValid() == false || defaultFont->HasGlyph(0xAC00) == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no Hangul default font on this machine. Font atlas checks skipped");
			manager->Shutdown();
			return;
		}

		// 서로 다른 한글 1,000자 x 12 · 16 · 20 · 24 · 32px(한 장의 2배가 넘는다): 빈칸 · 비우기 · 상한 넘김 없이 모두 아틀라스에 들어간다.
		PSharedPtr<PGameUIFont> font = Allocate<PGameUIFont>();
		font->LoadFromFile(defaultFont->GetPath());
		check(font->GetAtlasPageCount() == 0 && font->GetAtlasPageLimit() == PGameUIFont::DefaultAtlasPageLimit, "atlas: a new font opens no page until a glyph needs one");
		const int32 sizes[] = { 12, 16, 20, 24, 32 };
		int32 blanks = 0;
		PGameUIFont::BeginFrame();
		for (int32 size : sizes)
		{
			for (uint32 i = 0; i < 1000; ++i)
			{
				if (font->GetGlyph(0xAC00 + i, size).bInAtlas == false)
				{
					++blanks;
				}
			}
		}
		HGameUIFontAtlasStats stats = font->GetAtlasStats();
		JG_LOG(GameUI, ELogLevel::Info, "gameui.selftest: 1,000 Hangul x 5 sizes -> atlas pages %d, usage %d%%", stats.PageCount, (int32)(stats.Usage * 100.0f + 0.5f));
		check(blanks == 0 && font->GetGlyphCount() == 5000 && stats.PageCount >= 2 && stats.PageCount <= PGameUIFont::DefaultAtlasPageLimit
			&& stats.EvictedPageCount == 0 && stats.OverLimitCount == 0, "atlas: 1,000 Hangul x 5 sizes fill several pages without blanks or eviction");
		check(font->GetGlyph(0xAC00, 12).Page == 0 && font->GetGlyph(0xAC00 + 999, 32).Page == stats.PageCount - 1 && font->GetAtlasPixels(stats.PageCount - 1).size() == (size_t)PGameUIFont::AtlasSize * PGameUIFont::AtlasSize * 4,
			"atlas: glyphs know their page");

		font->ClearGlyphs();
		check(font->GetGlyphCount() == 0 && font->GetAtlasPageCount() == 0 && font->GetAtlasPixels(0).empty(), "atlas: ClearGlyphs empties the glyph cache and the pages");
		check(font->GetGlyph(0xAC00, 32).bInAtlas && font->GetAtlasPageCount() == 1, "atlas: glyphs come back after ClearGlyphs");

		// 상한 2장. 프레임 1: 32px 글자로 페이지 0 을 채워 페이지 1 로 넘어간다.
		font->ClearGlyphs();
		font->SetAtlasPageLimit(2);
		uint32 next = 0xAC00;
		PGameUIFont::BeginFrame();
		const PGameUIFont::HGlyph oldGlyph = font->GetGlyph(next++, 32);
		while (font->GetAtlasPageCount() < 2 && next <= 0xD7A3)
		{
			font->GetGlyph(next++, 32);
		}

		// 프레임 2: 새 글자만 쓴다 → 페이지 1 이 차면 이번 프레임에 쓰지 않은 페이지 0 을 비우고 다시 채운다.
		PGameUIFont::BeginFrame();
		const uint32              frameTwoCodepoint = next;
		const PGameUIFont::HGlyph frameTwoGlyph     = font->GetGlyph(next++, 32);
		while (font->GetAtlasStats().EvictedPageCount == 0 && next <= 0xD7A3)
		{
			font->GetGlyph(next++, 32);
		}
		stats = font->GetAtlasStats();
		check(oldGlyph.Page == 0 && frameTwoGlyph.Page == 1 && stats.EvictedPageCount == 1 && stats.PageCount == 2 && stats.OverLimitCount == 0,
			"atlas: at the page limit the oldest page not used this frame is evicted");
		check(isSameGlyphSlot(font->GetGlyph(frameTwoCodepoint, 32), frameTwoGlyph), "atlas: glyphs used this frame keep their atlas slot");
		const uint64 glyphsBeforeOld = font->GetGlyphCount();
		const PGameUIFont::HGlyph oldAgain = font->GetGlyph(0xAC00, 32);
		check(oldGlyph.Font == font.GetRawPointer() && oldAgain.bInAtlas && oldAgain.Page == 0 && font->GetGlyphCount() == glyphsBeforeOld + 1, "atlas: an evicted glyph is rasterized again when used");

		// 프레임 3: 두 페이지를 모두 이번 프레임에 쓰면 비울 페이지가 없다 → 상한을 넘겨 한 장 더 연다(경고 한 번).
		PGameUIFont::BeginFrame();
		const PGameUIFont::HGlyph pageOneGlyph  = font->GetGlyph(frameTwoCodepoint, 32);
		const PGameUIFont::HGlyph pageZeroGlyph = font->GetGlyph(0xAC00, 32);
		while (font->GetAtlasPageCount() < 3 && next <= 0xD7A3)
		{
			font->GetGlyph(next++, 32);
		}
		stats = font->GetAtlasStats();
		check(stats.PageCount == 3 && stats.OverLimitCount == 1 && stats.EvictedPageCount == 1
			&& isSameGlyphSlot(font->GetGlyph(frameTwoCodepoint, 32), pageOneGlyph) && isSameGlyphSlot(font->GetGlyph(0xAC00, 32), pageZeroGlyph),
			"atlas: when every page is in use this frame the limit is exceeded instead of evicting");

		// 그리기: 다른 페이지의 글자는 같은 글꼴의 다른 배치(페이지 번호)
		PSharedPtr<JGGameWidget> widget = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIText>  text   = widget->AddChild<PGameUIText>(PName("Atlas"));
		text->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(400.0f, 100.0f));
		text->SetFont(font);
		text->SetFontSize(32.0f);
		HList<uint32> codepoints;
		codepoints.push_back(0xAC00);
		codepoints.push_back(frameTwoCodepoint);
		text->SetText(HGameUIUtf8::Encode(codepoints));
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		check(list.Commands.size() == 2 && list.Commands[0].Font == font.GetRawPointer() && list.Commands[1].Font == font.GetRawPointer()
			&& list.Commands[0].FontPage == 0 && list.Commands[1].FontPage == 1, "atlas: glyphs on different pages go to different batches");

		// 대체 글꼴: Arial 에 없는 한글을 맑은 고딕의 폭 · 글리프로 배치하고 그린다.
		PSharedPtr<PGameUIFont> latin  = manager->LoadFont(PString("C:/Windows/Fonts/arial.ttf"));
		PSharedPtr<PGameUIFont> hangul = manager->LoadFont(PString("C:/Windows/Fonts/malgun.ttf"));
		if (latin.IsValid() == false || hangul.IsValid() == false || latin->HasGlyph(0xD55C) || hangul->HasGlyph(0xD55C) == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: Arial / Malgun Gothic not found. Fallback font checks skipped");
			manager->Shutdown();
			return;
		}

		latin->SetFallbackFont(hangul);
		check(latin->HasGlyph(0xD55C) && latin->GetFallbackFont().GetRawPointer() == hangul.GetRawPointer(), "fallback: the fallback font adds its glyphs");
		const PGameUIFont::HGlyph fallbackGlyph = latin->GetGlyph(0xD55C, 32);
		check(fallbackGlyph.bInAtlas && fallbackGlyph.Font == hangul.GetRawPointer() && nearlyEqual(fallbackGlyph.Advance, hangul->GetGlyph(0xD55C, 32).Advance, 0.001f)
			&& latin->GetGlyph('A', 32).Font == latin.GetRawPointer(), "fallback: a missing glyph comes from the fallback font (own glyphs stay)");
		check(nearlyEqual(latin->MeasureAdvance(0xD55C, 20.0f), hangul->MeasureAdvance(0xD55C, 20.0f), 0.001f) && latin->MeasureKerning('A', 0xD55C, 20.0f) == 0.0f,
			"fallback: measuring uses the fallback width, no kerning across fonts");
		HGameUITextLayout layout;
		latin->LayoutText(PString("A\xED\x95\x9C"), 20.0f, 0.0f, EGameUITextWrap::None, 0, false, layout);   // "A한"
		check(nearlyEqual(layout.Size.x, latin->MeasureAdvance('A', 20.0f) + hangul->MeasureAdvance(0xD55C, 20.0f), 0.001f), "fallback: mixed text width = own width + fallback width");

		text->SetFont(latin);
		text->SetText(PString("A\xED\x95\x9C"));
		manager->BuildDrawList(GTarget, list);
		check(list.Commands.size() == 2 && list.Commands[0].Font == latin.GetRawPointer() && list.Commands[1].Font == hangul.GetRawPointer(), "fallback: glyphs are batched by the font that has them");

		hangul->SetFallbackFont(latin);
		check(hangul->GetFallbackFont().IsValid() == false, "fallback: a looping fallback chain is refused");
		latin->SetFallbackFont(PSharedPtr<PGameUIFont>());
		check(latin->HasGlyph(0xD55C) == false && latin->GetGlyph(0xD55C, 32).Font == latin.GetRawPointer(), "fallback: clearing the fallback font draws the missing glyph box again");

		manager->Shutdown();
	}

	// 끌기 훅을 기록하는 검사용 요소(그리지 않는다).
	class PUITestDragElement : public PGameUIElement
	{
	public:
		HList<PString>* Events = nullptr;

	protected:
		virtual void onDragBegin(const HVector2& position) override
		{
			record("hook:begin");
		}
		virtual void onDragMove(const HVector2& position, const PSharedPtr<PGameUIElement>& target) override
		{
			record("hook:move");
		}
		virtual void onDragEnd(const HVector2& position, const PSharedPtr<PGameUIElement>& dropTarget, bool bCanceled) override
		{
			record(bCanceled ? "hook:cancel" : "hook:end");
		}

	private:
		void record(const char* hook)
		{
			if (Events != nullptr)
			{
				Events->push_back(PString(hook));
			}
		}
	};

	// 관리자 끌기 이벤트 기록(마지막 값).
	struct HDragLog
	{
		int32                      Begins = 0;
		int32                      Moves  = 0;
		int32                      Ends   = 0;
		PSharedPtr<PGameUIElement> Element;
		HVector2                   ElementStart;    // 시작 때 요소 위치(게임처럼 이동만큼 옮긴다)
		HVector2                   BeginPosition;
		HVector2                   MovePosition;
		PSharedPtr<PGameUIElement> MoveTarget;
		HVector2                   EndPosition;
		PSharedPtr<PGameUIElement> DropTarget;
		bool                       bCanceled = false;
	};

	void testDragDrop()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		HList<PString> events;
		PSharedPtr<PUITestWidget> hud = makeTestWidget(events, "H");
		manager->PushWidgetInstance(hud, EGameUILayer::Game);

		// 놓을 곳(아래) → 끌 수 있는 카드(위, 안에 버튼) → 끌기를 켜지 않은 버튼
		PSharedPtr<PGameUIImage> zone = hud->AddChild<PGameUIImage>(PName("Zone"));
		zone->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(500.0f, 100.0f), HVector2(200.0f, 200.0f));
		zone->SetHitTestVisible(true);
		PSharedPtr<PUITestDragElement> card = hud->AddChild<PUITestDragElement>(PName("Card"));
		card->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 100.0f), HVector2(100.0f, 140.0f));
		card->SetHitTestVisible(true);
		card->SetDraggable(true);
		card->Events = &events;
		PSharedPtr<PGameUIButton> cardButton = card->AddChild<PGameUIButton>(PName("CardButton"));   // (110, 190) ~ (190, 230)
		cardButton->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(10.0f, 90.0f), HVector2(80.0f, 40.0f));
		PSharedPtr<PGameUIButton> plain = hud->AddChild<PGameUIButton>(PName("Plain"));
		plain->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(100.0f, 500.0f), HVector2(200.0f, 80.0f));

		int32 cardButtonClicks = 0;
		int32 plainClicks      = 0;
		cardButton->OnClicked.AddLambda([&cardButtonClicks]()
			{
				++cardButtonClicks;
			});
		plain->OnClicked.AddLambda([&plainClicks]()
			{
				++plainClicks;
			});

		HDragLog log;
		manager->OnDragBegin.AddLambda([&log, &events](const PSharedPtr<PGameUIElement>& element, const HVector2& position)
			{
				++log.Begins;
				log.Element       = element;
				log.ElementStart  = element->GetPosition();
				log.BeginPosition = position;
				events.push_back(PString("drag:begin"));
			});
		manager->OnDragMove.AddLambda([&log, &events](const PSharedPtr<PGameUIElement>& element, const HVector2& position, const PSharedPtr<PGameUIElement>& target)
			{
				++log.Moves;
				log.MovePosition = position;
				log.MoveTarget   = target;
				events.push_back(PString("drag:move"));

				// 게임 몫: 끄는 요소를 포인터를 따라 옮긴다(엔진은 옮기지 않는다).
				PSharedPtr<PGameUIElement> moved = element;
				moved->SetPosition(log.ElementStart + (position - log.BeginPosition));
			});
		manager->OnDragEnd.AddLambda([&log, &events](const PSharedPtr<PGameUIElement>& element, const HVector2& position, const PSharedPtr<PGameUIElement>& dropTarget, bool bCanceled)
			{
				++log.Ends;
				log.EndPosition = position;
				log.DropTarget  = dropTarget;
				log.bCanceled   = bCanceled;
				events.push_back(PString(bCanceled ? "drag:cancel" : "drag:end"));
			});

		const EGameUIPointerButton left = EGameUIPointerButton::Left;

		// (a) 문턱(기본 6) 안의 움직임은 끌기가 아니다. 넘으면 시작 1번(누른 자리) + 이동(지금 자리)
		events.clear();
		check(nearlyEqual(manager->GetDragThreshold(), 6.0f), "drag: the default threshold is 6");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 155.0f, 150.0f), GTarget);
		check(log.Begins == 0 && manager->GetDraggedElement().IsValid() == false && manager->IsPointerDown(left), "drag: a move inside the threshold does not start a drag");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 160.0f, 150.0f), GTarget), "drag: a move while dragging is consumed");
		check(log.Begins == 1 && log.Element.GetRawPointer() == card.GetRawPointer() && isPoint(log.BeginPosition, HVector2(150.0f, 150.0f)), "drag: begin once with the pressed point");
		check(log.Moves == 1 && isPoint(log.MovePosition, HVector2(160.0f, 150.0f)) && log.MoveTarget.IsValid() == false, "drag: a move follows begin at once (nothing under the card)");
		check(joinEvents(events) == PString("hook:begin drag:begin hook:move drag:move"), "drag: element hooks run just before the manager events");
		check(manager->GetDraggedElement().GetRawPointer() == card.GetRawPointer() && isPoint(manager->GetPointerPosition(), HVector2(160.0f, 150.0f)), "drag: dragged element and pointer position");

		// (b) 놓을 곳: 카드가 포인터를 따라와 위에 있어도 판정에서 빠지고 그 아래 놓을 곳이 대상
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 600.0f, 200.0f), GTarget);
		check(log.MoveTarget.GetRawPointer() == zone.GetRawPointer() && manager->GetHoveredElement().GetRawPointer() == zone.GetRawPointer(), "drag: the element under the pointer is the move target and the hover");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 600.0f, 200.0f), GTarget), "drag: the release is consumed");
		const HRect& cardRect = card->GetLayoutRect();
		check(cardRect.left <= 600.0f && 600.0f < cardRect.right && cardRect.top <= 200.0f && 200.0f < cardRect.bottom, "drag: the card followed the pointer (it is on top at the drop)");
		check(log.Ends == 1 && log.bCanceled == false && log.DropTarget.GetRawPointer() == zone.GetRawPointer() && isPoint(log.EndPosition, HVector2(600.0f, 200.0f)), "drag: drop target is the zone under the card");
		check(manager->GetDraggedElement().IsValid() == false && manager->IsPointerDown(left) == false, "drag: state is cleared after the drop");

		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 600.0f, 200.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 900.0f, 900.0f), GTarget);
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 900.0f, 900.0f), GTarget), "drag: a release over empty space is still consumed");
		check(log.Begins == 2 && log.Ends == 2 && log.DropTarget.IsValid() == false && log.bCanceled == false, "drag: a drop on empty space has no target and is not a cancel");

		// (c) 버튼: 문턱 안에서 떼면 클릭. 끌기를 켜지 않은 버튼은 크게 움직여도 끌기가 없다
		card->SetPosition(HVector2(100.0f, 100.0f));
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 530.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 153.0f, 533.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 153.0f, 533.0f), GTarget);
		check(plainClicks == 1 && log.Begins == 2, "drag: a short press on a button is still a click");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 530.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 450.0f, 800.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 450.0f, 800.0f), GTarget);
		check(plainClicks == 1 && log.Begins == 2, "drag: a button without SetDraggable does not drag");

		// 끌 수 있는 카드 안 버튼: 그냥 누르면 클릭, 끌면 카드가 끌리고 버튼 누름은 풀린다(뗌이 클릭이 아니다)
		click(manager, 150.0f, 210.0f);
		check(cardButtonClicks == 1 && log.Begins == 2, "drag: a click on a button inside a draggable card is still a click");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 210.0f), GTarget);
		check(cardButton->IsPressed(), "drag: the inner button is pressed");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 170.0f, 210.0f), GTarget);
		check(log.Begins == 3 && log.Element.GetRawPointer() == card.GetRawPointer() && cardButton->IsPressed() == false, "drag: pressing a child drags the nearest draggable ancestor and releases the press");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 170.0f, 210.0f), GTarget);
		check(cardButtonClicks == 1 && log.Ends == 3 && log.bCanceled == false, "drag: the release after a drag is not a click");

		// 꺼진 요소 · 오른쪽 버튼은 끌리지 않는다
		card->SetPosition(HVector2(100.0f, 100.0f));
		card->SetEnabled(false);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 200.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 200.0f, 150.0f), GTarget);
		check(log.Begins == 3, "drag: a disabled draggable element does not drag");
		card->SetEnabled(true);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f, EGameUIPointerButton::Right), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 200.0f, 150.0f), GTarget);
		check(log.Begins == 3 && manager->IsPointerDown(EGameUIPointerButton::Right) && manager->IsPointerDown(left) == false, "drag: only the left button drags (the right press is tracked)");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 200.0f, 150.0f, EGameUIPointerButton::Right), GTarget);
		check(manager->IsPointerDown(EGameUIPointerButton::Right) == false, "drag: the right release is tracked");

		// (d) 취소: 스택이 바뀜(위젯 훅보다 먼저) · Menu 위젯 · CancelDrag · 뗌을 놓친 채 다시 누름. 취소 뒤 뗌은 끝을 또 주지 않는다
		events.clear();
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 170.0f, 150.0f), GTarget);
		PSharedPtr<PUITestWidget> next = makeTestWidget(events, "N");
		manager->PushWidgetInstance(next, EGameUILayer::Game);
		check(log.Ends == 4 && log.bCanceled && log.DropTarget.IsValid() == false && manager->GetDraggedElement().IsValid() == false, "drag: a widget pushed on the same layer cancels the drag");
		check(joinEvents(events) == PString("hook:begin drag:begin hook:move drag:move hook:cancel drag:cancel N:init H:deactivate N:activate"), "drag: the cancel comes before the widget hooks");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 170.0f, 150.0f), GTarget);
		check(log.Ends == 4, "drag: the release after a cancel does not end it again");
		next->DeactivateWidget();

		card->SetPosition(HVector2(100.0f, 100.0f));
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 170.0f, 150.0f), GTarget);
		PSharedPtr<PUITestWidget> menu = makeTestWidget(events, "M");
		menu->SetInputMode(EGameWidgetInputMode::Menu);
		manager->PushWidgetInstance(menu, EGameUILayer::Menu);
		check(log.Begins == 5 && log.Ends == 5 && log.bCanceled, "drag: a Menu widget on top cancels the drag");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 170.0f, 150.0f), GTarget);
		menu->DeactivateWidget();

		card->SetPosition(HVector2(100.0f, 100.0f));
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 170.0f, 150.0f), GTarget);
		manager->CancelDrag();
		check(log.Begins == 6 && log.Ends == 6 && log.bCanceled && manager->GetDraggedElement().IsValid() == false, "drag: CancelDrag cancels the drag");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 170.0f, 150.0f), GTarget);
		check(log.Ends == 6, "drag: a release after CancelDrag does not end it again");

		card->SetPosition(HVector2(100.0f, 100.0f));
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 170.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 530.0f), GTarget);
		check(log.Begins == 7 && log.Ends == 7 && log.bCanceled && manager->GetDraggedElement().IsValid() == false && plain->IsPressed(), "drag: a new left press while dragging (release lost) cancels the drag and presses as usual");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 150.0f, 530.0f), GTarget);
		check(plainClicks == 2, "drag: that new press clicks as usual");

		// Leave 는 호버만 푼다(끌기는 이어진다). 밖에서 떼면 놓은 곳 없이 끝난다 — 에디터 뷰포트는 밖에서 뗀 프레임에 Leave 다음 뗌을 보낸다
		card->SetPosition(HVector2(100.0f, 100.0f));
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 170.0f, 150.0f), GTarget);
		check(manager->IsPointerInside(), "drag: the pointer is inside after a move");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Leave, 0.0f, 0.0f), GTarget);
		check(log.Ends == 7 && manager->GetDraggedElement().GetRawPointer() == card.GetRawPointer() && manager->IsPointerInside() == false, "drag: Leave keeps the drag going");
		check(manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 1200.0f, 150.0f), GTarget), "drag: a release outside the target is consumed");
		check(log.Begins == 8 && log.Ends == 8 && log.bCanceled == false && log.DropTarget.IsValid() == false && isPoint(log.EndPosition, HVector2(1200.0f, 150.0f)), "drag: a release outside after Leave ends the drag with no drop target");

		// (e) 배율 0.5: 문턱 · 자리는 논리 단위(타깃 +2px = 논리 +4 는 문턱 안, +4px = 논리 +8 은 시작)
		card->SetPosition(HVector2(100.0f, 100.0f));
		const HVector2 halfTarget(500.0f, 500.0f);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 75.0f, 75.0f), halfTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 77.0f, 75.0f), halfTarget);
		check(log.Begins == 8, "drag: the threshold is in logical units (scale 0.5, +4 logical)");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 79.0f, 75.0f), halfTarget);
		check(log.Begins == 9 && isPoint(log.BeginPosition, HVector2(150.0f, 150.0f)) && isPoint(log.MovePosition, HVector2(158.0f, 150.0f)), "drag: begin and move positions are logical at scale 0.5");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 79.0f, 75.0f), halfTarget);
		check(log.Ends == 9 && log.bCanceled == false && isPoint(log.EndPosition, HVector2(158.0f, 150.0f)), "drag: end position is logical at scale 0.5");

		manager->SetDragThreshold(-3.0f);
		check(nearlyEqual(manager->GetDragThreshold(), 0.0f), "drag: a negative threshold becomes 0");
		manager->SetDragThreshold(6.0f);

		// 끄는 중 종료: 취소로 끝난다
		card->SetPosition(HVector2(100.0f, 100.0f));
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 150.0f, 150.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 170.0f, 150.0f), GTarget);
		manager->Shutdown();
		check(log.Begins == 10 && log.Ends == 10 && log.bCanceled, "drag: Shutdown cancels the drag");
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

	// 요청서(ER-017) 완료 기준 (a)~(d) + 이징 · 되풀이 · 왕복 · 지연 · 시간 배율 · 겹치기 · 덮인 위젯 멈춤 · 빠질 때 취소 · 게임 곡선(GG-16)
	void testTween()
	{
		// 이징: 모든 곡선이 0 에서 0, 1 에서 1. Back · Elastic 은 넘친다. 범위 밖 t 는 자른다
		bool bEndpoints = true;
		for (int32 ease = 0; ease < (int32)EGameUIEase::Count; ++ease)
		{
			const float32 start = HGameUIEase::Evaluate((EGameUIEase)ease, 0.0f);
			const float32 end   = HGameUIEase::Evaluate((EGameUIEase)ease, 1.0f);
			if (nearlyEqual(start, 0.0f, 0.0001f) == false || nearlyEqual(end, 1.0f, 0.0001f) == false)
			{
				bEndpoints = false;
			}
		}
		check(bEndpoints, "tween: every ease starts at 0 and ends at 1");
		float32 outBackMax    = 0.0f;
		float32 inBackMin     = 0.0f;
		float32 outElasticMax = 0.0f;
		for (int32 i = 1; i < 100; ++i)
		{
			const float32 t = (float32)i / 100.0f;
			outBackMax    = HMath::Max(outBackMax, HGameUIEase::Evaluate(EGameUIEase::OutBack, t));
			inBackMin     = HMath::Min(inBackMin, HGameUIEase::Evaluate(EGameUIEase::InBack, t));
			outElasticMax = HMath::Max(outElasticMax, HGameUIEase::Evaluate(EGameUIEase::OutElastic, t));
		}
		check(outBackMax > 1.05f && inBackMin < -0.05f && outElasticMax > 1.05f, "tween: Back and Elastic overshoot");
		check(nearlyEqual(HGameUIEase::Evaluate(EGameUIEase::InOutQuad, 0.5f), 0.5f) && nearlyEqual(HGameUIEase::Evaluate(EGameUIEase::InQuad, 0.5f), 0.25f)
			&& nearlyEqual(HGameUIEase::Evaluate(EGameUIEase::Linear, 2.0f), 1.0f) && nearlyEqual(HGameUIEase::Evaluate(EGameUIEase::Linear, -1.0f), 0.0f),
			"tween: ease values and clamping");

		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIImage>   card    = widget->AddChild<PGameUIImage>(PName("Card"));
		card->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.5f, 0.5f), HVector2(400.0f, 400.0f), HVector2(200.0f, 300.0f));

		// (a) 0.3초 · 배율 1 → 1.3 · 불투명도 1 → 0 · InOutQuad: 0.15초에 1.15 · 0.5, 0.3초에 끝값 · 끝 알림 한 번
		HTimelineLog logA;
		PSharedPtr<PGameUITimeline> pop = PGameUITimeline::Parallel();
		pop->Add(HGameUITween::To(card, EGameUITweenProperty::Scale, HVector2(1.3f, 1.3f), 0.3f, EGameUIEase::InOutQuad))
			.Add(HGameUITween::To(card, EGameUITweenProperty::Opacity, 0.0f, 0.3f, EGameUIEase::InOutQuad));
		watch(pop, logA);
		widget->PlayTimeline(pop);
		manager->Update(0.15f);
		check(nearlyEqual(card->GetRenderTransform().Scale.x, 1.15f) && nearlyEqual(card->GetOpacity(), 0.5f) && pop->IsPlaying() && logA.Finished == 0,
			"accept ER-017 (a): halfway values at 0.15 s");
		manager->Update(0.15f);
		check(nearlyEqual(card->GetRenderTransform().Scale.x, 1.3f) && nearlyEqual(card->GetOpacity(), 0.0f) && pop->IsFinished() && logA.Finished == 1 && logA.Canceled == 0,
			"accept ER-017 (a): end values and one finished event at 0.3 s");
		manager->Update(0.1f);
		check(logA.Finished == 1, "accept ER-017 (a): the finished event comes once");

		// (b) 차례(A 0.2 → 기다리기 0.1 → B 0.2)는 0.5초에 끝나고 함수는 순서대로 불린다
		HList<PString> calls;
		HTimelineLog   logB;
		PSharedPtr<PGameUITimeline> flight = PGameUITimeline::Sequence();
		flight->Call([&calls]()
				{
					calls.push_back(PString("start"));
				})
			.Add(HGameUITween::FromTo(card, EGameUITweenProperty::Translation, HVector2(-100.0f, 0.0f), HVector2(0.0f, 0.0f), 0.2f, EGameUIEase::Linear))
			.Call([&calls]()
				{
					calls.push_back(PString("A"));
				})
			.Wait(0.1f)
			.Call([&calls]()
				{
					calls.push_back(PString("wait"));
				})
			.Add(HGameUITween::FromTo(card, EGameUITweenProperty::Angle, 0.0f, 90.0f, 0.2f, EGameUIEase::Linear))
			.Call([&calls]()
				{
					calls.push_back(PString("B"));
				});
		watch(flight, logB);
		check(nearlyEqual(flight->GetDuration(), 0.5f), "tween: a sequence lasts as long as its steps together");
		widget->PlayTimeline(flight);
		check(joinEvents(calls) == PString("start") && nearlyEqual(card->GetRenderTransform().Translation.x, -100.0f), "tween: playing applies the first step at once");
		manager->Update(0.1f);
		check(nearlyEqual(card->GetRenderTransform().Translation.x, -50.0f), "tween: translation moves in a sequence");
		manager->Update(0.1f);
		check(joinEvents(calls) == PString("start A"), "accept ER-017 (b): the call after A comes when A ends (0.2 s)");
		manager->Update(0.1f);
		check(joinEvents(calls) == PString("start A wait"), "accept ER-017 (b): the wait ends at 0.3 s");
		manager->Update(0.1f);
		check(nearlyEqual(card->GetRenderTransform().Angle, 45.0f) && flight->IsPlaying(), "accept ER-017 (b): B runs after the wait");
		manager->Update(0.1f);
		check(joinEvents(calls) == PString("start A wait B") && flight->IsFinished() && logB.Finished == 1 && logB.Canceled == 0 && nearlyEqual(card->GetRenderTransform().Angle, 90.0f),
			"accept ER-017 (b): the sequence ends at 0.5 s with the calls in order");

		// (b) 동시 묶음은 가장 긴 단계와 함께 끝난다
		HTimelineLog logP;
		PSharedPtr<PGameUITimeline> together = PGameUITimeline::Parallel();
		together->Add(HGameUITween::To(card, EGameUITweenProperty::Position, HVector2(400.0f, 300.0f), 0.2f, EGameUIEase::Linear))
			.Add(HGameUITween::To(card, EGameUITweenProperty::Size, HVector2(100.0f, 150.0f), 0.5f, EGameUIEase::Linear))
			.Wait(0.3f);
		watch(together, logP);
		check(nearlyEqual(together->GetDuration(), 0.5f), "tween: a parallel timeline lasts as long as its longest step");
		widget->PlayTimeline(together);
		manager->Update(0.25f);
		check(nearlyEqual(card->GetPosition().y, 300.0f) && nearlyEqual(card->GetSize().x, 150.0f) && together->IsPlaying(), "tween: parallel steps run together");
		manager->Update(0.2f);
		check(together->IsPlaying() && logP.Finished == 0, "accept ER-017 (b): a parallel timeline waits for its longest step");
		manager->Update(0.1f);
		check(together->IsFinished() && logP.Finished == 1 && nearlyEqual(card->GetSize().y, 150.0f), "accept ER-017 (b): a parallel timeline ends with its longest step");

		// (c) 도중에 대상 요소를 지우면 오류 없이 취소로 끝난다
		PSharedPtr<PGameUIImage> doomed = widget->AddChild<PGameUIImage>(PName("Doomed"));
		HTimelineLog logC;
		PSharedPtr<PGameUITimeline> doomedFade = widget->PlayTween(HGameUITween::To(doomed, EGameUITweenProperty::Opacity, 0.0f, 1.0f));
		watch(doomedFade, logC);
		manager->Update(0.3f);
		widget->GetRoot()->RemoveChild(doomed);
		doomed = nullptr;
		manager->Update(0.1f);
		check(doomedFade->IsFinished() && logC.Finished == 1 && logC.Canceled == 1, "accept ER-017 (c): a vanished target ends the timeline as canceled");

		// (c) 같은 속성에 다른 묶음의 트윈이 시작되면 옛 묶음은 취소로 끝나고, 새 트윈은 지금 값에서 시작한다
		card->SetOpacity(1.0f);
		HTimelineLog logOld;
		HTimelineLog logNew;
		PSharedPtr<PGameUITimeline> oldFade = widget->PlayTween(HGameUITween::To(card, EGameUITweenProperty::Opacity, 0.0f, 1.0f, EGameUIEase::Linear));
		watch(oldFade, logOld);
		manager->Update(0.5f);
		PSharedPtr<PGameUITimeline> newFade = widget->PlayTween(HGameUITween::To(card, EGameUITweenProperty::Opacity, 1.0f, 0.5f, EGameUIEase::Linear));
		watch(newFade, logNew);
		manager->Update(0.25f);
		check(logOld.Finished == 1 && logOld.Canceled == 1 && nearlyEqual(card->GetOpacity(), 0.75f) && newFade->IsPlaying(),
			"accept ER-017 (c): a new tween on the same property replaces the old timeline and starts from the current value");
		manager->Update(0.25f);
		check(logNew.Finished == 1 && logNew.Canceled == 0 && nearlyEqual(card->GetOpacity(), 1.0f), "tween: the replacing tween finishes normally");

		// 같은 묶음 안의 단계끼리는 막지 않는다(되풀이 · 자식 묶음)
		HTimelineLog logBlink;
		PSharedPtr<PGameUITimeline> blink = PGameUITimeline::Sequence();
		blink->Add(HGameUITween::To(card, EGameUITweenProperty::Opacity, 0.0f, 0.1f, EGameUIEase::Linear))
			.Add(HGameUITween::To(card, EGameUITweenProperty::Opacity, 1.0f, 0.1f, EGameUIEase::Linear))
			.SetLoops(3);
		watch(blink, logBlink);
		widget->PlayTimeline(blink);
		for (int32 frame = 0; frame < 7; ++frame)
		{
			manager->Update(0.1f);
		}
		check(logBlink.Finished == 1 && logBlink.Canceled == 0 && nearlyEqual(card->GetOpacity(), 1.0f), "tween: steps of one timeline on the same property do not cancel each other");
		HTimelineLog logShared;
		PSharedPtr<PGameUITimeline> inner = PGameUITimeline::Sequence();
		inner->Add(HGameUITween::FromTo(card, EGameUITweenProperty::Opacity, 1.0f, 0.0f, 0.1f, EGameUIEase::Linear));
		PSharedPtr<PGameUITimeline> outer = PGameUITimeline::Parallel();
		outer->Add(inner).Add(HGameUITween::FromTo(card, EGameUITweenProperty::Opacity, 0.0f, 1.0f, 0.3f, EGameUIEase::Linear));
		watch(outer, logShared);
		widget->PlayTimeline(outer);
		for (int32 frame = 0; frame < 4; ++frame)
		{
			manager->Update(0.1f);
		}
		check(logShared.Finished == 1 && logShared.Canceled == 0 && nearlyEqual(card->GetOpacity(), 1.0f), "tween: a finished child timeline does not take a property from its parent");

		// (d) 끝으로 건너뛰기: 남은 트윈은 끝값, 남은 함수는 순서대로, 끝 알림 한 번(취소 아님)
		calls.clear();
		HTimelineLog logSkip;
		PSharedPtr<PGameUITimeline> intro = PGameUITimeline::Sequence();
		intro->Add(HGameUITween::FromTo(card, EGameUITweenProperty::Position, HVector2(0.0f, 0.0f), HVector2(200.0f, 0.0f), 1.0f, EGameUIEase::Linear))
			.Call([&calls]()
				{
					calls.push_back(PString("x"));
				})
			.Wait(1.0f)
			.Add(HGameUITween::To(card, EGameUITweenProperty::Angle, 30.0f, 1.0f))
			.Call([&calls]()
				{
					calls.push_back(PString("y"));
				});
		watch(intro, logSkip);
		widget->PlayTimeline(intro);
		manager->Update(0.2f);
		intro->Skip();
		check(nearlyEqual(card->GetPosition().x, 200.0f) && nearlyEqual(card->GetRenderTransform().Angle, 30.0f) && joinEvents(calls) == PString("x y")
			&& intro->IsFinished() && logSkip.Finished == 1 && logSkip.Canceled == 0,
			"accept ER-017 (d): Skip jumps to the end values, runs the remaining calls in order and finishes once");
		intro->Skip();
		manager->Update(0.1f);
		check(logSkip.Finished == 1, "accept ER-017 (d): Skip after the end does nothing");

		// 멈추기: 값은 그 자리, 남은 단계 · 함수는 하지 않고 취소로 끝
		calls.clear();
		HTimelineLog logStop;
		PSharedPtr<PGameUITimeline> slide = PGameUITimeline::Sequence();
		slide->Add(HGameUITween::FromTo(card, EGameUITweenProperty::Position, HVector2(0.0f, 0.0f), HVector2(100.0f, 0.0f), 1.0f, EGameUIEase::Linear))
			.Call([&calls]()
				{
					calls.push_back(PString("never"));
				});
		watch(slide, logStop);
		widget->PlayTimeline(slide);
		manager->Update(0.5f);
		slide->Stop();
		manager->Update(0.5f);
		check(nearlyEqual(card->GetPosition().x, 50.0f) && calls.empty() && logStop.Finished == 1 && logStop.Canceled == 1, "tween: Stop leaves the value in place, skips the rest and finishes as canceled");

		// 되풀이 · 왕복 · 끝없는 되풀이의 건너뛰기
		int32 laps = 0;
		HTimelineLog logLoop;
		PSharedPtr<PGameUITimeline> loop = PGameUITimeline::Sequence();
		loop->Add(HGameUITween::FromTo(card, EGameUITweenProperty::Opacity, 0.0f, 1.0f, 0.1f, EGameUIEase::Linear))
			.Call([&laps]()
				{
					++laps;
				})
			.SetLoops(3);
		watch(loop, logLoop);
		widget->PlayTimeline(loop);
		manager->Update(0.25f);
		check(laps == 2 && loop->IsPlaying() && nearlyEqual(card->GetOpacity(), 0.5f), "tween: loops repeat the steps");
		manager->Update(0.1f);
		check(laps == 3 && loop->IsFinished() && logLoop.Finished == 1, "tween: a timeline with 3 loops ends after the third pass");
		HTimelineLog logPulse;
		PSharedPtr<PGameUITimeline> pulse = PGameUITimeline::Sequence();
		pulse->Add(HGameUITween::FromTo(card, EGameUITweenProperty::Scale, HVector2(1.0f, 1.0f), HVector2(2.0f, 2.0f), 1.0f, EGameUIEase::Linear)).SetLoops(0, true);
		watch(pulse, logPulse);
		widget->PlayTimeline(pulse);
		manager->Update(1.25f);
		check(pulse->IsPlaying() && nearlyEqual(card->GetRenderTransform().Scale.x, 1.75f), "tween: a yoyo loop plays backwards on the second pass");
		manager->Update(1.25f);
		check(pulse->IsPlaying() && nearlyEqual(card->GetRenderTransform().Scale.x, 1.5f), "tween: an endless yoyo keeps going");
		pulse->Skip();
		check(pulse->IsFinished() && logPulse.Finished == 1 && logPulse.Canceled == 0 && nearlyEqual(card->GetRenderTransform().Scale.x, 2.0f),
			"tween: Skip ends an endless loop at the end of the current pass");

		// 지연: 그동안 움직이지 않고, 시작값이 없으면 실제로 시작하는 순간의 값에서 시작한다
		card->SetOpacity(1.0f);
		PSharedPtr<PGameUITimeline> late = widget->PlayTween(HGameUITween::To(card, EGameUITweenProperty::Opacity, 0.0f, 0.2f, EGameUIEase::Linear).SetDelay(0.1f));
		manager->Update(0.05f);
		check(nearlyEqual(card->GetOpacity(), 1.0f), "tween: nothing moves during the delay");
		card->SetOpacity(0.6f);
		manager->Update(0.15f);
		check(nearlyEqual(card->GetOpacity(), 0.3f), "tween: without a From value a tween starts from the value when it really starts");
		late->Skip();

		// 시간 배율(관리자): 묶음만 느려지거나 멈춘다
		manager->SetTweenTimeScale(0.5f);
		PSharedPtr<PGameUITimeline> slow = widget->PlayTween(HGameUITween::FromTo(card, EGameUITweenProperty::Position, HVector2(0.0f, 0.0f), HVector2(100.0f, 0.0f), 1.0f, EGameUIEase::Linear));
		manager->Update(0.5f);
		check(nearlyEqual(card->GetPosition().x, 25.0f), "tween: the manager time scale slows timelines");
		manager->SetTweenTimeScale(0.0f);
		manager->Update(0.5f);
		check(nearlyEqual(card->GetPosition().x, 25.0f) && slow->IsPlaying(), "tween: time scale 0 freezes timelines");
		manager->SetTweenTimeScale(-1.0f);
		check(manager->GetTweenTimeScale() == 0.0f, "tween: a negative time scale is 0");
		manager->SetTweenTimeScale(1.0f);
		slow->Skip();

		// 게임 곡선: 끝값을 넘었다가 돌아와도 된다
		PSharedPtr<PGameUITimeline> bounce = widget->PlayTween(HGameUITween::FromTo(card, EGameUITweenProperty::Position, HVector2(0.0f, 0.0f), HVector2(100.0f, 0.0f), 1.0f)
			.SetCurve(&overshootCurve));
		manager->Update(0.75f);
		check(nearlyEqual(card->GetPosition().x, 112.5f), "tween: a game curve can overshoot");
		manager->Update(0.25f);
		check(nearlyEqual(card->GetPosition().x, 100.0f) && bounce->IsFinished(), "tween: a game curve ends at its end value");

		// 색: 그림은 자기 색, 정하지 않은 글은 관리자 기본 글자 색에서 시작한다. 색이 없는 요소(버튼)는 그대로
		HGameUITextStyle textStyle;
		textStyle.Color = HLinearColor(0.2f, 0.4f, 0.6f, 1.0f);
		manager->SetDefaultTextStyle(textStyle);
		PSharedPtr<PGameUIText> label = widget->AddChild<PGameUIText>(PName("Label"));
		check(isColor(label->GetColor(manager.GetRawPointer()), textStyle.Color), "tween: a text element reads its color (the manager default when it has none)");
		widget->PlayTween(HGameUITween::To(label, EGameUITweenProperty::Color, HLinearColor(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, EGameUIEase::Linear));
		card->SetColor(HLinearColor(0.0f, 0.0f, 1.0f, 1.0f));
		widget->PlayTween(HGameUITween::To(card, EGameUITweenProperty::Color, HLinearColor(1.0f, 0.0f, 0.0f, 0.0f), 1.0f, EGameUIEase::Linear));
		manager->Update(0.5f);
		check(isColor(label->GetColor(), HLinearColor(0.6f, 0.2f, 0.3f, 1.0f)) && isColor(card->GetColor(), HLinearColor(0.5f, 0.0f, 0.5f, 0.5f)), "tween: color tweens on text and images");
		PSharedPtr<PGameUIButton> plainButton = widget->AddChild<PGameUIButton>(PName("PlainButton"));
		PSharedPtr<PGameUITimeline> tint = widget->PlayTween(HGameUITween::To(plainButton, EGameUITweenProperty::Color, HLinearColor(1.0f, 0.0f, 0.0f, 1.0f), 0.1f));
		manager->Update(0.6f);
		check(tint->IsFinished() && plainButton->HasStyle() == false, "tween: a color tween on an element without a color does nothing");

		// 끝 알림 처리기에서 다음 묶음을 시작해도 된다(다음 진행부터 돈다). 재생 중에는 단계를 더하지 않는다
		PSharedPtr<PGameUITimeline> first = widget->PlayTween(HGameUITween::To(card, EGameUITweenProperty::Angle, 10.0f, 0.1f));
		PSharedPtr<PGameUITimeline> second;
		first->OnFinished.AddLambda([&second, &widget, &card](bool)
			{
				second = widget->PlayTween(HGameUITween::To(card, EGameUITweenProperty::Angle, 20.0f, 0.1f));
			});
		manager->Update(0.15f);
		check(first->IsFinished() && second.IsValid() && second->IsPlaying(), "tween: a finished handler can start another timeline");
		manager->Update(0.1f);
		check(second->IsFinished() && nearlyEqual(card->GetRenderTransform().Angle, 20.0f), "tween: the chained timeline runs from the next update");
		PSharedPtr<PGameUITimeline> running = widget->PlayTween(HGameUITween::To(card, EGameUITweenProperty::Angle, 0.0f, 1.0f));
		running->Wait(5.0f);
		check(nearlyEqual(running->GetDuration(), 1.0f) && widget->PlayTimeline(running).GetRawPointer() == running.GetRawPointer() && running->IsPlaying(),
			"tween: steps are not added while playing, playing again keeps it going");
		running->Stop();

		// 덮인 위젯의 묶음은 멈추고, 위젯이 빠지면 OnShutdown 전에 취소된다
		HList<PString> events;
		PSharedPtr<PUITestWidget> owner = makeTestWidget(events, "W");
		manager->PushWidgetInstance(owner, EGameUILayer::Menu);
		PSharedPtr<PGameUIImage> badge = owner->AddChild<PGameUIImage>(PName("Badge"));
		PSharedPtr<PGameUITimeline> spin = owner->PlayTween(HGameUITween::FromTo(badge, EGameUITweenProperty::Angle, 0.0f, 100.0f, 1.0f, EGameUIEase::Linear));
		spin->OnFinished.AddLambda([&events](bool bCanceled)
			{
				events.push_back(PString(bCanceled ? "spin:canceled" : "spin:done"));
			});
		manager->Update(0.25f);
		PSharedPtr<PUITestWidget> cover = makeTestWidget(events, "C");
		manager->PushWidgetInstance(cover, EGameUILayer::Menu);
		manager->Update(0.5f);
		check(nearlyEqual(badge->GetRenderTransform().Angle, 25.0f), "tween: a covered widget's timelines pause");
		cover->DeactivateWidget();
		manager->Update(0.25f);
		check(nearlyEqual(badge->GetRenderTransform().Angle, 50.0f), "tween: they resume when the widget is on top again");
		events.clear();
		owner->DeactivateWidget();
		check(joinEvents(events) == PString("W:deactivate spin:canceled W:shutdown") && spin->IsFinished(), "tween: removing the widget cancels its timelines before OnShutdown");

		manager->Shutdown();
	}

	// GG-16(ER-017 나): 위젯 열림 · 닫힘 전환. 닫히는 위젯은 스택에서 바로 빠지고(입력 · 뒤로가기 없음) 전환이 끝날 때까지 그 레이어 맨 위에 그려진다
	void testTransition()
	{
		HList<PString> events;
		PSharedPtr<PGameUIManager> manager = makeManager();
		const HLinearColor hudColor(0.1f, 0.9f, 0.1f, 1.0f);
		const HLinearColor hud2Color(0.1f, 0.9f, 0.9f, 1.0f);
		const HLinearColor menuColor(0.9f, 0.1f, 0.1f, 1.0f);
		const HLinearColor dialogColor(0.1f, 0.1f, 0.9f, 1.0f);
		const HLinearColor dialog2Color(0.9f, 0.9f, 0.1f, 1.0f);
		const HRect        menuRect(300.0f, 300.0f, 700.0f, 700.0f);

		// (e) 전환이 없는 위젯은 지금까지와 같다: 바로 나타나고, 빼면 바로 OnShutdown
		PSharedPtr<PUITestWidget> hud = makeTestWidget(events, "H");
		manager->PushWidgetInstance(hud, EGameUILayer::Game);
		addPanel(hud, "HudPanel", hudColor, HRect(0.0f, 0.0f, 100.0f, 100.0f));
		check(nearlyEqual(drawnAlpha(manager, hudColor), 1.0f) && hud->GetOpenTransition().IsEnabled() == false, "transition: a widget without transitions appears at once");
		PSharedPtr<PUITestWidget> plain = makeTestWidget(events, "P");
		manager->PushWidgetInstance(plain, EGameUILayer::Menu);
		events.clear();
		plain->DeactivateWidget();
		check(joinEvents(events) == PString("P:deactivate P:shutdown") && plain->IsClosing() == false && plain->IsInStack() == false && plain->bClosingAtDeactivate == false,
			"accept ER-017 (e): without a close transition a widget shuts down at once");

		// 열림: 첫 그리기부터 닫힌 모습(불투명도 0 · 배율 0.8)에서 시작해 열린 모습으로. 배율은 화면 가운데 기준. 열리는 동안 입력은 받는다
		PSharedPtr<PUITestButtonWidget> menu = makeButtonWidget(HRect(400.0f, 400.0f, 600.0f, 500.0f));
		menu->Events = &events;
		menu->Tag    = PString("M");
		menu->SetInputMode(EGameWidgetInputMode::Menu);
		menu->SetBackHandler(true);
		menu->SetOpenTransition(HGameUITransition::FadeScale(0.2f, 0.8f, EGameUIEase::Linear));
		menu->SetCloseTransition(HGameUITransition::Fade(0.2f, EGameUIEase::Linear));
		events.clear();
		manager->PushWidgetInstance(menu, EGameUILayer::Menu);
		addPanel(menu, "MenuPanel", menuColor, menuRect);
		check(joinEvents(events) == PString("M:init M:activate") && nearlyEqual(menu->GetRoot()->GetOpacity(), 0.0f) && nearlyEqual(menu->GetRoot()->GetRenderTransform().Scale.x, 0.8f)
			&& nearlyEqual(drawnAlpha(manager, menuColor), 0.0f), "accept transition: open starts from the closed look on the first draw");
		manager->Update(0.1f);
		HGameUIDrawList halfOpen;
		manager->BuildDrawList(GTarget, halfOpen);
		const int32 halfOpenQuad = findQuad(halfOpen, menuColor);
		check(halfOpenQuad >= 0 && nearlyEqual(halfOpen.Vertices[(size_t)halfOpenQuad].Color.A, 0.5f) && nearlyEqual(halfOpen.Vertices[(size_t)halfOpenQuad].Position.x, 320.0f)
			&& nearlyEqual(halfOpen.Vertices[(size_t)halfOpenQuad].Position.y, 320.0f), "transition: halfway open (alpha 0.5, scale 0.9 around the screen center)");
		check(click(manager, 450.0f, 450.0f) && menu->Clicks == 1, "transition: an opening widget takes input");
		manager->Update(0.1f);
		check(nearlyEqual(menu->GetRoot()->GetOpacity(), 1.0f) && nearlyEqual(menu->GetRoot()->GetRenderTransform().Scale.x, 1.0f) && nearlyEqual(drawnAlpha(manager, menuColor), 1.0f),
			"accept transition: open ends at the open look");

		// 같은 스택: 맨 위가 열리는 동안 바로 아래 위젯도 그린다(덮여 멈춘 채). 끝나면 맨 위만
		PSharedPtr<PUITestWidget> hud2 = makeTestWidget(events, "H2");
		hud2->SetOpenTransition(HGameUITransition::Fade(0.2f, EGameUIEase::Linear));
		manager->PushWidgetInstance(hud2, EGameUILayer::Game);
		addPanel(hud2, "Hud2Panel", hud2Color, HRect(0.0f, 0.0f, 100.0f, 100.0f));
		manager->Update(0.1f);
		check(nearlyEqual(drawnAlpha(manager, hudColor), 1.0f) && nearlyEqual(drawnAlpha(manager, hud2Color), 0.5f) && hud->IsActive() == false,
			"accept transition: while the top opens the widget below it is still drawn (inactive)");
		manager->Update(0.1f);
		check(drawnAlpha(manager, hudColor) < 0.0f && nearlyEqual(drawnAlpha(manager, hud2Color), 1.0f), "transition: after the open transition only the top is drawn");
		hud2->DeactivateWidget();

		// 닫힘: 스택에서 바로 빠지고(OnDeactivated, 아래 위젯이 활성) 전환이 끝날 때까지 그린 뒤 OnShutdown. 입력 · 뒤로가기는 받지 않는다
		events.clear();
		menu->DeactivateWidget();
		check(joinEvents(events) == PString("M:deactivate") && menu->IsClosing() && menu->IsInStack() == false && menu->IsActive() == false
			&& manager->GetTopWidget(EGameUILayer::Menu) == nullptr && manager->FindWidget<PUITestButtonWidget>() == nullptr && manager->GetWidgetCount() == 1,
			"accept transition: a closing widget leaves the stack at once (OnDeactivated, OnShutdown later)");
		check(menu->bClosingAtDeactivate && hud->bClosingAtDeactivate == false, "transition: IsClosing() is already true in OnDeactivated of a closing widget (false when covered)");
		check(nearlyEqual(drawnAlpha(manager, menuColor), 1.0f), "accept transition: a closing widget is still drawn");
		check(click(manager, 450.0f, 450.0f) == false && menu->Clicks == 1 && manager->IsWorldInputBlocked() == false, "accept transition: a closing widget takes no input (it goes to the world)");
		check(manager->HandleBackAction() == false, "accept transition: a closing widget takes no back action");
		manager->Update(0.1f);
		check(nearlyEqual(drawnAlpha(manager, menuColor), 0.5f) && menu->IsClosing(), "transition: halfway closed");
		events.clear();
		manager->Update(0.1f);
		check(joinEvents(events) == PString("M:shutdown") && menu->IsClosing() == false && drawnAlpha(manager, menuColor) < 0.0f, "accept transition: OnShutdown when the close transition ends");
		check(nearlyEqual(menu->GetRoot()->GetOpacity(), 1.0f) && menu->GetRoot()->GetChildren().empty() && menu->GetManager() == nullptr, "transition: a closed widget's root look and tree are reset");

		// 닫히는 중에 다시 Push: 지금 모습에서 다시 열린다(OnInitialize · OnShutdown 없음)
		manager->PushWidgetInstance(menu, EGameUILayer::Menu);
		addPanel(menu, "MenuPanel", menuColor, menuRect);
		manager->Update(0.2f);
		menu->DeactivateWidget();
		manager->Update(0.1f);
		events.clear();
		check(manager->PushWidgetInstance(menu, EGameUILayer::Menu) && joinEvents(events) == PString("M:activate") && menu->IsClosing() == false && menu->IsInStack()
			&& nearlyEqual(menu->GetRoot()->GetOpacity(), 0.5f), "accept transition: pushing a closing widget reopens it from its current look without OnInitialize");
		manager->Update(0.1f);
		check(nearlyEqual(menu->GetRoot()->GetOpacity(), 0.75f), "transition: the reopen transition runs from the current look");
		events.clear();
		manager->Update(1.0f);
		check(events.empty() && nearlyEqual(menu->GetRoot()->GetOpacity(), 1.0f) && menu->IsActive() && nearlyEqual(drawnAlpha(manager, menuColor), 1.0f),
			"accept transition: a reopened widget is not shut down by its old close transition");

		// 열리는 중에 닫기: 지금 모습에서 닫힌다
		menu->DeactivateWidget();
		manager->Update(0.5f);
		manager->PushWidgetInstance(menu, EGameUILayer::Menu);
		addPanel(menu, "MenuPanel", menuColor, menuRect);
		manager->Update(0.1f);
		menu->DeactivateWidget();
		check(menu->IsClosing() && nearlyEqual(menu->GetRoot()->GetOpacity(), 0.5f), "accept transition: closing during the open transition starts from the current look");
		manager->Update(0.1f);
		check(nearlyEqual(menu->GetRoot()->GetOpacity(), 0.25f) && menu->IsClosing(), "transition: closing from the current look");
		events.clear();
		manager->Update(0.1f);
		check(joinEvents(events) == PString("M:shutdown"), "transition: a widget closed while opening shuts down after the close transition");

		// 닫히는 위젯의 묶음은 계속 돈다. 시간 배율도 전환에 든다
		manager->PushWidgetInstance(menu, EGameUILayer::Menu);
		PSharedPtr<PGameUIImage> menuPanel = addPanel(menu, "MenuPanel", menuColor, menuRect);
		manager->Update(0.2f);
		menu->PlayTween(HGameUITween::FromTo(menuPanel, EGameUITweenProperty::Angle, 0.0f, 100.0f, 1.0f, EGameUIEase::Linear));
		menu->DeactivateWidget();
		manager->SetTweenTimeScale(0.5f);
		manager->Update(0.2f);
		check(nearlyEqual(menuPanel->GetRenderTransform().Angle, 10.0f) && nearlyEqual(menu->GetRoot()->GetOpacity(), 0.5f), "transition: timelines run while closing, with the time scale");
		manager->SetTweenTimeScale(1.0f);

		// 닫히는 중에 비우기 · 종료: 전환 없이 바로 OnShutdown
		events.clear();
		manager->ClearLayer(EGameUILayer::Menu);
		check(joinEvents(events) == PString("M:shutdown") && menu->IsClosing() == false && drawnAlpha(manager, menuColor) < 0.0f, "accept transition: ClearLayer shuts a closing widget down at once");

		// 맨 위가 닫히면 아래 위젯 위에 닫히는 위젯을 그린다. 덮인 위젯을 빼면 전환 없이 바로 OnShutdown
		PSharedPtr<PUITestWidget> dialog = makeTestWidget(events, "D");
		PSharedPtr<PUITestWidget> dialog2 = makeTestWidget(events, "D2");
		dialog->SetCloseTransition(HGameUITransition::Fade(0.2f));
		dialog2->SetCloseTransition(HGameUITransition::FadeSlide(0.2f, HVector2(0.0f, 60.0f)));
		manager->PushWidgetInstance(dialog, EGameUILayer::Modal);
		addPanel(dialog, "DialogPanel", dialogColor, menuRect);
		manager->PushWidgetInstance(dialog2, EGameUILayer::Modal);
		addPanel(dialog2, "Dialog2Panel", dialog2Color, menuRect);
		dialog2->DeactivateWidget();
		HGameUIDrawList closingOver;
		manager->BuildDrawList(GTarget, closingOver);
		check(dialog->IsActive() && dialog2->IsClosing() && findQuad(closingOver, dialogColor) >= 0 && findQuad(closingOver, dialogColor) < findQuad(closingOver, dialog2Color),
			"accept transition: a closing widget is drawn over the widget that is now on top");
		manager->Update(0.1f);
		check(dialog2->GetRoot()->GetRenderTransform().Translation.y > 10.0f, "transition: a slide transition moves the root");
		manager->Update(0.2f);
		PSharedPtr<PUITestWidget> dialog3 = makeTestWidget(events, "D3");
		manager->PushWidgetInstance(dialog3, EGameUILayer::Modal);
		events.clear();
		manager->RemoveWidget(dialog);
		check(joinEvents(events) == PString("D:shutdown") && dialog->IsClosing() == false, "transition: removing a covered widget shuts it down without a transition");

		// 움직일 것이 없는 닫힘 전환은 바로 끝난다. 열림 전환 없이 다시 열면 바로 열린 모습
		HGameUITransition still;
		still.Duration = 0.2f;
		still.Opacity  = 1.0f;
		dialog3->SetCloseTransition(still);
		events.clear();
		dialog3->DeactivateWidget();
		check(joinEvents(events) == PString("D3:deactivate D3:shutdown"), "transition: a close transition that moves nothing ends at once");
		manager->PushWidgetInstance(dialog2, EGameUILayer::Modal);
		manager->Update(0.1f);
		dialog2->DeactivateWidget();
		manager->Update(0.1f);
		check(dialog2->IsClosing() && dialog2->GetRoot()->GetOpacity() < 1.0f, "transition: dialog closing");
		manager->PushWidgetInstance(dialog2, EGameUILayer::Modal);
		check(nearlyEqual(dialog2->GetRoot()->GetOpacity(), 1.0f) && nearlyEqual(dialog2->GetRoot()->GetRenderTransform().Translation.y, 0.0f),
			"transition: reopening without an open transition snaps to the open look");

		// 종료 중에 닫히는 위젯도 OnShutdown
		dialog2->DeactivateWidget();
		events.clear();
		manager->Shutdown();
		check(joinEvents(events) == PString("D2:shutdown H:deactivate H:shutdown") && dialog2->IsClosing() == false, "accept transition: Shutdown shuts a closing widget down (top layer first)");
	}

	// ---- 스크롤 칸(GG-20, ProjectAH ER-020 · 결정 D-35 가 + 나)
	bool wheelAt(PSharedPtr<PGameUIManager>& manager, float32 x, float32 y, float32 wheel, float32 wheelHorizontal = 0.0f)
	{
		HGameUIPointerEvent event = pointer(EGameUIPointerEventType::Wheel, x, y);
		event.Wheel           = wheel;
		event.WheelHorizontal = wheelHorizontal;
		return manager->HandlePointer(event, GTarget);
	}

	// 누르고 to 로 옮긴 뒤 뗀다(손잡이 끌기).
	void pressMoveRelease(PSharedPtr<PGameUIManager>& manager, const HVector2& from, const HVector2& to)
	{
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, from.x, from.y), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, to.x, to.y), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, to.x, to.y), GTarget);
	}

	HVector2 centerOf(const HRect& rect)
	{
		return HVector2((rect.left + rect.right) * 0.5f, (rect.top + rect.bottom) * 0.5f);
	}

	// 칸 내부 요소(보이는 칸 · 막대 · 손잡이)의 마지막 배치 사각형.
	HRect partRect(const PSharedPtr<PGameUIScrollBox>& box, const char* name)
	{
		PSharedPtr<PGameUIElement> part = box->FindChild(PName(name));
		return part.IsValid() ? part->GetLayoutRect() : HRect();
	}

	PSharedPtr<PGameUIScrollBox> addScrollBox(PSharedPtr<PGameUIElement> parent, const char* name, const HRect& rect)
	{
		PSharedPtr<PGameUIScrollBox> box = parent->AddChild<PGameUIScrollBox>(PName(name));
		box->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(rect.left, rect.top), HVector2(rect.right - rect.left, rect.bottom - rect.top));
		return box;
	}

	// 내용에 버튼 count 개를 세로로(간격 pitch) 붙이고 버튼마다 클릭 수를 clicks 에 센다.
	HList<PSharedPtr<PGameUIButton>> addRows(PSharedPtr<PGameUIScrollBox> box, int32 count, float32 width, float32 height, float32 pitch, HList<int32>& clicks)
	{
		HList<PSharedPtr<PGameUIButton>> rows;
		clicks.assign((size_t)count, 0);
		HList<int32>* counter = &clicks;
		for (int32 i = 0; i < count; ++i)
		{
			PSharedPtr<PGameUIButton> row = box->GetContent()->AddChild<PGameUIButton>(PName(PString::Format("Row%d", i)));
			row->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, (float32)i * pitch), HVector2(width, height));
			row->OnClicked.AddLambda([counter, i]()
				{
					++(*counter)[(size_t)i];
				});
			rows.push_back(row);
		}
		return rows;
	}

	// 글자 배치(글꼴 아틀라스)의 사각형 수. 배치 클립이 모두 clip 이고 사각형이 모두 clip 과 겹치면(칸 밖 사각형이 목록에 없으면) bAllInside.
	uint64 countGlyphQuads(const HGameUIDrawList& list, const HRect& clip, bool& bAllInside)
	{
		uint64 quads = 0;
		bAllInside = true;
		for (const HGameUIDrawCommand& command : list.Commands)
		{
			if (command.Font == nullptr)
			{
				continue;
			}
			if (isRect(command.Clip, clip.left, clip.top, clip.right, clip.bottom) == false)
			{
				bAllInside = false;
			}
			for (uint32 index = command.IndexOffset; index + 6 <= command.IndexOffset + command.IndexCount; index += 6)
			{
				const uint32 base = list.Indices[index];
				float32 top    = list.Vertices[base].Position.y;
				float32 bottom = top;
				for (uint32 corner = 1; corner < 4; ++corner)
				{
					top    = HMath::Min(top, list.Vertices[base + corner].Position.y);
					bottom = HMath::Max(bottom, list.Vertices[base + corner].Position.y);
				}
				if (bottom <= clip.top || top >= clip.bottom)
				{
					bAllInside = false;
				}
				++quads;
			}
		}
		return quads;
	}

	// 정점 vertexIndex 로 시작하는 사각형을 담은 배치. 없으면 nullptr.
	const HGameUIDrawCommand* findQuadCommand(const HGameUIDrawList& list, int32 vertexIndex)
	{
		for (const HGameUIDrawCommand& command : list.Commands)
		{
			for (uint32 index = command.IndexOffset; index + 6 <= command.IndexOffset + command.IndexCount; index += 6)
			{
				if ((int32)list.Indices[index] == vertexIndex)
				{
					return &command;
				}
			}
		}
		return nullptr;
	}

	// 완료 기준 (a): 6줄 칸의 20줄 글 — 휠 · 손잡이 끌기 · 막대 빈 곳 누르기로 끝까지, 글자는 칸으로 잘린다.
	void testScrollBoxText()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();   // 기준 1000x1000 = GTarget → 배율 1
		PSharedPtr<PGameUIFont>    font    = manager->GetDefaultFont();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		if (font.IsValid() == false)
		{
			++GSkipCount;
			JG_LOG(GameUI, ELogLevel::Warning, "gameui.selftest: no default font. Scroll text check skipped");
			manager->Shutdown();
			return;
		}

		// 칸 (100, 100)-(500, 280) = 줄 간격 30 의 6줄. 글 폭 = 칸 폭 − 막대 자리(12 + 2 × 2)
		PSharedPtr<PGameUIScrollBox> box = addScrollBox(widget->GetRoot(), "StoryBox", HRect(100.0f, 100.0f, 500.0f, 280.0f));
		int32    scrolledCount = 0;
		HVector2 lastScrolled;
		box->OnScrolled.AddLambda([&scrolledCount, &lastScrolled](const HVector2& offset)
			{
				++scrolledCount;
				lastScrolled = offset;
			});
		PString story;
		for (int32 line = 1; line <= 20; ++line)
		{
			story = story.Empty() ? PString::Format("Line %d", line) : PString::Format("%s\nLine %d", story, line);
		}
		PSharedPtr<PGameUIText> text = box->GetContent()->AddChild<PGameUIText>(PName("Story"));
		HGameUIParagraphStyle paragraph;
		paragraph.LineHeight = 30.0f;
		text->SetParagraphStyle(paragraph);
		text->SetFontSize(20.0f);
		text->SetText(story);
		const float32 textWidth  = 400.0f - 16.0f;
		const float32 textHeight = text->MeasureContent(textWidth, manager.GetRawPointer()).y;
		text->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(textWidth, textHeight));
		const float32 maxScroll   = textHeight - 180.0f;
		const uint64  totalGlyphs = 20 * 4 + 9 * 1 + 11 * 2;   // "Line" + 숫자(공백 · 줄바꿈은 사각형이 없다)

		HGameUIDrawList top;
		manager->BuildDrawList(GTarget, top);
		check(textHeight >= 570.0f && box->IsScrollBarVisible(true) && box->IsScrollBarVisible(false) == false
			&& isPoint(box->GetViewportSize(), HVector2(384.0f, 180.0f)) && nearlyEqual(box->GetMaxScroll().y, maxScroll),
			"scroll: a 20-line text in a 6-line box shows the vertical bar only (viewport 384 x 180)");
		bool bTopInside = false;
		const uint64 topQuads = countGlyphQuads(top, HRect(100.0f, 100.0f, 484.0f, 280.0f), bTopInside);
		check(bTopInside && topQuads > 0 && topQuads * 2 < totalGlyphs,
			"accept ER-020 (a): every glyph is clipped to the viewport and lines outside it are not in the draw list");

		// 휠 한 칸 = 60. 끝에 닿을 때까지, 넘길 때마다 OnScrolled 한 번
		const HVector2 boxCenter(300.0f, 190.0f);
		int32 wheelSteps = 0;
		while (box->GetScrollOffset().y < maxScroll - 0.01f && wheelSteps < 50)
		{
			wheelAt(manager, boxCenter.x, boxCenter.y, -1.0f);
			++wheelSteps;
		}
		const int32 expectedSteps = (int32)std::ceil(maxScroll / 60.0f);
		check(wheelSteps == expectedSteps && nearlyEqual(box->GetScrollOffset().y, maxScroll) && scrolledCount == expectedSteps && nearlyEqual(lastScrolled.y, maxScroll),
			"accept ER-020 (a): wheel steps of 60 reach the end, one OnScrolled per step");
		check(wheelAt(manager, boxCenter.x, boxCenter.y, -1.0f) && scrolledCount == expectedSteps && nearlyEqual(box->GetScrollOffset().y, maxScroll),
			"scroll: the wheel at the end is consumed over the box and changes nothing");
		HGameUIDrawList end;
		manager->BuildDrawList(GTarget, end);
		bool bEndInside = false;
		const uint64 endQuads = countGlyphQuads(end, HRect(100.0f, 100.0f, 484.0f, 280.0f), bEndInside);
		check(bEndInside && endQuads > 0 && endQuads * 2 < totalGlyphs, "scroll: at the end the glyphs are still clipped to the viewport");
		wheelAt(manager, boxCenter.x, boxCenter.y, 1.0f);
		check(nearlyEqual(box->GetScrollOffset().y, maxScroll - 60.0f), "scroll: wheel forward scrolls back up by one step");

		// 막대: 칸 안 오른쪽(486~498), 위아래 여백 2. 손잡이 길이 = 막대 × 보이는 칸 / 내용
		box->ScrollTo(HVector2(0.0f, 0.0f));
		manager->UpdateLayout(GTarget);
		const HRect   track      = partRect(box, "ScrollBox.VerticalBar");
		const HRect   thumb      = partRect(box, "ScrollBox.VerticalThumb");
		const float32 thumbSize  = thumb.bottom - thumb.top;
		check(isRect(track, 486.0f, 102.0f, 498.0f, 278.0f) && nearlyEqual(thumb.top, 102.0f) && nearlyEqual(thumbSize, HMath::Max(176.0f * 180.0f / textHeight, 24.0f), 0.05f),
			"scroll: the bar sits inside the right edge and the thumb is track x viewport / content long");

		// 손잡이 끌기: 반만큼 → 넘긴 양 반, 칸 밖 멀리 → 끝
		const HVector2 grab   = centerOf(thumb);
		const float32  travel = (track.bottom - track.top) - thumbSize;
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, grab.x, grab.y), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, grab.x, grab.y + travel * 0.5f), GTarget);
		const bool bHalf = nearlyEqual(box->GetScrollOffset().y, maxScroll * 0.5f, 0.05f);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, grab.x + 300.0f, grab.y + 1000.0f), GTarget);
		const bool bEnd = nearlyEqual(box->GetScrollOffset().y, maxScroll);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, grab.x + 300.0f, grab.y + 1000.0f), GTarget);
		check(bHalf && bEnd, "accept ER-020 (a): dragging the thumb follows the pointer and reaches the end past the box");

		// 막대 빈 곳 누르기: 손잡이 아래를 누를 때마다 보이는 칸의 7/8(157.5), 끝까지. 손잡이 위를 누르면 거꾸로
		box->ScrollTo(HVector2(0.0f, 0.0f));
		const HVector2 belowThumb(492.0f, 276.0f);
		check(click(manager, belowThumb.x, belowThumb.y) && nearlyEqual(box->GetScrollOffset().y, 157.5f), "scroll: pressing the track below the thumb pages down by 7/8 of the viewport");
		int32 pages = 1;
		while (box->GetScrollOffset().y < maxScroll - 0.01f && pages < 20)
		{
			click(manager, belowThumb.x, belowThumb.y);
			++pages;
		}
		const bool bPagedToEnd = nearlyEqual(box->GetScrollOffset().y, maxScroll) && pages == (int32)std::ceil(maxScroll / 157.5f);
		click(manager, 492.0f, 104.0f);
		check(bPagedToEnd && nearlyEqual(box->GetScrollOffset().y, maxScroll - 157.5f), "accept ER-020 (a): track presses reach the end and a press above the thumb pages up");

		manager->Shutdown();
	}

	// 완료 기준 (b): 칸보다 긴 버튼 40개 — 칸 밖 버튼은 안 눌리고, 넘긴 뒤 들어온 버튼은 눌린다. ScrollIntoView.
	void testScrollBoxRows()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIScrollBox> box = addScrollBox(widget->GetRoot(), "List", HRect(600.0f, 100.0f, 900.0f, 300.0f));
		HList<int32> clicks;
		const HList<PSharedPtr<PGameUIButton>> rows = addRows(box, 40, 280.0f, 40.0f, 50.0f, clicks);
		manager->UpdateLayout(GTarget);
		check(nearlyEqual(box->GetResolvedContentSize().y, 1990.0f) && nearlyEqual(box->GetMaxScroll().y, 1790.0f) && box->GetScrollDirection() == EGameUIScrollDirection::Vertical,
			"scroll: the automatic content size wraps the visible rows (40 x 50 - 10)");
		check(isRect(rows[3]->GetLayoutRect(), 600.0f, 250.0f, 880.0f, 290.0f) && isRect(rows[3]->CalculateLayoutRect(box->GetContent()->GetLayoutRect()), 600.0f, 250.0f, 880.0f, 290.0f),
			"scroll: content rows are laid out from the viewport's top-left (CalculateLayoutRect agrees)");

		// 칸 밖 버튼(Row10 = 화면 600~640)은 올리기 · 누르기가 안 되고 아래(빈 곳 · 월드)로 간다
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 700.0f, 620.0f), GTarget);
		const bool bNoHover = manager->GetHoveredElement().IsValid() == false;
		const bool bPassed  = click(manager, 700.0f, 620.0f) == false;
		check(bNoHover && bPassed && clicks[10] == 0, "accept ER-020 (b): a row outside the box is neither hovered nor clicked (the click goes below)");
		check(click(manager, 700.0f, 220.0f) && clicks[2] == 1, "scroll: a row inside the box is clicked");
		box->ScrollTo(HVector2(0.0f, 450.0f));
		check(click(manager, 700.0f, 170.0f) && clicks[10] == 1, "accept ER-020 (b): after ScrollTo the row that came into the box is clicked");
		check(click(manager, 700.0f, 220.0f) && clicks[2] == 1 && clicks[11] == 1, "accept ER-020 (b): the row that went out is not clicked at its old place (the row now there is)");

		// 칸 가장자리에 걸친 버튼은 보이는 부분만 눌린다(Row9 = 화면 90~130, 칸 위 100)
		box->ScrollTo(HVector2(0.0f, 460.0f));
		const bool bCutPart   = click(manager, 700.0f, 95.0f) == false && clicks[9] == 0;
		const bool bShownPart = click(manager, 700.0f, 120.0f) && clicks[9] == 1;
		check(bCutPart && bShownPart, "scroll: a row cut by the box edge is clicked only on its visible part");

		// ScrollIntoView: 가장 적게 넘긴다(여백 5)
		check(box->ScrollIntoView(rows[30], 5.0f) && nearlyEqual(box->GetScrollOffset().y, 1545.0f - 200.0f), "scroll: ScrollIntoView scrolls down the least to show a row below (margin 5)");
		check(box->ScrollIntoView(rows[2], 5.0f) && nearlyEqual(box->GetScrollOffset().y, 95.0f), "scroll: ScrollIntoView scrolls up to show a row above");
		check(box->ScrollIntoView(rows[3], 5.0f) && nearlyEqual(box->GetScrollOffset().y, 95.0f), "scroll: ScrollIntoView keeps the offset when the row is already shown");
		PSharedPtr<PGameUIButton> extra = box->GetContent()->AddChild<PGameUIButton>(PName("Extra"));
		extra->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 2400.0f), HVector2(280.0f, 40.0f));
		check(box->ScrollIntoView(extra) && nearlyEqual(box->GetScrollOffset().y, 2240.0f), "scroll: ScrollIntoView works on a row added this frame (before layout)");
		PSharedPtr<PGameUIButton> stranger = Allocate<PGameUIButton>();
		check(box->ScrollIntoView(stranger) == false && nearlyEqual(box->GetScrollOffset().y, 2240.0f), "scroll: ScrollIntoView refuses an element outside the content");
		extra->SetVisible(false);
		manager->UpdateLayout(GTarget);
		check(nearlyEqual(box->GetScrollOffset().y, 1790.0f), "scroll: when the content shrinks the offset follows (hidden rows do not count)");

		manager->Shutdown();
	}

	// 완료 기준 (c): 칸 밖 휠 · 다 들어간 칸(막대 없음) · 겹친 칸. 요소 공통 자식 잘라내기.
	void testScrollBoxRouting()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		const HGameUIScrollStyle   style   = manager->GetDefaultScrollStyle();

		// 다 들어가는 칸: 막대는 그려지지도 판정되지도 않는다. 휠은 칸 위에서 소비만
		PSharedPtr<PGameUIScrollBox> fit = addScrollBox(widget->GetRoot(), "Fit", HRect(100.0f, 100.0f, 400.0f, 400.0f));
		HList<int32> fitClicks;
		addRows(fit, 3, 200.0f, 40.0f, 50.0f, fitClicks);
		int32 fitScrolled = 0;
		fit->OnScrolled.AddLambda([&fitScrolled](const HVector2& offset)
			{
				++fitScrolled;
			});
		HGameUIDrawList fitList;
		manager->BuildDrawList(GTarget, fitList);
		check(fit->IsScrollBarVisible(true) == false && fit->IsScrollBarVisible(false) == false && findQuad(fitList, style.TrackColor) < 0 && findQuad(fitList, style.ThumbNormal) < 0,
			"accept ER-020 (c): content that fits shows no bar (none drawn)");
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 392.0f, 250.0f), GTarget);
		check(manager->GetHoveredElement().GetRawPointer() == fit.GetRawPointer(), "accept ER-020 (c): a hidden bar is not hit (the box is)");
		check(wheelAt(manager, 250.0f, 250.0f, -1.0f) && fitScrolled == 0 && nearlyEqual(fit->GetScrollOffset().y, 0.0f), "scroll: wheel over a box with nothing to scroll is consumed and scrolls nothing");

		// 칸 밖 휠: 빈 곳은 지금처럼 소비 안 함, 다른 UI 위는 소비만
		PSharedPtr<PGameUIScrollBox> list = addScrollBox(widget->GetRoot(), "List", HRect(500.0f, 100.0f, 800.0f, 300.0f));
		HList<int32> listClicks;
		addRows(list, 40, 280.0f, 40.0f, 50.0f, listClicks);
		PSharedPtr<PGameUIImage> panel = addPanel(widget, "Panel", HLinearColor(0.9f, 0.1f, 0.1f, 1.0f), HRect(850.0f, 500.0f, 950.0f, 600.0f));
		panel->SetHitTestVisible(true);
		check(wheelAt(manager, 950.0f, 950.0f, -1.0f) == false && nearlyEqual(list->GetScrollOffset().y, 0.0f), "accept ER-020 (c): wheel outside every box changes nothing (not consumed over empty space)");
		check(wheelAt(manager, 900.0f, 550.0f, -1.0f) && nearlyEqual(list->GetScrollOffset().y, 0.0f), "scroll: wheel over other UI is consumed as before and no box scrolls");
		check(wheelAt(manager, 650.0f, 200.0f, -1.0f) && nearlyEqual(list->GetScrollOffset().y, 60.0f), "scroll: wheel over a row in the box scrolls the box");

		// 겹친 칸: 안쪽부터, 안쪽이 그 방향 끝이면 바깥이 받는다. 거꾸로 굴리면 다시 안쪽부터
		PSharedPtr<PGameUIScrollBox> outer = addScrollBox(widget->GetRoot(), "Outer", HRect(100.0f, 500.0f, 400.0f, 800.0f));
		PSharedPtr<PGameUIScrollBox> inner = addScrollBox(outer->GetContent(), "Inner", HRect(0.0f, 0.0f, 250.0f, 150.0f));
		HList<int32> innerClicks;
		addRows(inner, 10, 200.0f, 40.0f, 50.0f, innerClicks);   // 490 → 넘길 수 있는 양 340
		PSharedPtr<PGameUIImage> filler = outer->GetContent()->AddChild<PGameUIImage>(PName("Filler"));
		filler->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 160.0f), HVector2(250.0f, 600.0f));   // 바깥 내용 760 → 460
		manager->UpdateLayout(GTarget);
		for (int32 step = 0; step < 6; ++step)
		{
			wheelAt(manager, 225.0f, 575.0f, -1.0f);
		}
		const bool bInnerFirst = nearlyEqual(inner->GetScrollOffset().y, 340.0f) && nearlyEqual(outer->GetScrollOffset().y, 0.0f);
		wheelAt(manager, 225.0f, 575.0f, -1.0f);
		const bool bThenOuter = nearlyEqual(inner->GetScrollOffset().y, 340.0f) && nearlyEqual(outer->GetScrollOffset().y, 60.0f);
		wheelAt(manager, 225.0f, 520.0f, 1.0f);
		const bool bBackInner = nearlyEqual(inner->GetScrollOffset().y, 280.0f) && nearlyEqual(outer->GetScrollOffset().y, 60.0f);
		check(bInnerFirst && bThenOuter && bBackInner, "accept ER-020 (c): nested boxes - the inner box takes the wheel until its end, then the outer box");

		// 요소 공통 자식 잘라내기(일반 판): 자식은 판 안에만 그리고, 판 밖으로 나간 부분은 판정하지 않는다
		PSharedPtr<PGameUIImage> clipPanel = addPanel(widget, "ClipPanel", HLinearColor(0.1f, 0.8f, 0.1f, 1.0f), HRect(500.0f, 500.0f, 600.0f, 600.0f));
		clipPanel->SetClipChildren(true);
		PSharedPtr<PGameUIImage> overflow = clipPanel->AddChild<PGameUIImage>(PName("Overflow"));
		overflow->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(50.0f, 50.0f), HVector2(150.0f, 150.0f));
		overflow->SetColor(HLinearColor(0.1f, 0.1f, 0.9f, 1.0f));
		overflow->SetHitTestVisible(true);
		HGameUIDrawList clipList;
		manager->BuildDrawList(GTarget, clipList);
		const HGameUIDrawCommand* overflowCommand = findQuadCommand(clipList, findQuad(clipList, overflow->GetColor()));
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 575.0f, 575.0f), GTarget);
		const bool bInsideHit = manager->GetHoveredElement().GetRawPointer() == overflow.GetRawPointer();
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 650.0f, 650.0f), GTarget);
		const bool bOutsideMiss = manager->GetHoveredElement().IsValid() == false;
		check(clipPanel->IsClipChildren() && overflowCommand != nullptr && isRect(overflowCommand->Clip, 500.0f, 500.0f, 600.0f, 600.0f) && bInsideHit && bOutsideMiss,
			"scroll: SetClipChildren on a plain panel clips the child drawing and its hit area");

		manager->Shutdown();
	}

	// 완료 기준 (d): 15° · 1.3배 부모 아래 칸 — 그리기는 돌린 칸을 감싸는 축 정렬 사각형으로, 판정은 돌린 칸 안만.
	void testScrollBoxTransform()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIImage> frame = addPanel(widget, "Frame", HLinearColor(0.3f, 0.3f, 0.3f, 1.0f), HRect(300.0f, 300.0f, 600.0f, 500.0f));
		frame->SetPivot(HVector2(0.5f, 0.5f));
		frame->SetPosition(HVector2(450.0f, 400.0f));
		HGameUIRenderTransform transform;
		transform.Angle = 15.0f;
		transform.Scale = HVector2(1.3f, 1.3f);
		frame->SetRenderTransform(transform);
		const HGameUIAffine expected = HGameUIAffine::FromRenderTransform(transform, HVector2(450.0f, 400.0f));

		PSharedPtr<PGameUIScrollBox> box = addScrollBox(frame, "Turned", HRect(0.0f, 0.0f, 300.0f, 200.0f));
		PSharedPtr<PGameUIButton> big = box->GetContent()->AddChild<PGameUIButton>(PName("Big"));
		big->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(284.0f, 600.0f));
		HGameUIDrawList list;
		manager->BuildDrawList(GTarget, list);
		PSharedPtr<PGameUIElement> viewport = box->FindChild(PName("ScrollBox.Viewport"));
		const HRect expectedClip = viewport->LocalToCanvasBounds(viewport->GetLayoutRect());
		const HGameUIDrawCommand* bigCommand = findQuadCommand(list, findQuad(list, big->ResolveStyle(manager.GetRawPointer()).Normal));
		check(isRect(viewport->GetLayoutRect(), 300.0f, 300.0f, 584.0f, 500.0f) && bigCommand != nullptr
			&& isRect(bigCommand->Clip, expectedClip.left, expectedClip.top, expectedClip.right, expectedClip.bottom),
			"accept ER-020 (d): under a 15 degree 1.3x parent the content is clipped to the box around the turned viewport");

		auto hoveredAt = [&manager](const HVector2& point)
		{
			manager->HandlePointer(pointer(EGameUIPointerEventType::Move, point.x, point.y), GTarget);
			return manager->GetHoveredElement();
		};
		const bool bInside      = hoveredAt(expected.Apply(HVector2(320.0f, 480.0f))).GetRawPointer() == big.GetRawPointer();
		const bool bCutOff      = hoveredAt(expected.Apply(HVector2(320.0f, 520.0f))).GetRawPointer() != big.GetRawPointer();   // 버튼 자리지만 칸 아래
		const bool bCornerMiss  = hoveredAt(HVector2(expectedClip.left + 3.0f, expectedClip.top + 3.0f)).GetRawPointer() != big.GetRawPointer();
		check(bInside && bCutOff && bCornerMiss, "accept ER-020 (d): hit testing follows the turned viewport (its corners and the cut part miss)");

		manager->Shutdown();
	}

	PSharedPtr<PGameUIScrollBox> addKeyedList(PSharedPtr<JGGameWidget> widget, const char* key, HList<int32>& clicks)
	{
		PSharedPtr<PGameUIScrollBox> box = addScrollBox(widget->GetRoot(), "Deck", HRect(100.0f, 100.0f, 400.0f, 300.0f));
		if (key != nullptr)
		{
			box->SetScrollKey(PName(key));
		}
		addRows(box, 40, 280.0f, 40.0f, 50.0f, clicks);
		return box;
	}

	// 완료 기준 (e): 열쇠 — 칸째 다시 만들어도 같은 열쇠면 넘긴 양 유지, 위젯이 닫히면 잊음. 내용만 다시 만들면 그대로(잘림).
	void testScrollBoxMemory()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		HList<int32> clicks;

		PSharedPtr<PGameUIScrollBox> first = addKeyedList(widget, "DeckList", clicks);
		manager->UpdateLayout(GTarget);
		first->ScrollTo(HVector2(0.0f, 300.0f));
		widget->GetRoot()->RemoveChild(first);
		PSharedPtr<PGameUIScrollBox> rebuilt = addKeyedList(widget, "DeckList", clicks);
		manager->UpdateLayout(GTarget);
		check(rebuilt->GetScrollKey() == PName("DeckList") && nearlyEqual(rebuilt->GetScrollOffset().y, 300.0f),
			"accept ER-020 (e): rebuilding the box with the same key starts at the remembered offset");
		check(click(manager, 200.0f, 120.0f) && clicks[6] == 1, "scroll: the rebuilt box is clicked at its remembered place (row 6)");

		// 내용만 다시 만들면 넘긴 양은 칸에 남고 새 내용 크기로 잘린다
		rebuilt->GetContent()->ClearChildren();
		addRows(rebuilt, 10, 280.0f, 40.0f, 50.0f, clicks);   // 490 → 넘길 수 있는 양 290
		manager->UpdateLayout(GTarget);
		check(nearlyEqual(rebuilt->GetScrollOffset().y, 290.0f), "scroll: rebuilding only the content keeps the offset, cut to the new content");

		widget->GetRoot()->RemoveChild(rebuilt);
		PSharedPtr<PGameUIScrollBox> other = addKeyedList(widget, "Other", clicks);
		PSharedPtr<PGameUIScrollBox> unkeyed = addKeyedList(widget, nullptr, clicks);
		manager->UpdateLayout(GTarget);
		check(nearlyEqual(other->GetScrollOffset().y, 0.0f) && nearlyEqual(unkeyed->GetScrollOffset().y, 0.0f), "scroll: another key or no key starts at the top");
		widget->GetRoot()->RemoveChild(other);
		widget->GetRoot()->RemoveChild(unkeyed);

		// 첫 배치 전에 코드로 넘기면 그 자리가 기억보다 앞선다
		PSharedPtr<PGameUIScrollBox> explicitBox = addKeyedList(widget, "DeckList", clicks);
		explicitBox->ScrollTo(HVector2(0.0f, 50.0f));
		manager->UpdateLayout(GTarget);
		check(nearlyEqual(explicitBox->GetScrollOffset().y, 50.0f), "scroll: ScrollTo before the first layout wins over the key memory");

		// 위젯이 닫히면(OnShutdown) 잊는다
		manager->RemoveWidget(widget);
		manager->PushWidgetInstance(widget, EGameUILayer::Game);
		PSharedPtr<PGameUIScrollBox> reopened = addKeyedList(widget, "DeckList", clicks);
		manager->UpdateLayout(GTarget);
		check(nearlyEqual(reopened->GetScrollOffset().y, 0.0f), "accept ER-020 (e): the key memory is forgotten when the widget shuts down");

		manager->Shutdown();
	}

	// 결정 (나): 부드럽게 넘기기(넘긴 양 트윈) — 휠 목표가 쌓이고, 손잡이 · ScrollTo 는 멈추고, 게임 트윈도 쓴다.
	void testScrollBoxSmooth()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIScrollBox> box = addScrollBox(widget->GetRoot(), "Smooth", HRect(100.0f, 100.0f, 400.0f, 300.0f));
		HList<int32> clicks;
		addRows(box, 40, 280.0f, 40.0f, 50.0f, clicks);   // 넘길 수 있는 양 1790
		HGameUIScrollStyle style;
		style.SmoothSeconds = 0.2f;
		box->SetStyle(style);
		int32 scrolled = 0;
		box->OnScrolled.AddLambda([&scrolled](const HVector2& offset)
			{
				++scrolled;
			});
		manager->UpdateLayout(GTarget);

		wheelAt(manager, 200.0f, 200.0f, -1.0f);
		const bool bNoJump = nearlyEqual(box->GetScrollOffset().y, 0.0f);
		manager->Update(0.1f);
		const float32 middle = box->GetScrollOffset().y;
		manager->Update(0.15f);
		check(box->HasStyle() && bNoJump && nearlyEqual(middle, 60.0f * 0.875f, 0.05f) && nearlyEqual(box->GetScrollOffset().y, 60.0f) && scrolled == 2,
			"accept D-35 (b): smooth scrolling eases to the wheel target over SmoothSeconds (OutCubic), OnScrolled each frame it moves");

		wheelAt(manager, 200.0f, 200.0f, -1.0f);
		manager->Update(0.05f);
		wheelAt(manager, 200.0f, 200.0f, -1.0f);
		manager->Update(0.3f);
		check(nearlyEqual(box->GetScrollOffset().y, 180.0f), "smooth: quick wheel steps add up from the moving target (60 + 60 + 60)");

		// 손잡이를 누르면 그 자리에서 멈춘다
		wheelAt(manager, 200.0f, 200.0f, -1.0f);
		manager->Update(0.05f);
		const float32 caught = box->GetScrollOffset().y;
		manager->UpdateLayout(GTarget);
		const HVector2 grab = centerOf(partRect(box, "ScrollBox.VerticalThumb"));
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, grab.x, grab.y), GTarget);
		manager->Update(0.3f);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, grab.x, grab.y), GTarget);
		check(caught > 180.0f && caught < 240.0f && nearlyEqual(box->GetScrollOffset().y, caught), "smooth: pressing the thumb stops the smooth scroll where it is");

		// ScrollTo 도 멈춘다
		wheelAt(manager, 200.0f, 200.0f, -1.0f);
		box->ScrollTo(HVector2(0.0f, 10.0f));
		manager->Update(0.3f);
		check(nearlyEqual(box->GetScrollOffset().y, 10.0f), "smooth: ScrollTo cancels the smooth scroll");

		// 게임이 넘긴 양 트윈을 직접 재생한다(지금 자리에서 시작, 끝을 넘는 목표는 끝에서 멈춘다)
		HTimelineLog gameLog;
		PSharedPtr<PGameUITimeline> toFive = widget->PlayTween(HGameUITween::To(box, EGameUITweenProperty::ScrollOffset, HVector2(0.0f, 500.0f), 0.2f, EGameUIEase::Linear));
		watch(toFive, gameLog);
		manager->Update(0.1f);
		const float32 halfway = box->GetScrollOffset().y;
		manager->Update(0.1f);
		check(nearlyEqual(halfway, 255.0f, 0.05f) && nearlyEqual(box->GetScrollOffset().y, 500.0f) && gameLog.Finished == 1 && gameLog.Canceled == 0,
			"accept D-35 (b): the ScrollOffset tween property scrolls a box from a game timeline");
		widget->PlayTween(HGameUITween::To(box, EGameUITweenProperty::ScrollOffset, HVector2(0.0f, 99999.0f), 0.1f));
		manager->Update(0.2f);
		check(nearlyEqual(box->GetScrollOffset().y, 1790.0f), "smooth: a tween past the end stops at the end");

		// 게임 트윈 중 휠: 같은 속성이라 게임 묶음은 취소, 휠 목표로 이어 간다
		HTimelineLog longLog;
		PSharedPtr<PGameUITimeline> slow = widget->PlayTween(HGameUITween::To(box, EGameUITweenProperty::ScrollOffset, HVector2(0.0f, 0.0f), 1.0f, EGameUIEase::Linear));
		watch(slow, longLog);
		manager->Update(0.1f);
		const float32 before = box->GetScrollOffset().y;
		wheelAt(manager, 200.0f, 200.0f, -1.0f);
		manager->Update(0.3f);
		check(longLog.Canceled == 1 && nearlyEqual(box->GetScrollOffset().y, HMath::Min(before + 60.0f, 1790.0f)), "smooth: the wheel takes over from a game scroll tween (that timeline is canceled)");

		manager->Shutdown();
	}

	// 결정 (나): 가로 막대 · 가로 휠 · 가로만 칸.
	void testScrollBoxHorizontal()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);

		// 두 방향: 내용 600 x 600, 칸 300 x 200 → 두 막대, 보이는 칸 284 x 184, 넘길 수 있는 양 316 x 416
		PSharedPtr<PGameUIScrollBox> both = addScrollBox(widget->GetRoot(), "Both", HRect(100.0f, 500.0f, 400.0f, 700.0f));
		both->SetScrollDirection(EGameUIScrollDirection::Both);
		PSharedPtr<PGameUIImage> sheet = both->GetContent()->AddChild<PGameUIImage>(PName("Sheet"));
		sheet->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(600.0f, 600.0f));
		manager->UpdateLayout(GTarget);
		check(both->IsScrollBarVisible(true) && both->IsScrollBarVisible(false) && isPoint(both->GetViewportSize(), HVector2(284.0f, 184.0f))
			&& isPoint(both->GetMaxScroll(), HVector2(316.0f, 416.0f)),
			"accept D-35 (b): content bigger both ways shows both bars and each bar takes its room");
		check(isRect(partRect(both, "ScrollBox.HorizontalBar"), 102.0f, 686.0f, 382.0f, 698.0f) && isRect(partRect(both, "ScrollBox.VerticalBar"), 386.0f, 502.0f, 398.0f, 682.0f),
			"scroll: the horizontal bar runs along the bottom and the corner between the bars stays empty");

		wheelAt(manager, 250.0f, 600.0f, 0.0f, 1.0f);
		const bool bHorizontalWheel = isPoint(both->GetScrollOffset(), HVector2(60.0f, 0.0f));
		wheelAt(manager, 250.0f, 600.0f, -1.0f);
		check(bHorizontalWheel && isPoint(both->GetScrollOffset(), HVector2(60.0f, 60.0f)), "scroll: the horizontal wheel scrolls sideways, the vertical wheel down");

		manager->UpdateLayout(GTarget);
		const HVector2 grab = centerOf(partRect(both, "ScrollBox.HorizontalThumb"));
		pressMoveRelease(manager, grab, HVector2(grab.x + 1000.0f, grab.y + 40.0f));
		check(isPoint(both->GetScrollOffset(), HVector2(316.0f, 60.0f)), "accept D-35 (b): dragging the horizontal thumb reaches the right end");
		both->ScrollTo(HVector2(0.0f, 60.0f));
		check(click(manager, 378.0f, 692.0f) && isPoint(both->GetScrollOffset(), HVector2(284.0f * 0.875f, 60.0f)), "scroll: pressing the horizontal track pages right by 7/8 of the viewport");

		// 가로만: 세로 휠도 가로로 넘긴다
		PSharedPtr<PGameUIScrollBox> strip = addScrollBox(widget->GetRoot(), "Strip", HRect(500.0f, 500.0f, 900.0f, 600.0f));
		strip->SetScrollDirection(EGameUIScrollDirection::Horizontal);
		PSharedPtr<PGameUIImage> film = strip->GetContent()->AddChild<PGameUIImage>(PName("Film"));
		film->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(1000.0f, 80.0f));
		manager->UpdateLayout(GTarget);
		wheelAt(manager, 700.0f, 540.0f, -1.0f);
		check(strip->IsScrollBarVisible(false) && strip->IsScrollBarVisible(true) == false && isPoint(strip->GetScrollOffset(), HVector2(60.0f, 0.0f)),
			"scroll: a horizontal-only box scrolls sideways with the vertical wheel");

		// 세로만 칸은 가로 휠을 받지 않는다(넘치는 폭은 잘린다)
		PSharedPtr<PGameUIScrollBox> column = addScrollBox(widget->GetRoot(), "Column", HRect(500.0f, 100.0f, 800.0f, 300.0f));
		HList<int32> clicks;
		addRows(column, 40, 500.0f, 40.0f, 50.0f, clicks);
		manager->UpdateLayout(GTarget);
		check(wheelAt(manager, 650.0f, 200.0f, 0.0f, 1.0f) && isPoint(column->GetScrollOffset(), HVector2(0.0f, 0.0f)) && column->IsScrollBarVisible(false) == false,
			"scroll: a vertical box ignores the horizontal wheel and shows no horizontal bar for wide rows");

		manager->Shutdown();
	}

	// 누른 요소가 이동을 받는 훅: 손잡이는 바깥 끌기를 시작하지 않고, 끄는 동안 호버는 그대로. 꺼진 칸은 받지 않는다.
	void testScrollBoxHeld()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		PSharedPtr<PGameUIImage> panel = addPanel(widget, "Panel", HLinearColor(0.3f, 0.3f, 0.3f, 1.0f), HRect(100.0f, 100.0f, 500.0f, 500.0f));
		panel->SetHitTestVisible(true);
		panel->SetDraggable(true);
		PSharedPtr<PGameUIScrollBox> box = addScrollBox(panel, "InPanel", HRect(50.0f, 50.0f, 350.0f, 250.0f));   // 화면 (150, 150)-(450, 350)
		HList<int32> clicks;
		addRows(box, 40, 280.0f, 40.0f, 50.0f, clicks);
		int32 dragBegins = 0;
		manager->OnDragBegin.AddLambda([&dragBegins](const PSharedPtr<PGameUIElement>& element, const HVector2& position)
			{
				++dragBegins;
			});
		manager->UpdateLayout(GTarget);

		PSharedPtr<PGameUIElement> thumb = box->FindChild(PName("ScrollBox.VerticalThumb"));
		const HVector2 grab = centerOf(thumb->GetLayoutRect());
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, grab.x, grab.y), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, grab.x, grab.y + 40.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 250.0f, 200.0f), GTarget);   // 버튼 위를 지나간다
		const bool bHoverKept = manager->GetHoveredElement().GetRawPointer() == thumb.GetRawPointer();
		const float32 dragged = box->GetScrollOffset().y;
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 250.0f, 200.0f), GTarget);
		check(dragBegins == 0 && dragged > 0.0f && bHoverKept && manager->GetHoveredElement().IsValid() && manager->GetHoveredElement().GetRawPointer() != thumb.GetRawPointer(),
			"scroll: dragging the thumb inside a draggable panel scrolls without starting the panel drag, and hover waits until release");
		const int32 clicksBefore = clicks[5] + clicks[6] + clicks[7] + clicks[8] + clicks[9] + clicks[10];
		pressMoveRelease(manager, HVector2(250.0f, 200.0f), HVector2(250.0f, 260.0f));
		check(dragBegins == 1 && clicks[5] + clicks[6] + clicks[7] + clicks[8] + clicks[9] + clicks[10] == clicksBefore,
			"scroll: pressing a row and moving still drags the draggable panel (existing rule)");

		// 꺼진 칸: 휠 · 손잡이를 받지 않는다(입력은 막는다)
		box->ScrollTo(HVector2(0.0f, 100.0f));
		box->SetEnabled(false);
		manager->UpdateLayout(GTarget);
		const HVector2 disabledGrab = centerOf(thumb->GetLayoutRect());
		const bool bWheelConsumed = wheelAt(manager, 250.0f, 250.0f, -1.0f);
		pressMoveRelease(manager, disabledGrab, HVector2(disabledGrab.x, disabledGrab.y + 60.0f));
		check(bWheelConsumed && nearlyEqual(box->GetScrollOffset().y, 100.0f), "scroll: a disabled box takes neither the wheel nor the thumb");

		manager->Shutdown();
	}

	// ---- 일반 요소 휠 받기(GG-21, ProjectAH ER-021 · 결정 D-36 가)
	struct HWheelLog
	{
		int32           Count           = 0;
		PGameUIElement* Element         = nullptr;
		float32         Wheel           = 0.0f;
		float32         WheelHorizontal = 0.0f;
		HVector2        Position;
	};

	void watchWheel(PSharedPtr<PGameUIElement> element, HWheelLog& log)
	{
		element->OnWheel.AddLambda([&log](PGameUIElement& receiver, float32 wheel, float32 wheelHorizontal, const HVector2& position)
			{
				++log.Count;
				log.Element         = &receiver;
				log.Wheel           = wheel;
				log.WheelHorizontal = wheelHorizontal;
				log.Position        = position;
			});
	}

	// 파생 요소: onWheel 이 bAccept 를 돌려준다(false 면 바깥에 넘긴다).
	class PUITestWheelElement : public PGameUIElement
	{
	public:
		int32 Calls   = 0;
		bool  bAccept = true;

	protected:
		virtual bool onWheel(float32 wheel, float32 wheelHorizontal, const HVector2& position) override
		{
			++Calls;
			return bAccept;
		}
	};

	// 요소의 그리기 변환 피벗 자리(배치 사각형 왼쪽 위 + 피벗 × 크기).
	HVector2 pivotPointOf(const PGameUIElement& element)
	{
		const HRect& rect = element.GetLayoutRect();
		return HVector2(rect.left + element.GetPivot().x * (rect.right - rect.left), rect.top + element.GetPivot().y * (rect.bottom - rect.top));
	}

	// 포인터 기준 확대(검토서 ER-021 §3, 회전 없는 요소): q = CanvasToLocal(포인터), P = 피벗 자리, t' = t − (s' − s) × (q − P).
	void zoomAtPointer(PGameUIElement& element, const HVector2& pointerPoint, float32 factor)
	{
		const HVector2 local = element.CanvasToLocal(pointerPoint);
		const HVector2 pivot = pivotPointOf(element);
		HGameUIRenderTransform transform = element.GetRenderTransform();
		const HVector2 scale(transform.Scale.x * factor, transform.Scale.y * factor);
		transform.Translation = HVector2(transform.Translation.x - (scale.x - transform.Scale.x) * (local.x - pivot.x),
			transform.Translation.y - (scale.y - transform.Scale.y) * (local.y - pivot.y));
		transform.Scale = scale;
		element.SetRenderTransform(transform);
	}

	// 요청서(ER-021) 완료 기준: 휠을 켠 일반 요소가 휠(칸 수 · 포인터 자리)을 받는다. 휠을 안 켠 화면은 그대로.
	void testWheelInput()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		const HVector2 zero(0.0f, 0.0f);

		// 지도(입력 받기 · 휠 받기) (100, 100)-(500, 400) 안에 입력 받는 표시 (150, 150)-(200, 200)
		PSharedPtr<PGameUIImage> map = addPanel(widget, "Map", HLinearColor(0.2f, 0.3f, 0.2f, 1.0f), HRect(100.0f, 100.0f, 500.0f, 400.0f));
		map->SetHitTestVisible(true);
		const bool bOffByDefault = map->IsWheelInput() == false;
		map->SetWheelInput(true);
		PSharedPtr<PGameUIButton> pin = map->AddChild<PGameUIButton>(PName("Pin"));
		pin->SetLayout(zero, zero, HVector2(50.0f, 50.0f), HVector2(50.0f, 50.0f));
		HWheelLog mapLog;
		watchWheel(map, mapLog);
		PSharedPtr<PGameUIScrollBox> defaultBox = Allocate<PGameUIScrollBox>();
		check(bOffByDefault && map->IsWheelInput() && pin->IsWheelInput() == false && defaultBox->IsWheelInput(),
			"wheel: wheel input is off by default and on for scroll boxes; SetWheelInput turns it on");

		const bool bConsumed = wheelAt(manager, 300.0f, 300.0f, -1.0f);
		check(bConsumed && mapLog.Count == 1 && mapLog.Element == map.GetRawPointer() && nearlyEqual(mapLog.Wheel, -1.0f) && nearlyEqual(mapLog.WheelHorizontal, 0.0f)
			&& isPoint(mapLog.Position, HVector2(300.0f, 300.0f)), "accept ER-021: a plain element with wheel input gets OnWheel (element, notches, pointer) and the wheel is consumed");
		wheelAt(manager, 175.0f, 175.0f, 0.25f, -0.5f);
		check(mapLog.Count == 2 && nearlyEqual(mapLog.Wheel, 0.25f) && nearlyEqual(mapLog.WheelHorizontal, -0.5f) && isPoint(mapLog.Position, HVector2(175.0f, 175.0f)),
			"wheel: over a child without wheel input the wheel goes up to the element, fractional notches pass through as they are");

		// 휠 받기를 안 켠 패널 · 빈 곳은 지금과 같다
		PSharedPtr<PGameUIImage> plain = addPanel(widget, "Plain", HLinearColor(0.4f, 0.2f, 0.2f, 1.0f), HRect(600.0f, 100.0f, 800.0f, 300.0f));
		plain->SetHitTestVisible(true);
		const bool bPlainConsumed = wheelAt(manager, 700.0f, 200.0f, -1.0f);
		check(bPlainConsumed && mapLog.Count == 2 && wheelAt(manager, 900.0f, 900.0f, -1.0f) == false,
			"wheel: without wheel input the wheel is only consumed over UI and passes through over empty space (unchanged)");

		// 입력 받기를 안 켠 휠 요소는 빈 곳에서 받지 못한다(누르기와 같은 판정 길)
		PSharedPtr<PGameUIImage> ghost = addPanel(widget, "Ghost", HLinearColor(0.2f, 0.2f, 0.4f, 1.0f), HRect(600.0f, 500.0f, 800.0f, 700.0f));
		ghost->SetWheelInput(true);
		HWheelLog ghostLog;
		watchWheel(ghost, ghostLog);
		check(wheelAt(manager, 700.0f, 600.0f, -1.0f) == false && ghostLog.Count == 0, "wheel: a wheel element without hit testing gets nothing over its empty area (the same path as clicks)");

		// 꺼진 휠 요소는 건너뛴다(판정은 되어 소비)
		map->SetEnabled(false);
		const bool bDisabledConsumed = wheelAt(manager, 300.0f, 300.0f, -1.0f);
		map->SetEnabled(true);
		check(bDisabledConsumed && mapLog.Count == 2, "wheel: a disabled wheel element is skipped");

		manager->Shutdown();
	}

	// 포인터 기준 확대 식(t' = t − (s' − s)(q − P))과 배율 · 옮기기 트윈 한 묶음 — 돌리고 키운 부모 아래에서도 포인터 아래 점이 그대로.
	void testWheelZoom()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		const HVector2 zero(0.0f, 0.0f);

		// 판(가운데 (500, 500), 10° · 1.2배) 안의 지도(피벗 가운데, 1.3배 · 옮기기 (20, -10))
		PSharedPtr<PGameUIElement> board = widget->AddChild<PGameUIElement>(PName("Board"));
		board->SetLayout(zero, HVector2(0.5f, 0.5f), HVector2(500.0f, 500.0f), HVector2(800.0f, 600.0f));
		HGameUIRenderTransform boardTransform;
		boardTransform.Angle = 10.0f;
		boardTransform.Scale = HVector2(1.2f, 1.2f);
		board->SetRenderTransform(boardTransform);
		PSharedPtr<PGameUIImage> map = board->AddChild<PGameUIImage>(PName("Map"));
		map->SetLayout(zero, HVector2(0.5f, 0.5f), HVector2(400.0f, 300.0f), HVector2(400.0f, 300.0f));
		HGameUIRenderTransform mapTransform;
		mapTransform.Scale       = HVector2(1.3f, 1.3f);
		mapTransform.Translation = HVector2(20.0f, -10.0f);
		map->SetRenderTransform(mapTransform);
		map->SetHitTestVisible(true);
		map->SetWheelInput(true);
		map->OnWheel.AddLambda([](PGameUIElement& element, float32 wheel, float32 wheelHorizontal, const HVector2& pointerPoint)
			{
				zoomAtPointer(element, pointerPoint, std::pow(1.15f, wheel));
			});
		manager->UpdateLayout(GTarget);

		const HVector2 pointerPoint(520.0f, 470.0f);
		const HVector2 before = map->CanvasToLocal(pointerPoint);
		const bool bConsumed = wheelAt(manager, pointerPoint.x, pointerPoint.y, 2.0f);
		manager->UpdateLayout(GTarget);
		const HVector2 after = map->CanvasToLocal(pointerPoint);
		check(bConsumed && nearlyEqual(map->GetRenderTransform().Scale.x, 1.3f * 1.15f * 1.15f, 0.001f) && isPoint(before, after, 0.01f),
			"accept ER-021: zooming with t' = t - (s' - s)(q - P) keeps the point under the pointer (under a turned, scaled parent)");
		wheelAt(manager, pointerPoint.x, pointerPoint.y, -0.5f);
		manager->UpdateLayout(GTarget);
		check(isPoint(map->CanvasToLocal(pointerPoint), before, 0.01f) && nearlyEqual(map->GetRenderTransform().Scale.x, 1.3f * std::pow(1.15f, 1.5f), 0.001f),
			"wheel: a half notch zooms by 1.15 ^ 0.5 around the same point");

		// 부드럽게: 배율 · 옮기기 트윈을 한 동시 묶음(같은 길이 · 이징)으로 — 움직이는 동안에도 포인터 아래 점이 그대로
		const HVector2 tweenPointer(470.0f, 520.0f);
		const HVector2 anchorLocal = map->CanvasToLocal(tweenPointer);
		const HVector2 pivot       = pivotPointOf(*map);
		const HGameUIRenderTransform start = map->GetRenderTransform();
		const float32  targetScale = start.Scale.x * 1.6f;
		const HVector2 targetTranslation(start.Translation.x - (targetScale - start.Scale.x) * (anchorLocal.x - pivot.x),
			start.Translation.y - (targetScale - start.Scale.y) * (anchorLocal.y - pivot.y));
		PSharedPtr<PGameUITimeline> zoom = PGameUITimeline::Parallel();
		zoom->Add(HGameUITween::To(map, EGameUITweenProperty::Scale, HVector2(targetScale, targetScale), 0.2f, EGameUIEase::OutCubic))
			.Add(HGameUITween::To(map, EGameUITweenProperty::Translation, targetTranslation, 0.2f, EGameUIEase::OutCubic));
		widget->PlayTimeline(zoom);
		manager->Update(0.07f);
		manager->UpdateLayout(GTarget);
		const HVector2 midway   = map->CanvasToLocal(tweenPointer);
		const float32  midScale = map->GetRenderTransform().Scale.x;
		manager->Update(0.2f);
		manager->UpdateLayout(GTarget);
		const HVector2 end = map->CanvasToLocal(tweenPointer);
		check(midScale > start.Scale.x + 0.01f && midScale < targetScale - 0.01f && isPoint(midway, anchorLocal, 0.01f) && isPoint(end, anchorLocal, 0.01f)
			&& nearlyEqual(map->GetRenderTransform().Scale.x, targetScale, 0.001f), "accept ER-021: a parallel Scale + Translation tween (same length and ease) keeps the pointer point fixed while it plays");

		manager->Shutdown();
	}

	// 받는 순서: 스크롤 칸과 한 길(가장 안쪽부터, 칸은 그 방향 끝이면 바깥). 파생 onWheel 의 false · 스크롤 칸 휠 끄기는 바깥으로.
	void testWheelPriority()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);
		const HVector2 zero(0.0f, 0.0f);

		// 휠 지도 (100, 100)-(500, 500) 안의 스크롤 칸 (100, 100)-(400, 250)(줄 5개 → 넘길 양 90)
		PSharedPtr<PGameUIImage> map = addPanel(widget, "Map", HLinearColor(0.2f, 0.3f, 0.2f, 1.0f), HRect(100.0f, 100.0f, 500.0f, 500.0f));
		map->SetHitTestVisible(true);
		map->SetWheelInput(true);
		HWheelLog mapLog;
		watchWheel(map, mapLog);
		PSharedPtr<PGameUIScrollBox> inner = addScrollBox(map, "Inner", HRect(0.0f, 0.0f, 300.0f, 150.0f));
		HList<int32> innerClicks;
		addRows(inner, 5, 200.0f, 40.0f, 50.0f, innerClicks);
		HWheelLog innerLog;
		watchWheel(inner, innerLog);
		manager->UpdateLayout(GTarget);
		wheelAt(manager, 200.0f, 175.0f, -1.0f);
		wheelAt(manager, 200.0f, 175.0f, -1.0f);
		const bool bBoxFirst = nearlyEqual(inner->GetScrollOffset().y, 90.0f) && mapLog.Count == 0;
		wheelAt(manager, 200.0f, 175.0f, -1.0f);
		check(bBoxFirst && mapLog.Count == 1 && innerLog.Count == 0,
			"accept ER-021: a scroll box inside a wheel element scrolls first and passes the wheel on at its end (the box sends no OnWheel)");
		wheelAt(manager, 300.0f, 400.0f, -1.0f);
		check(mapLog.Count == 2, "wheel: over the element outside the inner box the element gets the wheel");

		// 스크롤 칸 안의 휠 요소: 그 요소 위에서는 요소(가장 안쪽), 다른 줄 위에서는 칸
		PSharedPtr<PGameUIScrollBox> list = addScrollBox(widget->GetRoot(), "List", HRect(600.0f, 100.0f, 900.0f, 300.0f));
		HList<int32> listClicks;
		addRows(list, 20, 280.0f, 40.0f, 50.0f, listClicks);
		PSharedPtr<PGameUIImage> zoomable = list->GetContent()->AddChild<PGameUIImage>(PName("Zoomable"));
		zoomable->SetLayout(zero, zero, HVector2(200.0f, 0.0f), HVector2(60.0f, 40.0f));   // 첫 줄 오른쪽 (800, 100)-(860, 140)
		zoomable->SetHitTestVisible(true);
		zoomable->SetWheelInput(true);
		HWheelLog zoomableLog;
		watchWheel(zoomable, zoomableLog);
		manager->UpdateLayout(GTarget);
		wheelAt(manager, 830.0f, 120.0f, -1.0f);
		const bool bInnermost = zoomableLog.Count == 1 && nearlyEqual(list->GetScrollOffset().y, 0.0f);
		wheelAt(manager, 700.0f, 220.0f, -1.0f);
		check(bInnermost && zoomableLog.Count == 1 && nearlyEqual(list->GetScrollOffset().y, 60.0f),
			"wheel: a wheel element inside a scroll box takes the wheel over itself, the box scrolls over other rows");

		// 파생 요소가 false 를 돌려주면 바깥(지도)이 받는다, true 면 거기서 끝
		PSharedPtr<PUITestWheelElement> derived = map->AddChild<PUITestWheelElement>(PName("Derived"));
		derived->SetLayout(zero, zero, HVector2(0.0f, 300.0f), HVector2(100.0f, 100.0f));   // (100, 400)-(200, 500)
		derived->SetHitTestVisible(true);
		derived->SetWheelInput(true);
		derived->bAccept = false;
		manager->UpdateLayout(GTarget);
		const int32 mapBefore = mapLog.Count;
		wheelAt(manager, 150.0f, 450.0f, -1.0f);
		const bool bPassed = derived->Calls == 1 && mapLog.Count == mapBefore + 1;
		derived->bAccept = true;
		wheelAt(manager, 150.0f, 450.0f, -1.0f);
		check(bPassed && derived->Calls == 2 && mapLog.Count == mapBefore + 1, "wheel: a derived onWheel returning false passes the wheel outward, true keeps it");

		// 스크롤 칸의 휠 받기를 끄면 바깥에 넘긴다
		inner->ScrollTo(HVector2(0.0f, 0.0f));
		inner->SetWheelInput(false);
		const int32 mapBefore2 = mapLog.Count;
		wheelAt(manager, 200.0f, 175.0f, -1.0f);
		check(inner->IsWheelInput() == false && nearlyEqual(inner->GetScrollOffset().y, 0.0f) && mapLog.Count == mapBefore2 + 1,
			"wheel: a scroll box with wheel input off passes the wheel to the outer element");

		manager->Shutdown();
	}

	// 끄는 동안: 휠을 켠 끄는 요소가 먼저(끄는 동안의 판정은 끄는 요소를 뺀다), 안 켠 끄는 요소면 그 아래 스크롤 칸이 넘긴다(지금 규칙).
	void testWheelWhileDragging()
	{
		PSharedPtr<PGameUIManager> manager = makeManager();
		PSharedPtr<JGGameWidget>   widget  = manager->PushWidget<JGGameWidget>(EGameUILayer::Game);

		PSharedPtr<PGameUIImage> map = addPanel(widget, "Map", HLinearColor(0.2f, 0.3f, 0.2f, 1.0f), HRect(100.0f, 100.0f, 500.0f, 500.0f));
		map->SetHitTestVisible(true);
		map->SetDraggable(true);
		map->SetWheelInput(true);
		HWheelLog mapLog;
		watchWheel(map, mapLog);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 300.0f, 300.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 320.0f, 300.0f), GTarget);
		const bool bDragging = manager->GetDraggedElement().GetRawPointer() == map.GetRawPointer();
		const bool bConsumed = wheelAt(manager, 320.0f, 300.0f, 1.0f);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 320.0f, 300.0f), GTarget);
		check(bDragging && bConsumed && mapLog.Count == 1 && nearlyEqual(mapLog.Wheel, 1.0f) && manager->GetDraggedElement().IsValid() == false,
			"accept ER-021: the dragged element with wheel input gets the wheel while it is dragged");

		PSharedPtr<PGameUIScrollBox> list = addScrollBox(widget->GetRoot(), "List", HRect(600.0f, 100.0f, 900.0f, 300.0f));
		HList<int32> clicks;
		addRows(list, 20, 280.0f, 40.0f, 50.0f, clicks);
		PSharedPtr<PGameUIImage> card = addPanel(widget, "Card", HLinearColor(0.6f, 0.5f, 0.2f, 1.0f), HRect(600.0f, 500.0f, 700.0f, 600.0f));
		card->SetHitTestVisible(true);
		card->SetDraggable(true);
		manager->UpdateLayout(GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Down, 650.0f, 550.0f), GTarget);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Move, 700.0f, 200.0f), GTarget);
		const bool bCardDragging = manager->GetDraggedElement().GetRawPointer() == card.GetRawPointer();
		wheelAt(manager, 700.0f, 200.0f, -1.0f);
		manager->HandlePointer(pointer(EGameUIPointerEventType::Up, 700.0f, 200.0f), GTarget);
		check(bCardDragging && nearlyEqual(list->GetScrollOffset().y, 60.0f) && mapLog.Count == 1,
			"wheel: while dragging an element without wheel input the scroll box under the pointer scrolls (unchanged)");

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
		testDefaultFont();
		testTextWrap();
		testTextInput();
		testTextFormat();
		testTextFormatAcceptance();
		testBoldAcceptance();
		testBoldStrength();
		testSharpBold();
		testRenderTransform();
		testFontAtlas();
		testDragDrop();
		testStackLifecycle();
		testRouting();
		testBackAction();
		testDisabledAndStyle();
		testTween();
		testTransition();
		testScrollBoxText();
		testScrollBoxRows();
		testScrollBoxRouting();
		testScrollBoxTransform();
		testScrollBoxMemory();
		testScrollBoxSmooth();
		testScrollBoxHorizontal();
		testScrollBoxHeld();
		testWheelInput();
		testWheelZoom();
		testWheelPriority();
		testWheelWhileDragging();
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
		"Run the game UI self tests (widget stacks, input routing, back, styles, layout, font, text wrap, text input, drag and drop, tweens, transitions, scroll boxes, wheel input). No GPU needed",
		&executeSelfTest);
}
