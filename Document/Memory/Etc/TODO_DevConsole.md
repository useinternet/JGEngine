# DevConsole TODO (DevConsole 창 — 2026-10-07~ JGEditor 담당 · 콘솔 명령 시스템 `Core/ConsoleCommand` 은 Core 담당)

> 2026-10-11 문서 정리: 콘솔 명령 시스템 항목(보류 B-1)은 `Core/TODO.md` 첫 표로 옮겼다. JGConsole 프로그램 항목(7-2 커밋 · 보류 B-12)은 DevOps 담당. 항목 ID 는 그대로.

갱신 2026-10-19 20:5x(커밋 상태 정정 — 엔진 총괄 배정: 5-1 은 `26ac012`, 7-1 은 `7b8ee6e` 로 10-17 15:03 엔진 총괄 묶음 커밋에 들어갔다 → 5-2 · 7-2 끝, 완료 이력으로 옮김). 이전: 2026-10-03 10시(7-1 JGConsole 앞 옵션 `-game` 구현 · 검증 — 사용자 결정 D-30 (나)), 2026-10-02 23시(7-0 ProjectAH ER-005 검토 — D-27, 결정 D-30). 이전: 2026-10-02 11시(5-1 사용자 결정 (a) 구현 · 검증, 6-3 · 6-7 커밋 확인). 이전: 2026-10-01 11시(UI 보강 6-5 · 카테고리 드롭다운 6-6 추가), 2026-09-30 22시(로그 필터 · 명령 미리보기 6-1 · 6-2 추가, 1-7 · 2-5 · 3-5 커밋 확인). 구 TODO(`Files/DevConsole_TODO.md`, 2026-09-29 작성 · 09-30 갱신)를 통합·최신화했다. **항목 번호는 구 TODO 그대로**(다른 트랙 문서가 참조: `Document/Memory/Graphics/Files/Graphics_TODO.md` 5-25·5-31, `Document/Memory/Server/Files/Network_TODO.md`, 게임모듈 분석 R8, GFW TODO B-3). 현황·구조·함정·검증 방법은 `현황.md` §1.
기준: 엔진 기반 구축 · 과설계 금지. 지금 구체적 문제가 있는 항목만 미완료 표에 두고, "나중에 필요할지도"는 보류 표에 다시 볼 조건과 함께 둔다. `결정 필요`는 착수 전 사용자 확인. 항목이 끝나면 완료 이력 표로 옮기고 근거를 적는다.
**Phase 0~4 는 `4c8ac73`, 6-1 로그 필터 · 6-2 명령 미리보기 · 6-5 UI 보강 · 6-6 카테고리 드롭다운은 `736ddbe`(10-01 12:16, `GUI.h/.cpp` 최종본은 `efba16a` 16:58)에 커밋됐다**(2026-10-02 `git log -- <파일>` 확인). 5-1 은 사용자 결정 (a)(2026-10-02, 진행현황 D-19 ④)로 프로젝트 없이 띄운 에디터도 GameFrameWorks 를 연결하게 했다 — 구현 · 검증 완료, 10-17 `26ac012` 로 커밋(`JGEditor.cpp` 한 파일). ProjectAH ER-005("JGConsole 에서 게임 모듈 명령 실행")는 검토(7-0, D-27) 뒤 사용자 결정 D-30 (나)로 JGConsole 앞 옵션 `-game` 을 구현 · 검증했다(7-1, 10-03, 10-17 `7b8ee6e` 로 커밋 — `Main.cpp` 한 파일). 남은 것은 다른 트랙이 만들 명령이다(5-2 · 7-2 커밋은 10-17 에 끝났다).

---

## 다음에 할 일 Top 3 (권장 순서 — 착수 대상은 사용자가 정한다)

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **B-1 승격 판단(조건부)** — 런처 실행 인자로 명령 실행 | Graphics 5-25가 리드백 덤프를 콘솔 명령으로 바꾸기로 하면(그때 자동 검증 루프가 PNG를 잃는다) | 그 시점에 B-1을 미완료 표로 올리고 문법(`JGLauncher.exe -exec="..."` 류)과 실행 시점(첫 프레임 뒤 `Submit`)을 정한다. 그 전에는 하지 않는다 |

