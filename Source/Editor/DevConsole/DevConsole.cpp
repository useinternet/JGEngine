#include "PCH/PCH.h"
#include "DevConsole.h"
#include <algorithm>

namespace
{
	constexpr uint64  MaxHistoryLines   = 64;
	constexpr int32   MaxCandidateRows  = 10;
	constexpr int32   MaxCategoryColumn = 16;     // 카테고리 열 최대 글자 수(더 길면 자른다)
	constexpr float32 PanelGap          = 6.0f;   // 도구줄 · 로그 · 입력줄 패널 사이
	constexpr float32 InputBarPadding   = 22.0f;  // 입력줄 패널 = 입력칸 높이 + 안쪽 여백 위아래 10 + 테두리 1 · 1

	// 두 sRGB 색을 섞는다(메모리 통계 창과 같은 방식). 줄 강조 띠는 강조색을 패널 색 쪽으로 섞은 같은 계열의 어두운 색이다.
	uint32 mixRGB(uint32 InFrom, uint32 InTo, float32 InAmount)
	{
		uint32 Result = 0;
		for (uint32 Shift = 0; Shift <= 16; Shift += 8)
		{
			const float32 From = static_cast<float32>((InFrom >> Shift) & 0xFF);
			const float32 To   = static_cast<float32>((InTo >> Shift) & 0xFF);
			Result |= static_cast<uint32>(From + (To - From) * InAmount + 0.5f) << Shift;
		}
		return Result;
	}

	// 팔레트 — 메모리 통계 창(DevStatistics/MemoryStatistics.cpp)과 같은 값(sRGB). HGUI::DisplayColor 로 바꿔 넘긴다.
	const uint32 SurfaceRGB  = 0x1a1a19;
	const uint32 RaisedRGB   = 0x242423;
	const uint32 AccentRGB   = 0x3987e5;
	const uint32 WarningRGB  = 0xfab219;
	const uint32 CriticalRGB = 0xd03b3b;

	const HLinearColor ColorPage          = HGUI::DisplayColor(0x0d0d0d);
	const HLinearColor ColorSurface       = HGUI::DisplayColor(SurfaceRGB);
	const HLinearColor ColorField         = HGUI::DisplayColor(0x121211);    // 검색칸 · 드롭다운 바탕
	const HLinearColor ColorFieldHover    = HGUI::DisplayColor(0x232322);
	const HLinearColor ColorRaised        = HGUI::DisplayColor(RaisedRGB);   // 팝업 · 미리보기
	const HLinearColor ColorRaisedHover   = HGUI::DisplayColor(0x2e2e2c);
	const HLinearColor ColorBorder        = HGUI::DisplayColor(0x313130);
	const HLinearColor ColorBorderStrong  = HGUI::DisplayColor(0x464644);
	const HLinearColor ColorTextPrimary   = HGUI::DisplayColor(0xffffff);
	const HLinearColor ColorTextSecondary = HGUI::DisplayColor(0xc3c2b7);    // Info 줄 본문
	const HLinearColor ColorTextMuted     = HGUI::DisplayColor(0x898781);    // 카테고리 열 · 안내 · 개수
	const HLinearColor ColorAccent        = HGUI::DisplayColor(AccentRGB);   // Info 칩 · 선택 · 프롬프트
	const HLinearColor ColorCommand       = HGUI::DisplayColor(0x6aa7f0);    // 입력한 명령의 에코 줄
	const HLinearColor ColorWarning       = HGUI::DisplayColor(WarningRGB);
	const HLinearColor ColorCritical      = HGUI::DisplayColor(CriticalRGB);
	const HLinearColor ColorErrorText     = HGUI::DisplayColor(0xf07070);    // 어두운 바탕에서 읽히는 밝은 빨강
	const HLinearColor ColorWarningRow    = HGUI::DisplayColor(mixRGB(SurfaceRGB, WarningRGB, 0.10f));
	const HLinearColor ColorErrorRow      = HGUI::DisplayColor(mixRGB(SurfaceRGB, CriticalRGB, 0.14f));
	const HLinearColor ColorSelectedRow   = HGUI::DisplayColor(mixRGB(RaisedRGB, AccentRGB, 0.30f));
	const HLinearColor ColorFocusBorder   = HGUI::DisplayColor(mixRGB(SurfaceRGB, AccentRGB, 0.70f));
	const HLinearColor ColorScrollGrab    = HGUI::DisplayColor(0x3a3a38);
	const HLinearColor ColorScrollGrabHot = HGUI::DisplayColor(0x4c4c49);

