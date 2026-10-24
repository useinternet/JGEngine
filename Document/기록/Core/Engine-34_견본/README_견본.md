# Core

엔진의 가장 아래 계층인 정적 라이브러리다. 모든 엔진 모듈 · 게임 모듈 · 엔진 프로그램이 링크한다.

> 견본(Engine-34). 계획 확인 뒤 `Source/Runtime/Core/README.md` 로 둔다. 영역 줄 · 주요 클래스 표는 Engine-38 에서 모두 채운다.

## 역할

- 엔진 시작 · 프레임 · 종료(`GCoreSystem`)와 프레임 단계 스케줄러(`GScheduleGlobalSystem`)
- 모듈 DLL 연결(`IModuleInterface` · `GModuleGlobalSystem`)
- 메모리 할당 · 스마트 포인터 · GC(`Allocate` · `PSharedPtr` · `PWeakPtr`)
- 리플렉션 · JSON 직렬화(`JGCLASS` · `JGPROPERTY` · `JGObject`)
- 콘솔 명령 · 델리게이트 · 파일 · 로그 · 창 · 입력 · 수학 · 문자열

## 의존 모듈

없음. 외부 라이브러리는 spdlog(로그) · rapidjson(JSON)이다. 정적 라이브러리라 모듈 DLL마다 사본이 링크되지만, 모듈 연결 때 실행 파일의 `GCoreSystem` 하나에 이어진다.

## 주요 클래스

| 클래스 | 헤더 | 하는 일 |
|---|---|---|
| `GCoreSystem` | `CoreSystem.h` | 전역 시스템을 등록 순서대로 보관하고 시작 · 프레임 · 종료를 진행 |
| `IModuleInterface` · `JG_MODULE_IMPL` | `Misc/Module.h` | 모듈 DLL 하나의 진입점 |
| `PSharedPtr` · `PWeakPtr` · `Allocate` | `Memory/Memory.h` | 엔진 객체 할당과 참조 |
| `JGObject` · `JGCLASS` · `JGPROPERTY` | `Object/JGObject.h` · `Object/ObjectDefines.h` | 리플렉션 객체와 표시 매크로 |
| `GScheduleGlobalSystem` | `Thread/Scheduler.h` | 프레임 단계 · 시간 예약, 작업 스레드 |
| `HDelegate` · `HMulticastDelegate` | `Misc/Delegate.h` | 함수 보관 · 이벤트 구독 |
| `HAutoConsoleCommand` | `ConsoleCommand/ConsoleCommandGlobalSystem.h` | 모듈 연결 때 자동 등록되는 콘솔 명령 |
| `HFileHelper` | `FileIO/FileHelper.h` | 파일 읽기 · 쓰기, 엔진 · 프로젝트 디렉터리 |
| `JG_LOG` | `Misc/Log.h` | 로그 |

`Core.h` 하나가 콘솔 명령 · 모듈 헤더를 뺀 대부분을 포함한다.

## 사용 예시

게임 모듈에 콘솔 명령 하나를 더한다. 모듈이 연결되면 에디터 DevConsole 창과 `JGConsole.exe -game sample.greet World`로 실행된다.

```cpp
#include "PCH/PCH.h"
#include "Core.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

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

## 문서

- API 세부 사용법: 엔진 위키「Core API Reference」
- 구조 · 흐름 설명: 엔진 위키「00. Core」
