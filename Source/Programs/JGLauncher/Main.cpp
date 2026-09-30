#include "PCH/PCH.h"
#include "Core.h"

using namespace std;

int main()
{
	// 에디터 호스트. 게임 프로젝트의 Bin 에서 실행하면 JGEditor 가 .jgproject 의 게임 · 에디터 모듈까지 올린다.
	HCoreSystemArguments args;
	args.LaunchModule = "JGEditor";

	GCoreSystem::Create(args);

	while (GCoreSystem::Update()) {}

	GCoreSystem::Destroy();
	return 0;
}