	// 이 창 안의 항목(입력칸 · 체크박스 · 드롭다운 · 스크롤바 · 팝업)을 팔레트로 물들인다. 창 틀 · 탭은 다른 창과 같게 둔다.
	// 넣은 개수를 돌려준다(그만큼 PopStyleColor).
	int32 pushThemeColors()
	{
		int32 Count = 0;
		const auto push = [&Count](EGUIColor InTarget, const HLinearColor& InColor)
			{
				HGUI::PushStyleColor(InTarget, InColor);
				++Count;
			};

		push(EGUIColor::TextDisabled, ColorTextMuted);
		push(EGUIColor::Border, ColorBorder);
		push(EGUIColor::FrameBackground, ColorField);
		push(EGUIColor::FrameBackgroundHovered, ColorFieldHover);
		push(EGUIColor::FrameBackgroundActive, ColorFieldHover);
		push(EGUIColor::CheckMark, ColorAccent);
		push(EGUIColor::Header, ColorRaisedHover);
		push(EGUIColor::HeaderHovered, ColorRaisedHover);
		push(EGUIColor::HeaderActive, ColorSelectedRow);
		push(EGUIColor::Button, ColorField);
		push(EGUIColor::ButtonHovered, ColorFieldHover);
		push(EGUIColor::ButtonActive, ColorRaisedHover);
		push(EGUIColor::PopupBackground, ColorRaised);
		push(EGUIColor::ScrollbarBackground, ColorSurface);
		push(EGUIColor::ScrollbarGrab, ColorScrollGrab);
		push(EGUIColor::ScrollbarGrabHovered, ColorScrollGrabHot);
		push(EGUIColor::ScrollbarGrabActive, ColorAccent);
		push(EGUIColor::Separator, ColorBorder);
		return Count;
	}

	// 세 자리마다 쉼표 (1,284)
	PString formatCount(uint64 InValue)
	{
		const std::string Digits = std::to_string(InValue);
		std::string Result;
		Result.reserve(Digits.size() + Digits.size() / 3);
		for (size_t Index = 0; Index < Digits.size(); ++Index)
		{
			if (Index > 0 && (Digits.size() - Index) % 3 == 0)
			{
				Result.push_back(',');
			}
			Result.push_back(Digits[Index]);
		}
		return PString(Result.c_str());
	}

	char toLowerAscii(char InChar)
	{
		if (InChar >= 'A' && InChar <= 'Z')
		{
			return static_cast<char>(InChar - 'A' + 'a');
		}

		return InChar;
	}

	bool isBlank(char InChar)
	{
		return InChar == ' ' || InChar == '\t';
	}

	// InLowerWord 는 이미 소문자다. ASCII 만 대소문자를 무시한다(한글 등 나머지 바이트는 그대로 비교).
	bool containsIgnoreCase(const HRawString& InText, const HRawString& InLowerWord)
	{
		const HRawString::const_iterator Found = std::search(InText.begin(), InText.end(), InLowerWord.begin(), InLowerWord.end(),
			[](char InTextChar, char InWordChar)
			{
				return toLowerAscii(InTextChar) == InWordChar;
			});
		return Found != InText.end();
	}

	// 검색 "a, b, -c" → 보일 낱말 {a, b}, 숨길 낱말 {c}. 낱말의 앞뒤 공백을 떼고 소문자로 바꾸며 빈 낱말은 버린다.
	struct HLogFilter
	{
		HList<HRawString> Includes;
		HList<HRawString> Excludes;
	};

