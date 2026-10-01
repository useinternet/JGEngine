#pragma once
#include "Actors/Actor.h"
#include "Actors/GameplayEntityActor.h"
#include "Actors/GameplayCue.h"
#include "Core/GameplayBoardLayout.h"
#include "GameMaster/GameMaster.h"
#include "GameMasterActor.generation.h"

// 연출 중 입력 정책.
enum class EGameplayInputPolicy : int32
{
	Block = 0,        // 연출이 끝날 때까지 새 명령 거부
	Buffer,           // 명령을 버퍼에 쌓고 연출이 끝나면 순서대로 실행
	SkipAndExecute,   // 남은 연출을 즉시 스킵하고 실행
};

// JGGameMasterActor::Submit 의 결과.
enum class EGameMasterActorSubmit : int32
{
	Rejected = 0,
	Executed,
	PendingChoice,
	Buffered,
	Sent,             // 클라: 호스트로 보냈다. 이벤트는 승인되면 오고, 거절은 세션의 OnLocalCommandRejected 로 온다
};

class PGameMasterActorObserver;
class PGameplaySession;

// 연출자. PGameMaster 를 들고 이벤트를 큐(Cue)로 재생하며, 엔티티 ↔ 액터 표를 갖는다.
//   Submit → (입력 정책) → 세션 → GameMaster 실행 → 이벤트 목록 → 큐 인스턴스 순차 재생 (프레임 틱)
// 게임 인스턴스 월드의 액터는 BeginPlay 에서 GameMaster 를 게임 인스턴스의 세션에 붙이고 EndPlay 에서 뗀다.
// 명령은 세션만 GameMaster 에 낸다 (싱글도 Standalone 세션). 원격 · 승인 명령은 세션이 바로 적용하고 이 액터는 이벤트만 재생한다.
// 게임 인스턴스 밖 월드(자체 테스트가 직접 만든 월드)에서는 세션 없이 GameMaster 에 바로 낸다.
// 스폰된 엔티티의 액터는 그 이벤트의 큐가 시작하기 전에 만들고, 파괴된 엔티티의 액터는 그 큐가 끝난 뒤에 없앤다 (사망 연출이 액터를 쓴다).
// 상태 교체(되돌리기 · 로드 · 리플레이 · 문서 수신) 시 바인딩을 전부 다시 만든다.
JGCLASS()
class GAMEFRAMEWORKS_API JGGameMasterActor : public JGActor
{
	JG_GENERATED_CLASS_BODY

	friend class PGameMasterActorObserver;

private:
	PSharedPtr<PGameMaster>                _gameMaster;
	PSharedPtr<PGameMasterActorObserver> _observer;
	HList<PSharedPtr<JGGameplayCue>>      _cuePrototypes;

	HDeque<HGameplayEvent>    _pendingEvents;
	PSharedPtr<JGGameplayCue> _activeCue;
	HGameplayEvent            _activeEvent;   // 재생 중인 큐의 이벤트. 큐가 끝나면 파괴 바인딩을 적용한다
	HDeque<HGameplayCommand>  _bufferedCommands;
	EGameplayInputPolicy      _inputPolicy = EGameplayInputPolicy::Buffer;

	HHashMap<uint64, PWeakPtr<JGActor>> _actorsByEntity;
	HGameplayBoardLayout                _boardLayout;

public:
	JGGameMasterActor() = default;
	virtual ~JGGameMasterActor() = default;

	// GameMaster. 게임 모듈이 만든 것을 받거나 없으면 새로 만든다.
	void                    SetGameMaster(PSharedPtr<PGameMaster> gameMaster);
	PSharedPtr<PGameMaster> GetGameMaster() const;
	PSharedPtr<PGameMaster> GetOrCreateGameMaster();

	// 큐 등록. 프로토타입 하나가 이벤트 종류 하나를 맡는다. 먼저 등록된 것이 먼저 물어본다.
	void  RegisterCue(PSharedPtr<JGGameplayCue> prototype);
	int32 RegisterCuesFromReflection();

	template<class T>
	PSharedPtr<T> RegisterCue()
	{
		PSharedPtr<T> prototype = Allocate<T>();
		RegisterCue(prototype);
		return prototype;
	}

	// 게임 시작. 권한 쪽(IsAuthority)에서만 부른다. 세션에 붙여 세션이 시작하고 참가자에게 초기 상태를 보낸다.
	// 클라는 호스트가 보낸 시작을 받는다 (부르면 false).
	bool StartGame(uint64 seed);
	// 이 액터의 GameMaster 가 붙은 세션. 게임 인스턴스 밖 월드이거나 붙기 전이면 nullptr.
	PSharedPtr<PGameplaySession> GetSession() const;

	// 로컬 입력의 명령 제출. 정책에 따라 즉시 실행 · 버퍼 · 스킵 후 실행. 실행은 세션을 거친다.
	EGameMasterActorSubmit Submit(const HGameplayCommand& command, PString* outReason = nullptr);
	void SetInputPolicy(EGameplayInputPolicy policy);
	EGameplayInputPolicy GetInputPolicy() const;

	// 연출 진행 상태
	bool  IsBusy() const;
	int32 PendingEventCount() const;
	void  SkipAll();

	// 바인딩
	PSharedPtr<JGActor>           FindActor(const HGameplayEntityId& id) const;
	void                          BindActor(const HGameplayEntityId& id, PSharedPtr<JGActor> actor);
	void                          UnbindActor(const HGameplayEntityId& id);
	void                          RebuildBindings();

	// GameMaster 보드를 월드에 놓는 방법 (칸 좌표 ↔ 월드 위치, 보드 피킹). 기본은 보드 없음.
	void                        SetBoardLayout(const HGameplayBoardLayout& layout);
	const HGameplayBoardLayout& GetBoardLayout() const;

protected:
	virtual void OnBeginPlay() override;
	virtual void OnTick(float32 deltaSeconds) override;
	virtual void OnEndPlay() override;

	// 엔티티가 생겼을 때 액터를 만든다. 기본은 JGGameplayEntityActor. 게임이 파생해 외형을 정한다.
	virtual PSharedPtr<JGGameplayEntityActor> SpawnActorForEntity(const HGameplayEntityId& id, const HGameplayEvent& event);
	// 엔티티가 파괴됐을 때. 기본은 액터 파괴.
	virtual void DestroyActorForEntity(const HGameplayEntityId& id, PSharedPtr<JGActor> actor);

private:
	void onGameplayEvents(const HList<HGameplayEvent>& events);
	void onGameplayStateReplaced();
	void applyBindingBeforeCue(const HGameplayEvent& event);   // EntitySpawned → 액터 생성 · 바인딩
	void applyBindingAfterCue(const HGameplayEvent& event);    // EntityDestroyed → 바인딩 해제 · 액터 파괴
	bool startNextCue();
	void finishActiveCue();
	void flushBufferedCommands();
	EGameMasterActorSubmit executeNow(const HGameplayCommand& command, PString* outReason);

	PSharedPtr<PGameplaySession> findGameInstanceSession() const;   // 이 액터가 게임 인스턴스 월드에 있으면 그 세션
	void bindSession();
	void unbindSession();
};
