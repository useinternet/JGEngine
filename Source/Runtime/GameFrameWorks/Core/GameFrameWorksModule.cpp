#include "PCH/PCH.h"
#include "Core/GameFrameWorksModule.h"
#include "Core/GameInstance.h"
#include "Core/WorldSelfTest.h"
#include "GameMaster/GameMasterSelfTest.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

JG_MODULE_IMPL(HGameFrameWorksModule, GAMEFRAMEWORKS_C_API)

namespace
{
	// gmtest — GameMaster 커널 + 월드/게임 인스턴스 자체 검증(그래픽 없음).
	// GameFrameWorks 가 연결된 프로세스에서 쓸 수 있다: JGConsole(시작할 때 연결), 프로젝트 모드 에디터(게임 모듈이 연결).
	// 월드 테스트의 게임 인스턴스 절만 모듈의 게임 인스턴스로 월드를 로드 · 언로드한다. 이미 월드가 있으면(에디터에서 게임이 도는 중) 그 절은 건너뛰고,
	// 나머지는 자기 전용 PGameMaster / PWorld 를 쓰므로 에디터 안에서 돌려도 게임 월드를 건드리지 않는다.
	bool executeGameMasterSelfTest(const HConsoleCommandArgs&)
	{
		int32 failures = PGameMasterSelfTest::Run();
		failures += PWorldSelfTest::Run();

		if (failures == 0)
		{
			JG_LOG(GameFrameWorks, ELogLevel::Info, "gmtest: OK (0 failures)");
		}
		else
		{
			JG_LOG(GameFrameWorks, ELogLevel::Error, "gmtest: FAILED (%d failures)", failures);
		}

		return failures == 0;
	}

	HAutoConsoleCommand GameMasterSelfTestCommand(
		"gmtest",
		"gmtest",
		"Run the GameFrameWorks self tests (GameMaster kernel + world)",
		&executeGameMasterSelfTest);
}

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
		// 엔트리 클래스와 세션(연결) · 게임 UI(떠 있는 화면)는 이어받는다.
		instance->SetEntryClass(_gameInstance->GetEntryClass());
		instance->_session = _gameInstance->detachSession();
		instance->_ui      = _gameInstance->detachUI();
		_gameInstance->shutdown();
		_gameInstance.Reset();
	}

	_gameInstance = instance;
	_gameInstance->init();
}