	HLogFilter parseLogFilter(const PString& InFilterText)
	{
		HLogFilter Filter;
		const HRawString& Text = InFilterText.GetRawString();

		uint64 Begin = 0;
		while (Begin <= Text.size())
		{
			uint64 End = Text.find(',', Begin);
			if (End == HRawString::npos)
			{
				End = Text.size();
			}

			uint64 WordBegin = Begin;
			uint64 WordEnd   = End;
			while (WordBegin < WordEnd && isBlank(Text[WordBegin]))
			{
				++WordBegin;
			}
			while (WordEnd > WordBegin && isBlank(Text[WordEnd - 1]))
			{
				--WordEnd;
			}

			bool bExclude = false;
			if (WordBegin < WordEnd && Text[WordBegin] == '-')
			{
				bExclude = true;
				++WordBegin;
			}

			if (WordBegin < WordEnd)
			{
				HRawString Word = Text.substr(WordBegin, WordEnd - WordBegin);
				std::transform(Word.begin(), Word.end(), Word.begin(), &toLowerAscii);
				if (bExclude)
				{
					Filter.Excludes.push_back(Word);
				}
				else
				{
					Filter.Includes.push_back(Word);
				}
			}

			Begin = End + 1;
		}

		return Filter;
	}

	bool passLogFilter(const HRawString& InLine, const HLogFilter& InFilter)
	{
		for (const HRawString& Word : InFilter.Excludes)
		{
			if (containsIgnoreCase(InLine, Word))
			{
				return false;
			}
		}

		if (InFilter.Includes.empty())
		{
			return true;
		}

		for (const HRawString& Word : InFilter.Includes)
		{
			if (containsIgnoreCase(InLine, Word))
			{
				return true;
			}
		}

		return false;
	}

	// "[Category]: 본문" → Category, 본문. 이 모양이 아니면 빈 이름과 줄 전체.
	void splitLogCategory(const HRawString& InText, HRawString& OutCategory, HRawString& OutBody)
	{
		OutCategory.clear();
		if (InText.size() > 3 && InText[0] == '[')
		{
			const uint64 Close = InText.find("]: ");
			if (Close != HRawString::npos && Close > 1)
			{
				OutCategory = InText.substr(1, Close - 1);
				OutBody     = InText.substr(Close + 3);
				return;
			}
		}

		OutBody = InText;
	}

	PString categoryLabel(const PString& InName)
	{
		return InName.Empty() ? PString("(no category)") : InName;
	}

	// 입력줄의 첫 낱말(명령 이름)과 그 뒤에 공백이 있는지(인자를 치는 중인지).
	void splitCommandName(const PString& InText, PString& OutName, bool& bOutTypingArgs)
	{
		const HRawString& Text = InText.GetRawString();

		uint64 NameBegin = 0;
		while (NameBegin < Text.size() && isBlank(Text[NameBegin]))
		{
			++NameBegin;
		}

		uint64 NameEnd = NameBegin;
		while (NameEnd < Text.size() && isBlank(Text[NameEnd]) == false)
		{
			++NameEnd;
		}

		OutName        = Text.substr(NameBegin, NameEnd - NameBegin).c_str();
		bOutTypingArgs = NameEnd < Text.size();
	}
}

void JGDevConsole::OnInitialize()
{
	InputKeyHandler.BindRaw(this, &JGDevConsole::onInputKey);
}

void JGDevConsole::OnLayout(const HWidgetLayout& InLayout)
{

}

void JGDevConsole::OnGenerateGUI()
{
	refreshLogLines();

	HGUI::FillWindowBackground(ColorPage);
	const int32 ThemeColorCount = pushThemeColors();
	HGUI::PushStyleVar(EGUIStyleVar::FrameRounding, 4.0f);
	HGUI::PushStyleVar(EGUIStyleVar::FrameBorderSize, 1.0f);
	HGUI::PushStyleVar(EGUIStyleVar::PopupRounding, 6.0f);
	HGUI::PushStyleVar(EGUIStyleVar::ScrollbarSize, 10.0f);
	HGUI::PushStyleVar(EGUIStyleVar::ScrollbarRounding, 4.0f);

	generateToolbar();
	generateLogView();
	generateInputBar();

	HGUI::PopStyleVar(5);
	HGUI::PopStyleColor(ThemeColorCount);
}

