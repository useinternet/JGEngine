# 콘솔 명령 시스템 할 일 목록 (순차 진행용)

작성 2026-09-29. 근거는 `Document/DevConsole_현황분석_2026-09-29.md`(§2 문제 목록, §3 없는 기능)와 `Document/Memory/2026-09-29_DevConsole_현황분석.md`.
위에서부터 순서대로 진행한다. Phase 0(결정) → 1(Core 레지스트리·파서·전역 변수 등록·JGConsole) → 2(출력 기반: GUI 래퍼·로그 버퍼) → 3(DevConsole 창) → 4(연결·문서) 순서이며, 각 단계 끝에 검증과 커밋 항목이 있다.
한 항목이 끝나면 `[x]`로 바꾸고, 한 단계가 끝나면 `Document/Memory/2026-09-29_DevConsole_현황분석.md` 끝의 "진행 기록"에 한 줄 추가한다.
`결정 필요` 표시 항목은 착수 전에 사용자 확인을 받는다. Phase 0 결정(2026-09-29)은 아래 항목에 반영돼 있다. 파일:행 표기는 2026-09-29 소스 기준이므로 편집 후에는 어긋날 수 있다.
진행 상태: Phase 0 완료(2026-09-29 사용자 결정: 0-1~0-4 권장안, 입력 히스토리 포함, 0-5 전역 변수 등록). Phase 1~4 미착수. 명령을 어떻게 등록하는지는 Phase 1 앞의 "명령 등록 방법" 절을 본다.

**목표.** 어느 모듈이든 같은 방식으로 이름을 붙여 명령을 등록하고, 런처의 DevConsole 창과 `JGConsole.exe <명령>`(헤드리스) 양쪽에서 실행한다. 두 경로는 서로 다른 프로세스이며 서로 통신하지 않는다("명령 등록 방법" 첫 표).

**이 목록이 끝나면 명령으로 구현할 항목** (지금은 모두 명령 수단이 없어 막혀 있거나 하드코딩으로 우회 중):

| 항목 | 실행 위치 | 지금 방식 |
|---|---|---|
| Graphics_TODO 5-25 DevScene 리드백 덤프 | 런처 (Devkit) | 메시 로드 30프레임 뒤 자동 실행 |
| Graphics_TODO 5-31 FBX 임포트 진입점 | 런처 (임포터가 Graphics 모듈에 있음) | 호출하는 코드 없음. 임시 스니펫으로만 검증 |
| GameFrameWorks_TODO 1-2 `simrun <file> -repeat N` | JGConsole | 미구현 (argv 하드코딩 구조) |
| `게임모듈_사전작업_분석_2026-09-29.md` R8 `modtest <Module>` | JGConsole | 미구현 |
| `리슨서버_설계방안_2026-09-29.md` 4단계 `nethost` / `netjoin` | JGConsole | 미구현 |

검증 루프(모든 단계 공통):
파일을 추가·삭제했으면 `Build/BatchFiles`에서 `JGBuildTool.exe`(종료 시 세그폴트 139, 산출물은 정상) → MSBuild DevelopEngine|x64 오류 0. Core는 모든 모듈이 링크하므로 Core를 바꾸면 전체를 다시 빌드한다. `tasklist`에 다른 세션의 MSBuild/cl/link가 보이면 메인 트리 빌드를 하지 말고 worktree에서 검증한다(`Document/Memory/2026-09-28_GameFrameWorks_리뷰.md` "검증 절차" 1-4단계, `-p:BuildProjectReferences=false`).
→ 헤드리스: `Bin/DevelopEngine`에서 `JGConsole.exe console.selftest` 종료 코드 0, 회귀로 `JGConsole.exe gmtest` 75/75 · 종료 코드 0.
→ 런처: `Document/Memory/tools/crashwalk/crashwalk.exe`로 60초 실행하고 표준 출력을 저장한다(공유 `jg_log.txt`는 다른 세션이 덮어쓴다). 종료 코드 0, `[error]`/`[critical]` 0, 1-2의 "남은 명령" 경고 0.
→ 창 입력(Phase 3): PowerShell `WScript.Shell` `AppActivate`/`SendKeys`로 시도한다. 입력이 안 들어가면 사용자에게 확인을 요청한다.

설계 규칙(이 목록 진행 중 지킬 것):
- 레지스트리와 명령 실행은 **메인 스레드 전용**이다. `Register`/`Unregister`/`Execute`/`Submit` 모두 메인 스레드에서만 부른다.
- **등록 해제는 자동이거나, 등록한 쪽의 책임이다.** 기본 방식인 `HAutoConsoleCommand`는 모듈 연결·해제에 맞춰 자동으로 등록·해제된다(1-3). `Register` API로 등록했으면 등록한 쪽이 직접 해제한다: 모듈은 `ShutdownModule`, JGConsole은 `GCoreSystem::Destroy` 전. 핸들러는 모듈 상태를 쓰는데, `DisconnectModule`은 `ShutdownModule` 직후 모듈 객체를 해제한다(`Module.cpp:208-217`). 남은 등록은 1-2의 종료 경고로 드러난다.
- **사용자 입력을 서식 자리에 넣지 않는다.** `JG_LOG(ConsoleCommand, Level, "%s", Line)`처럼 인자로 넘긴다. `AddLog`는 서식을 두 번 거친다(`Log.h:40-41`). GUI 표시는 2-1 뒤의 `HGUI::Text`로만 한다.
- 인자 변환에 `PString::ToInt`/`ToFloat`를 쓰지 않는다. 둘은 `std::stoi`/`stof`라 잘못된 입력에 예외를 던지고, 엔진에는 catch가 없다(종료 코드 3). `HConsoleCommandArgs::TryGet*`만 쓴다.
- 레지스트리는 `HMulticastDelegate`를 쓰지 않는다(현황분석 §2-4·5). 명령 하나에 `HDelegate` 하나다.
- 엔진 컨테이너를 static에 두지 않는다. 레지스트리 데이터는 전역 시스템 객체의 멤버로 둔다. `HAutoConsoleCommand` 전역 변수는 포인터만 든다. 로그 싱크 안에서는 엔진 풀과 `JG_LOG`를 쓰지 않는다(std 컨테이너만).
- 명령 이름·설명·출력 문자열은 영어로 쓴다. ImGui 기본 폰트에는 한글 글리프가 없다(GUI_TODO 백로그 "폰트").

