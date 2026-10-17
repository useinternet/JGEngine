#include "PCH/PCH.h"
#include "AudioModule.h"
#include "AudioGlobalSystem.h"

JG_MODULE_IMPL(HAudioModule, AUDIO_C_API)

JGType HAudioModule::GetModuleType() const
{
	return JGTYPE(HAudioModule);
}

void HAudioModule::StartupModule()
{
	JG_LOG(Audio, ELogLevel::Trace, "Startup Audio Module...");

	// Create 뒤에 연결해도(JGConsole) 바로 쓰이도록 Start() 를 기다리지 않고 여기서 시작한다
	GCoreSystem::RegisterSystemInstance<GAudioGlobalSystem>();
	GAudioGlobalSystem::GetInstance().Initialize();
}

void HAudioModule::ShutdownModule()
{
	// StartupModule 에서 등록한 짝. 엔진 종료에서는 시스템 Destroy 가 먼저 장치 · 소리를 닫았다.
	if (GAudioGlobalSystem::IsValid() == true)
	{
		GAudioGlobalSystem::GetInstance().Shutdown();
	}
	GCoreSystem::UnRegisterSystemInstance<GAudioGlobalSystem>();

	JG_LOG(Audio, ELogLevel::Trace, "Shutdown Audio Module...");
}