PString  JGDevConsole::GetTitleName() const
{
	return "DevConsole";
}

const  HGuid& JGDevConsole::GetGUID() const
{
	return GetStaticGUID();
}

void JGDevConsole::refreshLogLines()
{
	const uint64 Serial = GLogGlobalSystem::GetInstance().GetLogSerial();
	if (Serial == LogSerial)
	{
		return;
	}

	LogSerial = Serial;
	GLogGlobalSystem::GetInstance().GetRecentLogs(LogLines);

	InfoCount    = 0;
	WarningCount = 0;
	ErrorCount   = 0;
	for (const HLogLine& Line : LogLines)
	{
		if (Line.Level == ELogLevel::Error || Line.Level == ELogLevel::Critical)
		{
			++ErrorCount;
		}
		else if (Line.Level == ELogLevel::Warning)
		{
			++WarningCount;
		}
		else
		{
			++InfoCount;
		}
	}

	rebuildLogRows();
	rebuildVisibleLines();
}

void JGDevConsole::rebuildLogRows()
{
	// 1) 줄마다 카테고리 · 본문을 나눈다.
	HList<PString> LineCategoryNames;
	LineCategoryNames.reserve(LogLines.size());
	LogRows.clear();
	LogRows.resize(LogLines.size());
	for (uint64 LineIndex = 0; LineIndex < LogLines.size(); ++LineIndex)
	{
		HRawString Category;
		HRawString Body;
		splitLogCategory(LogLines[LineIndex].Text.GetRawString(), Category, Body);

		HDevConsoleLogRow& Row = LogRows[LineIndex];
		Row.Body         = Body.c_str();
		Row.Level        = LogLines[LineIndex].Level;
		Row.bCommandEcho = Category == "ConsoleCommand" && Body.starts_with("> ");
		LineCategoryNames.push_back(PString(Category.c_str()));
	}

	// 2) 목록에 없는 카테고리는 켜진 채로 넣는다(이름순). 버퍼에서 밀려난 카테고리도 지우지 않는다(끈 상태를 지킨다).
	HHashMap<PString, uint32> IndexByName;
	for (uint32 CategoryIndex = 0; CategoryIndex < static_cast<uint32>(Categories.size()); ++CategoryIndex)
	{
		IndexByName.emplace(Categories[CategoryIndex].Name, CategoryIndex);
	}

	bool bAddedCategory = false;
	for (const PString& Name : LineCategoryNames)
	{
		if (IndexByName.contains(Name))
		{
			continue;
		}

		HDevConsoleLogCategory NewCategory;
		NewCategory.Name = Name;
		IndexByName.emplace(Name, static_cast<uint32>(Categories.size()));
		Categories.push_back(NewCategory);
		bAddedCategory = true;
	}

	if (bAddedCategory)
	{
		std::sort(Categories.begin(), Categories.end(), [](const HDevConsoleLogCategory& InLeft, const HDevConsoleLogCategory& InRight)
			{
				return InLeft.Name.GetRawString() < InRight.Name.GetRawString();
			});

		IndexByName.clear();
		for (uint32 CategoryIndex = 0; CategoryIndex < static_cast<uint32>(Categories.size()); ++CategoryIndex)
		{
			IndexByName.emplace(Categories[CategoryIndex].Name, CategoryIndex);
		}
	}

	// 3) 줄 수를 세고 카테고리 열을 채운다. 열 폭은 지금 줄이 있는 카테고리 중 가장 긴 이름(최대 MaxCategoryColumn).
	for (HDevConsoleLogCategory& Category : Categories)
	{
		Category.LineCount = 0;
	}

	for (uint64 LineIndex = 0; LineIndex < LogRows.size(); ++LineIndex)
	{
		const uint32 CategoryIndex = IndexByName.at(LineCategoryNames[LineIndex]);
		LogRows[LineIndex].CategoryIndex = CategoryIndex;
		++Categories[CategoryIndex].LineCount;
	}

	int32 ColumnWidth = 4;
	for (const HDevConsoleLogCategory& Category : Categories)
	{
		if (Category.LineCount > 0)
		{
			ColumnWidth = HMath::Max(ColumnWidth, static_cast<int32>(categoryLabel(Category.Name).Length()));
		}
	}
	ColumnWidth = HMath::Min(ColumnWidth, MaxCategoryColumn);

	for (HDevConsoleLogRow& Row : LogRows)
	{
		Row.CategoryColumn = PString::Format("%-*.*s", ColumnWidth, ColumnWidth, categoryLabel(Categories[Row.CategoryIndex].Name));
	}
}