제외한 것(지금 이것이 필요한 항목이 없어서 넣지 않음):
- 콘솔 변수(CVar), Tab 자동완성, 스크립트 파일 실행, 콘솔 토글 키, 런처 실행 인자로 명령 실행
- 등록 매크로(전역 변수 선언으로 충분하다)
- Core 멀티캐스트 델리게이트 §2-4·5 수정(새 구조에서는 그 경로를 타는 코드가 없다)
- `HGUI::InputText` 512바이트 고정(GUI_TODO 2-9. 콘솔은 2-3의 새 입력 함수를 쓴다)

---

## Phase 0. 결정 — 완료 2026-09-29

- [x] **0-1. 레지스트리 위치** — 결정 2026-09-29: Core 전역 시스템(권장안)
  권장: Core 전역 시스템(`GGlobalSystemInstance<T>`, `CoreSystem.h:147-179`)으로 두고, DevConsole은 입력창과 로그 뷰만 맡는다. 근거: 위 대기 항목 5개 중 3개는 JGConsole(GUI·Graphics 없음)에서, 2개는 런처(Devkit, Graphics 임포터)에서 실행된다. 양쪽이 같은 레지스트리를 봐야 한다.
  대안: DevConsole 모듈 안에 유지 → Runtime 모듈(Devkit)이 Editor 모듈에 의존해야 하고, JGConsole에서는 쓸 수 없다.

- [x] **0-2. 이름** — 결정 2026-09-29: (A) `ConsoleCommand`
  (A) `ConsoleCommand` (권장): `GConsoleCommandGlobalSystem`, `HConsoleCommandArgs`, `HConsoleCommandDesc`, `HConsoleCommandDelegate`, 로그 `[ConsoleCommand]`, 폴더 `Core/ConsoleCommand/`. 위험: 이름이 길다. `Console`이 JGConsole(프로그램)·DevConsole(모듈)과 겹치지만, 셋 다 이 시스템의 입구라 뜻이 어긋나지 않는다.
  (B) `DevCommand`: `GDevCommandGlobalSystem`, `HDevCommandArgs`, `HDevCommandDesc`, `[DevCommand]`. 위험: Devkit 타입(`JGDevScene`, `JGDevFeature`, `JGDevSettings`)과 접두어가 같아서 Devkit 소속처럼 읽힌다. 게임 빌드에서도 쓰면 "Dev"가 틀린 말이 된다.
  전역 변수 등록 타입(0-5)은 언리얼 `FAutoConsoleCommand`에 맞춰 `HAutoConsoleCommand`로 한다.

- [x] **0-3. 명령 문법** — 결정 2026-09-29: 아래 권장안 그대로
  - 이름은 소문자 `영역.동작`(예: `devscene.readback`)이고 대소문자를 구분하지 않는다. 기존 JGConsole 이름 `gmtest`/`simtest`는 그대로 둔다.
  - 인자는 위치 인자(`help gmtest`), 이름 인자 `-name=value`, 플래그 `-name` 세 가지다. 값은 첫 번째 `=` 뒤 전부다. 같은 이름이 두 번 오면 뒤의 값을 쓴다.
  - `-name value`(공백으로 구분) 형식은 받지 않는다. 플래그인지 값인지 구분할 수 없기 때문이다. GameFrameWorks_TODO 1-2의 `-repeat N`은 `-repeat=N`으로 고쳐 적는다(4-1). 음수 위치 인자도 같은 이유로 받지 않는다(`-offset=-1`로 쓴다).
  - `"..."`로 공백이 들어간 값을 묶는다. 백슬래시는 이스케이프 문자가 아니다(Windows 경로를 그대로 쓴다). 따옴표가 닫히지 않으면 오류다.
  - 숫자 변환에 실패하면 핸들러가 false를 돌려주고, 레지스트리가 사용법을 출력한다.

- [x] **0-4. 1차 범위** — 결정 2026-09-29: 권장안 + 입력 히스토리 포함
  포함: 등록·실행·`help`(Phase 1), JGConsole 연결(1-4), 콘솔 창 로그 뷰(3-2), 입력 히스토리 ↑/↓(2-3 (b), 3-3).
  제외 목록은 위 "제외한 것"과 같다.

