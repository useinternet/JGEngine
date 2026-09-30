#include "PCH/PCH.h"
#include "DevConsole.h"
#include "GUI.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

namespace
{
	constexpr uint64 MaxHistoryLines = 64;

	const HLinearColor LogColorWarning = HLinearColor(1.0f, 0.82f, 0.35f, 1.0f);
	const HLinearColor LogColorError   = HLinearColor(1.0f, 0.42f, 0.42f, 1.0f);
}

void JGDevConsole::OnInitialize()
{

}

void JGDevConsole::OnLayout(const HWidgetLayout& InLayout)
{

}

void JGDevConsole::OnGenerateGUI()
{
	refreshLogLines();

	// 아래에 입력줄 한 줄 자리를 남기고 나머지를 로그 뷰로 쓴다.
	HGUI::BeginChild("##DevConsoleLog", HVector2(0.0f, -HGUI::GetFrameHeightWithSpacing()));
	for (const HLogLine& Line : LogLines)
	{
		if (Line.Level == ELogLevel::Error || Line.Level == ELogLevel::Critical)
		{
			HGUI::Text(Line.Text, LogColorError);
		}
		else if (Line.Level == ELogLevel::Warning)
		{
			HGUI::Text(Line.Text, LogColorWarning);
		}
		else
		{
			HGUI::Text(Line.Text);
		}
	}
	HGUI::EndChild(true);

	if (HGUI::InputTextWithHistory("DevConsoleInput", CommandText, History, HistoryPos))
	{
		submitCommand();
	}
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
