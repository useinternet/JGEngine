#include "PCH/PCH.h"
#include "Core.h"

using namespace std;

namespace
{
	bool hasArgument(int argc, char** argv, const char* argument)
	{
		for (int i = 1; i < argc; ++i)
		{
			if (std::strcmp(argv[i], argument) == 0)
			{
				return true;
			}
		}

		return false;
	}
}

int main(int argc, char** argv)
{
	// 창을 만들기 전에 모니터별 DPI 인식으로(D-22 ②): 에디터 · -game 창을 실제 픽셀로 그린다(Windows 가 늘려 흐리게 하지 않음).
	HPlatform::EnableDpiAwareness();

	// 에디터 호스트. 게임 프로젝트의 Bin 에서 실행하면 JGEditor 가 .jgproject 의 게임 · 에디터 모듈까지 올린다.
	// -game: 에디터 없이 게임만(DevOps GP-D3 · D5) — GameHost 가 창 · 게임 화면을 맡고, codeGen 은 런타임 모듈만 올린다
	//        (JGBuildTool 이 쓴 Temp/CodeGen/runtime_module_info.json). 개발 중 패키지 없이 게임 화면을 볼 때 쓴다.
	HCoreSystemArguments args;
	args.LaunchModule = "JGEditor";
	if (hasArgument(argc, argv, "-game") == true)
	{
		args.LaunchModule = "GameHost";
		args.Flags = args.Flags | ECoreSystemFlags::RuntimeModulesOnly;
	}

	GCoreSystem::Create(args);

	while (GCoreSystem::Update()) {}

	GCoreSystem::Destroy();
	return 0;
}
