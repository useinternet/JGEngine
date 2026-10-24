#include "PCH/PCH.h"
#include "Core/GameFrameWorkModule.h"
#include "Core/GameInstance.h"
#include "Audio/AudioGlobalSystem.h"
#include "Asset/AssetDatabase.h"
#include "SelfTest/GameMasterSelfTest.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

JG_MODULE_IMPL(HGameFrameWorkModule, GAMEFRAMEWORK_C_API)

JGType HGameFrameWorkModule::GetModuleType() const
{
	return JGTYPE(HGameFrameWorkModule);
}

void HGameFrameWorkModule::StartupModule()
{
	JG_LOG(GameFrameWork, ELogLevel::Info, "GameFrameWork module startup");

	// 소리(Audio/): 시스템만 올린다. 장치는 처음 소리를 낼 때 연다(HAudio::Play · PlayMusic · Preload) — 에디터가 켤 때마다 장치를 열지 않게
	GCoreSystem::RegisterSystemInstance<GAudioGlobalSystem>();

	// 에셋 DB(Asset/): 시작할 때 Content 의 에셋을 모두 읽고, 텍스처 · 메시는 읽는 즉시 GPU 자원을 만든다.
	// 그래서 Graphics 가 먼저 켜진 프로세스(에디터 · GameHost)에서만 켠다. Graphics 없이 GameFrameWork 만 켜는 JGConsole 에는 에셋 DB 가 없다(사용자 결정 H-092).
	if (GModuleGlobalSystem::GetInstance().FindModule(PName("Graphics")) != nullptr)
	{
		GCoreSystem::RegisterSystemInstance<GAssetDatabase>();
	}

	_gameInstance = Allocate<JGGameInstance>();
	_gameInstance->init();

	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::Update, PTaskDelegate::CreateRaw(this, &HGameFrameWorkModule::tick));
}

void HGameFrameWorkModule::ShutdownModule()
{
	_bShutdown = true;

	if (_gameInstance != nullptr)
	{
		_gameInstance->shutdown();
		_gameInstance.Reset();
	}

	// 에셋(메시 · 텍스처)을 Graphics 모듈보다 먼저 놓는다 — 에디터 · GameHost 는 GameFrameWork 를 Graphics 보다 먼저 내린다.
	// 그래야 DisconnectModule 의 GC Flush 가 Graphics 가 살아 있을 때 메시/버퍼를 정리한다. 등록하지 않았으면 아무 일도 하지 않는다.
	GCoreSystem::UnRegisterSystemInstance<GAssetDatabase>();

	// StartupModule 에서 등록한 짝. 엔진 종료에서는 시스템 Destroy 가 먼저 장치 · 소리를 닫았다.
	if (GAudioGlobalSystem::IsValid() == true)
	{
		GAudioGlobalSystem::GetInstance().Shutdown();
	}
	GCoreSystem::UnRegisterSystemInstance<GAudioGlobalSystem>();

	JG_LOG(GameFrameWork, ELogLevel::Info, "GameFrameWork module shutdown");
}

PSharedPtr<JGGameInstance> HGameFrameWorkModule::GetGameInstance() const
{
	return _gameInstance;
}

PSharedPtr<JGGameInstance> HGameFrameWorkModule::ReplaceGameInstanceByClass(PSharedPtr<JGClass> instanceClass)
{
	if (instanceClass == nullptr)
	{
		return nullptr;
	}
	PSharedPtr<JGGameInstance> instance = RawDynamicCast<JGGameInstance>(AllocateByClass(instanceClass));
	if (instance == nullptr)
	{
		JG_LOG(GameFrameWork, ELogLevel::Error, "ReplaceGameInstanceByClass: class is not a JGGameInstance");
		return nullptr;
	}
	replaceGameInstance(instance);
	return instance;
}

int32 HGameFrameWorkModule::RunGameMasterSelfTest()
{
	return PGameMasterSelfTest::Run().Failed;
}

void HGameFrameWorkModule::tick()
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

void HGameFrameWorkModule::replaceGameInstance(PSharedPtr<JGGameInstance> instance)
{
	if (instance == nullptr)
	{
		return;
	}

	if (_gameInstance != nullptr)
	{
		if (_gameInstance->HasWorld() == true)
		{
			JG_LOG(GameFrameWork, ELogLevel::Warning, "ReplaceGameInstance: active world will be unloaded");
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