**다른 트랙 후속(5-1 로 낡은 주석 — 알림만, 동작 무관)**: ① 게임 템플릿 `Build/Templates/GameProject/Source/{PROJECT_NAME}/{PROJECT_NAME}Module.cpp:15` "런처는 GameFrameWorks 를 연결하지 않으므로 게임 모듈이 연결한다" → 에디터는 이제 미리 연결해 두고(이때 `ConnectModule` 은 true 만 돌려준다), 에디터 없이 게임만 띄울 때를 위해 게임 모듈의 연결은 그대로 필요하다는 뜻으로 고친다(DevOps) ② `Source/Runtime/GameFrameWorks/Core/GameFrameWorksModule.cpp:13` `gmtest` 주석 "프로젝트 모드 에디터(게임 모듈이 연결)" → "에디터(엔진 모듈로 연결)"(GameFrameWorks, `gmtest` 공동 파일). 같은 내용의 문서 줄: `GameFrameWorks/현황.md` 102 · 130 · 244(결정 4 — 이제 결정됨), `DevOps/현황.md` 56("엔진 모듈 6개").

(5-2 · 7-2 커밋 묶음은 끝났다 — 10-17 15:03 엔진 총괄 묶음 커밋, 10-19 `git log -1 -- <파일>` 확인: 5-2 의 `JGEditor.cpp` · `Files/2026-10-02_devconsole_5-1_{verify.txt, noproject.png, project.png}` 는 `26ac012`, 7-2 의 JGConsole `Main.cpp`(DevOps GP-D4 `-runCook` 별칭 포함)는 `7b8ee6e`, 검토서 `Files/ER-005_JGConsole게임모듈명령_검토_2026-10-02.md` · `Files/2026-10-03_jgconsole_game_option_verify.txt` 는 `26ac012`. 바이너리(`Bin/`)는 D-40(`6b9d212`)으로 추적을 그만둬 넣지 않았다.)

(6-3 · 6-7 커밋 묶음은 끝났다: DevConsole 5파일 · Core `ConsoleCommandGlobalSystem.h/.cpp` · `GUIDefines.h` · `ConsoleCommandSelfTest.cpp` 는 `736ddbe`, `GUI.h/.cpp` 는 `736ddbe` 다음 `efba16a` 가 마지막. 묶음 목록과 대조용 패치 위치는 완료 이력 6-3 · 6-7 과 `Files/2026-10-01_DevConsole_UI보강_카테고리필터.md`.)

---

## 미완료 항목 (엔진 범위)

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| — | 없음 — 5-2 · 7-2(커밋)는 10-17 에 끝나 완료 이력으로 옮겼다 | — | — | — | — | — |

**다른 트랙이 구현할 명령**(이 트랙 항목이 아님 — 사용법 연결은 4-1에서 끝났다. 선언 방법은 `Source/Runtime/Core/ConsoleCommand/ConsoleCommandGlobalSystem.h` 상단 주석):

| 트랙 · 항목 | 명령(안) | 선언 위치 | 메모 |
|---|---|---|---|
| Graphics 5-25 DevScene 리드백 덤프 | `devscene.readback [-path=<dir>] [-count=N]` | Devkit .cpp 파일 범위 | DevConsole 입력은 프레임 밖 실행이라 `ReadbackTextureImmediate` 프레임 안 경고가 없다. 자동 검증 루프가 PNG를 쓰므로 명령화하면 B-1이 필요해진다 |
| Graphics 5-31 FBX 임포트 진입점 | `asset.import <fbx> -out=<Content 경로>` | Graphics(또는 에디터) .cpp | 공백 경로는 따옴표. `-name value` 불가 |
| GameModule R8 헤드리스 모듈 검증 — **완료 2026-09-30 (GameModule — 지금 DevOps, 커밋 `736ddbe`)** | `module.test <Module> [<Module>...]`(이름 규칙대로 `modtest` 에서 바꿈) | `Source/Programs/JGConsole/ModuleCommands.cpp` | 고정 목록 대신 명령 인자로 받은 모듈을 `ConnectModule` → 역순 `DisconnectModule`. 그 사이 Error/Critical 로그 수를 `GLogGlobalSystem::GetLogSerial` · `GetRecentLogs`(2-2)로 센다. `Main.cpp` 는 그대로. 검증 `Document/Memory/DevOps/Files/2026-09-30_GameModule_마무리_검증.txt` |
| GameFrameWorks B-3 헤드리스 러너(보류) | `<이름> <file> [-repeat=N]` | JGConsole .cpp 하나 또는 GFW 모듈 | 반복마다 `GMemoryGlobalSystem::Flush()`(JGConsole은 GC를 돌리지 않음). 이름은 구 문서의 `simrun` 가정을 버리고 GFW가 정한다 |
| Server `net.test` 이동 — **완료(Server 트랙, 커밋 `736ddbe`)** | `net.test` | `Source/Runtime/GameFrameWorks/Network/GameplayNetSelfTest.cpp` | GFW 가 선언하므로 JGConsole · 에디터(10-02 5-1 부터 프로젝트 없이도) 어디서나 보인다. `net.host/net.join`은 루프를 도는 프로세스용이라 JGConsole `NetCommands.cpp` 에 남았다 |