- [x] **0-5. 등록 방식** — 결정 2026-09-29: 전역 변수 `HAutoConsoleCommand`가 기본
  언리얼처럼 전역 변수로 선언만 한다. 실제 등록은 그 DLL의 모듈이 연결될 때 한다(Source 엔진 `CON_COMMAND`와 같은 방식). 동적인 경우만 `Register` API를 쓴다.
  생성자에서 바로 등록하지 않는 이유는 "명령 등록 방법"의 마지막 문단에 있다.

---

## 명령 등록 방법 (Phase 1~3이 구현할 사용 방식)

명령 하나는 **이름 + 사용법 + 설명 + 핸들러**다. 핸들러는 `bool (const HConsoleCommandArgs&)`이다. 등록하면 `help` 목록에 자동으로 나온다.
등록은 두 가지다. **기본은 전역 변수 `HAutoConsoleCommand` 선언**이다. 핸들러를 특정 객체에 묶어야 하는 경우처럼 동적인 경우에만 `Register` API를 쓴다.

실행 경로도 두 가지이고, **어느 쪽도 다른 프로세스를 띄우지 않는다**. 레지스트리는 Core 전역 시스템이라 프로세스마다 하나씩 따로 있다.

| 경로 | 프로세스 | 실행 방식 | 보이는 명령 |
|---|---|---|---|
| 런처의 DevConsole 창 | 실행 중인 JGLauncher 자신 | 입력한 줄을 `Submit` → 같은 프로세스의 메인 스레드가 프레임 끝에 핸들러 함수를 호출 | `help` + 런처가 연결한 모듈(Devkit, Graphics …)이 선언한 명령 |
| `JGConsole.exe <명령>` | 사람·검증 스크립트가 직접 실행하는 별도 프로그램(지금의 `gmtest`와 같음) | 시작 → 인자로 받은 명령 하나를 `Execute` → 종료 코드를 남기고 끝 | `help` + JGConsole이 선언한 명령(`gmtest`, `simrun` …) + 핸들러가 연결한 모듈의 명령 |

- 런처에서 입력한 명령이 JGConsole.exe를 띄우는 일은 없다. JGConsole.exe가 실행 중인 런처에 명령을 보내는 일도 없다. 밖에서 실행 중인 런처로 명령을 보내는 기능(IPC)은 이 목록에 없다.
- 그래서 `gmtest`는 JGConsole에서만, `devscene.readback`은 런처에서만 보인다.

### 1) 전역 변수로 선언 (기본)

모듈 DLL이든 JGConsole이든, 아무 .cpp의 **파일 범위**에 선언한다. `StartupModule`/`ShutdownModule`은 고치지 않는다.

```cpp
// Devkit 모듈의 아무 .cpp — 예: Graphics_TODO 5-25 리드백 덤프
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

static HAutoConsoleCommand ReadbackCommand(
	"devscene.readback",
	"devscene.readback [-path=<dir>] [-count=N]",
	"Save G-buffer albedo and scene readbacks as PNG",
	[](const HConsoleCommandArgs& InArgs)
	{
		PString Directory = "Temp/Readback";
		InArgs.TryGetString("path", Directory);          // -path가 없으면 기본값을 그대로 둔다

		int32 Count = 1;
		if (InArgs.Has("count") && InArgs.TryGetInt("count", Count) == false)
		{
			return false;                                // 숫자가 아님 → 레지스트리가 Usage를 출력
		}

		// 모듈 상태는 찾아서 쓴다. 명령은 모듈이 연결된 동안만 등록돼 있으므로 null이 아니다.
		HDevKitModule* DevKit = GModuleGlobalSystem::GetInstance().FindModule<HDevKitModule>();
		// ... DevKit(또는 GUI 모듈의 DevScene 위젯)에서 리드백 저장 ...
		JG_LOG(Devkit, ELogLevel::Info, "readback saved: %s", Directory);
		return true;
	});
```

```cpp
// JGConsole — 명령마다 .cpp 하나(예: GameFrameWorks_TODO 1-2의 SimRunCommand.cpp). Main.cpp는 고치지 않는다.
static HAutoConsoleCommand SimRunCommand(
	"simrun",
	"simrun <file> [-repeat=N]",
	"Run a gameplay command script headless",
	[](const HConsoleCommandArgs& InArgs)
	{
		if (InArgs.GetPositionalCount() < 1)
		{
			return false;
		}

		int32 Repeat = 1;
		if (InArgs.Has("repeat") && InArgs.TryGetInt("repeat", Repeat) == false)
		{
			return false;
		}

		if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
		{
			return false;
		}

		return runScript(InArgs.GetPositional(0), Repeat);   // runScript는 예시
	});
```

**언제 등록·해제되나** (1-3이 구현)

| 선언한 곳 | 등록 | 해제 |
|---|---|---|
| 모듈 DLL | 그 모듈의 `StartupModule` 직후(모듈 시스템이 자동으로) | `ShutdownModule` 직전(`DisconnectModule`, 종료 시 모듈 일괄 해제 둘 다) |
| JGConsole·런처 EXE | `GCoreSystem::Create` 안, 레지스트리가 만들어질 때 | 레지스트리 `Destroy`(모듈이 모두 해제된 뒤) |

