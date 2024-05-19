#include "PCH/PCH.h"
#include "Core.h"

using namespace std;

int main()
{
	HCoreSystemArguments args;
	args.LaunchModule = "JGDev_Graphics";

	GCoreSystem::Create(args);

	while (GCoreSystem::Update()) {}

	GCoreSystem::Destroy();
	return 0;
}


