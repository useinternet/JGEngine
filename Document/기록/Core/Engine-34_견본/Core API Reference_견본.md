---
팀: JGEngine
분류: 레퍼런스
모듈: Core
갱신: 2026-10-24
---
# Core API Reference

Core 모듈의 공개 C++ API다. 게임 모듈은 `Core.h`와 아래 헤더를 include해서 쓴다. 구조와 흐름은 [[00. Core]]가 설명한다.

| 영역 | 항목 |
|---|---|
| 콘솔 명령 | [HAutoConsoleCommand](#hautoconsolecommand-자동-등록-콘솔-명령) · [HConsoleCommandArgs](#hconsolecommandargs-콘솔-명령-인자) |
| 모듈 · 시작 | (작성 예정) GCoreSystem · IModuleInterface · JG_MODULE_IMPL · GModuleGlobalSystem |
| 메모리 | (작성 예정) Allocate · PSharedPtr · PWeakPtr · IMemoryObject |
| 리플렉션 · 직렬화 | (작성 예정) JGObject · JGCLASS · JGPROPERTY · StaticClass · SaveObject · LoadObject |
| 그 밖 | (작성 예정) 스케줄러 · 델리게이트 · 파일 · 로그 · 입력 · 수학 · 문자열 |

## 콘솔 명령

### HAutoConsoleCommand (자동 등록 콘솔 명령)

**개요** — 파일 범위 전역 변수로 선언하는 콘솔 명령이다. 선언한 모듈이 연결되면 등록되고, 모듈이 내려가면 해제된다. 에디터 DevConsole 창과 명령줄 도구 JGConsole이 같은 명령을 실행하므로, 게임 모듈의 시험 · 점검 명령을 이것으로 만든다.

**선언** — `#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"`. 핸들러에서 `JG_LOG`를 쓰려면 `Core.h`도 include한다(이 헤더는 로그를 포함하지 않는다).

```cpp
class HAutoConsoleCommand
{
public:
	using HandlerFunction = bool (*)(const HConsoleCommandArgs&);

	HAutoConsoleCommand(const char* InName, const char* InUsage, const char* InDescription, HandlerFunction InHandler);
};
```

**매개변수 / 반환값**

| 이름 | 형 | 뜻 |
|---|---|---|
| `InName` | `const char*` | 명령 이름. 소문자 `영역.동작`(예: `audio.selftest`). 대소문자를 가리지 않는다 |
| `InUsage` | `const char*` | 사용법 한 줄. `help`와 실패 때 출력한다 |
| `InDescription` | `const char*` | `help` 목록의 한 줄 설명(영어) |
| `InHandler` | `HandlerFunction` | 명령 본문. 캡처 없는 람다나 정적 함수 |
| 핸들러 반환값 | `bool` | `true` 성공. `false` 실패 — 레지스트리가 사용법을 출력하고 JGConsole은 종료 코드 1 |

**사용 예시** — 2026-10-24 JGConsole에서 컴파일 · 실행 확인.

```cpp
#include "PCH/PCH.h"
#include "Core.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

// 게임 모듈의 아무 .cpp 파일 범위에 둔다. 모듈이 연결되면 등록되고, 모듈이 내려가면 해제된다.
static HAutoConsoleCommand GreetCommand(
	"sample.greet",
	"sample.greet <name> [-count=N]",
	"Print a greeting N times",
	[](const HConsoleCommandArgs& InArgs)
	{
		if (InArgs.GetPositionalCount() < 1)
		{
			return false;   // false 를 돌려주면 레지스트리가 사용법(두 번째 인자)을 출력한다
		}

		int32 count = 1;
		if (InArgs.Has("count") && InArgs.TryGetInt("count", count) == false)
		{
			return false;
		}

		for (int32 i = 0; i < count; ++i)
		{
			JG_LOG(Sample, ELogLevel::Info, "Hello, %s", InArgs.GetPositional(0));
		}
		return true;
	});
```

| 실행 | 결과 |
|---|---|
| `JGConsole.exe sample.greet World -count=2` | `Hello, World` 두 줄, 종료 코드 0 |
| `JGConsole.exe sample.greet` | `'sample.greet' failed. Usage: sample.greet <name> [-count=N]`, 종료 코드 1 |
| `JGConsole.exe help sample.greet` | `Usage: sample.greet <name> [-count=N]` |

게임 모듈에 선언한 명령은 게임 프로젝트 Bin의 `JGConsole.exe -game <명령>`으로 실행한다.

**주의사항**

| 항목 | 내용 |
|---|---|
| 스레드 | 생성은 정적 초기화(DLL 로드) 중에 일어난다. 등록 · 실행 · 해제는 메인 스레드에서 한다 |
| 소유권 · 수명 | 정적 저장 기간 객체여야 한다. 소멸자가 목록에서 빠지지 않으므로 지역 변수나 동적 할당으로 만들면 안 된다 |
| 호출 순서 | 생성자 안에서는 로그 · 메모리 · 레지스트리를 쓸 수 없다. 등록은 그 모듈의 `StartupModule`이 끝난 직후, 해제는 `ShutdownModule` 직전에 엔진이 한다 |
| 에러 처리 | 이름이 비었거나 빈칸 · 따옴표가 있거나 이미 등록된 이름이면 오류 로그만 남기고 등록하지 않는다. 먼저 등록된 명령이 남는다 |

관련: [HConsoleCommandArgs](#hconsolecommandargs-콘솔-명령-인자)

### HConsoleCommandArgs (콘솔 명령 인자)

**개요** — 콘솔 명령 한 줄을 나눈 인자다. `HAutoConsoleCommand`의 핸들러가 받는다. 문법은 `<이름> <위치 인자>... -플래그 -키=값 "따옴표 값"`이다.

**선언** — `#include "ConsoleCommand/ConsoleCommandArgs.h"`(`ConsoleCommandGlobalSystem.h`가 포함한다).

```cpp
class HConsoleCommandArgs
{
public:
	const PString& GetName() const;
	uint32  GetPositionalCount() const;
	PString GetPositional(uint32 InIndex) const;
	bool Has(const PString& InName) const;
	bool TryGetString(const PString& InName, PString& OutValue) const;
	bool TryGetInt(const PString& InName, int32& OutValue) const;
};
```

**매개변수 / 반환값**

| 함수 | 매개변수 | 반환값 |
|---|---|---|
| `GetName` | — | 명령 이름(소문자) |
| `GetPositionalCount` | — | 위치 인자 수. 이름과 `-키` 인자는 세지 않는다 |
| `GetPositional` | `InIndex` 0부터 | 그 인자. 범위 밖이면 빈 문자열 |
| `Has` | `InName` `-` 없이, 대소문자 무시 | `-키` 또는 `-키=값`이 있으면 `true`. 값은 보지 않는다 |
| `TryGetString` | `InName`, `OutValue` | 있으면 `true`와 값. 없으면 `false`, `OutValue`는 그대로 |
| `TryGetInt` | `InName`, `OutValue` | 문자열 전체가 정수면 `true`와 값. 없거나 정수가 아니면(`12x`) `false`, `OutValue`는 그대로 |

**사용 예시** — [HAutoConsoleCommand](#hautoconsolecommand-자동-등록-콘솔-명령)의 예시. 기본값을 먼저 넣고 `TryGetInt`를 부르며, "주어졌는데 틀린 값"은 `Has`와 함께 가린다.

**주의사항**

| 항목 | 내용 |
|---|---|
| 스레드 | 핸들러 안에서 메인 스레드로만 쓴다 |
| 소유권 · 수명 | 핸들러가 도는 동안만 유효하다. 값을 남기려면 `PString`으로 복사한다 |
| 호출 순서 | 제약 없음 |
| 에러 처리 | 실패는 `false` 반환으로만 알린다(예외 없음). `"-키 값"`(빈칸 구분)은 받지 않고, `-`로 시작하는 토큰은 모두 이름 인자다. 음수 값은 `-offset=-1`처럼 쓴다 |

## 변경 이력

| 날짜 | 변경된 API | 변경 내용 | 게임 코드 영향 | 일감 |
|---|---|---|---|---|
| 2026-10-24 | HAutoConsoleCommand · HConsoleCommandArgs | 처음 씀(견본) | 없음 | Engine-34 |
