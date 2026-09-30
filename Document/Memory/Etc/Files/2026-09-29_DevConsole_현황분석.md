# 2026-09-29 작업 기록 — DevConsole 현황 분석 (콘솔 명령 시스템 착수 전)

다른 AI 에이전트나 다음 세션이 이어서 작업할 수 있도록 현재 상황과 이번 작업 내용을 기록한다.

## 요청
"DevConsole 기능을 확장해서 콘솔 명령어를 만들 수 있는 시스템을 만들 것이다. 현재 상황부터 확인해줘."

## 결과
분석 문서 **`Document/DevConsole_현황분석_2026-09-29.md`** 작성. 소스 변경 없음. 빌드·실행 없음(다른 세션의 MSBuild·link가 돌고 있어서 메인 트리 빌드를 피했다). 모든 항목은 코드 읽기로 확인했고 재현은 하지 않았다.

### 한 줄 요약
DevConsole은 입력창 + 간이 파서 + 전체 브로드캐스트뿐이고 등록하는 코드가 0곳이다. 등록 API는 assert 반전으로 첫 호출에서 죽고, 위젯이 열리기 전에는 등록이 실패하며, 레지스트리가 GUI 위젯 안에 있어서 Runtime 모듈·JGConsole이 쓸 수 없다. → Core 레지스트리 + DevConsole UI 구조를 권장했다.