- DLL이 로드만 되고 모듈로 연결되지 않으면 등록되지 않는다. 예: 런처는 GameFrameWorks DLL을 클래스 목록용으로만 로드한다. 그래서 명령이 보인다는 것은 모듈이 연결됐다는 뜻이다.
- 헤드리스 전용 명령은 JGConsole에 선언하고, 필요한 모듈은 핸들러에서 연결한다(위 `simrun`, 지금의 `gmtest` `Main.cpp:20`). 런처 콘솔에서도 쓸 명령은 모듈에 선언한다.
- 이름이 겹치면 나중 등록이 실패하고 오류 로그가 남는다. 먼저 등록한 쪽이 남는다.

**전역 변수 규칙**
- 핸들러는 **캡처 없는 람다나 정적 함수**만 된다(함수 포인터로 저장한다). 모듈 상태는 위 예처럼 `FindModule<T>()`로 찾는다. 언리얼 `FAutoConsoleCommand`와 같은 방식이다.
- **파일 범위**(`static` 또는 익명 네임스페이스)에만 선언한다. 함수 안의 `static`은 처음 호출될 때 만들어지므로 등록 시점을 놓친다.
- **Core에는 선언하지 않는다.** Core는 정적 라이브러리다. 그래서 그 .obj를 참조하는 곳이 없으면 링커가 빼 버리고, 참조하면 DLL·EXE마다 사본이 생겨 같은 이름이 여러 번 등록된다. Core의 `help`는 레지스트리가 `Register`로 직접 등록한다.

### 2) `Register` API (동적인 경우만)

캡처가 필요하거나 특정 객체에 묶인 핸들러, 조건에 따라 등록하는 명령, 자체 테스트에 쓴다. **등록한 쪽이 직접 해제한다.**

```cpp
// 모듈 객체의 멤버 함수에 묶는 경우
void HDevKitModule::StartupModule()
{
	HConsoleCommandDesc Desc;
	Desc.Name        = "devscene.readback";
	Desc.Usage       = "devscene.readback [-path=<dir>]";
	Desc.Description = "Save G-buffer albedo and scene readbacks as PNG";
	Desc.Handler.BindRaw(this, &HDevKitModule::ExecReadback);
	GConsoleCommandGlobalSystem::GetInstance().Register(Desc);
}

void HDevKitModule::ShutdownModule()
{
	GConsoleCommandGlobalSystem::GetInstance().Unregister("devscene.readback");
}
```

- JGConsole의 `main`에서 `Register`를 쓰면, 등록·실행·해제를 한 함수 안에서 끝낸다. `main`의 지역 엔진 컨테이너(`HConsoleCommandDesc`, `HList`)는 `GCoreSystem::Destroy` 뒤에 해제돼서 크래시하기 때문이다.
- GC 객체(위젯 등)는 등록 주체가 되지 않는다. 소멸이 GC Flush 때라 해제 시점을 보장할 수 없다. 소유 모듈이 등록하고, 핸들러에서 위젯을 찾는다.

### 핸들러 규칙 (두 방식 공통)

| 항목 | 규칙 |
|---|---|
| 반환값 | true면 성공. false면 레지스트리가 `Usage:` 줄을 출력하고, JGConsole 종료 코드가 1이 된다 |
| 인자 읽기 | `GetPositional(i)`(없으면 빈 문자열), `Has(name)`(플래그, 값은 무시), `TryGetString`/`TryGetInt(name, out)`. `TryGet*`은 인자가 없거나 변환에 실패하면 false를 돌려주고 out을 바꾸지 않는다. 그래서 기본값을 먼저 넣고 부르면 된다. "주어졌는데 틀린 값"을 가리려면 위 예처럼 `Has`와 함께 쓴다 |
| 출력 | `JG_LOG(자기 카테고리, ...)`로 쓰면 DevConsole 창(Info 이상), `jg_log.txt`, 표준 출력에 함께 나간다. 사용자 문자열은 `"%s"` 인자로만 넘긴다 |
| 실행 시점 | DevConsole 입력은 `Submit`으로 들어가 다음 `GCoreSystem::Update` 끝(프레임 밖)에 실행된다. 그래서 핸들러 안에서 GPU 리드백·위젯 열기·모듈 연결/해제를 해도 된다. `Execute`(JGConsole, 코드에서 직접 호출)는 즉시 실행하므로, 프레임 안의 코드에서는 `Submit`을 쓴다 |
| 스레드 | 메인 스레드에서 돈다. 오래 걸리는 작업은 그동안 프레임을 멈춘다 |
| 인자 객체 | 핸들러가 끝나면 사라진다. 참조를 보관하지 말고 필요한 값만 복사한다 |

**언리얼처럼 생성자에서 바로 등록하지 않는 이유.**
- 정적 초기화는 `LoadDll` 도중에 돈다. 그런데 모듈 DLL의 Core 사본은 그 뒤 `Link_Module`에서야 연결되므로(`Module.cpp:98 → 113 → 147`), 생성자 시점에는 `GetInstance()`가 null이다. 언리얼은 Core가 공유 DLL이고 콘솔 매니저를 처음 호출할 때 만들어서 이 문제가 없다.
- 전역 변수가 엔진 컨테이너·델리게이트를 들고 있으면, 풀이 해제된 뒤 DLL 언로드 때 소멸자가 크래시한다.
- 그래서 `HAutoConsoleCommand`는 포인터만 들고 자기 DLL의 목록에 이어지기만 한다. 등록은 모듈 연결 때 한다.

