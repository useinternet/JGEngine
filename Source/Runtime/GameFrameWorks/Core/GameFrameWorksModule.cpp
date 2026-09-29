#include "PCH/PCH.h"
#include "Core/GameFrameWorksModule.h"
#include "Core/GameInstance.h"
#include "GameMaster/GameMasterSelfTest.h"

JG_MODULE_IMPL(HGameFrameWorksModule, GAMEFRAMEWORKS_C_API)

JGType HGameFrameWorksModule::GetModuleType() const
{
	return JGTYPE(HGameFrameWorksModule);
}

void HGameFrameWorksModule::StartupModule()
{
	JG_LOG(GameFrameWorks, ELogLevel::Info, "GameFrameWorks module startup");

	_gameInstance = Allocate<JGGameInstance>();
	_gameInstance->init();

	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::Update, PTaskDelegate::CreateRaw(this, &HGameFrameWorksModule::tick));
}

void HGameFrameWorksModule::ShutdownModule()
{
	_bShutdown = true;

	if (_gameInstance != nullptr)
	{
		_gameInstance->shutdown();
		_gameInstance.Reset();
	}

	JG_LOG(GameFrameWorks, ELogLevel::Info, "GameFrameWorks module shutdown");
}

PSharedPtr<JGGameInstance> HGameFrameWorksModule::GetGameInstance() const
{
	return _gameInstance;
}

PSharedPtr<JGGameInstance> HGameFrameWorksModule::ReplaceGameInstanceByClass(PSharedPtr<JGClass> instanceClass)
{
	if (instanceClass == nullptr)
	{
		return nullptr;
	}
	PSharedPtr<JGGameInstance> instance = RawDynamicCast<JGGameInstance>(AllocateByClass(instanceClass));
	if (instance == nullptr)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Error, "ReplaceGameInstanceByClass: class is not a JGGameInstance");
		return nullptr;
	}
	replaceGameInstance(instance);
	return instance;
}

int32 HGameFrameWorksModule::RunGameMasterSelfTest()
{
	return PGameMasterSelfTest::Run();
}

void HGameFrameWorksModule::tick()
{
	if (_bShutdown == true || _gameInstance == nullptr)
	{
		return;
	}

	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	float32 deltaSeconds = 0.0f;
	if (_bHasLastTick == true)
	{
		deltaSeconds = std::chrono::duration<float32>(now - _lastTick).count();
		if (deltaSeconds > 0.25f)
		{
			deltaSeconds = 0.25f;
		}
	}
	_lastTick     = now;
	_bHasLastTick = true;

	_gameInstance->Tick(deltaSeconds);
}

void HGameFrameWorksModule::replaceGameInstance(PSharedPtr<JGGameInstance> instance)
{
	if (instance == nullptr)
	{
		return;
	}

	if (_gameInstance != nullptr)
	{
		if (_gameInstance->HasWorld() == true)
		{
			JG_LOG(GameFrameWorks, ELogLevel::Warning, "ReplaceGameInstance: active world will be unloaded");
		}
		// 엔트리 클래스는 이어받는다.
		instance->SetEntryClass(_gameInstance->GetEntryClass());
		_gameInstance->shutdown();
		_gameInstance.Reset();
	}

	_gameInstance = instance;
	_gameInstance->init();
}
