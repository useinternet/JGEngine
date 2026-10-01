#pragma once
#include "GUIDefines.h"

// InputTextWithHistory 가 호출 쪽 키 처리기에 먼저 넘기는 키(콘솔 명령 자동완성).
enum class EGUIInputTextKey
{
	Up,
	Down,
	Tab,
};

// InOutText 는 지금 입력줄 글자다. true 를 돌려주면 입력줄을 InOutText 로 바꾸고 기본 동작(↑/↓ = 히스토리)을 하지 않는다.
JG_DECLARE_DELEGATE_RET(HGUIInputTextKeyDelegate, bool, EGUIInputTextKey, PString&)


struct HPlotBarGroupsArguments
{
	PString  TitleName;
	HVector2 PlotSize = HVector2(-1, -1);;
	HList<PString>   GroupLabels;
	HList<PString>   DataLabels;
	HList<float64> Datas;
};

// InteractiveImage 의 이번 프레임 마우스 입력. 버튼 배열은 EGUIMouseButton 순서.
struct HGUIImageInput
{
	bool     bHovered = false;
	HVector2 LocalPosition;                                 // 이미지 왼쪽 위 기준 픽셀
	bool     bClicked[(int32)EGUIMouseButton::Count] = {};  // 이미지 위에서 이번 프레임에 누름
	bool     bDown[(int32)EGUIMouseButton::Count]    = {};  // 이미지에서 시작한 누름이 이어지는 중 (끌기)
	HVector2 MouseDelta;                                    // 이번 프레임 마우스 이동 (픽셀)
	float32  Wheel = 0.0f;                                  // 이미지 위에 있을 때만
	HVector2 ScreenPosition;                                // 이미지 왼쪽 위 (ImGui 화면 좌표. IME 창 자리 계산용)
};


// SegmentedBar 한 구간. Fraction 은 막대 전체 폭에 대한 비율(0~1).
struct HGUIBarSegment
{
	float32      Fraction = 0.0f;
	HLinearColor Color;
};

// PlotLines 인자. 모든 계열이 XValues 를 같이 쓰고, Datas 에는 계열 순서로 XValues.size() 개씩 넣는다.
struct HPlotLinesArguments
{
	PString  TitleName;                                   // 보이지 않는 ID
	HVector2 PlotSize = HVector2(-1, 160);
	HList<float64>      XValues;
	HList<PString>      SeriesLabels;
	HList<HLinearColor> SeriesColors;
	HList<float32>      SeriesFillAlphas;                 // 선 아래 면의 불투명도. 0 이거나 없으면 선만
	HList<float64>      Datas;
	float64 XMin = 0.0;                                   // X 축 범위(고정). Y 축은 0 부터 가장 큰 값보다 조금 위까지
	float64 XMax = 1.0;
	PString XAxisFormat = "%g";
	PString YAxisFormat = "%g";
	PString ValueFormat = "%g";                           // 마우스를 올렸을 때 풍선 도움말의 값
	bool    bShowLegend = true;                           // false 면 범례를 호출 쪽이 그린다 (LegendItem)
	HLinearColor GridColor     = HLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
	HLinearColor AxisTextColor = HLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	HLinearColor SurfaceColor  = HLinearColor(0.0f, 0.0f, 0.0f, 1.0f);   // 그래프 뒤 표면. 마우스 위치 표식의 테두리
};