---

## 보류 (지금 필요한 곳이 없음 — 다시 볼 조건과 함께)

| ID | 항목 | 다시 볼 조건 | 메모 |
|---|---|---|---|
| B-1 | **→ `Core/TODO.md` 로 옮김(2026-10-11, 콘솔 명령 시스템)** — 런처 실행 인자로 명령 실행(`JGLauncher.exe -exec="devscene.readback"` 류) | Graphics 5-25를 명령으로 바꿔 자동 검증 루프가 리드백 PNG를 잃을 때 | Phase 0 제외 목록에 있었다. 구현하면 첫 프레임 뒤 `Submit`(프레임 밖) |
| B-3 | `HGUI::InputText` 512바이트 고정(GUI_TODO 2-9) | `HGUI::InputText` 호출자가 생길 때 | 현재 호출자 0. 콘솔은 `InputTextWithHistory`(`CallbackResize`, 가변) |
| B-4 | 한글 글리프 폰트(GUI 백로그 "폰트") | 콘솔 출력·설명에 한글이 필요해질 때 | 그때까지 이름·설명·출력은 영어 |
| B-5 | `DevConsole.module.json` 의존 `Graphics`·`Asset` 정리 | 모듈 의존을 정리할 일이 생길 때 | DevConsole 소스는 Core·GUI 헤더만 포함. 빌드에 문제 없음 |
| B-6 | 창 입력 검증 스크립트 경로 인자를 메인 트리 기본값으로 | 3-4 회귀를 다시 돌릴 때 | `Files/tools/devconsole_ui_check.ps1.txt`의 `-Exe/-WorkDir/-OutPath` 기본값이 삭제된 worktree 경로다. 인자로 넘기면 그대로 쓸 수 있다. 2026-09-30 확인: 이 PC 에서는 PostMessage 클릭이 그대로 먹지 않는다(`현황.md` §1.5). 새 스크립트 `Files/tools/devconsole_filter_preview_check.ps1.txt`(경로 인자 필수, BOM) + `devconsole_filter_preview_temp_snippets.cpp.txt` (C) 를 쓴다 |
| B-7 | 인자 자동완성(예: `help <명령>` 의 명령 이름, `module.test <Module>`) | 인자 후보를 치기 번거로운 명령이 생길 때 | 명령마다 후보 공급자(`HConsoleCommandDesc` 에 델리게이트 추가)가 필요하다. 지금 미리보기는 인자 입력 중 Usage 만 보여 준다 |
| B-8 | 후보 목록을 마우스로 누르기 | 사용자 요청 시 | 툴팁이 `NoInputs` 라 지금은 키보드(↑/↓/Tab)만 |
| B-9 | 로그 지우기 · 복사 · 필터 상태를 실행 사이에 저장 · 카테고리 "이것만 보기"(하나만 켜기) · 카테고리별 색 | 사용자 요청 시, 또는 로그가 많아져 지금 필터로 모자랄 때 | (카테고리 드롭다운은 6-6 에서 끝났다.) 필터 · 레벨 · 카테고리 상태는 위젯 객체에 남는다(창을 닫았다 열어도 유지, 실행 사이에는 없음) |
| B-10 | 에디터 창 공용 팔레트(남은 것) · 필터 칩 알파 다시 맞추기 | 세 번째 편집기 창이 같은 팔레트를 쓸 때, 또는 칩이 너무 옅다고 느낄 때 | **10-01 Graphics 5-14 로 출력이 8비트 sRGB 가 됐다**: `HGUI::DisplayColor` 는 변환 없이 옮기고, 메모리 통계 창의 `displayColor` 는 `HGUI::DisplayColor` 를 부르게 바뀌었다(Graphics 세션, 커밋 `efba16a`). 남은 것: `MemoryStatistics.cpp` 와 `DevConsole.cpp` 가 같은 sRGB 팔레트 값을 각자 둔다. `GUI.cpp` 필터 칩 알파 0.10/0.16 은 선형 출력 때 맞춘 값이라 지금은 옅다(주석에 적어 둠) |
| B-11 | 창 항목이 없는 `imgui.ini`(새 게임 프로젝트 · 처음 여는 창)에서 DevConsole 창이 아주 작게 뜬다(10-02 검증 게임: Pos 60,60 · Size 32x108) | 사용자가 새 프로젝트에서 콘솔을 열다 불편을 말할 때, 또는 GUI 트랙이 위젯 기본 크기를 정할 때 | 엔진 어디에도 `SetNextWindowSize(…, FirstUseEver)` 가 없어 창이 내용(자식 패널)에 맞춰 줄어든다. 고치려면 `JGWidget` 에 처음 크기(ini 항목이 없을 때만)를 두는 GUI 쪽 방법과 DevConsole 만 정하는 방법이 있다 — 다른 창(Data Table Editor 등)도 같은지 함께 본다. 근거 `Files/2026-10-02_devconsole_5-1_verify.txt` 6절 |
| B-12 | JGConsole `-module=<Name>` 일반 연결 옵션 · `module.connect <Name>` 명령(ER-005 가 대안으로 적은 것) | 게임 모듈 말고 따로 연결할 모듈이 생길 때(예: 그래픽 모듈이 있어야 도는 헤드리스 명령) | ER-005 검토(7-0)에서 넣지 않기로 권고: 지금 따로 연결할 모듈은 게임 모듈뿐이라 `-game` 으로 충분. `module.connect` 는 JGConsole 이 실행 한 번에 명령 하나라 쓸모가 없고, 에디터 콘솔에서는 실행 중에 아무 모듈이나 붙이게 된다 |