void JGDevConsole::rebuildVisibleLines()
{
	const HLogFilter Filter = parseLogFilter(FilterText);

	VisibleLines.clear();
	for (uint32 LineIndex = 0; LineIndex < static_cast<uint32>(LogRows.size()); ++LineIndex)
	{
		const HDevConsoleLogRow& Row = LogRows[LineIndex];

		bool bLevelShown = bShowInfo;
		if (Row.Level == ELogLevel::Error || Row.Level == ELogLevel::Critical)
		{
			bLevelShown = bShowError;
		}
		else if (Row.Level == ELogLevel::Warning)
		{
			bLevelShown = bShowWarning;
		}

		if (bLevelShown == false || Categories[Row.CategoryIndex].bVisible == false)
		{
			continue;
		}

		if (passLogFilter(LogLines[LineIndex].Text.GetRawString(), Filter))
		{
			VisibleLines.push_back(LineIndex);
		}
	}
}

void JGDevConsole::generateToolbar()
{
	HGUI::BeginPanel("##DevConsoleToolbar", HVector2(0.0f, 0.0f), ColorSurface, ColorBorder);

	// 모든 항목을 매 프레임 그려야 하므로 |= 로 모은다.
	bool bChanged = false;
	bChanged |= HGUI::ToggleChip("##LevelInfo", "Info", formatCount(InfoCount), ColorAccent, bShowInfo);
	HGUI::SameLine(6.0f);
	bChanged |= HGUI::ToggleChip("##LevelWarning", "Warn", formatCount(WarningCount), ColorWarning, bShowWarning);
	HGUI::SameLine(6.0f);
	bChanged |= HGUI::ToggleChip("##LevelError", "Error", formatCount(ErrorCount), ColorCritical, bShowError);
	HGUI::SameLine(14.0f);
	bChanged |= generateCategoryCombo();

	// 오른쪽 끝에 보이는 줄 / 전체 줄. 검색칸은 그 앞까지 늘어난다.
	const PString Summary      = PString::Format("%s / %s lines", formatCount(VisibleLines.size()), formatCount(LogLines.size()));
	const float32 SummaryWidth = HGUI::CalcTextWidth(Summary);
	HGUI::SameLine(14.0f);
	bChanged |= HGUI::InputTextWithHint("DevConsoleSearch", "Search  (word, -exclude)", FilterText, -(SummaryWidth + 14.0f));
	HGUI::SameLine(14.0f);
	HGUI::TextRight(Summary, ColorTextMuted);

	HGUI::EndPanel();

	if (bChanged)
	{
		rebuildVisibleLines();
	}
}

