#include "PCH/PCH.h"
#include "AudioCheckModule.h"
#include "Core/GameInstance.h"
#include "UI/GameUI.h"
#include "AudioCheckEntryActor.h"
#include "AudioCheckHUD.h"

JG_MODULE_IMPL(HAudioCheckModule, AUDIOCHECK_C_API)

JGType HAudioCheckModule::GetModuleType() const
{
	return JGTYPE(HAudioCheckModule);
}

void HAudioCheckModule::StartupModule()
{
	// 에디터(JGEditor)와 JGConsole 은 GameFrameWorks 를 먼저 연결한다 — 이미 연결돼 있으면 true 만 돌려준다.
	// 다른 호스트(예: 게임 단독 실행)에서도 돌도록 게임 모듈이 직접 연결해 둔다.
	if (GModuleGlobalSystem::GetInstance().ConnectModule("GameFrameWorks") == false)
	{
		JG_LOG(AudioCheck, ELogLevel::Critical, "Fail Connect GameFrameWorks Module...");
		return;
	}

	// 소리(오디오 재생, D-37): module.json 의존 + 연결
	if (GModuleGlobalSystem::GetInstance().ConnectModule("Audio") == false)
	{
		JG_LOG(AudioCheck, ELogLevel::Critical, "Fail Connect Audio Module...");
		return;
	}

	// 월드를 만들 때마다 엔진이 이 엔트리 액터를 스폰하고 OnEnterWorld 를 부른다.
	JGGameInstance::Get().SetEntryClass<JGAudioCheckEntryActor>();
	JGGameInstance::Get().LoadWorld(PName("AudioCheck"));

	// 검증 화면은 게임 인스턴스의 UI 에 올린다
	JGGameInstance::Get().GetUI()->PushWidget<JGAudioCheckHUD>(EGameUILayer::Game);

	JG_LOG(AudioCheck, ELogLevel::Info, "Startup AudioCheck Module...");
}

void HAudioCheckModule::ShutdownModule()
{
	// 게임이 만든 화면은 게임 모듈이 내려갈 때 내린다(위젯 클래스가 이 DLL 에 있다).
	if (JGGameInstance::HasInstance())
	{
		JGGameInstance::Get().GetUI()->ClearAllWidgets();
	}
	JG_LOG(AudioCheck, ELogLevel::Info, "Shutdown AudioCheck Module...");
}