### 근거 상세 (분석 문서 §2 보충)
- **§2-1 assert 반전이 실제로 켜져 있는 이유**: `JG_CHECK(x)` = `assert(x)`(`CoreDefines.h:51`). `DevConsole.module.json`의 DevelopEngine 구성은 `DebugConfig`이고 `jgengine.lua:8-15` DebugConfig에 NDEBUG가 없다(Confirm/Release에만 있음). PCH·Core 헤더에도 NDEBUG 재정의가 없다. `RegisterConsoleCommand`는 `GetCheckedGUIModule()`을 `FindWidget`보다 먼저 부르므로 위젯이 열렸든 아니든 abort한다.
- **§2-4 브로드캐스트 인자 이동**: `HMulticastDelegate<HDevConsoleArguments>::BroadCast(Args... args)`가 리스너마다 `Callback.Execute(std::forward<Args>(args)...)` → Args가 값 타입이라 rvalue 캐스트 → `HDelegate::Execute(Args... args)`(값 매개변수)가 이동 생성된다. `HDevConsoleArguments`는 사용자 선언 복사/이동/소멸자가 없어서 암시적 이동 생성자가 멤버별 이동을 한다. `HHashMap`(std::unordered_map)은 이동되어 원본이 비고, `PString`은 사용자 선언 소멸자와 `operator=(const PString&) = default` 때문에 이동 생성자가 없어서 복사된다. 결과적으로 두 번째 리스너부터 `Command`는 있지만 `Params`는 비어 있다. 다른 멀티캐스트(`HWndProc`, `HOnResize`, `HOnPresent`, `HOnUpdate`, `HOnGUI`, `HOnMainMenuGUI`)는 인자가 포인터·정수·참조뿐이라 영향이 없다.
- **§2-5 브로드캐스트 중 Remove**: `Remove`(`Delegate.h:832`)는 잠금 중이면 `_events[i].Callback.Clear()`만 하고 `_events[i].Handle`은 유효한 채로 둔다(호출자 핸들만 Reset). 다음 `BroadCast`는 `Handle.IsValid()`만 보고 `Execute`를 부르고 → `DELEGATE_ASSERT(_allocator.HasAllocation())`(`:680`)에서 assert. 같은 이유로 브로드캐스트 중 `Add`는 `_events.emplace_back`이 재할당하면서 지금 실행 중인 인라인 델리게이트 저장소를 옮길 수 있다(코드상 위험, 미확인).
- **§2-7 변환 예외**: `PString::ToInt`는 `std::stoi`, `ToFloat`는 `std::stof`이라 `"abc"`에 `std::invalid_argument`, 범위 초과에 `std::out_of_range`를 던진다. 메모리 기록에 따르면 엔진에는 catch가 없어서 종료 코드 3(terminate)이 된다.
- **§2-6 파서 세부**: `Split(' ')`은 `std::getline` 기반이라 연속 공백은 빈 토큰을 만든다(인자 쪽은 빈 토큰을 건너뛰지만 명령 토큰은 Trim도 하지 않는다). 인자 값은 `Split('=')`의 [1]만 쓴다. `Params.emplace`라 같은 이름이 두 번 나오면 첫 값이 남는다. `PString::operator==`는 해시 비교(`String.cpp:108`)이고 `std::hash<PString>`도 같은 해시를 쓴다.
- **위젯 수명**: `OpenWidget<T>`는 첫 호출 때 `Allocate<T>()`로 만들어서 `Widgets` 맵에 넣고 닫아도 지우지 않는다(`GUIModule.h:35-47`). 따라서 한 번 열고 나면 등록이 유지되지만, 열기 전 등록은 실패하고 GUI 모듈이 다시 만들어지면 등록이 사라진다. `JGDevConsole`에는 `DEVCONSOLE_API`가 없어서 다른 DLL은 `HDevConsoleModule`을 통해서만 등록할 수 있다.
- **실행 시점**: `PDX12GUIBackend::NewFrame`이 GraphicsBegin 버킷에서 돈다(`DX12GUIBackend.cpp:87`) → `OnGUI` → 위젯 `OnGenerateGUI` → 여기서 명령이 실행된다. GPU 리드백 같은 명령은 프레임 중간에 돈다(참고: GUI_TODO 1-3의 `ReadPixelsImmediate called inside a frame` 경고).
- **모듈 해제**: `DisconnectModule`(`Module.cpp:188-220`)은 ShutdownModule → 등록 해제 → GC Flush → 모듈 객체 Deallocate 순서로 진행하고, DLL은 `GCoreSystem::Destroy` 끝에서 내린다. Core 레지스트리를 만들면 모듈이 등록한 핸들러(모듈 멤버를 캡처)는 그 모듈의 ShutdownModule에서 반드시 해제해야 한다. 소유 모듈 기준 일괄 해제나 RAII 핸들이 필요하다.
- **헤드리스**: JGConsole은 `GCoreSystem::Update()`를 부르지 않아서 스케줄러가 돌지 않는다. 명령 실행을 큐에 넣는 구조로 만들면 JGConsole에서는 큐를 직접 비워 줘야 한다.
- **출력**: `GLogGlobalSystem`(`Log.h:28-53`)은 파일 싱크(`jg_log.txt`) + stdout 컬러 싱크뿐이다. spdlog 버전은 1.10.0이라 `callback_sink`(1.11+)는 없고 `ringbuffer_sink.h`와 `base_sink`는 있다. `AddLog`는 포맷을 두 번 거친다(`Log.h:40-41`). 사용자 입력을 로그 텍스트 자리에 그대로 넣으면 `%`가 해석되므로 반드시 `"%s"` 인자로 넘겨야 한다.
- **리플렉션**: `JGFUNCTION`은 헤더 툴이 파싱하고 `FunctionMap`/`BindFunction` 코드를 생성하지만(`HeaderTool.cpp:1062-1175`) 쓰는 클래스가 0개다. `JGFunction::Invoke<Ret, Args...>`(`ObjectGlobals.h:142`)는 타입을 컴파일 타임에 알아야 한다.

### 관련 TODO 교차점
- GUI_TODO(2026-09-21, 전부 미체크): 1-6(assert 반전·static 캐시), 1-9(로그 카테고리), 2-9(InputText 512), 3-2(위젯 GUID 정리, DevConsole의 수동 `GetGUID` 오버라이드 삭제). 새 콘솔 작업에 포함하거나 순서를 맞출 것.
- 명령을 기다리는 곳: Graphics_TODO 5-25(리드백 덤프 → 콘솔 명령), Memory_TODO 0-2(당시 콘솔이 없어서 `JG_MEMTEST` 환경 변수를 씀, 훅은 제거됨), GameFrameWorks 이름변경안의 `simtest`→`gmtest`(JGConsole argv).

### 작업 트리 상태 (2026-09-29 확인)
- `Source/Editor/DevConsole` 미커밋 변경 없음. `Source/Runtime/GUI/GUI.cpp`에 다른 작업의 미커밋 변경(`PlotBarGroups` 틱 배열 수정, 2026-09-28 메모리 위젯 크래시)이 있다. DevConsole과는 무관하니 건드리지 말 것.
- 확인 시점에 다른 세션의 MSBuild 4개와 link 1개가 돌고 있었다. 빌드 전에 `tasklist`로 확인하고, 겹치면 worktree에서 검증한다(`jgengine-tool-quirks` 메모리 참고).

