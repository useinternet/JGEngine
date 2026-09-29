# DevConsole 현황 분석 (2026-09-29)

대상: `Source/Editor/DevConsole` 6개 파일 285줄과 연관 코드(Core 델리게이트·문자열·로그, GUI 위젯).
코드 읽기 기준이다. 빌드·실행은 하지 않았다(다른 세션의 MSBuild가 도는 중이었다). 근거 상세는 `Document/Memory/2026-09-29_DevConsole_현황분석.md`.

## 결론

- DevConsole은 **입력창 1개 + 간이 파서 + 전체 브로드캐스트**만 있는 뼈대다. 명령을 등록하는 코드가 엔진 전체에 **한 곳도 없어서** 실제로 동작하는 명령이 없다.
- 등록 API는 **첫 호출에서 assert로 죽는다**(검사 조건이 반대로 되어 있음). 이걸 고쳐도 콘솔 창을 한 번 열기 전에는 등록이 조용히 실패한다. 지금 구조로는 모듈이 명령을 등록할 수 없다.
- 레지스트리가 Editor 모듈의 GUI 위젯 안에 있어서 Runtime 모듈과 JGConsole(헤드리스)에서 쓸 수 없다. **명령 레지스트리는 Core 전역 시스템으로 옮기고 DevConsole은 UI만 맡는 구조**를 권장한다(§6).

## 1. 현재 구조

`JGLauncher` → `JGDev_Graphics`가 `ConnectModule("DevConsole")`(`JGDev_Graphics.cpp:58`) → 메뉴 `Windows/DevConsole` 등록(`DevConsoleModule.cpp:27-35`) → 메뉴를 누르면 위젯 생성·열기 → Enter → `HDevConsoleArguments` 파싱 → `OnDevConsole.BroadCast`로 모든 리스너 호출(`DevConsole.cpp:15-24`).

| 파일 | 줄 | 내용 |
|---|---|---|
| `DevConsoleDefines.h` | 69 | API 매크로, 파서 `HDevConsoleArguments`(헤더 인라인) |
| `DevConsole.h/.cpp` | 38/45 | 위젯 `JGDevConsole`: `HGUI::InputText("Cmd")` + 이벤트 `HOnDevConsole` |
| `DevConsoleModule.h/.cpp` | 26/76 | 메뉴 등록, `Register/UnRegisterConsoleCommand`(위젯에 위임) |
| `DevConsole.module.json` | 31 | 의존: Core·Graphics·Asset·GUI |

- 문법: 공백으로 자른 첫 토큰이 명령이고, `-name` / `-name=value` 형태만 인자로 받는다. 예) `dump -path=a.png -force` → 명령 `dump`, 인자 {path: a.png, force: ""}.
- 사용처: 등록 호출 0곳, 모듈 연결 1곳(JGDev_Graphics). JGConsole·JGEditor는 쓰지 않는다.
- DevConsole 폴더에 커밋 안 된 변경은 없다(마지막 변경 2026-09-16 `c300fd3`).

## 2. 코드로 확인한 문제 (재현은 안 함)

| # | 위치 | 문제 | 결과 |
|---|---|---|---|
| 1 | `DevConsoleModule.cpp:73` | `JG_CHECK(GUIModule == nullptr)` 조건이 반대로 되어 있다. DevelopEngine은 DebugConfig(NDEBUG 없음)라 assert가 켜져 있다 | `RegisterConsoleCommand` 첫 호출에서 abort (GUI_TODO 1-6, 아직 안 됨) |
| 2 | `DevConsoleModule.cpp:45-49`, `GUIModule.h:35-71` | 위젯은 첫 `OpenWidget` 때 만들어진다. 그 전에는 `FindWidget`이 null을 돌려준다 | 1을 고쳐도 `StartupModule`에서 한 등록은 빈 핸들을 받고 조용히 실패 |
| 3 | `DevConsoleModule.cpp:67` | GUI 모듈 포인터를 `static`으로 캐시한다 | GUI를 다시 연결하면 댕글링 (GUI_TODO 1-6) |
| 4 | `Delegate.h:907-917` | `BroadCast`가 루프 안에서 값 인자를 `std::forward`로 넘긴다 → 첫 리스너가 `Params`(HHashMap)를 이동해 가져간다 | 리스너가 2개 이상이면 두 번째부터 인자가 빈 상태로 온다(`Command`는 PString에 이동 생성자가 없어서 남는다). 값 타입 인자를 쓰는 멀티캐스트는 지금 이것뿐 |
| 5 | `Delegate.h:832-855, 678-682` | 브로드캐스트 도중 `Remove`는 콜백만 비우고 슬롯 핸들은 유효한 채로 둔다 | 명령 핸들러 안에서 등록을 해제하면 다음 입력 때 "Delegate is not bound" assert |
| 6 | `DevConsoleDefines.h:21-51` | 위치 인자를 버린다. 앞에 공백이 있으면 명령이 빈 문자열이 된다. 따옴표를 지원하지 않는다. 값의 두 번째 `=` 뒤를 자른다. 대소문자를 구분한다 | `stat fps`의 `fps`가 사라지고 `-path="a b"`가 깨진다 |
| 7 | `String.cpp:365-373` | `ToInt/ToFloat`가 `std::stoi/stof`를 써서 잘못된 입력에 예외를 던진다. 엔진에 catch가 없다 | `-count=abc`를 변환하면 프로세스 종료(코드 3) |
| 8 | `GUI.cpp:59-71` | 입력 버퍼가 512바이트로 고정이고 `memcpy_s` 길이 검사가 없다 | 512자 이상 넣으면 디버그 CRT에서 중단 (GUI_TODO 2-9) |
| 9 | `DevConsoleModule.cpp:23` | 로그 카테고리가 `DevStatistics`로 복붙되어 있다 | GUI_TODO 1-9 |