---

## Phase 1. Core 레지스트리 · 파서 · 전역 변수 등록 · JGConsole (헤드리스로 끝까지 검증)

- [ ] **1-1. 인자 파서 `HConsoleCommandArgs`** — 새 파일 `Source/Runtime/Core/ConsoleCommand/ConsoleCommandArgs.h/.cpp`(JGBuildTool 재실행)
  `static bool Tokenize(const PString& InLine, HList<PString>& OutTokens, PString& OutError)`: 공백으로 나누고 `"..."`를 묶는다. 닫히지 않은 따옴표는 false와 오류 문구를 돌려준다.
  토큰 목록으로 만든다. 첫 토큰이 이름(소문자화)이다. `-`로 시작하는 토큰은 이름 인자로, 첫 `=` 기준으로 이름(소문자화)과 값을 나눈다. 나머지는 위치 인자다.
  조회 함수: `GetName`, `GetPositionalCount`, `GetPositional(i)`(범위 밖이면 빈 문자열), `Has(name)`, `TryGetString(name, out)`, `TryGetInt(name, out)`. `TryGetInt`는 `std::from_chars`로 변환하고, 문자열 전체를 소비했는지 확인한다(예외 없음). `TryGet*`은 실패하면 out을 바꾸지 않는다(기본값 패턴, "핸들러 규칙").
  옛 파서(`DevConsoleDefines.h:12-68`)의 결함을 모두 없앤다: 위치 인자를 버림, 앞 공백이면 명령이 빈 문자열, 따옴표 없음, 두 번째 `=` 뒤를 자름, 대소문자 구분(현황분석 §2-6).
  완료 조건: 1-5 자체 테스트의 파서 케이스 통과.

- [ ] **1-2. 레지스트리 `GConsoleCommandGlobalSystem`** — 새 파일 `Core/ConsoleCommand/ConsoleCommandGlobalSystem.h/.cpp`, `CoreSystem.cpp:35-41, 99-105`
  등록 정보 `HConsoleCommandDesc { Name, Usage, Description, Handler }`. `Handler`는 `JG_DECLARE_DELEGATE_RET(HConsoleCommandDelegate, bool, const HConsoleCommandArgs&)`이다.
  `bool Register(const HConsoleCommandDesc&)`: 이름을 소문자로 바꾼다. 빈 이름·중복 이름이면 오류 로그를 남기고 false. `bool Unregister(const PString& InName)`, `bool IsRegistered(const PString& InName) const`.
  `bool Execute(const PString& InLine)`와 토큰 목록 오버로드 `Execute(const HList<PString>&)`(1-4용):
  - 빈 줄은 무시한다.
  - 에코 로그 `> <줄>`을 남긴다.
  - 없는 이름이면 "Unknown command ... (type help)"를 남기고 false.
  - 핸들러의 **복사본**으로 실행한다. 핸들러 안에서 자기 자신을 `Unregister`해도 안전하게 하려는 것이다(옛 멀티캐스트 §2-5 문제의 재발 방지).
  - 핸들러가 false를 돌려주면 사용법 로그를 남긴다.
  `void Submit(const PString& InLine)`: 줄을 큐에 넣고 `Update()`에서 실행한다. `Update()`는 공개 함수 `ExecutePending()`을 부르고, JGConsole 자체 테스트는 이 함수를 직접 부른다(JGConsole은 `GCoreSystem::Update`를 돌리지 않는다). 큐는 로컬로 옮긴 뒤 비우므로, 실행 중에 `Submit`된 줄은 다음 프레임에 실행된다. `GCoreSystem::Update`는 스케줄러의 모든 버킷(GraphicsBegin~UpdateWindow) 뒤에 이 시스템을 부르므로, 명령은 **프레임 밖**에서 돈다. 이유는 두 가지다. GUI 위젯 순회 중에 실행되지 않고(GUI_TODO 2-6 위험), 리드백 명령이 "ReadbackTextureImmediate called inside a frame" 스톨 경고를 내지 않는다(`ResourceStagingManager.cpp:412-416`, 5-25 대상).
  등록 순서: `GStringTable` 다음, `GModuleGlobalSystem` 앞(`CoreSystem.cpp:39-40` 사이). 그러면 Update는 스케줄러 뒤, Destroy는 모듈 해제 뒤에 온다. `GCoreSystem::Destroy`의 `UnRegisterSystemInstance` 목록에서는 `GMemoryGlobalSystem`보다 앞에 둔다(엔진 컨테이너 보유).
  `Destroy()`: 남은 등록이 있으면 이름마다 경고 한 줄("not unregistered by its owner")을 남기고 비운다. 핸들러가 잡은 `PSharedPtr`가 종료 GC보다 늦게 풀려 힙을 깨는 경로를 막는다(같은 종류의 종료 크래시: `Document/2026-09-17_종료크래시_ResourceStateTracker_분석.md`, Graphics_TODO 4-6의 `GAssetDatabase` 해제).
  내장 명령 `help [name]`: 인자가 없으면 이름순 목록(이름 + 설명), 있으면 그 명령의 사용법과 설명을 출력한다. 레지스트리 생성자에서 `Register`로 등록하고 `Destroy`에서 해제한다(남은 등록 경고 대상 아님).
  완료 조건: 1-5 자체 테스트 통과, 런처 종료 시 남은 명령 경고 0.