## 다음 작업자에게
- 할 일 목록은 **`Document/DevConsole_TODO.md`**(Phase 0 결정 4개 → 1 Core 레지스트리·파서·JGConsole → 2 출력 기반 → 3 DevConsole 창 → 4 연결·문서). Phase 0의 `결정 필요` 4개(위치·이름·문법·범위)는 착수 전에 사용자 확인을 받는다. Phase 1 이후 항목은 권장안 기준으로 적혀 있으니, 결정이 다르게 나면 해당 항목을 먼저 고친다.
- 새 파일을 추가하면 JGBuildTool 재실행(vcxproj 글롭), JGCLASS 변경은 JGHeaderTool 재실행. 헤드리스 검증은 JGConsole에 명령 실행 경로를 붙이면 `JGConsole.exe <명령>`으로 할 수 있다(런처는 GUI 확인용).
- 정적 저장소에 엔진 컨테이너(`HHashMap` 등)를 두면 DLL 언로드 때 크래시한다. 레지스트리는 전역 시스템 객체의 멤버로 둔다.

## 진행 기록
- 2026-09-29 분석 문서와 이 기록 작성. 소스 변경 없음.
- 2026-09-29 `Document/DevConsole_TODO.md` 작성. 소스 변경 없음.
  TODO를 쓰면서 새로 확인한 사실:
  - **대기 항목 5개**: Graphics 5-25·5-31, GameFrameWorks 1-2 `simrun`, 게임모듈 R8 `modtest`, 리슨서버 `nethost/netjoin`. 이 중 3개가 JGConsole(헤드리스)이라 Core 레지스트리 권장의 근거가 됐다.
  - **`HGUI::Text` 서식 해석 버그**(`GUI.cpp:49-57`)가 이미 증상을 낸다. 메모리 통계 창 줄 `"%5.1f%%   %7u"`(`MemoryStatistics.cpp:135`)가 1차 서식 뒤 `12.5%   123`이 되고, ImGui가 `%   123`을 변환 지정자로 먹는다. 그래서 `2026-09-28_phase2_memwidget_capture.png`에서 usage 값에 `%`가 없고 peak 열이 비어 있다.
  - **ImGui는 GUI.dll 밖으로 노출되지 않는다**(`IMGUI_API` 비어 있음, `imconfig.h:26-27`). ImGui를 직접 부르는 파일은 GUI 모듈 6개뿐이라 DevConsole은 HGUI 래퍼를 늘려야 한다.
  - **JGConsole은 알 수 없는 명령도 종료 코드 0**이다(`Main.cpp:43-47`). 검증 스크립트의 오타를 못 잡는다.
  - **GameFrameWorks_TODO 1-2가 `-repeat N`(공백 구분)으로 적혀 있어** 권장 문법(`-name=value`)과 다르다. 0-3이 확정되면 4-1에서 문구를 고친다.
  - **Update·Destroy 순서**: 스케줄러는 `GScheduleGlobalSystem::Update` 한 번에 모든 버킷을 돌린다. 그래서 `GStringTable` 다음·`GModuleGlobalSystem` 앞에 등록한 시스템은 Update가 프레임 밖이고, Destroy가 모듈 해제 뒤가 된다. `ReadbackTextureImmediate`의 프레임 안 경고 기준은 `BeginFrame`~`OnFrameSubmitted`다(`ResourceStagingManager.cpp:506, 520`).
  - 현황분석 §6의 "Core 델리게이트 §2-4·5 수정" 권장을 **제외로 바꿨다**. 남은 멀티캐스트 사용처는 인자가 단순하고, 브로드캐스트 중 해제가 없다(`RemoveAll`은 `GUIBackend.cpp:11-12`의 종료 경로뿐).
  - GUI_TODO 1-6·1-9에 "DevConsole 트랙에서 처리" 표시와 `HGUI::Text` 참고 줄을 추가했다.
