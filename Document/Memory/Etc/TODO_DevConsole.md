# DevConsole TODO (콘솔 명령 시스템)

갱신 2026-09-30. 구 TODO(`Files/DevConsole_TODO.md`, 2026-09-29 작성 · 09-30 갱신)를 통합·최신화했다. **항목 번호는 구 TODO 그대로**(다른 트랙 문서가 참조: `Document/Memory/Graphics/Files/Graphics_TODO.md` 5-25·5-31, `Document/Memory/Server/Files/Network_TODO.md`, 게임모듈 분석 R8, GFW TODO B-3). 현황·구조·함정·검증 방법은 `현황.md` §1.
기준: 엔진 기반 구축 · 과설계 금지. 지금 구체적 문제가 있는 항목만 미완료 표에 두고, "나중에 필요할지도"는 보류 표에 다시 볼 조건과 함께 둔다. `결정 필요`는 착수 전 사용자 확인. 항목이 끝나면 완료 이력 표로 옮기고 근거를 적는다.
**이 트랙에 남은 구현 항목은 없다.** 남은 것은 커밋(사용자)과 결정 하나, 그리고 다른 트랙이 만들 명령이다.

---

## 다음에 할 일 Top 3 (권장 순서 — 착수 대상은 사용자가 정한다)

| 순서 | 항목 | 착수 조건 | 완료 기준 |
|---|---|---|---|
| 1 | **1-7 · 2-5 · 3-5 커밋(한 번에)** — 사용자 | `tasklist`에 다른 세션 빌드 없음. `Bin/DevelopEngine`에서 `JGConsole.exe console.selftest` 57/57 · `gmtest` OK · `net.test all` 66/66 재확인. `grep -rn "TEMP" Source/Editor/DevConsole` 0건 | 아래 커밋 묶음이 한 커밋(또는 Core 변경이 먼저). `git add -A`로 미추적 파일 6개(`Core/ConsoleCommand/` 4, `ConsoleCommandSelfTest.cpp`, `NetCommands.cpp`)가 빠지지 않음 |
| 2 | **5-1 프로젝트 없이 띄운 에디터에서 GameFrameWorks 명령 노출** `결정 필요` | 사용자 결정 (a) 노출 / (b) 현행 유지 | (a): `Source/Editor/JGEditor/JGEditor.cpp:19` `EDITOR_ENGINE_MODULES`에 `"GameFrameWorks"` 추가(연결 순서는 Graphics·GUI 뒤) → 런처 콘솔 `help`에 `gmtest`가 나오고 실행 시 `gmtest: OK`, 종료 시 남은 명령 경고 0, 60초 회귀 종료 0. (b): 이 항목을 제외 표로 옮기고 이유를 적는다 |
| 3 | **B-1 승격 판단(조건부)** — 런처 실행 인자로 명령 실행 | Graphics 5-25가 리드백 덤프를 콘솔 명령으로 바꾸기로 하면(그때 자동 검증 루프가 PNG를 잃는다) | 그 시점에 B-1을 미완료 표로 올리고 문법(`JGLauncher.exe -exec="..."` 류)과 실행 시점(첫 프레임 뒤 `Submit`)을 정한다. 그 전에는 하지 않는다 |

**커밋 묶음(Top 1)** — 아래는 함께 들어가야 빌드된다(`Main.cpp`·`NetCommands.cpp`·`GameFrameWorksModule.cpp`가 Core `ConsoleCommand/`를 포함한다):