bool JGDevConsole::generateCategoryCombo()
{
	const uint32 CategoryCount = static_cast<uint32>(Categories.size());
	uint32 VisibleCount = 0;
	for (const HDevConsoleLogCategory& Category : Categories)
	{
		if (Category.bVisible)
		{
			++VisibleCount;
		}
	}

	PString Preview = "All categories";
	if (CategoryCount > 0 && VisibleCount == 0)
	{
		Preview = "No categories";
	}
	else if (VisibleCount < CategoryCount)
	{
		Preview = PString::Format("%u of %u categories", VisibleCount, CategoryCount);
	}

	// 닫힌 드롭다운의 글자 · 화살표는 한 단계 흐리게 한다(도구줄에서 가장 밝은 것이 되지 않게). 펼친 목록은 원래 색으로 되돌린다.
	// 목록 안에서 넣은 색은 목록 안에서 빼야 한다(ImGui 는 창마다 Push/Pop 짝을 검사한다).
	HGUI::PushStyleColor(EGUIColor::Text, ColorTextSecondary);
	if (HGUI::BeginCombo("DevConsoleCategories", Preview, 200.0f) == false)
	{
		HGUI::PopStyleColor();
		return false;
	}
	HGUI::PushStyleColor(EGUIColor::Text, ColorTextPrimary);

	// All: 전부 켜져 있으면 체크, 일부만 켜져 있으면 가운데 표시. 누르면 전부 켜고(일부 · 없음일 때) 전부 켜져 있었으면 전부 끈다.
	bool bChanged = false;
	bool bAll = CategoryCount > 0 && VisibleCount == CategoryCount;
	const bool bMixed = VisibleCount > 0 && VisibleCount < CategoryCount;
	if (HGUI::Checkbox("All", bAll, bMixed))
	{
		for (HDevConsoleLogCategory& Category : Categories)
		{
			Category.bVisible = bAll;
		}
		bChanged = true;
	}
	HGUI::SameLine(16.0f);
	HGUI::TextRight(formatCount(LogLines.size()), ColorTextMuted);
	HGUI::Separator();

	for (HDevConsoleLogCategory& Category : Categories)
	{
		// ## 뒤는 ID 다. 카테고리 이름이 "All" 이어도 위 항목과 겹치지 않는다.
		if (HGUI::Checkbox(PString::Format("%s##Category", categoryLabel(Category.Name)), Category.bVisible))
		{
			bChanged = true;
		}
		HGUI::SameLine(16.0f);
		HGUI::TextRight(formatCount(Category.LineCount), ColorTextMuted);
	}

	HGUI::PopStyleColor();
	HGUI::EndCombo();
	HGUI::PopStyleColor();
	return bChanged;
}

void JGDevConsole::generateLogView()
{
	// 아래에 입력줄 패널과 간격을 남기고 나머지를 로그 패널로 쓴다.
	HGUI::PushStyleColor(EGUIColor::ChildBackground, ColorSurface);
	HGUI::PushStyleVar(EGUIStyleVar::ChildRounding, 6.0f);
	HGUI::PushStyleVar(EGUIStyleVar::WindowPadding, HVector2(10.0f, 8.0f));
	HGUI::BeginChild("##DevConsoleLog", HVector2(0.0f, -(HGUI::GetFrameHeight() + InputBarPadding + PanelGap)));
	HGUI::PopStyleVar(2);
	HGUI::PopStyleColor();

	if (VisibleLines.empty())
	{
		HGUI::Text(LogLines.empty() ? "No log lines yet." : "No log lines match the filters.", ColorTextMuted);
	}

	for (const uint32 LineIndex : VisibleLines)
	{
		const HDevConsoleLogRow& Row = LogRows[LineIndex];

		HLinearColor BodyColor = ColorTextSecondary;
		if (Row.Level == ELogLevel::Error || Row.Level == ELogLevel::Critical)
		{
			HGUI::HighlightLine(ColorErrorRow, ColorCritical);
			BodyColor = ColorErrorText;
		}
		else if (Row.Level == ELogLevel::Warning)
		{
			HGUI::HighlightLine(ColorWarningRow, ColorWarning);
			BodyColor = ColorWarning;
		}
		else if (Row.bCommandEcho)
		{
			BodyColor = ColorCommand;
		}

		HGUI::Text(Row.CategoryColumn, ColorTextMuted);
		HGUI::SameLine(12.0f);
		HGUI::Text(Row.Body, BodyColor);
	}

	HGUI::EndChild(true);
}

void JGDevConsole::generateInputBar()
{
	HGUI::BeginPanel("##DevConsoleInputBar", HVector2(0.0f, HGUI::GetFrameHeight() + InputBarPadding), ColorSurface, bInputActive ? ColorFocusBorder : ColorBorder);

	HGUI::AlignTextToFramePadding();
	HGUI::Text(">", ColorAccent);
	HGUI::SameLine(8.0f);

	// 입력칸의 바탕 · 테두리는 패널이 맡는다.
	HGUI::PushStyleColor(EGUIColor::FrameBackground, ColorSurface);
	HGUI::PushStyleColor(EGUIColor::FrameBackgroundHovered, ColorSurface);
	HGUI::PushStyleColor(EGUIColor::FrameBackgroundActive, ColorSurface);
	HGUI::PushStyleVar(EGUIStyleVar::FrameBorderSize, 0.0f);
	const bool bEntered = HGUI::InputTextWithHistory("DevConsoleInput", CommandText, History, HistoryPos, InputKeyHandler,
		"Type a command      Tab  complete      Up/Down  history");
	HGUI::PopStyleVar();
	HGUI::PopStyleColor(3);

	if (bEntered)
	{
		submitCommand();
	}

	// 입력칸 바로 뒤에 부른다(직전 항목 = 입력칸 기준으로 위치와 포커스를 본다).
	generateCommandPreview();

	HGUI::EndPanel();
}

