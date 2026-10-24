#include "PCH/PCH.h"
#include "GUI.h"
#include "Imgui/imgui.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

// gui.selftest [-exit] — 컨텐츠 브라우저용 HGUI 감싸개(TreeNode · TreePop · IsItemDoubleClicked, Engine-29)를 확인한다. 에디터 DevConsole 에서 쓴다.
// 따로 만든 ImGui 컨텍스트(창 · 렌더러 없음)에 마우스 입력을 흉내 내 몇 프레임 돌린다. 콘솔 명령은 GUI 프레임 밖에서 돌므로
// 에디터 컨텍스트를 잠시 바꿔도 된다. 결과는 끝날 때 로그 한 줄(OK/FAILED)로 나온다.
namespace
{
	// 한 프레임에 그린 항목의 결과와 자리(ImGui 화면 좌표)
	struct HGUISelfTestFrame
	{
		bool   bRootOpen    = false;
		bool   bRootClicked = false;
		ImVec2 RootMin;
		ImVec2 RootMax;
		bool   bLeafOpen    = false;
		bool   bLeafClicked = false;
		ImVec2 LeafMin;
		ImVec2 LeafMax;
		bool   bDefaultOpen = false;
		bool   bTileDoubleClicked = false;
		ImVec2 TileMin;
		ImVec2 TileMax;
	};

	class HGUISelfTest
	{
		int32 _passed   = 0;
		int32 _failures = 0;

	public:
		bool Run()
		{
			ImGuiContext* editorContext = ImGui::GetCurrentContext();
			ImGuiContext* testContext   = ImGui::CreateContext();
			ImGui::SetCurrentContext(testContext);

			ImGuiIO& io = ImGui::GetIO();
			io.IniFilename = nullptr;   // 에디터 imgui.ini 를 건드리지 않는다
			io.LogFilename = nullptr;
			io.DisplaySize = ImVec2(800.0f, 600.0f);
			io.DeltaTime   = 1.0f / 60.0f;
			io.Fonts->AddFontDefault();
			io.Fonts->Build();

			runCases();

			// 지우는 컨텍스트가 지금 컨텍스트가 아니면 DestroyContext 가 지금 컨텍스트(에디터)를 그대로 둔다.
			ImGui::SetCurrentContext(editorContext);
			ImGui::DestroyContext(testContext);

			if (_failures == 0)
			{
				JG_LOG(GUISelfTest, ELogLevel::Info, "gui.selftest: OK (%d/%d)", _passed, _passed);
				return true;
			}
			JG_LOG(GUISelfTest, ELogLevel::Error, "gui.selftest: FAILED (%d failures, %d passed)", _failures, _passed);
			return false;
		}

	private:
		void runCases()
		{
			HGUISelfTestFrame f = frame();
			check(f.bRootOpen == false, "TreeNode: closed by default");
			check(f.bDefaultOpen, "TreeNode: bDefaultOpen opens on the first frame");
			check(f.bLeafOpen, "TreeNode: a leaf returns true (TreePop like an open node)");

			// 이름을 누르면 고르기(OutClicked)이고 펼침은 그대로다.
			const ImVec2 rootLabel(f.RootMin.x + 60.0f, centerY(f.RootMin, f.RootMax));
			HGUISelfTestFrame pressed = click(rootLabel);
			f = frame();
			check(pressed.bRootClicked, "TreeNode: a label click sets OutClicked");
			check(f.bRootOpen == false, "TreeNode: a label click does not open the node");

			// 화살표를 누르면 펼치고, 고르기로 치지 않는다.
			const ImVec2 rootArrow(f.RootMin.x + ImGui::GetFontSize() * 0.5f, centerY(f.RootMin, f.RootMax));
			pressed = click(rootArrow);
			f = frame();
			check(pressed.bRootClicked == false, "TreeNode: an arrow click does not set OutClicked");
			check(f.bRootOpen, "TreeNode: an arrow click opens the node");

			// 펼친 노드의 자식 줄이 생겨 아래 항목이 내려갔다 — 이번 프레임 자리를 쓴다.
			const ImVec2 leafLabel(f.LeafMin.x + 60.0f, centerY(f.LeafMin, f.LeafMax));
			pressed = click(leafLabel);
			check(pressed.bLeafClicked, "TreeNode: a leaf label click sets OutClicked");

			const ImVec2 tile((f.TileMin.x + f.TileMax.x) * 0.5f, centerY(f.TileMin, f.TileMax));
			pressed = click(tile);
			check(pressed.bTileDoubleClicked == false, "IsItemDoubleClicked: one click is not a double click");
			pressed = press(true);
			press(false);
			check(pressed.bTileDoubleClicked, "IsItemDoubleClicked: a second click within the double-click time");
		}