| 구분 | 파일 |
|---|---|
| Core 새 파일 | `Source/Runtime/Core/ConsoleCommand/ConsoleCommandArgs.h/.cpp`, `ConsoleCommandGlobalSystem.h/.cpp` |
| Core 수정 | `Source/Runtime/Core/CoreSystem.cpp`, `Core/Misc/Module.h/.cpp`, `Core/Misc/Log.h/.cpp` |
| GUI | `Source/Runtime/GUI/GUI.h/.cpp`(이 트랙 변경만 들어 있음 — `PlotBarGroups` 수정은 `d457b92`에 이미 커밋됨) |
| DevConsole | `Source/Editor/DevConsole/DevConsole.h/.cpp`, `DevConsoleDefines.h`, `DevConsoleModule.h/.cpp` |
| JGConsole | `Source/Programs/JGConsole/Main.cpp`, `ConsoleCommandSelfTest.cpp`(새), `NetCommands.cpp`(새 — Server 트랙 파일이지만 이 Core 변경에 의존) |
| GameFrameWorks | `Source/Runtime/GameFrameWorks/Core/GameFrameWorksModule.cpp`(`gmtest` 선언 — GFW TODO 1-8 묶음과 같은 파일이라 그쪽과 순서 조정) |
| 바이너리 | `Bin/DevelopEngine/` 메인 트리 빌드 산출물(`Core.lib`, `DevConsole.dll`, `GUI.dll`, `JGConsole.exe`, `GameFrameWorks.dll`, `JGEditor.dll` 등 — 다른 트랙 변경도 포함돼 있음) |
| 문서 | `Document/Memory/Etc/*`(이 문서 3개 + `Files/` 이동분), 다른 트랙 TODO에 추가한 연결 줄(Graphics·GFW·게임모듈·Network) |
| 제외 | `Core/Memory/*`, `Devkit/DevScene.cpp`, `Graphics/**`, `GameFrameWorks/**`(위 한 파일 제외), `Bin/DevelopEngine/GameMasterSelfTest_*.json`, `imgui.ini`, `jg_log.txt` |

---

## 미완료 항목 (엔진 범위)

| ID | 항목 | 우선순위 | 상태 | 선행조건 | 완료 기준 | 비고 |
|---|---|---|---|---|---|---|
| 1-7 | Phase 1 커밋 "Core 콘솔 명령 레지스트리와 전역 변수 등록 추가, JGConsole 명령 디스패치 전환" | 높음 | 대기(사용자) | 검증 3종 재확인 | 위 커밋 묶음 | 2-5·3-5와 한 커밋 권장 |
| 2-5 | Phase 2 커밋 "HGUI::Text 서식 해석 수정, 최근 로그 버퍼, HGUI 스크롤·입력 래퍼" | 높음 | 대기(사용자) | 1-7과 함께 | Core(Log)·GUI 파일 | GUI 트랙 P1 커밋(1-11)보다 먼저 들어가야 `GUI.cpp` 충돌이 없다 |
| 3-5 | Phase 3 커밋 "DevConsole을 콘솔 명령 레지스트리 UI로 재작성" | 높음 | 대기(사용자) | 1-7과 함께 | DevConsole 모듈 5파일 | |
| 5-1 | 프로젝트 없이 띄운 에디터에서 GameFrameWorks 명령(`gmtest`) 노출 여부 | 중간 | **결정 대기** | 사용자 결정 | Top 2 | 2026-09-30 12:31 사용자 테스트에서 `gmtest` Unknown. 버그가 아니라 연결 범위 문제(프로세스별 레지스트리, `현황.md` §1.5 첫 줄). 사용자는 "어디서든 등록돼 있길" 기대했음 |

**다른 트랙이 구현할 명령**(이 트랙 항목이 아님 — 사용법 연결은 4-1에서 끝났다. 선언 방법은 `Source/Runtime/Core/ConsoleCommand/ConsoleCommandGlobalSystem.h` 상단 주석):

| 트랙 · 항목 | 명령(안) | 선언 위치 | 메모 |
|---|---|---|---|
| Graphics 5-25 DevScene 리드백 덤프 | `devscene.readback [-path=<dir>] [-count=N]` | Devkit .cpp 파일 범위 | DevConsole 입력은 프레임 밖 실행이라 `ReadbackTextureImmediate` 프레임 안 경고가 없다. 자동 검증 루프가 PNG를 쓰므로 명령화하면 B-1이 필요해진다 |
| Graphics 5-31 FBX 임포트 진입점 | `asset.import <fbx> -out=<Content 경로>` | Graphics(또는 에디터) .cpp | 공백 경로는 따옴표. `-name value` 불가 |
| GameModule R8 헤드리스 모듈 검증 | `modtest <Module>` | JGConsole .cpp 하나 | 게임 모듈 이름은 프로젝트마다 다름 → 고정 목록(`CONSOLE_ENGINE_MODULES`) 불가, R8에서 로드 방식 결정 |
| GameFrameWorks B-3 헤드리스 러너(보류) | `<이름> <file> [-repeat=N]` | JGConsole .cpp 하나 또는 GFW 모듈 | 반복마다 `GMemoryGlobalSystem::Flush()`(JGConsole은 GC를 돌리지 않음). 이름은 구 문서의 `simrun` 가정을 버리고 GFW가 정한다 |
| Server `net.test` 이동(권고) | `net.test` | GFW `Network/` .cpp | JGConsole이 시작 시 GFW를 연결하므로 옮겨도 헤드리스에서 보인다. `net.host/net.join`은 루프를 도는 프로세스용이라 JGConsole 유지 |