- [ ] **1-3. 전역 변수 등록 `HAutoConsoleCommand`와 모듈 훅** — `Core/ConsoleCommand/ConsoleCommandGlobalSystem.h/.cpp`, `Core/Misc/Module.h:19-29`, `Module.cpp:147, 208, 250`
  클래스: 생성자 `(const char* InName, const char* InUsage, const char* InDescription, bool (*InHandler)(const HConsoleCommandArgs&))`.
  - 멤버는 포인터 넷과 다음 노드 포인터뿐이다.
  - 생성자는 이 바이너리의 목록 머리(`static HAutoConsoleCommand* Head`)에 자기를 잇기만 한다. 엔진 풀·레지스트리·로그는 부르지 않는다. `Head`는 상수 초기화라 정적 초기화 순서 문제가 없다.
  - 소멸자는 비어 있다(DLL 언로드 때 안전).
  `static void RegisterAll()` / `UnregisterAll()`: 이 바이너리의 목록을 레지스트리에 `Register`/`Unregister`한다. Core는 정적 라이브러리라 DLL·EXE마다 `Head`가 따로 있으므로, 각 바이너리는 자기 목록만 다룬다.
  **모듈 훅**: `IModuleInterface`에 비순수 가상 함수 `RegisterAutoConsoleCommands()` / `UnregisterAutoConsoleCommands()`를 `protected`로 둔다(기본 구현이 `RegisterAll`/`UnregisterAll`). 모듈 클래스는 자기 DLL 안에서 만들어지므로(`_Create_Module_Interface_`) 가상 호출은 그 DLL의 사본으로 가고, 그 DLL의 목록을 본다. `JG_MODULE_IMPL`은 바꾸지 않는다.
  `GModuleGlobalSystem` 호출 위치: 등록은 `StartupModule` 직후(`Module.cpp:147`), 해제는 `ShutdownModule` 직전 두 곳(`:208` `DisconnectModule`, `:250` `Destroy`).
  **EXE 목록**: 레지스트리 생성자가 `RegisterAll()`을 부르고, 레지스트리 `Destroy`가 남은 등록 검사 전에 `UnregisterAll()`을 부른다. 레지스트리 객체는 EXE의 `GCoreSystem::Create`가 만들므로 EXE의 사본이 불린다.
  `IModuleInterface`의 vtable이 바뀌므로 모든 모듈을 다시 빌드한다(Core 변경이라 어차피 전체 빌드).
  완료 조건: 1-5 자체 테스트의 전역 변수 케이스 통과, 1-6의 모듈 훅 확인.

- [ ] **1-4. JGConsole을 레지스트리로 전환** — `Source/Programs/JGConsole/Main.cpp:14-51`
  `gmtest`와 별칭 `simtest`를 `Main.cpp` 파일 범위의 `HAutoConsoleCommand`로 선언한다(핸들러는 지금의 `runGameMasterSelfTest() == 0`).
  `main`은 `GCoreSystem::Create` → `runCommandLine(argc, argv)` → `GCoreSystem::Destroy`만 한다. `runCommandLine`은 다음을 하고, 지역 컨테이너가 `Destroy` 전에 해제되도록 함수로 분리한다.
  - `argv[1..]`을 토큰 목록 그대로 `Execute`에 넘긴다. CRT가 이미 따옴표를 풀었으므로 줄로 다시 합치지 않는다.
  - 인자가 없으면 `help`를 실행한다.
  앞으로 JGConsole 명령(`simrun`, `modtest`, `nethost` …)은 명령마다 .cpp 파일 하나로 추가한다. 여러 트랙이 동시에 `Main.cpp`를 고치지 않게 된다.
  종료 코드: 성공 0, 실패·알 수 없는 명령 1. **지금은 알 수 없는 명령도 종료 코드 0이라** 검증 스크립트가 오타(`gmtset`)를 통과로 본다(`Main.cpp:43-47`). 동작 변화: `gmtest`의 모듈 연결 실패는 2에서 1이 된다.
  완료 조건: `JGConsole.exe gmtest` 75/75 · 종료 0(출력 형식 동일), `JGConsole.exe simtest` 동일, `JGConsole.exe gmtset` 종료 1, `JGConsole.exe` → help 목록.

- [ ] **1-5. 자체 테스트 `console.selftest`** — 새 파일 `Source/Programs/JGConsole/ConsoleCommandSelfTest.h/.cpp`(Core에 테스트 코드를 넣지 않는다. GameFrameWorks `gmtest`와 같은 방식)
  `console.selftest` 자체를 `HAutoConsoleCommand`로 선언한다. 실행된다는 것이 곧 EXE 목록 등록의 확인이다.
  케이스:
  - 파서: 따옴표 값 / 닫히지 않은 따옴표 / 값 안의 `=` / 플래그 / 대소문자 / 앞뒤·연속 공백 / 같은 이름 두 번 / `TryGetInt` 실패(`abc`, `12x`, 범위 초과)
  - 레지스트리(`Register` API): 중복 등록 거부 / 알 수 없는 명령이면 false / 핸들러 안에서 자기 `Unregister` 후 다음 `Execute`에서 크래시 없음 / `Submit`한 줄은 `ExecutePending` 전에는 실행 안 됨
  - 전역 변수: `IsRegistered("gmtest")`가 true / 선언한 명령과 같은 이름을 `Register`하면 거부
  - `help %s %n` 실행이 크래시 없이 끝남(에코 줄 내용 확인은 2-2에서 추가)
  출력: `console.selftest: OK (N/N)` 또는 실패 케이스 이름.
  완료 조건: 종료 코드 0.