class GUI_API HGUI
{
public:
	static void SameLine();
	static void NextLine();
	static void Text(const PString& InStr);
	static void Text(const PString& InStr, const HLinearColor& InColor);
	static bool InputText(const PString& InName, PString& OutStr);
	static void PlotBarGroups(const HPlotBarGroupsArguments& InArgs);
	static void PlotTest();
	static bool Selectable(const PString& InName, bool bSelected, ESelectableFlags InFlags = ESelectableFlags::None, const HVector2& InSize = HVector2(0, 0));
	static void Image(uint64 InTextureID, const HVector2& InSize);
	// 누른 프레임에 true(InOutValue 가 바뀐다).
	static bool Checkbox(const PString& InName, bool& InOutValue);
	// 한 줄 입력. 비어 있으면 InHint 를 흐리게 보여 준다. 글자가 바뀐 프레임에 true.
	// InWidth 0 은 기본 폭, 음수는 오른쪽 끝에서 그만큼 뺀 폭(-1 = 끝까지). 라벨은 그리지 않는다.
	static bool InputTextWithHint(const PString& InName, const PString& InHint, PString& InOutStr, float32 InWidth = 0.0f);
	// 직전 항목이 입력을 받는 중이면 true(입력줄에 포커스가 있다).
	static bool IsItemActive();
	// 직전 항목(입력줄 등) 바로 위에 붙는 툴팁 창. 맨 위에 그려지고 입력·포커스를 가져가지 않는다. 내용이 늘면 위로 자란다.
	// InName 은 창 ID 다(보이지 않는다). InExtraGap 은 줄 간격에 더 띄울 픽셀(항목이 패널 안에 있으면 패널 여백만큼). 짝으로 EndTooltip 을 부른다.
	static void BeginTooltipAboveItem(const PString& InName, float32 InExtraGap = 0.0f);
	static void EndTooltip();

	// 스크롤 영역. InSize 의 0 은 남은 공간 전부, 음수는 남은 공간에서 그만큼 뺀 크기다(ImGui 규칙). EndChild 는 항상 부른다.
	static void BeginChild(const PString& InName, const HVector2& InSize);
	// bStickToBottom: 스크롤이 맨 아래였으면 새 줄이 생겨도 맨 아래를 유지한다(로그 뷰).
	static void EndChild(bool bStickToBottom = false);
	// 한 줄 높이 + 줄 간격. BeginChild 의 음수 높이로 아래에 입력줄 자리를 남길 때 쓴다.
	static float32 GetFrameHeightWithSpacing();
	// 한 줄 입력(가로 전체). Enter 면 true. 창이 처음 뜰 때와 Enter 뒤에 포커스를 둔다.
	// ↑/↓ 로 InHistory 를 오간다. InOutHistoryPos 는 호출 쪽이 보관한다(-1 = 새 줄, 0.. = 오래된 것부터).
	// InKeyHandler 가 묶여 있으면 ↑/↓/Tab 을 먼저 넘긴다. Tab 은 이때만 입력줄이 받는다(없으면 ImGui 기본 = 다음 항목으로 이동).
	// InHint 는 비어 있을 때 흐리게 보이는 안내 글자다.
	static bool InputTextWithHistory(const PString& InName, PString& InOutStr, const HList<PString>& InHistory, int32& InOutHistoryPos,
		const HGUIInputTextKeyDelegate& InKeyHandler = HGUIInputTextKeyDelegate(), const PString& InHint = PString());

