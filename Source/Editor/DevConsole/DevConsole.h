#pragma once
#include "Widget.h"
#include "WidgetComponent.h"
#include "DevConsoleDefines.h"

#include "DevConsole.generation.h"

// 콘솔 창. 위쪽은 최근 로그(Info 이상), 아래쪽은 명령 입력줄이다.
// 명령은 Core 의 GConsoleCommandGlobalSystem 에 등록돼 있고, 이 창은 입력한 줄을 Submit 으로 넘긴다(다음 프레임 끝, 프레임 밖에서 실행).
// 명령을 만드는 방법은 ConsoleCommand/ConsoleCommandGlobalSystem.h 상단 주석을 본다.
JGCLASS()
class JGDevConsole : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

private:
	PString CommandText;

	// 로그 뷰. GLogGlobalSystem 의 serial 이 바뀐 프레임에만 다시 복사한다.
	HList<HLogLine> LogLines;
	uint64          LogSerial = 0;

	// 입력 히스토리. 최근 64줄, 직전 줄과 같으면 다시 넣지 않는다. 창을 닫았다 열어도 유지된다(위젯 객체가 남는다).
	HList<PString> History;
	int32          HistoryPos = -1;

public:
	virtual void OnInitialize() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnGenerateGUI() override;

	virtual PString  GetTitleName() const override;
	virtual const  HGuid& GetGUID() const override;

private:
	void refreshLogLines();
	void submitCommand();
};
