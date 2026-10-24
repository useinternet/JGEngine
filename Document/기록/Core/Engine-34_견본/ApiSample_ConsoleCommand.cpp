// Engine-34 견본 — Core API Reference「HAutoConsoleCommand」· 「HConsoleCommandArgs」사용 예시 원본.
// Reference 와 README 의 예시는 이 파일에서 그대로 옮긴다. 컴파일 · 실행 확인: Temp/wt/engine34 의 JGConsole 에 넣어
//   JGConsole.exe sample.greet World -count=2   → Hello, World 두 줄, 종료 코드 0
//   JGConsole.exe sample.greet                  → 사용법 경고, 종료 코드 1
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