	// 누른 프레임에 true. InSize 0 은 글자에 맞춘 크기.
	static bool Button(const PString& InName, const HVector2& InSize = HVector2(0, 0));
	static void Separator();
	// 접을 수 있는 구역 머리. 펼쳐져 있으면 true (닫는 호출 없음).
	static bool CollapsingHeader(const PString& InName, bool bDefaultOpen = true);
	// 마우스 입력을 받는 이미지 (씬 뷰포트). 이미지 위에서 누른 버튼은 창 이동 대신 이 항목이 받는다.
	// InName 은 같은 창 안에서 항목을 구분하는 ID 다 (보이지 않는다).
	static HGUIImageInput InteractiveImage(const PString& InName, uint64 InTextureID, const HVector2& InSize);
private:

public:
	// ---- 대시보드 (통계 창 등) ----
	// 앞 항목과 InSpacing 픽셀 띄워 같은 줄에 놓는다.
	static void SameLine(float32 InSpacing);
	static void Spacing();
	static HVector2 GetContentRegionAvail();
	// GUI 가 시작된 뒤 흐른 시간(초)
	static float64 GetTime();
	static void PushFont(EGUIFont InFont);
	static void PopFont();
	static void PushStyleColor(EGUIColor InTarget, const HLinearColor& InColor);
	static void PopStyleColor(int32 InCount = 1);
	// 남은 폭의 오른쪽 끝에 붙여 쓴다 (표의 숫자 열).
	static void TextRight(const PString& InStr);
	static void TextRight(const PString& InStr, const HLinearColor& InColor);
	// 바로 앞 항목에 마우스가 올라가 있으면 풍선 도움말을 띄운다.
	static void ItemTooltip(const PString& InText);
	// 창 안쪽(제목 · 스크롤바 제외)을 InColor 로 칠한다. 창의 다른 항목보다 먼저 부른다.
	static void FillWindowBackground(const HLinearColor& InColor);
	// 배경 · 테두리 · 둥근 모서리 · 안쪽 여백이 있는 구역. InSize 의 0 은 가로면 남은 폭, 세로면 내용 높이에 맞춘다. EndPanel 은 항상 부른다.
	static void BeginPanel(const PString& InName, const HVector2& InSize, const HLinearColor& InBackground, const HLinearColor& InBorder);
	static void EndPanel();
	// 가로 막대. 구간을 왼쪽부터 이어 칠하고 남은 부분은 InTrack 으로 칠한다. 구간 사이는 2px 띄운다.
	// InSize.x 0 은 남은 폭 전부. 한 줄 높이보다 낮으면 줄 가운데에 놓는다.
	static void SegmentedBar(const HList<HGUIBarSegment>& InSegments, const HVector2& InSize, const HLinearColor& InTrack);
	// 글자 딱지 (상태 표시). 뜻은 글자가 전하고 색은 보조다.
	static void Badge(const PString& InText, const HLinearColor& InBackground, const HLinearColor& InTextColor);
	// 범례 한 항목: 색 사각형 + 이름
	static void LegendItem(const PString& InLabel, const HLinearColor& InColor);
	// 표. BeginTable 이 true 일 때만 채우고 EndTable 을 부른다(ImGui 규칙).
	static bool BeginTable(const PString& InName, int32 InColumnCount);
	// InWidth > 0 이면 고정 폭, 0 이면 남은 폭을 나눠 쓴다. bAlignRight 는 머리글을 오른쪽에 붙인다(숫자 열).
	static void TableSetupColumn(const PString& InLabel, float32 InWidth = 0.0f, bool bAlignRight = false);
	static void TableHeadersRow();
	static void TableNextRow();
	static bool TableNextColumn();
	static void EndTable();
	// 탭. BeginTabBar · BeginTabItem 이 true 일 때만 각각 EndTabBar · EndTabItem 을 부른다.
	static bool BeginTabBar(const PString& InName);
	static void EndTabBar();
	static bool BeginTabItem(const PString& InLabel);
	static void EndTabItem();
	// 선 그래프. 마우스를 올리면 가장 가까운 표본에 세로선을 긋고 값을 보여 준다.
	static void PlotLines(const HPlotLinesArguments& InArgs);

public:
	// 키보드 (씬 뷰포트가 게임 UI 로 Esc · 글자 입력을 넘길 때). 지금 그리는 창(자식 창 포함)에 포커스가 있나.
	static bool IsWindowFocused();
	// 이번 프레임에 눌렸나. bRepeat 면 누르고 있는 동안 키 반복 간격마다 다시 true.
	static bool IsKeyPressed(EGUIKey InKey, bool bRepeat = false);
	static bool IsCtrlDown();
	static bool IsShiftDown();
	static bool IsAltDown();
	// 이번 프레임에 들어온 확정 글자(WM_CHAR · IME 확정). 제어 문자(Backspace · Enter 의 글자 등)도 그대로 있다. BMP 밖 글자는 U+FFFD.
	static void GetInputCharacters(HList<uint32>& OutCodepoints);
	// 편집 키(방향 · Home · End · PageUp · PageDown · Enter · Tab · Space · Backspace · Delete · Esc)를 ImGui 키보드 내비게이션에서 가져온다.
	// 글자를 편집하는 동안 매 프레임 부른다(다음 프레임까지 내비게이션이 그 키를 쓰지 않는다). IsKeyPressed 로는 계속 읽힌다.
	static void ClaimTextEditKeys();
	// 이번 프레임 OS IME 조합 · 후보 창 자리(ImGui 화면 좌표, 커서 위 끝)와 줄 높이. 지금 창의 뷰포트에 둔다. ImGui 입력란이 활성이면 그쪽이 덮어쓴다.
	static void SetTextInputPosition(const HVector2& InScreenPosition, float32 InLineHeight);
	// 클립보드(UTF-8). 비었으면 빈 글.
	static PString GetClipboardText();
	static void SetClipboardText(const PString& InText);

public:
	// ---- 편집기 창 테마 · 콘솔 ----
	// 화면에 보일 색(sRGB 0xRRGGBB)을 GUI 에 넘길 선형 색으로 바꾼다. GUI 는 선형 버퍼에 그린 뒤 sRGB 로 인코딩해 내보내므로
	// sRGB 값을 그대로 넘기면 옅게 뜬다(ImGui 창 배경 0.06 이 #454545 로 보인다). 출력 색 공간이 바뀌면(Graphics 5-14) 이 함수를 고친다.
	static HLinearColor DisplayColor(uint32 InRGB, float32 InAlpha = 1.0f);
	// 모서리 · 테두리 · 여백 같은 모양 값을 잠시 바꾼다. 값의 형은 EGUIStyleVar 주석을 따른다. Push 한 만큼 Pop 한다.
	static void PushStyleVar(EGUIStyleVar InTarget, float32 InValue);
	static void PushStyleVar(EGUIStyleVar InTarget, const HVector2& InValue);
	static void PopStyleVar(int32 InCount = 1);
	// 입력칸 · 버튼 한 줄 높이(글자 + 위아래 여백)
	static float32 GetFrameHeight();
	static float32 CalcTextWidth(const PString& InText);
	// 같은 줄의 입력칸 · 버튼과 글자 높이를 맞춘다(입력칸 앞 글자 앞에 부른다).
	static void AlignTextToFramePadding();
	// 체크박스. bMixed 면 일부만 켜진 상태(가로줄)로 그린다("전체" 항목). 누르면 InOutValue 가 바뀌고 true.
	static bool Checkbox(const PString& InName, bool& InOutValue, bool bMixed);
	// 켜고 끄는 알약 모양 버튼(필터 칩): 색 점 + InLabel + 흐린 InTrailing(개수 등). 켜져 있으면 InAccent 로 물든다. 누른 프레임에 true(InOutOn 이 바뀐다).
	// InId 는 같은 창 안의 ID 다(보이지 않는다 — 라벨 · 개수가 바뀌어도 같은 항목).
	static bool ToggleChip(const PString& InId, const PString& InLabel, const PString& InTrailing, const HLinearColor& InAccent, bool& InOutOn);
	// 드롭다운. 닫혀 있을 때 InPreview 를 보인다. BeginCombo 가 true 일 때만 내용을 그리고 EndCombo 를 부른다(ImGui 규칙).
	// 안의 체크박스를 눌러도 닫히지 않는다(바깥을 누르거나 Esc 로 닫힌다). InWidth 0 은 기본 폭.
	static bool BeginCombo(const PString& InName, const PString& InPreview, float32 InWidth = 0.0f);
	static void EndCombo();
	// 다음에 그릴 한 줄 뒤를 보이는 창 폭만큼 InBackground 로 칠하고 왼쪽 끝에 InAccentWidth 픽셀 막대를 InAccent 로 칠한다(로그 줄 · 목록 선택 강조).
	// 커서는 움직이지 않는다. 이어진 줄을 칠하면 줄 간격까지 덮어 한 띠로 보인다.
	static void HighlightLine(const HLinearColor& InBackground, const HLinearColor& InAccent, float32 InAccentWidth = 2.0f);
};