void JGDevConsole::generateCommandPreview()
{
	// 입력줄에 포커스가 없으면(다른 곳을 눌렀거나 Enter 를 친 프레임) 보이지 않는다.
	bInputActive = HGUI::IsItemActive();
	if (bInputActive == false)
	{
		return;
	}

	syncCommandPreview(CommandText);
	if (PreviewName.Empty())
	{
		return;
	}

	// 여백 · 테두리 색은 창을 만들 때 읽으므로 바로 되돌린다. 입력칸이 패널 안에 있으니 패널 위 테두리 바깥에 띄운다.
	HGUI::PushStyleVar(EGUIStyleVar::WindowPadding, HVector2(10.0f, 8.0f));
	HGUI::PushStyleColor(EGUIColor::Border, ColorBorderStrong);
	HGUI::BeginTooltipAboveItem("##DevConsolePreview", InputBarPadding * 0.5f + 2.0f);
	HGUI::PopStyleColor();
	HGUI::PopStyleVar();

	if (bPreviewUsage)
	{
		if (Candidates.empty())
		{
			HGUI::Text(PString::Format("Unknown command '%s'", PreviewName), ColorErrorText);
			HGUI::Text("help lists every command", ColorTextMuted);
		}
		else
		{
			HGUI::Text("Usage", ColorTextMuted);
			HGUI::SameLine(8.0f);
			HGUI::Text(Candidates[0].Usage, ColorTextPrimary);
			HGUI::Text(Candidates[0].Description, ColorTextMuted);
		}
	}
	else if (Candidates.empty())
	{
		HGUI::Text(PString::Format("No command contains '%s'", PreviewName), ColorTextMuted);
	}
	else
	{
		const int32 RowCount = HMath::Min(static_cast<int32>(Candidates.size()), MaxCandidateRows);
		int32 NameWidth = 0;
		for (int32 Row = 0; Row < RowCount; ++Row)
		{
			NameWidth = HMath::Max(NameWidth, static_cast<int32>(Candidates[Row].Name.Length()));
		}

		if (static_cast<int32>(Candidates.size()) > RowCount)
		{
			HGUI::Text(PString::Format("%d more", static_cast<int32>(Candidates.size()) - RowCount), ColorTextMuted);
		}

		// 가장 잘 맞는 후보(0번)를 맨 아래, 입력줄 바로 위에 그린다. ↑ 를 누를수록 위로 올라간다.
		for (int32 Row = RowCount - 1; Row >= 0; --Row)
		{
			const HConsoleCommandInfo& Candidate = Candidates[Row];
			const bool bSelected = Row == CandidatePos;
			if (bSelected)
			{
				HGUI::HighlightLine(ColorSelectedRow, ColorAccent);
			}
			HGUI::Text(PString::Format("%-*s", NameWidth, Candidate.Name), bSelected ? ColorTextPrimary : ColorTextSecondary);
			HGUI::SameLine(14.0f);
			HGUI::Text(Candidate.Description, ColorTextMuted);
		}

		HGUI::Separator();
		HGUI::Text("Tab  complete      Up/Down  select      Enter  run", ColorTextMuted);
	}

	HGUI::EndTooltip();
}

