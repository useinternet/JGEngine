#pragma once
#include "Widget.h"
#include "WidgetComponent.h"
#include "GUI.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "DevConsoleDefines.h"

#include "DevConsole.generation.h"

// 콘솔 창. 위에서부터 도구줄(로그 필터), 최근 로그(Info 이상), 명령 입력줄이다. 색은 메모리 통계 창과 같은 팔레트다.
// 명령은 Core 의 GConsoleCommandGlobalSystem 에 등록돼 있고, 이 창은 입력한 줄을 Submit 으로 넘긴다(다음 프레임 끝, 프레임 밖에서 실행).
// 명령을 만드는 방법은 ConsoleCommand/ConsoleCommandGlobalSystem.h 상단 주석을 본다.
//
// 로그 필터(도구줄): 세 조건을 모두 통과한 줄만 보인다.
//   레벨 칩 Info / Warn / Error(Critical 포함, 옆 숫자는 버퍼에 있는 줄 수) — 눌러서 켜고 끈다.
//   카테고리 드롭다운 — 로그 줄의 "[Category]: " 접두어로 모은 카테고리 체크박스. 맨 위 All 은 전부 켜고 끄며, 일부만 켜져 있으면 가로줄로 보인다.
//     새로 나타난 카테고리는 켜진 채로 들어온다. 접두어가 없는 줄은 "(no category)" 다.
//   검색 — 쉼표로 나눈 낱말, 대소문자 무시. "-낱말" 은 그 낱말이 든 줄을 숨기고, 나머지 낱말이 있으면 그중 하나가 든 줄만 보인다.
// 로그 줄: 카테고리 열(흐림) + 본문. 경고 · 오류 줄은 색 띠와 왼쪽 막대로 강조하고, 입력한 명령의 에코 줄("> ...")은 파란 글자다.
// 명령 미리보기(입력줄에 포커스가 있을 때 입력줄 위):
//   이름을 치는 중이면 이름에 친 글자가 든 명령 목록(앞부분 일치 먼저, 가장 잘 맞는 것이 입력줄 바로 위).
//   ↑/↓ = 후보를 골라 입력줄에 넣는다(↓ 로 끝까지 내리면 친 글자로 돌아간다). Tab = 고른 후보(없으면 가장 잘 맞는 후보)로 완성하고 공백을 붙인다.
//   이름 뒤에 공백을 쳤거나(인자 입력 중) ↑ 로 불러온 히스토리 줄이면 그 명령의 Usage 와 설명, 없는 이름이면 빨간 줄을 보인다.
//   후보 목록이 없을 때 ↑/↓ 는 히스토리를 오간다.

// 로그 카테고리 하나(드롭다운의 체크박스 한 줄)
struct HDevConsoleLogCategory
{
	PString Name;            // "[Core]: ..." 의 Core. 접두어가 없는 줄은 빈 이름
	uint32  LineCount = 0;   // 지금 버퍼에 있는 줄 수. 버퍼에서 밀려나 0 이 돼도 목록에 남겨 켜고 끈 상태를 지킨다
	bool    bVisible  = true;
};

// 로그 한 줄의 그리기 정보. 로그가 바뀐 프레임에만 만든다.
struct HDevConsoleLogRow
{
	PString   CategoryColumn;          // 카테고리 이름을 열 폭에 맞춰 공백으로 채운 것
	PString   Body;                    // "[Category]: " 뒤 본문(접두어가 없으면 줄 전체)
	uint32    CategoryIndex = 0;       // Categories 인덱스
	ELogLevel Level         = ELogLevel::Info;
	bool      bCommandEcho  = false;   // 입력한 명령의 에코 줄([ConsoleCommand]: > ...)
};

JGCLASS()
class JGDevConsole : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

private:
	PString CommandText;

	// 로그 뷰. GLogGlobalSystem 의 serial 이 바뀐 프레임에만 다시 복사하고 Rows · Categories 를 다시 만든다.
	HList<HLogLine>           LogLines;
	HList<HDevConsoleLogRow>  LogRows;
	uint64                    LogSerial = 0;

	// 로그 필터. VisibleLines 는 필터를 통과한 LogLines 의 인덱스이고, 로그나 필터가 바뀐 프레임에만 다시 만든다.
	PString                        FilterText;
	bool                           bShowInfo    = true;
	bool                           bShowWarning = true;
	bool                           bShowError   = true;
	uint32                         InfoCount    = 0;
	uint32                         WarningCount = 0;
	uint32                         ErrorCount   = 0;
	HList<HDevConsoleLogCategory>  Categories;   // 이름순
	HList<uint32>                  VisibleLines;

	// 입력 히스토리. 최근 64줄, 직전 줄과 같으면 다시 넣지 않는다. 창을 닫았다 열어도 유지된다(위젯 객체가 남는다).
	HList<PString> History;
	int32          HistoryPos = -1;

	// 명령 미리보기. PreviewText 는 미리보기를 만든 입력줄 글자다(↑/↓ 로 후보를 고르는 동안에는 사용자가 친 글자 그대로 남는다).
	// 목록 모드면 Candidates 가 후보(가장 잘 맞는 것 먼저)이고, 사용법 모드(bPreviewUsage)면 이름이 같은 명령 하나 또는 빈 목록이다.
	PString                    PreviewText;
	PString                    PreviewName;
	bool                       bPreviewUsage   = false;
	bool                       bPreviewHistory = false;
	HList<HConsoleCommandInfo> Candidates;
	int32                      CandidatePos = -1;   // -1 = 고르지 않음(입력줄 = 친 글자), 0 = 가장 잘 맞는 후보
	HGUIInputTextKeyDelegate   InputKeyHandler;     // OnInitialize 에서 onInputKey 에 묶는다
	bool                       bInputActive = false; // 지난 프레임에 입력줄에 포커스가 있었나(입력줄 테두리 색)

public:
	virtual void OnInitialize() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnGenerateGUI() override;

	virtual PString  GetTitleName() const override;
	virtual const  HGuid& GetGUID() const override;

private:
	void refreshLogLines();
	void rebuildLogRows();
	void rebuildVisibleLines();
	void generateToolbar();
	bool generateCategoryCombo();
	void generateLogView();
	void generateInputBar();
	void generateCommandPreview();
	void syncCommandPreview(const PString& InText);
	bool onInputKey(EGUIInputTextKey InKey, PString& InOutText);
	void submitCommand();
};