## 3. 없는 기능

| 기능 | 현재 |
|---|---|
| 이름 → 명령 레지스트리, 설명·도움말 | 없음. 모든 리스너가 모든 입력을 받아서 이름을 직접 비교해야 함 |
| 결과 반환·오류 메시지 | 없음. 핸들러가 `void`이고 모르는 명령도 조용히 무시 |
| 모듈이 해제될 때 등록 해제 | 없음. `ShutdownModule`이 빈 함수(`DevConsoleModule.cpp:38-41`) |
| 예외 없는 타입 변환 | 없음 (§2-7) |
| 콘솔 변수(CVar) | 엔진 전체에 없음 |
| 출력 창 | 없음. 로그는 파일·stdout 싱크로만 나감(`Log.cpp:12, 20-22`) |
| 히스토리·자동완성·`help`/`list` | 없음. Enter를 누르면 입력창 포커스도 풀림(ImGui 기본값 `ConfigInputTextEnterKeepActive = false`) |
| 헤드리스 실행·실행 인자·스크립트 | 없음. JGConsole은 `argv[1] == "simtest"`만 하드코딩(`Main.cpp:43`) |
| 실행 시점 제어 | 없음. ImGui 생성 도중(GraphicsBegin 버킷, `DX12GUIBackend.cpp:87`)에 바로 실행되므로 GPU 작업을 하는 명령이 프레임 중간에 돈다 |

## 4. 재사용할 수 있는 기반

| 기반 | 위치 | 비고 |
|---|---|---|
| 전역 시스템 `GGlobalSystemInstance<T>` | `CoreSystem.h:147-`, `CoreSystem.cpp:35-41` | 레지스트리를 둘 자리. 모듈 연결(`:55-61`)보다 먼저 생성된다 |
| 단일 델리게이트 `HDelegate` | `Delegate.h:580-700` | 핸들러 타입(Raw/SP/Lambda/Static). 멀티캐스트는 §2-4·5 때문에 디스패치에 쓰기 어렵다 |
| spdlog 1.10 `ringbuffer_sink`/`base_sink` | `ThirdParty/spdlog/sinks` | 출력 창에 보일 로그 수집 |
| ImGui InputText 콜백(History/Completion) | ImGui 1.91.1 | 히스토리·자동완성 |
| `JGFUNCTION` 리플렉션 | `HeaderTool.cpp:1062-`, `ObjectGlobals.h:121-150` | 쓰는 곳이 없다. `Invoke<Ret, Args...>`는 컴파일 타임 타입만 받으므로 문자열 명령에는 바로 못 쓴다 |

## 5. 명령이 생기면 바로 쓸 항목

| 항목 | 지금 방식 |
|---|---|
| Graphics_TODO 5-25 DevScene 리드백 덤프 | 메시 로드 30프레임 뒤 자동 실행(`DevScene.cpp:196-199`) |
| Memory_TODO 0-2 풀 고갈 재현 | 환경 변수 `JG_MEMTEST` 훅으로 대신함(검증 후 제거됨) |
| JGConsole `simtest` (이름변경안에서 `gmtest` 검토 중) | argv 하드코딩 |

## 6. 다음 단계에서 정할 것

| 결정 | 권장 | 이유 |
|---|---|---|
| 레지스트리 위치 | Core 전역 시스템에 두고 DevConsole은 UI만 | DevConsole에 두면 Runtime 모듈이 Editor 모듈에 의존해야 하고 JGConsole에서 못 쓴다 |
| 1차 범위 | 명령 등록·실행, help/list, 출력 창, 히스토리 | CVar·자동완성·스크립트/실행 인자는 2차 |
| 인자 문법 | `-name=value`는 유지하고 위치 인자·따옴표 추가, 변환은 Try 계열 | 기존 규칙과 호환되고 변환에서 예외가 안 난다 |
| 기존 TODO | GUI_TODO 1-6·1-9·2-9를 이번 작업에 포함 | 구조를 바꾸면 1-6 코드는 없어진다 |
| Core 델리게이트 §2-4·5 | 별도 항목으로 수정 | 콘솔이 멀티캐스트를 안 써도 Core 버그로 남는다 |
