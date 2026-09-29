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
- 사용자가 결정할 것(분석 문서 §6): 레지스트리 위치(권장 Core 전역 시스템), 1차 범위(권장: 명령·help/list·출력 창·히스토리, CVar는 2차), 인자 문법(권장: `-name=value` 유지 + 위치 인자·따옴표, Try 변환). 결정이 나면 `Document/DevConsole_TODO.md`를 단계별로 만들고 이 파일 끝에 진행 기록을 한 줄씩 남긴다.
- 새 파일을 추가하면 JGBuildTool 재실행(vcxproj 글롭), JGCLASS 변경은 JGHeaderTool 재실행. 헤드리스 검증은 JGConsole에 명령 실행 경로를 붙이면 `JGConsole.exe <명령>`으로 할 수 있다(런처는 GUI 확인용).
- 정적 저장소에 엔진 컨테이너(`HHashMap` 등)를 두면 DLL 언로드 때 크래시한다. 레지스트리는 전역 시스템 객체의 멤버로 둔다.

## 진행 기록
- 2026-09-29 분석 문서와 이 기록 작성. 소스 변경 없음.