---

## 완료 이력

| ID | 항목 | 완료일 | 검증 근거 |
|---|---|---|---|
| 0-1 | 레지스트리 위치 = Core 전역 시스템(`GGlobalSystemInstance<T>`), DevConsole은 UI만 | 2026-09-29 | 사용자 결정. `Files/DevConsole_TODO.md` Phase 0 |
| 0-2 | 이름 = `ConsoleCommand`(`GConsoleCommandGlobalSystem`, `HConsoleCommandArgs`, `HConsoleCommandDesc`, `HAutoConsoleCommand`, 로그 `[ConsoleCommand]`) | 2026-09-29 | 같은 문서 |
| 0-3 | 문법: 소문자 `영역.동작`, 위치 인자 + `-name=value` + `-flag`, `"..."`, `-name value` 불가, 숫자 실패 → false + Usage | 2026-09-29 | 같은 문서 |
| 0-4 | 1차 범위: 등록·실행·`help`, JGConsole 연결, 로그 뷰, 입력 히스토리 | 2026-09-29 | 같은 문서 |
| 0-5 | 등록 방식 = 파일 범위 전역 변수 `HAutoConsoleCommand` 기본, 모듈 연결 때 등록, `Register` API는 동적 경우만 | 2026-09-29 | 같은 문서(사용자 질문 "언리얼처럼 전역 변수로") |
| 1-1 | 인자 파서 `HConsoleCommandArgs`(`Tokenize`, `NormalizeName`, `TryGetInt`는 `std::from_chars`) | 2026-09-29 | `console.selftest` 파서 케이스 전부 통과 |
| 1-2 | 레지스트리 `GConsoleCommandGlobalSystem`(`Register/Unregister/IsRegistered/Execute×2/Submit/ExecutePending`, 내장 `help`, `Destroy` 잔여 경고, 핸들러 복사본 실행) | 2026-09-29 | `Files/2026-09-29_devconsole_verify.txt`; 런처 종료 시 남은 명령 경고 0 |
| 1-3 | `HAutoConsoleCommand`(바이너리별 `Head` 목록) + `IModuleInterface::Register/UnregisterAutoConsoleCommands` 훅(`Module.cpp:169, 230, 273`) + EXE 목록(레지스트리 생성자/Destroy) | 2026-09-29 | 같은 파일 "모듈 훅" 절: `test.autocmd` 연결 전 0 → 연결 후 DLL 안에서 실행 → 해제 후 Unknown |
| 1-4 | JGConsole을 레지스트리로 전환(argv 토큰 `Execute`, 종료 코드 0/1, 인자 없으면 `help`, 명령마다 .cpp 하나) | 2026-09-29 | `gmtest` OK 종료 0 · `gmtset` 종료 1 · `help nosuch.command` 종료 1 · `net.test transport` OK |
| 1-5 | 자체 테스트 `console.selftest`(파서·레지스트리·전역 변수·`help %s %n`) | 2026-09-29 | 52 → 57(2-2 로그 5) / 57, 종료 0 |
| 1-6 | Phase 1 검증(격리 worktree Rebuild, 헤드리스 전부, 모듈 훅, 런처 60초 종료 0 · `[error]` 0 · live 0) | 2026-09-29 | 같은 파일 |
| 2-1 | `HGUI::Text`가 문자열을 printf 서식으로 넘기던 버그 → `TextUnformatted`(색은 `PushStyleColor`) | 2026-09-29 | `Files/2026-09-29_devconsole_ui_memorywindow.png`(peak 열·`%` 표시) vs 이전 증상 `Document/Memory/Core/Files/2026-09-28_phase2_memwidget_capture.png` |
| 2-2 | 최근 로그 버퍼 `PRecentLogSink`(Info 이상 1024줄, std 컨테이너), `GLogGlobalSystem::GetLogSerial/GetRecentLogs` | 2026-09-29 | 자체 테스트 5케이스(serial 증가, 마지막 줄 일치, trace 제외, `%s %n` 에코 그대로, 오류 레벨 유지) |
| 2-3 | `HGUI::BeginChild/EndChild(bStickToBottom)/GetFrameHeightWithSpacing/InputTextWithHistory` | 2026-09-29 | 3-4 창 확인 |
| 2-4 | Phase 2 검증 | 2026-09-29 | 3-4와 함께. scratchpad worktree 증분 빌드가 `Log.h` 변경을 놓치는 함정 발견(Rebuild로 해결) |
| 3-1 | 옛 경로 삭제(`HDevConsoleArguments`, `HOnDevConsole`, `Register/UnRegisterConsoleCommand`, `GetCheckedGUIModule`), 로그 카테고리 `DevConsole` | 2026-09-29 | grep 0건. GUI_TODO 1-6 대체, 1-9 일부 |
| 3-2 | 콘솔 창(로그 뷰 레벨별 색·맨 아래 고정 + 입력줄 → `Submit`) | 2026-09-29 | `Files/2026-09-29_devconsole_ui_log.png` |
| 3-3 | 입력 히스토리 64줄, ↑/↓, 직전 중복 제외, 창을 닫아도 유지 | 2026-09-29 | `Files/2026-09-29_devconsole_ui_history.png` |
| 3-4 | Phase 3 검증(PostMessage 입력, `test.echo`(Devkit)·`test.openmem`(DevStatistics) 임시 명령이 `help`에 나오고 실행, 자동 해제로 종료 경고 0, 명령으로 창 열기 크래시 없음) | 2026-09-29 | 같은 파일, `Files/tools/devconsole_ui_check.ps1.txt`. 임시 코드 제거 확인 |
| 4-1 | 대기 항목 5곳에 사용법 연결 + `ConsoleCommandGlobalSystem.h` 상단 주석 | 2026-09-30 | Graphics_TODO 5-25·5-31, GFW TODO B-3(`-repeat=N`), 게임모듈 R8, Network_TODO |
| 4-2 | 기록 정리(현황분석 상단 해결 요약, GUI_TODO 1-6 대체 표시, Memory 진행 기록) | 2026-09-30 | 각 문서 |
| — | 메인 트리 통합 · 전체 빌드 오류 0 · 재검증(헤드리스 7종, 런처 60초) | 2026-09-30 00시 | `Files/2026-09-29_devconsole_verify.txt` 마지막 절 |
| — | 별칭 `simtest` 제거(사용자 요청) | 2026-09-30 11:2x | JGConsole 빌드 오류 0, `console.selftest` 57/57, `simtest` Unknown 종료 1. GFW TODO·리뷰 검증 절차 문서에 표시 |
| — | `gmtest`를 `GameFrameWorks/Core/GameFrameWorksModule.cpp`에 `HAutoConsoleCommand`로 이동, JGConsole 시작 시 GameFrameWorks 연결(`Main.cpp` `CONSOLE_ENGINE_MODULES`) | 2026-09-30 11:37 | GFW·JGConsole 빌드 → `gmtest` OK(세션 기록). 사용자 결정 "모듈의 테스트 명령은 그 모듈에 선언" |
| — | 검증용 임시 코드 제거 확인(메인 트리 `test.*` 0건, `TEMP` 0건, 바이너리도 깨끗) | 2026-09-30 | `현황.md` §1.4(12:31 사용자 실행 로그로 교차 확인) |
| — | (Server 트랙) `net.test/net.host/net.join`을 `NetCommands.cpp`의 `HAutoConsoleCommand`로, `Main.cpp` 임시 분기 제거 | 2026-09-30 | `Document/Memory/Server/Files/Network_TODO.md` "겹치는 트랙" |
| 1-7 · 2-5 · 3-5 | Phase 1~3 커밋 | 2026-09-30 (`4c8ac73` "문서 정리", 17:30) | 22시 `git log -1 -- <파일>` 로 확인: `Core/ConsoleCommand/` 4개, `CoreSystem.cpp`, `Misc/Module.*`, `Misc/Log.*`, `GUI.h/.cpp`, DevConsole 5개, JGConsole `Main.cpp`(`CONSOLE_ENGINE_MODULES`) · `ConsoleCommandSelfTest.cpp` · `NetCommands.cpp`, `GameFrameWorksModule.cpp`(`gmtest`) 모두 `4c8ac73` |
| 6-1 | 로그 필터: 레벨 체크박스 3개(줄 수 표시) + 글자 필터(쉼표 낱말, `-` 제외, 대소문자 무시). 로그 · 필터가 바뀐 프레임에만 `VisibleLines` 재계산 | 2026-09-30 22시(커밋 `736ddbe`) | 격리 워크트리 창 입력: `temp preview` → 1줄, `-[Core], -[memory]` → Core · Memory 줄 숨김, Info 끔 → 경고 2 · 오류 1. `Files/2026-09-30_devconsole_filter_{word,exclude,level}.png` |
| 6-2 | 명령 미리보기: 입력줄 위 툴팁(이름 입력 중 = 후보 목록, 인자 입력 중 · 히스토리 줄 = Usage + 설명, 없는 이름 = 빨간 줄), ↑/↓ 후보 선택, Tab 완성 + 공백. Core `FindCommands` · HGUI 키 처리기 · 툴팁 추가. 제외 표 "Tab 자동완성" 해제(사용자 요청) | 2026-09-30 22시(커밋 `736ddbe`) | `console.selftest` 64/64(`FindCommands` 7케이스), 창 입력 9단계 OK(`Files/2026-09-30_devconsole_preview_{list,select,usage,unknown}.png`), 런처 60초 종료 0 · `[error]` 0 · live 0. 상세 `Files/2026-09-30_devconsole_filter_preview_verify.txt`, 설계 · 함정 `Files/2026-09-30_DevConsole_로그필터_명령미리보기.md` |
| 6-5 | UI 보강: 메모리 통계 창과 같은 팔레트 · 패널(도구줄 · 로그 · 입력줄), 레벨 칩(알약 · 색 점 · 개수), 로그 줄 카테고리 열 + 경고 · 오류 띠 · 명령 에코 파랑, 입력줄 프롬프트 · 안내 · 포커스 테두리, 미리보기 패널 스타일 · 선택 띠 · 키 안내, 빈 결과 안내 | 2026-10-01 11시(커밋 `736ddbe`) | 스냅샷 워크트리 빌드 오류 0 · 경고 0, 창 캡처 `Files/2026-10-01_devconsole_ui_{overview,preview,usage}.png`, 런처 회귀 종료 0 · `[error]` 0 · live 0. `Files/2026-10-01_devconsole_ui_polish_verify.txt` |
| 6-6 | 카테고리 드롭다운: 로그 `[Category]:` 접두어로 모은 체크박스(이름순 · 줄 수), 맨 위 `All`(전부 켬 / 끔, 일부면 혼합 표시), 닫힘 표시 `All categories` / `N of M` / `No categories`. 레벨 · 검색과 AND | 2026-10-01 11시(커밋 `736ddbe`) | 클릭 검증: Core · Memory 끔 → `5 of 7`, All → 전부 켬 → 전부 끔(빈 로그 안내) → 다시 켬. `Files/2026-10-01_devconsole_ui_{categories,categories_none,levels}.png` |
| 6-8 | 6-5 · 6-6 메인 트리 적용 · 확인 | 2026-10-01 11:05 | 5파일 복사(스냅샷 뒤 변경 없음 확인, 다른 세션 빌드가 끝난 뒤) → 메인 트리 전체 빌드 159초 오류 0 · 경고 0 → 메인 Bin `console.selftest` 64/64 · `gmtest` OK → 레이아웃 복원으로 콘솔을 연 런처에서 새 화면 · 미리보기 · Usage 확인, 종료 0 · live 0. `Files/2026-10-01_devconsole_ui_maintree.png`, 검증 기록 5절 |
| 6-4 | 메인 트리 적용 · 확인 | 2026-09-30 22:18 | 소스 적용 22:08(5개 파일 워크트리본과 바이트 동일, `GUI.h/.cpp` 는 다른 세션 추가분만 차이), 바뀐 .cpp 5개 컴파일 확인. 22:09 다른 세션 전체 빌드 산출물에 이 변경이 들어간 것을 바이너리 문자열로 확인 → 메인 Bin `console.selftest` 64/64 · `gmtest` OK · 런처 60초 종료 0 · `[error]` 0 · live 0. 콘솔 창 UI 는 메인 트리에서 자동 확인하지 않음(임시 코드가 필요, 같은 소스로 워크트리에서 확인) |
| 6-3 · 6-7 | 커밋(6-1·6-2 · 6-5·6-6) | 2026-10-01 (`736ddbe` "엔진 구현" 12:16, `GUI.h/.cpp` 는 `efba16a` "게임 구현" 16:58 이 마지막) | 10-02 `git log -- <파일>` 확인: `ConsoleCommandGlobalSystem.h/.cpp` · `GUIDefines.h` · `DevConsole.h/.cpp` · `ConsoleCommandSelfTest.cpp` 는 `736ddbe`, `GUI.h/.cpp` 는 `736ddbe` · `efba16a`. 해당 소스의 작업 트리 변경 없음. 사용자가 여러 트랙을 한 번에 커밋했다 |
| 5-1 | 프로젝트 없이 띄운 에디터도 GameFrameWorks 를 연결(사용자 결정 (a), 2026-10-02 — 진행현황 D-7 · D-19 ④): `JGEditor.cpp` `EDITOR_ENGINE_MODULES` 끝에 `"GameFrameWorks"`(게임 모듈이 연결하던 때와 같은 시작 · 종료 순서), `openSceneViewport` 는 게임 모듈을 연결했을 때만(GFW 가 늘 있어 `HasInstance()` 로는 못 가름). 결과: 에디터 콘솔에 GFW 명령 5개(`gmtest` · `gameui.selftest` · `datatable.selftest` · `datatable.validate` · `net.test`)가 프로젝트 없이도 보인다 | 2026-10-02 10:5x (커밋 `26ac012` 10-17 — 5-2) | `Files/2026-10-02_devconsole_5-1_verify.txt`: 메인 JGEditor 빌드 경고 0 · 오류 0. 프로젝트 없는 에디터 — `help` 6개에 `gmtest`, `gmtest: OK (0 failures)`, "not unregistered" 0, live 0, 종료 0, 씬 뷰포트 자동 열림 없음(저장된 ini 에 `JGSceneViewport=1` 없음). 검증 게임 DcCheck 에디터 — `Fail Connect` 0, 씬 뷰포트 자동 열림(`JGSceneViewport=1`), `gmtest` OK, GFW 시작 · 종료 각 1번, 종료 0 · live 0. `[error]` 는 gmtest 의 의도된 거부 검사 2줄뿐(JGConsole 과 같음). 캡처 `Files/2026-10-02_devconsole_5-1_{noproject,project}.png` |
| B-2 | (Server 트랙) `net.test` 를 GameFrameWorks `Network/GameplayNetSelfTest.cpp` 로 이동 | 2026-10-01 (커밋 `736ddbe`) | 10-02 확인: `NetCommands.cpp` 에는 `net.host` · `net.join` 만, 에디터 `help` 에 `net.test` 가 보인다 |
| 7-0 | ProjectAH ER-005 "JGConsole 에서 게임 모듈 명령 실행" 검토(사용자 결정 D-27 — 검토만, 코드 변경 없음) | 2026-10-02 23시 | `Files/ER-005_JGConsole게임모듈명령_검토_2026-10-02.md`: 요청서 "엔진 현황" 4줄 모두 HEAD `0bd6356` 과 맞음. 선택지 4개 비교 — 늘 연결은 `module.test <게임>` 을 빈 검사로 만들고 게임 Bin 의 `gmtest` · `net.test` 월드 절을 건너뛰게 함 → 권고 `-game`(반나절). 겹침: DevOps GP-D4 `-runCook` 별칭(같은 함수) · GP-D5 런타임 모듈 목록, GFW B-3. 결정 항목 진행현황 D-30 |
| 7-1 | JGConsole 앞 옵션 **`-game`**(ProjectAH ER-005, 사용자 결정 D-30 (나)): `JGConsole.exe [-game] <명령>` — 게임 Bin 에서 `.jgproject` `GameModules` 를 GameFrameWorks 다음에 연결(에디터 모듈 제외)한 뒤 실행. 엔진 Bin 이면 오류 · 게임 모듈 연결 실패면 명령을 돌리지 않고 종료 1. `-game` 없이 모르는 명령이면 "Commands of game modules need -game" 안내(결과 · 종료 코드 그대로). DevOps `-runCook` 별칭은 옵션 뒤 첫 토큰에 그대로 적용 | 2026-10-03 10:0x (커밋 `7b8ee6e` 10-17 — 7-2) | `Files/2026-10-03_jgconsole_game_option_verify.txt`: worktree `Temp/wt/er005` 빌드 48초 오류 0 → 메인 반영(다른 세션 메인 전체 빌드에 포함, `JGConsole.exe` 문자열 확인). 엔진 Bin(worktree · 메인): `console.selftest` 64/64 · `gmtest` OK · `-game gmtest` 1 · `-runCook` 별칭 그대로 · `module.test GameFrameWorks` OK. 검증 게임 2개(`Temp/Games/Er005Chk` worktree 엔진 · `Er005Main` 메인 엔진, 임시 명령은 게임 소스에만): `-game` 성공 0 · 실패 1 · help 에 게임 명령, `-game` 없으면 Unknown + 안내 1, `module.test <게임>` · `gmtest`(World 64) · `console.selftest` 그대로, `-game gmtest` World 50(검토서대로), DLL 없으면 명령 안 돌고 1, 에디터 모듈 미연결 · live 0. 게임 세션에 쓰는 법을 보냄 |
| 5-2 | 커밋 "프로젝트 없이 띄운 에디터도 GameFrameWorks 연결"(5-1) | 2026-10-17 15:03 (엔진 총괄 묶음 커밋 `26ac012`, 푸시 `efba16a..e111df8`) | 커밋 전 메인 빌드 오류 0 · 자체 테스트 9가지 통과(진행현황 7절 10-17 15:0x). 10-19 `git log -1 -- <파일>`: `Source/Editor/JGEditor/JGEditor.cpp`(커밋 안 엔진 모듈 목록에 `"GameFrameWorks"`) · `Files/2026-10-02_devconsole_5-1_{verify.txt, noproject.png, project.png}` = `26ac012`. 작업 트리 변경 없음. `Bin/` 은 D-40(`6b9d212`)으로 추적하지 않음 |
| 7-2 | 커밋 "JGConsole 앞 옵션 -game(게임 모듈 명령)"(7-1) | 2026-10-17 15:03 (엔진 총괄 묶음 커밋 `7b8ee6e` — DevOps 커밋) | 10-19 `git log -1 -- <파일>`: `Source/Programs/JGConsole/Main.cpp`(커밋 안 `-game` 안내 글 · DevOps GP-D4 `-runCook` 별칭 포함) = `7b8ee6e`, 검토서 `Files/ER-005_JGConsole게임모듈명령_검토_2026-10-02.md` · `Files/2026-10-03_jgconsole_game_option_verify.txt` = `26ac012`. 작업 트리 변경 없음 |