- [ ] **1-6. Phase 1 검증** — 공통 검증 루프.
  - 헤드리스 전부.
  - 모듈 훅: `GameFrameWorksModule.cpp`에 임시 전역 명령 `test.autocmd`를 하나 넣는다(새 파일을 만들지 않아 프로젝트 재생성이 없다). `console.selftest` 임시 케이스로 GameFrameWorks 연결 전 없음 → 연결 후 있음·실행됨 → `DisconnectModule` 후 없음을 확인한다. 확인 후 임시 코드를 제거하고 `git diff`로 확인한다. GameFrameWorks 트랙의 미커밋 변경과 섞이지 않게, 착수 전 `git status`를 본다.
  - 런처 60초 회귀: DevConsole은 아직 옛 코드라 동작 변화 없음. 종료 코드 0, 남은 명령 경고 0.

- [ ] **1-7. 커밋** — "Core 콘솔 명령 레지스트리와 전역 변수 등록 추가, JGConsole 명령 디스패치 전환" (사용자가 직접 커밋). Core/ConsoleCommand, Core/Misc/Module.h/.cpp, CoreSystem.cpp, JGConsole.

---

## Phase 2. 출력 기반 (GUI 래퍼 · 최근 로그 버퍼)

- [ ] **2-1. `HGUI::Text`가 문자열을 printf 서식으로 해석하는 버그** — `Source/Runtime/GUI/GUI.cpp:49-57`(라벨 `:64`도 같음)
  `ImGui::Text(InStr.GetCStr())`와 `ImGui::TextColored(color, InStr.GetCStr())`가 문자열을 서식으로 쓴다. `TextUnformatted`(색 있는 쪽은 `PushStyleColor`/`PopStyleColor`)로 바꾼다.
  지금 이미 틀리게 그려지고 있다: 메모리 통계 창의 한 줄(`MemoryStatistics.cpp:135`, `"%5.1f%%   %7u"`)이 1차 서식에서 `12.5%   123`이 되고, `HGUI::Text`에서 `%   123`이 다시 변환 지정자로 먹힌다. 그래서 usage 값에 `%`가 없고 peak 열이 전부 비어 있다(`Document/Memory/2026-09-28_phase2_memwidget_capture.png`).
  콘솔 로그 뷰는 사용자가 친 문자열을 보여 주므로, `%s`가 들어가면 빈 가변 인자를 읽어 크래시할 수 있다.
  완료 조건: 메모리 통계 창에 peak 값과 `%`가 보인다(캡처).

- [ ] **2-2. 최근 로그 버퍼** — `Source/Runtime/Core/Misc/Log.h/.cpp`(`Log.cpp:20-22` 싱크 추가 자리)
  Info 이상 최근 1024줄을 레벨과 함께 보관하는 싱크를 추가한다. `spdlog::sinks::base_sink<std::mutex>` 파생이고 저장소는 std 컨테이너다. spdlog 1.10의 `ringbuffer_sink`는 `final`이라 변경 카운터를 붙일 수 없다.
  `GLogGlobalSystem`에 두 함수를 둔다: `uint64 GetLogSerial()`(줄이 추가될 때마다 증가), `void GetRecentLogs(HList<HLogLine>& OutLines)`(레벨 + 문자열). 콘솔 창은 serial이 바뀐 프레임에만 복사한다.
  싱크 안에서는 엔진 풀·`JG_LOG`를 쓰지 않는다(풀의 로그 재진입 가드 `MemoryPool.cpp:36-55`와 같은 이유).
  일반 실행(추적 빌드 아님)의 Info 이상 줄 수는 적다. 2026-09-29 15:15 `jg_log.txt` 179줄 중 info 52 · warning 2줄이었다.
  완료 조건: 1-5 자체 테스트에 두 케이스를 추가하고 통과: "로그 한 줄 → `GetRecentLogs` 마지막 줄 일치, serial 증가", "`help %s %n`의 에코 줄이 글자 그대로 남음".

- [ ] **2-3. HGUI 래퍼 추가** — `Source/Runtime/GUI/GUI.h/.cpp`
  ImGui는 GUI.dll 밖으로 노출되지 않는다(`imconfig.h:26-27`의 `IMGUI_API`가 주석 처리됨). 그래서 DevConsole은 `HGUI`만 쓸 수 있다. GUI_TODO 3-7의 권장안(즉시 모드 함수는 `HGUI` 정적 함수)대로 두 가지를 추가한다.
  (a) `BeginChild(name, size)` / `EndChild()`: 스크롤 영역. 스크롤이 맨 아래에 있었으면 새 줄이 생겨도 맨 아래를 유지한다.
  (b) `InputTextWithHistory(name, InOutStr, history, InOutHistoryPos)`: Enter면 true를 돌려주고, 입력 뒤에도 포커스를 유지한다(ImGui 기본 `ConfigInputTextEnterKeepActive = false`라 지금은 Enter마다 포커스가 풀린다). ↑/↓로 history를 오간다. 버퍼 길이는 고정하지 않는다(`ImGuiInputTextFlags_CallbackResize`).
  완료 조건: 빌드 통과, 3-2/3-3에서 사용.