void JGDevConsole::syncCommandPreview(const PString& InText)
{
	// ↑ 로 불러온 히스토리 줄을 고치면 히스토리 보기를 끝낸다. 다음 ↑ 는 가장 최근 줄부터다.
	if (HistoryPos >= 0)
	{
		const bool bRecalledLine = HistoryPos < static_cast<int32>(History.size()) && History[HistoryPos].GetRawString() == InText.GetRawString();
		if (bRecalledLine == false)
		{
			HistoryPos = -1;
		}
	}

	// ↑/↓ 로 입력줄을 후보 이름으로 바꾼 상태면 목록을 그대로 둔다.
	if (CandidatePos >= 0 && CandidatePos < static_cast<int32>(Candidates.size())
		&& Candidates[CandidatePos].Name.GetRawString() == InText.GetRawString())
	{
		return;
	}

	const bool bHistory = HistoryPos >= 0;
	if (CandidatePos < 0 && bHistory == bPreviewHistory && InText.GetRawString() == PreviewText.GetRawString())
	{
		return;
	}

	PreviewText     = InText;
	bPreviewHistory = bHistory;
	CandidatePos    = -1;
	Candidates.clear();

	bool bTypingArgs = false;
	splitCommandName(InText, PreviewName, bTypingArgs);
	if (PreviewName.Empty())
	{
		return;
	}

	GConsoleCommandGlobalSystem::GetInstance().FindCommands(PreviewName, Candidates);

	// 인자를 치는 중이거나 히스토리 줄이면 이름이 같은 명령 하나만 남긴다(사용법 모드).
	bPreviewUsage = bTypingArgs || bHistory;
	if (bPreviewUsage)
	{
		const PString Name = HConsoleCommandArgs::NormalizeName(PreviewName);
		const HList<HConsoleCommandInfo>::iterator Exact = std::find_if(Candidates.begin(), Candidates.end(), [&Name](const HConsoleCommandInfo& InInfo)
			{
				return InInfo.Name.GetRawString() == Name.GetRawString();
			});

		if (Exact == Candidates.end())
		{
			Candidates.clear();
		}
		else
		{
			const HConsoleCommandInfo Found = *Exact;
			Candidates.clear();
			Candidates.push_back(Found);
		}
	}
}

bool JGDevConsole::onInputKey(EGUIInputTextKey InKey, PString& InOutText)
{
	syncCommandPreview(InOutText);

	// 후보 목록이 떠 있을 때만 가로챈다. 아니면 기본 동작(↑/↓ = 히스토리, Tab = 아무것도 안 함).
	if (PreviewName.Empty() || bPreviewUsage || Candidates.empty())
	{
		return false;
	}

	if (InKey == EGUIInputTextKey::Tab)
	{
		// 인자를 칠 수 있게 공백을 붙인다. 다음 프레임에 사용법 모드로 바뀐다.
		PString Completed = Candidates[HMath::Max(CandidatePos, 0)].Name;
		Completed.Append(" ");
		InOutText = Completed;
		return true;
	}

	// 목록은 입력줄 위로 쌓인다. ↑ = 입력줄에서 멀어지는 쪽(덜 맞는 후보), ↓ = 가까워지는 쪽. -1 은 친 글자 그대로.
	const int32 RowCount = HMath::Min(static_cast<int32>(Candidates.size()), MaxCandidateRows);
	if (InKey == EGUIInputTextKey::Up)
	{
		CandidatePos = HMath::Min(CandidatePos + 1, RowCount - 1);
	}
	else
	{
		CandidatePos = HMath::Max(CandidatePos - 1, -1);
	}

	InOutText = (CandidatePos >= 0) ? Candidates[CandidatePos].Name : PreviewText;
	return true;
}

void JGDevConsole::submitCommand()
{
	PString Line = CommandText;
	Line.Trim();

	CommandText.Reset();
	HistoryPos = -1;

	if (Line.Empty())
	{
		return;
	}

	if (History.empty() || History.back().GetRawString() != Line.GetRawString())
	{
		History.push_back(Line);
		if (History.size() > MaxHistoryLines)
		{
			History.erase(History.begin());
		}
	}

	// 프레임 밖(다음 GCoreSystem::Update 끝)에서 실행한다. 지금은 위젯 GUI 순회 도중이다.
	GConsoleCommandGlobalSystem::GetInstance().Submit(Line);
}