---

## 제외 항목 (Phase 0 결정 2026-09-29 — 지금 필요한 곳이 없어 넣지 않음)

| 항목 | 이유 | 다시 넣을 조건 |
|---|---|---|
| 콘솔 변수(CVar) | 엔진 전체에 쓰는 곳 없음 | 런타임 토글 값이 여럿 생겨 명령 하나씩 만들기가 번거로울 때 |
| ~~Tab 자동완성~~ | **해제 2026-09-30**: 사용자 요청("명령어 프리뷰")으로 6-2 에서 구현 | — |
| 스크립트 파일 실행 | 필요한 곳 없음 | GFW B-3 헤드리스 러너가 명령 스크립트를 정의할 때(그쪽 항목) |
| 콘솔 토글 키 | 메뉴 `Windows/DevConsole`로 충분 | 사용자 요청 시 |
| 등록 매크로 | 전역 변수 선언 한 줄로 충분. 매크로는 인자 이름이 숨고 디버깅 때 코드가 덜 보임 | — |
| Core 멀티캐스트 델리게이트 §2-4·5 수정(`Delegate.h:907-917` 값 인자 이동, `:832-855` 브로드캐스트 중 `Remove`) | 새 구조는 멀티캐스트를 쓰지 않는다. 남은 사용처(`JWindow.h`, `DX12FrameBuffer.h`, `GUIBackend.h`)는 인자가 포인터·정수·참조뿐이고 브로드캐스트 중 해제 없음 | 값 타입 인자를 브로드캐스트하거나 브로드캐스트 중 `Remove`하는 코드가 생길 때(Core 트랙) |
| 런처 ↔ JGConsole 통신(IPC) | 두 경로는 별개 프로세스로 설계 | — |
