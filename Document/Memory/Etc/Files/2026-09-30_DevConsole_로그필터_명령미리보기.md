# 2026-09-30 DevConsole 로그 필터 · 명령 미리보기 (6-1 · 6-2)

사용자 요청(2026-09-30 21:2x): "DevConsole에 기능을 추가하고싶어. 로그 필터링하고 명령어 프리뷰 기능을 넣고싶어".
TODO 제외 표의 "Tab 자동완성"(다시 넣을 조건: 명령이 수십 개가 될 때)은 이 요청으로 해제했다. 세부 동작은 아래 1절처럼 이 세션이 관례(ImGui 콘솔 예제의 `incl,-excl` 필터, UE 콘솔 자동완성)를 따라 정했다 — 사용자가 바꿀 수 있다.

**상태: 구현 · 검증 완료, 미커밋.** 기능 검증은 격리 워크트리(HEAD `4c8ac73`)에서 했고 소스를 메인 트리에 옮겼다. 메인 트리 Bin 은 22:09 다른 세션의 전체 빌드에 이 변경이 들어가 있고, 그 Bin 으로 헤드리스 · 런처 60초까지 확인했다(3절). 증적 `2026-09-30_devconsole_filter_preview_verify.txt`.

## 1. 동작 (DevConsole 창)

| 기능 | 동작 |
|---|---|
| 레벨 필터 | 맨 위 줄 체크박스 `Info (n)` `Warning (n)` `Error (n)`. 괄호 안은 최근 로그 버퍼(1024줄)에 있는 그 레벨 줄 수. Error 는 Critical 포함 |
| 글자 필터 | 같은 줄 오른쪽 입력칸(힌트 `Filter: word, -exclude (comma separated)`). 쉼표로 나눈 낱말, 앞뒤 공백 무시, ASCII 대소문자 무시. `-낱말` 이 든 줄은 숨긴다. 나머지 낱말이 하나라도 있으면 그중 하나가 든 줄만 보인다. 낱말 순서와 무관(제외가 먼저). 로그 줄은 `[Category]: 본문` 이라 `-[Core]` 처럼 카테고리로도 거른다 |
| 후보 목록 | 입력줄에 포커스가 있고 이름을 치는 중(첫 낱말 뒤 공백 없음)이면 입력줄 바로 위 툴팁. 이름에 친 글자가 든 명령, 앞부분 일치 먼저, 각 묶음 안은 이름순. 가장 잘 맞는 후보가 맨 아래(입력줄 바로 위). 최대 10줄, 넘으면 맨 위 `... N more`. 한 줄 = `이름(28칸) 설명`(`help` 목록과 같은 모양) |
| ↑ / ↓ | 후보 목록이 있으면 후보를 골라 입력줄에 이름을 넣는다(↑ = 입력줄에서 멀어짐, ↓ 로 끝까지 내리면 친 글자로 돌아감). 후보 목록이 없으면(빈 줄, 인자 입력 중, 후보 0개, 히스토리 보는 중) 기존대로 히스토리 |
| Tab | 고른 후보(없으면 가장 잘 맞는 후보) 이름 + 공백으로 완성 → 사용법 모드 |
| 사용법 모드 | 이름 뒤에 공백을 쳤거나 ↑ 로 불러온 히스토리 줄이면 `Usage: <usage>` 와 설명. 없는 이름이면 빨간 `Unknown command '<name>'` |
| 후보 0개 | 흐린 `No command contains '<text>'` |
| Enter | 기존과 같다. 입력줄 글자를 그대로 `Submit`(후보를 자동으로 고르지 않는다) |

## 2. 바꾼 것

