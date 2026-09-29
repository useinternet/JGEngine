#pragma once
#include "Core/GameFrameWorksDefines.h"
#include "GameInstance.generation.h"

class PWorld;
class JGGameEntryActor;

// 프로세스 수명의 게임 객체. 월드의 소유 · 로드 · 언로드 · 틱을 맡는다 (언리얼의 GameInstance 자리).
// 모듈이 하나를 만들어 소유한다. 게임은 PWorld 를 직접 만지지 않고 이 객체를 통해서만 월드를 다룬다.
// 게임은 파생해 프로필 · 캠페인 · 설정처럼 월드보다 오래 사는 상태를 둔다 (HGameFrameWorksModule::ReplaceGameInstance).
JGCLASS()
class GAMEFRAMEWORKS_API JGGameInstance : public JGObject
{
	JG_GENERATED_CLASS_BODY

	friend class HGameFrameWorksModule;

private:
	PSharedPtr<PWorld>  _world;        // 활성 월드. 지금은 하나
	PSharedPtr<JGClass> _entryClass;   // 월드 생성 직후 스폰할 엔트리 액터 클래스 (JGGameEntryActor 파생)
	bool                _bInitialized = false;

	static JGGameInstance* s_instance;

public:
	HMulticastDelegate<PSharedPtr<PWorld>> OnWorldLoaded;      // 엔트리 액터 스폰 후 · BeginPlay 전
	HMulticastDelegate<PSharedPtr<PWorld>> OnWorldUnloading;   // EndPlay 전

public:
	JGGameInstance() = default;
	virtual ~JGGameInstance() = default;

	static bool             HasInstance();
	static JGGameInstance&  Get();

	// 엔트리 액터 클래스. 월드를 로드할 때마다 엔진이 이 클래스를 스폰하고 OnEnterWorld 를 부른다.
	template<class T>
	void SetEntryClass()
	{
		SetEntryClass(StaticClass<T>());
	}

	void                SetEntryClass(PSharedPtr<JGClass> entryClass);
	PSharedPtr<JGClass> GetEntryClass() const;

	// 활성 월드가 있으면 먼저 언로드한다.
	PSharedPtr<PWorld> LoadWorld(const PName& name = PName());
	void               UnloadWorld();
	PSharedPtr<PWorld> GetWorld() const;
	bool               HasWorld() const;
	PSharedPtr<JGGameEntryActor> GetEntryActor() const;

	// 모듈이 프레임마다 부른다.
	void Tick(float32 deltaSeconds);

protected:
	virtual void OnInit() {}
	virtual void OnShutdown() {}
	virtual void OnWorldReady(PSharedPtr<PWorld> world) {}     // 엔트리 액터 스폰 후 · BeginPlay 전
	virtual void OnWorldUnload(PSharedPtr<PWorld> world) {}    // EndPlay 전

private:
	void init();
	void shutdown();
};