- 2026-09-29 **Phase 0 결정**(사용자): Core 전역 시스템 / 이름 `ConsoleCommand` / 문법 권장안 그대로(`area.action`, 위치 인자 + `-name=value` + `-name`, 따옴표, `-name value` 불가) / 입력 히스토리 포함.
  사용자 질문 "명령 등록은 어떻게 하나"에 답해 TODO에 **"명령 등록 방법"** 절을 추가했다. 내용: 모듈 예시(Devkit `devscene.readback`, `BindRaw`), JGConsole 예시(`simrun`, `BindLambda`), 등록 주체별 등록·해제 위치, 핸들러 규칙(반환값, `TryGet*` 기본값 패턴, 출력, 실행 시점, 스레드), 정적 자동 등록을 두지 않는 이유(`Module.cpp:98 → 113 → 147`: 정적 초기화가 `Link_Module` 전에 돈다).
  JGConsole 예시는 등록·실행·해제를 `runCommandLine` 한 함수 안에서 끝낸다. `main`의 지역 엔진 컨테이너는 `GCoreSystem::Destroy` 뒤에 해제되기 때문이다. 1-3 구현 때 이 형태를 따른다. 소스 변경 없음.
- 2026-09-29 사용자 질문 두 가지에 답해 TODO를 고쳤다.
  - (1) "JGConsole.exe로 실행하면 프로세스를 띄우나?" → 아니다. 런처 콘솔은 같은 프로세스 안에서 실행하고, JGConsole.exe는 따로 실행하는 프로그램이다. 레지스트리는 프로세스마다 있다. TODO에 경로 표를 추가했다.
  - (2) "언리얼처럼 전역 변수로 등록하면?" → **0-5 결정: 전역 변수 `HAutoConsoleCommand`가 기본.** 생성자는 포인터만 들고 바이너리별 목록(`static Head`, Core가 정적 라이브러리라 DLL·EXE마다 따로 있다)에 이어지기만 한다. 등록은 모듈 연결 때 한다(Source 엔진 `CON_COMMAND` 방식).
  - 모듈 훅은 `IModuleInterface`의 비순수 가상 함수 `RegisterAutoConsoleCommands`/`UnregisterAutoConsoleCommands`로 한다. 모듈 객체가 자기 DLL에서 만들어지므로 가상 호출이 그 DLL의 사본(그 DLL의 목록)으로 간다. `GModuleGlobalSystem`이 `StartupModule` 직후(`Module.cpp:147`)와 `ShutdownModule` 직전(`:208`, `:250`)에 부른다. EXE 목록은 레지스트리 생성자/`Destroy`가 맡는다.
  - 규칙: 캡처 없는 람다·정적 함수만 된다(모듈 상태는 `FindModule`로 찾는다). 파일 범위에만 선언한다. Core에는 선언하지 않는다(링커 제거 또는 바이너리마다 중복 등록). `Register` API는 동적인 경우·자체 테스트용으로 남긴다.
  - Phase 1 번호가 바뀌었다: 1-3 신설(HAutoConsoleCommand + 모듈 훅), JGConsole 전환 1-4, 자체 테스트 1-5, 검증 1-6(GameFrameWorks 임시 전역 명령으로 훅 확인), 커밋 1-7. JGConsole 명령은 앞으로 명령마다 .cpp 하나로 추가한다. 세 트랙이 `Main.cpp`를 동시에 고치지 않게 하려는 것이다. 소스 변경 없음.