| 파일 | 내용 |
|---|---|
| `Source/Runtime/Core/ConsoleCommand/ConsoleCommandGlobalSystem.h/.cpp` | `HConsoleCommandInfo { Name, Usage, Description }`, `FindCommands(InText, OutCommands) const`. 결과에 핸들러를 담지 않는다(아래 함정 1). 멤버 추가 없음 → 클래스 배치 불변 |
| `Source/Runtime/GUI/GUI.h/.cpp` | `EGUIInputTextKey`, `HGUIInputTextKeyDelegate`, `InputTextWithHistory(..., InKeyHandler = {})`(묶여 있으면 ↑/↓/Tab 을 먼저 넘기고, Tab 은 이때만 `CallbackCompletion`), `Checkbox`, `InputTextWithHint`, `IsItemActive`, `BeginTooltipAboveItem/EndTooltip` |
| `Source/Editor/DevConsole/DevConsole.h/.cpp` | 필터 줄(`generateFilterBar`), 필터 적용(`rebuildVisibleLines`, 로그 · 필터가 바뀐 프레임에만), 미리보기(`syncCommandPreview` · `generateCommandPreview` · `onInputKey`). 새 파일 없음 → PreBuild 불필요, 반사 멤버 없음 → JGHeaderTool 불필요 |
| `Source/Programs/JGConsole/ConsoleCommandSelfTest.cpp` | `FindCommands` 7케이스 → 64검사 |

미리보기 상태(`syncCommandPreview`, 입력줄 뒤와 키 처리 앞에서 부른다):
- `HistoryPos >= 0` 인데 입력줄이 그 히스토리 줄과 다르면 사용자가 고친 것 → `HistoryPos = -1`(다음 ↑ 는 최신 줄부터).
- `CandidatePos >= 0` 이고 입력줄 = 그 후보 이름이면 목록을 그대로 둔다(↑/↓ 로 고르는 중, `PreviewText` 는 친 글자).
- 그 밖에 입력줄 · 히스토리 여부가 바뀌면 다시 만든다: 첫 낱말로 `FindCommands`, 인자 입력 중이거나 히스토리 줄이면 이름이 같은 하나만 남긴다(사용법 모드).
- 편집 이벤트(`CallbackEdit`)를 쓰지 않고 글자 비교로 편집을 알아낸다. ImGui 는 프레임당 콜백 이벤트 하나만 보내고(Tab > ↑ > ↓ > Edit) 키 처리기는 콜백 안의 지금 글자로 먼저 동기화한다.

## 3. 검증 요약 (상세 `2026-09-30_devconsole_filter_preview_verify.txt`)

| 항목 | 결과 |
|---|---|
| 워크트리 빌드 | 오류 0 · C++ 경고 0(1차에서 `std::min/max` 매크로 충돌 → `HMath::Min/Max`) |
| `console.selftest` | 64/64, 종료 0 · `gmtest` OK · `gmtset` 종료 1 |
| 런처 창 입력 9단계 | 전부 OK. 캡처 `2026-09-30_devconsole_preview_{list,select,usage,unknown}.png`, `2026-09-30_devconsole_filter_{word,exclude,level}.png` |
| 런처 60초(임시 코드 없음) | 종료 0, `[error]/[critical]` 0, 남은 명령 경고 0, PSO 2, live blocks 0, 경고는 기존 2줄 |
| 메인 트리 | 소스 적용(5개 파일은 워크트리본과 바이트 동일, `GUI.h/.cpp` 는 다른 세션 추가분만 차이), 바뀐 .cpp 5개 컴파일 확인(오류 · 경고 0). 이 세션은 GUI.dll 만 따로 링크하지 않았다(GUI 트랙이 `JGWidget` 배치를 바꾸는 중이라 다른 DLL 과 어긋날 수 있었다). 22:09 다른 세션의 전체 빌드 산출물에 이 변경이 들어간 것을 바이너리 문자열로 확인 → 메인 Bin `console.selftest` 64/64 · `gmtest` OK · 런처 60초 종료 0 · `[error]` 0 · live 0 · 리드백 PNG 바이트 동일. 콘솔 창 UI 는 메인 트리에서 자동 확인하지 않음(임시 코드 필요) |

## 4. 함정 · 다음 작업자 참고

