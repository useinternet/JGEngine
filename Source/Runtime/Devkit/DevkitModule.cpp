#include "PCH/PCH.h"
#include "DevkitModule.h"

JG_MODULE_IMPL(HDevKitModule, DEVKIT_C_API)

HDevKitModule::~HDevKitModule()
{
}

JGType HDevKitModule::GetModuleType() const
{
	return JGTYPE(HDevKitModule);
}

void HDevKitModule::StartupModule()
{

}

void HDevKitModule::ShutdownModule()
{

}