- [ ] **2-4. Phase 2 검증** — 공통 검증 루프 + 2-1 캡처(메모리 통계 창 peak 열).

- [ ] **2-5. 커밋** — "HGUI::Text 서식 해석 수정, 최근 로그 버퍼, HGUI 스크롤·입력 래퍼" (사용자가 직접 커밋). Core(Log), GUI.

---

## Phase 3. DevConsole 창

- [ ] **3-1. 옛 경로 삭제** — `DevConsoleDefines.h:12-68`, `DevConsole.h:11, 21, 32-33`, `DevConsole.cpp:37-45`, `DevConsoleModule.h:20-24`, `DevConsoleModule.cpp:43-75`
  `HDevConsoleArguments`, `HOnDevConsole`, `Register/UnRegisterConsoleCommand`, `GetCheckedGUIModule`를 지운다. 호출자는 0곳이다. `DevConsoleDefines.h`에는 API 매크로만 남긴다.
  **GUI_TODO 1-6은 이 항목으로 대체된다**(고칠 코드가 없어진다). 같이 고칠 것: `DevConsoleModule.cpp:23` 로그 카테고리 `DevStatistics` → `DevConsole`, 메시지 "DevConsoleModule Need GUI Module"(GUI_TODO 1-9의 DevConsole 부분).
  완료 조건: 빌드 통과, `grep -rn "HDevConsoleArguments\|HOnDevConsole\|RegisterConsoleCommand" Source` 0건. 1-3의 `RegisterAutoConsoleCommands`는 이름이 달라 걸리지 않는다.

- [ ] **3-2. 콘솔 창** — `Source/Editor/DevConsole/DevConsole.h/.cpp`(JGCLASS 유지. 반사 멤버가 없으면 JGHeaderTool 재실행 불필요)
  위쪽: 로그 뷰. `HGUI::BeginChild`로 입력줄 높이를 뺀 영역에 레벨별 색으로 줄을 그린다(warning 노랑, error·critical 빨강). 로그 줄은 `GetLogSerial`이 바뀐 프레임에만 다시 복사한다.
  아래쪽: 입력줄 `HGUI::InputTextWithHistory`. Enter면 `GConsoleCommandGlobalSystem::Submit(줄)`을 부르고 입력을 비운다.
  완료 조건: 3-4의 창 확인.

- [ ] **3-3. 입력 히스토리** — `DevConsole.h/.cpp`
  최근 64줄을 보관한다. 직전 줄과 같은 줄은 다시 넣지 않는다. 창을 닫았다 열어도 유지한다(위젯 객체는 닫아도 남는다, `GUIModule.h:35-47`). 실행 사이에는 유지하지 않는다.
  완료 조건: 3-4의 ↑ 확인.

- [ ] **3-4. Phase 3 검증** — 공통 검증 루프 + 런처에서 `Windows/DevConsole`을 열고 확인한다.
  확인할 것:
  - `help` → 로그 뷰에 목록이 나온다.
  - `nosuchcmd` → 빨간 오류 줄.
  - `help %s %n` → 크래시 없이 글자 그대로 보인다.
  - ↑ → 직전 명령이 복원된다.
  - Enter 뒤에도 입력줄 포커스가 유지된다.
  모듈 경로(런처): `DevkitModule.cpp`에 임시 전역 명령 `test.echo`를 선언한다. `StartupModule`/`ShutdownModule`은 고치지 않는다. 콘솔에서 실행해 출력을 확인하고, 종료 시 남은 명령 경고가 0인지 본다(자동 해제). 임시 코드는 확인 후 제거하고 `git diff`로 확인한다.
  캡처 1장을 `Document/Memory/`에 남긴다.

- [ ] **3-5. 커밋** — "DevConsole을 콘솔 명령 레지스트리 UI로 재작성" (사용자가 직접 커밋). DevConsole 모듈 전체.

---

## Phase 4. 연결 · 문서

- [ ] **4-1. 대기 항목에 사용 방법 연결**
  이 문서 "명령 등록 방법"의 전역 변수 예시(줄여서)와 문법 요약을 `ConsoleCommandGlobalSystem.h` 상단 주석으로 옮긴다. 구현하면서 API가 바뀌었으면 이 문서의 절도 같이 고친다.
  대기 항목 5곳의 문구에 "명령은 `HAutoConsoleCommand` 전역 변수로 선언(`ConsoleCommandGlobalSystem.h` 상단 예시)"을 한 줄 추가한다: Graphics_TODO 5-25·5-31, GameFrameWorks_TODO 1-2(`-repeat N` → `-repeat=N`), 게임모듈 R8, 리슨서버 4단계. JGConsole 명령은 명령마다 .cpp 하나로 추가한다는 점도 적는다.
  5-25는 자동 검증 루프가 리드백 PNG를 쓰므로, 명령으로 바꿀 때 실행 인자 경로가 필요하다는 점도 적는다(이 목록에서는 제외).
  완료 조건: 각 문서에 반영.

- [ ] **4-2. 기록 정리** — `DevConsole_현황분석_2026-09-29.md` 상단에 "이 목록으로 해결됨" 요약 한 줄, GUI_TODO 1-6에 완료(대체) 표시, Memory 파일 진행 기록.