| 함정 | 내용 |
|---|---|
| 1. 목록 조회에 핸들러를 복사하지 않는다 | `HConsoleCommandDesc` 의 델리게이트 사본은 가상 함수 표가 명령을 등록한 모듈 DLL 안에 있다. 미리보기가 들고 있다가 그 모듈이 내려간 뒤 지우면 소멸자가 빈 주소로 간다. 그래서 `FindCommands` 는 문자열만 담은 `HConsoleCommandInfo` 를 돌려준다(지금 `DisconnectModule` 은 DLL 을 내리지 않지만, 게임 모듈 다시 불러오기 같은 경우를 막는다) |
| 2. `std::min/max` | 엔진 PCH 가 `windows.h` 를 `NOMINMAX` 없이 넣는다. `HMath::Min/Max` 를 쓴다 |
| 3. 미리보기 툴팁 배경 | 기본 `PopupBg` 알파 0.94 는 GUI 가 sRGB 로 출력되면서 뒤 글자가 밝게 비친다(창 배경 0.06 → 69/255 실측). `BeginTooltipAboveItem` 이 `SetNextWindowBgAlpha(1.0f)` 로 불투명하게 한다. 같은 이유로 다른 ImGui 툴팁 · 팝업도 조금 비친다(Graphics 5-14 색 공간) |
| 4. `IsItemActive` · `BeginTooltipAboveItem` 은 입력줄 바로 뒤 | 둘 다 "직전 항목"을 본다. `generateCommandPreview` 는 `InputTextWithHistory` 바로 다음에 부른다(사이에 항목을 그리지 않는다) |
| 5. 창 입력 검증(PostMessage) | 이 PC 에서는 클릭이 그대로 먹지 않는다. ImGui Win32 백엔드가 실제 커서 위치로 마우스가 올라간 뷰포트를 정하기 때문이다(커서가 런처 밖이면 허공 클릭 → 입력줄 포커스까지 풀림). 워크트리에 `Files/tools/devconsole_filter_preview_temp_snippets.cpp.txt` (C) 를 넣고 `JG_UITEST=1` 로 띄운다. 사용자 커서를 움직이거나 SendKeys 를 쓰지 않는다(09-29 사고) |
| 6. 검증 스크립트 인코딩 | `.ps1` 은 UTF-8 **BOM** 으로 저장한다. BOM 이 없으면 PowerShell 5.1 이 CP949 로 읽어 한글로 끝나는 주석 줄이 다음 줄을 삼킨다(클릭 줄이 조용히 빠졌다) |
| 7. 동시 편집 | 이 작업 중 `GUI.h/.cpp` 를 다른 세션 둘이 고쳤다(GFW Phase 2: `Button` · `Separator` · `CollapsingHeader` · `InteractiveImage` · `EGUIMouseButton`, 22:10 누군가 `HGUIBarSegment` · `HPlotLinesArguments`). 이 트랙 추가분과 이름이 겹치지 않는다. 커밋 전에 `grep -n "BeginTooltipAboveItem\|InputTextWithHint\|HGUIInputTextKeyDelegate" Source/Runtime/GUI/GUI.h` 로 남아 있는지 본다. 빠졌으면 `2026-09-30_devconsole_filter_preview.patch`(HEAD 기준 7파일)와 대조해 추가분만 다시 넣는다. 22:10 에 Memory UI-1 이 `HGUI` 에 대시보드 함수 28개 · `EGUIColor` · `EGUIFont` 를 더 넣었다(이름 충돌 없음) |

## 5. 남은 것

| 항목 | 누가 · 언제 | 완료 기준 |
|---|---|---|
| (선택) 런처에서 직접 보기 | 사용자 | `Windows/DevConsole` 에서 `he` → `help` 후보, Tab → `Usage: help [command]`, 필터 `-[Core]` 로 Core 줄 사라짐 |
| 커밋 | 사용자 | `TODO_DevConsole.md` 커밋 묶음 6-3(`GUI.h/.cpp` 는 다른 세션 변경과 같은 파일) |

보류(지금 필요한 곳 없음 — `TODO_DevConsole.md` 보류 표): 인자 자동완성(명령 인자 후보, 명령마다 후보 공급자가 필요), 마우스로 후보 누르기(툴팁이 `NoInputs`), 카테고리 드롭다운(글자 필터 `[Category]` 로 대신), 로그 지우기 · 복사, 필터 · 레벨 상태를 실행 사이에 저장.
