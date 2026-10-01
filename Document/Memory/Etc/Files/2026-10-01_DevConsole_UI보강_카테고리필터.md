# 2026-10-01 DevConsole UI 보강 · 카테고리 드롭다운 (6-5 · 6-6)

사용자 요청(2026-10-01 오전): "요거 UI 좀 멋있게 보강해주고, 필터 관련해서 iNFO, Warn, Error는 좋은데 카테고리 필터링은 드롭박스 형태로 가고, 드롭 박스내 아이템에서 체크박스 형태로 하고 싶어. all 옵션도 있어서 이거 체크하면 전체 카테고리 체크하도록되고".
앞선 작업(6-1 로그 필터 · 6-2 명령 미리보기)은 `2026-09-30_DevConsole_로그필터_명령미리보기.md`. 이번 변경은 그 위에 얹었다(둘 다 미커밋).

**상태: 구현 · 검증 완료, 메인 트리 적용 · 빌드 완료, 미커밋.** 증적 `2026-10-01_devconsole_ui_polish_verify.txt`, 캡처 `2026-10-01_devconsole_ui_*.png` 7장(메인 트리 1장 포함), 패치 `2026-10-01_devconsole_ui_polish.patch`(스냅샷 기준 5파일).

## 1. 화면

| 영역 | 모습 · 동작 |
|---|---|
| 창 | 메모리 통계 창(Memory UI-1)과 같은 팔레트: 바탕 #0d0d0d, 패널 #1a1a19 · 테두리 #313130 · 둥근 모서리 6, 글자 흰색/#c3c2b7/#898781, 강조 파랑 #3987e5, 경고 #fab219, 오류 #d03b3b. 패널 사이 6px |
| 도구줄 패널 | 레벨 칩 `Info n` `Warn n` `Error n`(알약 모양 · 색 점 · 흐린 개수, 켜짐 = 그 색으로 옅게 물든 바탕 + 색 테두리, 꺼짐 = 회색) → 카테고리 드롭다운 → 검색칸(`Search (word, -exclude)`, 남은 폭) → 오른쪽 끝 `보이는 / 전체 lines` |
| 카테고리 드롭다운 | 닫힘: `All categories` / `N of M categories` / `No categories`(글자 한 단계 흐림). 펼침: 맨 위 `All`(전체 줄 수) + 구분선 + 카테고리별 체크박스(이름순, 오른쪽에 줄 수). **All**: 전부 켜져 있으면 체크, 일부만이면 가운데 표시(혼합), 전부 꺼져 있으면 빈칸. 누르면 일부 · 없음 → 전부 켬, 전부 → 전부 끔. 체크박스를 눌러도 닫히지 않는다(바깥 클릭 · Esc 로 닫힘) |
| 카테고리 출처 | 로그 줄 `"[Category]: 본문"` 의 Category. 접두어가 없는 줄은 `(no category)`. 새 카테고리는 켜진 채로 들어오고, 버퍼(1024줄)에서 밀려나 0줄이 돼도 목록에 남아 켜고 끈 상태를 지킨다 |
| 필터 조합 | 레벨 칩 AND 카테고리 AND 검색. 검색 문법은 6-1 그대로(쉼표 낱말, `-` 제외, 대소문자 무시) |
| 로그 패널 | 줄 = 카테고리 열(흐림, 가장 긴 이름에 맞춘 고정폭, 최대 16자) + 본문. Info 본문 #c3c2b7, 경고 주황 · 오류 밝은 빨강 글자 + 줄 전체 옅은 띠 + 왼쪽 2px 막대, 입력한 명령의 에코 줄(`[ConsoleCommand]: > ...`)은 파란 글자. 보이는 줄이 없으면 `No log lines match the filters.` |
| 입력줄 패널 | `>` 프롬프트(파랑) + 입력칸(바탕 · 테두리 없음) + 안내 `Type a command   Tab complete   Up/Down history`. 포커스가 있으면 패널 테두리가 파랑 |
| 명령 미리보기 | 6-2 동작 그대로. 띄운 패널(#242423, 테두리 #464644, 여백 10 · 8), 이름 열 고정폭 + 흐린 설명, 선택 후보는 파란 띠 + 왼쪽 막대, 하단 `Tab complete   Up/Down select   Enter run`. 입력줄 패널 위 테두리 바깥에 뜬다 |

## 2. 바꾼 것 (5파일, 새 파일 없음)

| 파일 | 내용 |
|---|---|
| `GUI/GUIDefines.h` | `EGUIColor` 에 17개 추가(ChildBackground · Frame* · CheckMark · Header* · Button* · TextDisabled · Scrollbar* · Separator — 기존 값 유지, 끝에 붙임), 새 `EGUIStyleVar`(FrameRounding · FrameBorderSize · PopupRounding · ChildRounding · ScrollbarSize · ScrollbarRounding = float32, FramePadding · ItemSpacing · WindowPadding = HVector2) |
| `GUI/GUI.h/.cpp` | 클래스 끝 새 구역 "편집기 창 테마 · 콘솔": `DisplayColor(0xRRGGBB)`(sRGB → GUI 가 받는 선형 색), `PushStyleVar`×2 · `PopStyleVar`, `GetFrameHeight`, `CalcTextWidth`, `AlignTextToFramePadding`, `Checkbox(name, value, bMixed)`, `ToggleChip`, `BeginCombo/EndCombo`, `HighlightLine`. 기존 함수: `InputTextWithHistory` 에 `InHint`, `BeginTooltipAboveItem` 에 `InExtraGap`(둘 다 기본값 — 부르는 곳은 DevConsole 뿐) |
| `Editor/DevConsole/DevConsole.h/.cpp` | `HDevConsoleLogCategory` · `HDevConsoleLogRow`, 카테고리 목록 · 줄 정보(`rebuildLogRows`, 로그가 바뀐 프레임에만), 화면을 `generateToolbar` · `generateCategoryCombo` · `generateLogView` · `generateInputBar` 로 나눔, 테마 색 · 모양 Push/Pop. 6-1 · 6-2 로직(검색 해석 · 미리보기 동기화 · 키 처리 · 제출)은 그대로 |

Core · 다른 모듈은 바꾸지 않았다. `console.selftest` 는 64 그대로.

## 3. 검증 요약 (상세 `2026-10-01_devconsole_ui_polish_verify.txt`)

| 항목 | 결과 |
|---|---|
| 스냅샷 워크트리 빌드 | 오류 0 · C++ 경고 0(첫 시도부터) |
| 헤드리스 | `console.selftest` 64/64, `gmtest` OK |
| 창 입력(클릭 포함) | 도구줄 · 로그 강조 · 미리보기 · Usage · 드롭다운(카테고리 끄기 → `5 of 7`, All 혼합 → 전부 켬 → 전부 끔 → 빈 로그 안내) · 레벨 칩 · 검색 모두 OK. 실행마다 의도한 임시 로그 말고 오류 0, 남은 명령 경고 0, live blocks 0, 종료 코드 0 |
| 런처 회귀(임시 코드 없음) | 종료 코드 0 · `[error]` 0 · live 0(crashwalk 는 이 PC 에서 창을 못 닫아 WM_CLOSE 를 직접 보냄 — 아래 함정 4) |
| 메인 트리 | 5파일 적용(스냅샷 뒤 아무도 안 고침 확인 후 복사) → 전체 빌드 159초 오류 0 · 경고 0 → `console.selftest` 64/64 · `gmtest` OK → 콘솔을 레이아웃 복원으로 연 런처에서 새 화면 · 미리보기 · Usage · Unknown 확인, 오류는 일부러 친 것 1줄, live 0, 종료 0(ini 는 원복). `2026-10-01_devconsole_ui_maintree.png` |

## 4. 함정 · 다음 작업자 참고

| 함정 | 내용 |
|---|---|
| 1. 다른 세션의 미커밋 GUI 함수에 기댐 | `BeginPanel` · `TextRight` · `FillWindowBackground` · `PushStyleColor`(Memory UI-1, 미커밋)를 쓴다. HEAD 워크트리로는 빌드되지 않아 메인 트리 상태를 복사한 스냅샷 워크트리를 썼다. 커밋은 Memory UI-1 의 GUI 변경과 같거나 그 뒤여야 한다 |
| 2. 색은 `HGUI::DisplayColor` 로 | GUI 는 선형 FP16 버퍼에 그린 뒤 sRGB 로 내보낸다. sRGB 값을 그대로 넘기면 옅게 뜬다. 메모리 통계 창은 같은 변환을 자기 파일에 따로 갖고 있다(`MemoryStatistics.cpp` `displayColor`) → 한 줄로 `HGUI::DisplayColor` 로 바꿀 수 있다(그 트랙 파일이라 손대지 않음, TODO 보류 B-10) |
| 3. 알파는 진하게 보인다 | 선형 공간 알파 섞기 + sRGB 출력이라 0.18 도 진하다. 칩 바탕 0.10, 줄 띠는 알파 대신 sRGB 로 미리 섞은 불투명 색(`mixRGB`) |
| 4. crashwalk 가 이 PC 에서 창을 못 닫음 | NVIDIA NvFBC 스레드가 디버거 아래 SetThreadName 예외를 계속 던져 crashwalk 의 무이벤트 200ms 조건이 오지 않는다(10-01 GameGUI 세션 측정). 런처 회귀는 WM_CLOSE 를 직접 보내거나 `Files/tools/devconsole_ui_runner.ps1.txt`(디버거 없음, 끝에 WM_CLOSE)로 한다 |
| 5. Push/Pop 짝은 창마다 | 드롭다운 펼친 목록(별도 팝업 창) 안에서 넣은 색은 그 안에서 뺀다. 목록 밖에서 넣고 안에서 빼면 ImGui 가 창 끝에서 짝 불일치로 멈춘다 |
| 6. 모양 값 형 | `EGUIStyleVar` 는 float32 / HVector2 가 정해져 있다(주석). 다른 형으로 부르면 ImGui assert |
| 7. 클릭 검증 | PostMessage 클릭은 워크트리에 `devconsole_filter_preview_temp_snippets.cpp.txt` (C) `JG_UITEST` 가 있어야 먹는다. 러너 스크립트는 BOM 으로 저장 |

## 5. 남은 것 · 보류

| 항목 | 메모 |
|---|---|
| 커밋 | 사용자. `TODO_DevConsole.md` 6-3(6-1 · 6-2)과 6-7(이번) — 같은 파일이라 한 커밋이 자연스럽다. `GUI.h/.cpp/GUIDefines.h` 는 Memory UI-1 · GFW · 게임 UI 변경과 같은 파일 |
| 보류 | 카테고리 "이것만 보기"(한 개만 켜기) · 카테고리 색 구분 · 로그 지우기 / 복사 · 필터 상태를 실행 사이에 저장 · 에디터 공용 팔레트 헤더(두 창이 같은 값을 따로 둠) — 다시 볼 조건은 TODO 보류 표 |