- 2026-09-29~30 **Phase 1~4 구현·검증 완료**(사용자: "모든 페이즈 순서대로 진행"). 커밋은 사용자 몫.
  - 진행 방식: 다른 세션이 Core(`Memory.cpp/.h`)를 고치며 빌드 중이어서, HEAD 격리 worktree(`<scratchpad>/wt`)에서 구현·검증한 뒤 메인 트리에 옮겼다. 옮긴 뒤 JGBuildTool 재생성 → 메인 트리 전체 빌드 오류 0 → 헤드리스·런처 회귀 통과. 증거는 `Document/Memory/2026-09-29_devconsole_verify.txt`, 캡처는 `2026-09-29_devconsole_ui_{log,history,memorywindow}.png`.
  - 검증 요약:
    - `console.selftest` 57/57, `gmtest`/`simtest` 통과, `gmtset` 종료 1, 인자 없으면 help.
    - 모듈 훅: 로드만 된 DLL은 미등록 → 연결 시 등록 → 해제 시 제거.
    - 런처 창 입력: help, 모듈 명령 실행, 오류 줄 빨강, `%s` 글자 그대로, ↑/↓ 히스토리, 명령으로 창 열기.
    - 런처 60초: 종료 0, `[error]` 0, PSO 2, live 블록 0, 남은 명령 경고 0.
  - **scratchpad worktree 함정**: %TEMP% 아래라 MSBuild 추적이 헤더 변경을 놓친다(MSB8029). `Log.h`에 멤버를 넣은 뒤 증분 빌드에서 `CoreSystem.cpp`가 다시 컴파일되지 않았고, 모든 프로세스가 종료 때 "HEAP CORRUPTION DETECTED" CRT 대화상자에서 멈췄다. Rebuild로 해결했다. 메인 트리는 정상이다.
  - **SendKeys 사고**: 런처를 전경 창으로 못 만든 채 `SendKeys`를 보내서, 당시 활성 창에 `help` 등 몇 줄이 입력됐을 수 있다(2026-09-29 22:5x, 사용자에게 알림). 그 뒤로는 런처 창에만 PostMessage(입력줄 클릭 + WM_CHAR/WM_KEYDOWN)로 보냈다. 스크립트 `Document/Memory/tools/devconsole_ui_check.ps1.txt`. 입력줄 좌표(창 기준 500,895)는 이 imgui.ini 배치 기준이다. 창을 옮기면 `-ClickX/-ClickY`로 바꾼다. 콘솔이 자동으로 열리지 않으므로 검증할 때만 `DevConsoleModule::StartupModule`에 `OpenWidget<JGDevConsole>()`를 임시로 넣는다.
  - **Network 세션과 병합**: 통합 직전(22:42) Network 세션이 `Main.cpp`에 `net.*` 분기와 `NetCommands.cpp`를 넣었다. `Main.cpp`는 레지스트리 판으로 쓰되, `net.*`는 `runNetCommand`로 보내는 임시 분기를 `main`에 남겼다(`net.test transport` 통과). 옮기는 방법은 `Network_TODO.md` 14행에 적었다. `NetCommands.cpp`는 건드리지 않았다.
  - **커밋 대상(이 트랙)**:
    - 새 파일: `Source/Runtime/Core/ConsoleCommand/` 4개, `Source/Programs/JGConsole/ConsoleCommandSelfTest.cpp`
    - 수정: `Core/CoreSystem.cpp`, `Core/Misc/{Module.h,Module.cpp,Log.h,Log.cpp}`, `GUI/{GUI.h,GUI.cpp}`, `Editor/DevConsole/` 5개, `Programs/JGConsole/Main.cpp`
    - 문서: `Document/DevConsole_*`, 이 파일, `Document/Memory/2026-09-29_devconsole_*`, `Document/Memory/tools/devconsole_ui_check.ps1.txt`, 그리고 GUI_TODO·Graphics_TODO·GameFrameWorks_TODO·게임모듈 분석·Network_TODO에 추가한 연결 줄
    - **`Main.cpp`는 Network 세션의 `NetCommands.cpp`(미추적)와 같이 커밋해야 빌드된다**(`runNetCommand` 참조).
    - `Core/Memory/*`, `Devkit/DevScene.cpp`, `Graphics/Classes/StaticMesh.*`, `GameFrameWorks/**`, `NetCommands.cpp`는 다른 트랙 것이다.
    - `Bin/DevelopEngine/*`는 메인 트리 빌드로 갱신됐다(다른 트랙 변경 포함).
  - 2026-09-30 후속:
    - Network 트랙이 `net.*`를 `NetCommands.cpp`의 `HAutoConsoleCommand`로 옮기고 `Main.cpp`의 임시 분기를 지웠다. 이제 `NetCommands.cpp`가 이 트랙의 Core 변경에 의존한다(같이 커밋).
    - 런처 호스트가 JGDev_Graphics에서 JGEditor로 바뀌었고, JGEditor도 DevConsole을 연결한다(`JGEditor.cpp:19`).
    - 사용자 요청으로 별칭 `simtest`를 제거했다(`Main.cpp`, 자체 테스트 검사 한 줄). JGConsole만 빌드해 오류 0, `console.selftest` 57/57, `gmtest` 통과, `simtest`는 Unknown·종료 1. GFW 리뷰 절차 문서에도 표시했다.
  - 남은 것: 명령을 쓰는 쪽(Graphics 5-25·5-31, GFW 헤드리스 러너, 게임모듈 R8)은 각 트랙 몫이다. `HGUI::InputText` 512(GUI_TODO 2-9)는 콘솔이 쓰지 않아 그대로다. 런처 실행 인자로 명령 실행, CVar, 자동완성은 제외 결정 그대로다.