---

## 보류 (지금 필요한 곳이 없음 — 다시 볼 조건과 함께)

| ID | 항목 | 다시 볼 조건 | 메모 |
|---|---|---|---|
| B-1 | 런처 실행 인자로 명령 실행(`JGLauncher.exe -exec="devscene.readback"` 류) | Graphics 5-25를 명령으로 바꿔 자동 검증 루프가 리드백 PNG를 잃을 때 | Phase 0 제외 목록에 있었다. 구현하면 첫 프레임 뒤 `Submit`(프레임 밖) |
| B-2 | `net.test`를 GameFrameWorks `Network/`로 이동 | Server 트랙 판단 | 위 표의 권고와 같음. 핸들러 안 `ConnectModule("GameFrameWorks")`는 제거 가능(무해) |
| B-3 | `HGUI::InputText` 512바이트 고정(GUI_TODO 2-9) | `HGUI::InputText` 호출자가 생길 때 | 현재 호출자 0. 콘솔은 `InputTextWithHistory`(`CallbackResize`, 가변) |
| B-4 | 한글 글리프 폰트(GUI 백로그 "폰트") | 콘솔 출력·설명에 한글이 필요해질 때 | 그때까지 이름·설명·출력은 영어 |
| B-5 | `DevConsole.module.json` 의존 `Graphics`·`Asset` 정리 | 모듈 의존을 정리할 일이 생길 때 | DevConsole 소스는 Core·GUI 헤더만 포함. 빌드에 문제 없음 |
| B-6 | 창 입력 검증 스크립트 경로 인자를 메인 트리 기본값으로 | 3-4 회귀를 다시 돌릴 때 | `Files/tools/devconsole_ui_check.ps1.txt`의 `-Exe/-WorkDir/-OutPath` 기본값이 삭제된 worktree 경로다. 인자로 넘기면 그대로 쓸 수 있다 |

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
| 2-1 | `HGUI::Text`가 문자열을 printf 서식으로 넘기던 버그 → `TextUnformatted`(색은 `PushStyleColor`) | 2026-09-29 | `Files/2026-09-29_devconsole_ui_memorywindow.png`(peak 열·`%` 표시) vs 이전 증상 `Document/Memory/Memory/Files/2026-09-28_phase2_memwidget_capture.png` |
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

---

## 제외 항목 (Phase 0 결정 2026-09-29 — 지금 필요한 곳이 없어 넣지 않음)

| 항목 | 이유 | 다시 넣을 조건 |
|---|---|---|
| 콘솔 변수(CVar) | 엔진 전체에 쓰는 곳 없음 | 런타임 토글 값이 여럿 생겨 명령 하나씩 만들기가 번거로울 때 |
| Tab 자동완성 | 명령 수가 적어 `help`로 충분 | 명령이 수십 개가 될 때 |
| 스크립트 파일 실행 | 필요한 곳 없음 | GFW B-3 헤드리스 러너가 명령 스크립트를 정의할 때(그쪽 항목) |
| 콘솔 토글 키 | 메뉴 `Windows/DevConsole`로 충분 | 사용자 요청 시 |
| 등록 매크로 | 전역 변수 선언 한 줄로 충분. 매크로는 인자 이름이 숨고 디버깅 때 코드가 덜 보임 | — |
| Core 멀티캐스트 델리게이트 §2-4·5 수정(`Delegate.h:907-917` 값 인자 이동, `:832-855` 브로드캐스트 중 `Remove`) | 새 구조는 멀티캐스트를 쓰지 않는다. 남은 사용처(`JWindow.h`, `DX12FrameBuffer.h`, `GUIBackend.h`)는 인자가 포인터·정수·참조뿐이고 브로드캐스트 중 해제 없음 | 값 타입 인자를 브로드캐스트하거나 브로드캐스트 중 `Remove`하는 코드가 생길 때(Core 트랙) |
| 런처 ↔ JGConsole 통신(IPC) | 두 경로는 별개 프로세스로 설계 | — |