		// 그 자리로 옮긴 뒤 누르고 뗀다. 누른 프레임의 결과를 돌려준다(누름 · 뗌은 프레임을 나눠 넣는다).
		HGUISelfTestFrame click(const ImVec2& inPosition)
		{
			ImGui::GetIO().AddMousePosEvent(inPosition.x, inPosition.y);
			frame();
			HGUISelfTestFrame pressed = press(true);
			press(false);
			return pressed;
		}

		HGUISelfTestFrame press(bool bDown)
		{
			ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, bDown);
			return frame();
		}

		// 매 프레임 같은 것을 그린다: Root(자식 하나) · Leaf(고른 잎) · Opened(처음부터 펼침) · Tile(두 번 누르기)
		HGUISelfTestFrame frame()
		{
			HGUISelfTestFrame result;
			ImGui::NewFrame();
			ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
			ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f));
			ImGui::Begin("GUISelfTest", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

			result.bRootOpen = HGUI::TreeNode("Root", false, result.bRootClicked);
			result.RootMin   = ImGui::GetItemRectMin();
			result.RootMax   = ImGui::GetItemRectMax();
			if (result.bRootOpen)
			{
				bool bChildClicked = false;
				if (HGUI::TreeNode("Child", false, bChildClicked, true))
				{
					HGUI::TreePop();
				}
				HGUI::TreePop();
			}

			result.bLeafOpen = HGUI::TreeNode("Leaf", true, result.bLeafClicked, true);
			result.LeafMin   = ImGui::GetItemRectMin();
			result.LeafMax   = ImGui::GetItemRectMax();
			if (result.bLeafOpen)
			{
				HGUI::TreePop();
			}

			bool bOpenedClicked = false;
			result.bDefaultOpen = HGUI::TreeNode("Opened", false, bOpenedClicked, false, true);
			if (result.bDefaultOpen)
			{
				HGUI::TreePop();
			}

			HGUI::Selectable("Tile", false);
			result.bTileDoubleClicked = HGUI::IsItemDoubleClicked();
			result.TileMin = ImGui::GetItemRectMin();
			result.TileMax = ImGui::GetItemRectMax();

			ImGui::End();
			ImGui::EndFrame();
			return result;
		}

		static float32 centerY(const ImVec2& inMin, const ImVec2& inMax)
		{
			return (inMin.y + inMax.y) * 0.5f;
		}

		void check(bool bCondition, const char* inWhat)
		{
			if (bCondition)
			{
				++_passed;
				JG_LOG(GUISelfTest, ELogLevel::Info, "[PASS] %s", PString(inWhat));
				return;
			}
			++_failures;
			JG_LOG(GUISelfTest, ELogLevel::Error, "[FAIL] %s", PString(inWhat));
		}
	};

	bool executeGUISelfTest(const HConsoleCommandArgs& inArgs)
	{
		HGUISelfTest selfTest;
		const bool bPassed = selfTest.Run();
		if (inArgs.Has("exit"))
		{
			GCoreSystem::RequestExit(bPassed ? 0 : 1);
		}
		return bPassed;
	}

	HAutoConsoleCommand GUISelfTestCommand(
		"gui.selftest",
		"gui.selftest [-exit]",
		"Simulate mouse input on a separate ImGui context and check the HGUI tree node and double-click wrappers",
		&executeGUISelfTest);
}
