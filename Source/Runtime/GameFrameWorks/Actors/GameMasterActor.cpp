#include "PCH/PCH.h"
#include "Actors/GameMasterActor.h"
#include "Core/World.h"
#include "Core/GameInstance.h"

// GameMaster 관찰자. JGActor 와 IGameplayObserver 를 한 클래스에 다중 상속하면 IMemoryObject 뿌리가 둘이 되므로 분리한다.
class PGameMasterActorObserver : public IGameplayObserver
{
	JGGameMasterActor* _gameMasterActor = nullptr;

public:
	PGameMasterActorObserver() = default;
	explicit PGameMasterActorObserver(JGGameMasterActor* gameMasterActor)
		: _gameMasterActor(gameMasterActor)
	{
	}
	virtual ~PGameMasterActorObserver() = default;

	void Clear()
	{
		_gameMasterActor = nullptr;
	}

	virtual void OnGameplayEvents(const HGameplayState& state, const HList<HGameplayEvent>& events) override
	{
		if (_gameMasterActor != nullptr)
		{
			_gameMasterActor->onGameplayEvents(events);
		}
	}

	virtual void OnGameplayStateReplaced(const HGameplayState& state) override
	{
		if (_gameMasterActor != nullptr)
		{
			_gameMasterActor->onGameplayStateReplaced();
		}
	}
};

void JGGameMasterActor::SetGameMaster(PSharedPtr<PGameMaster> gameMaster)
{
	unbindSession();

	if (_gameMaster != nullptr && _observer != nullptr)
	{
		_gameMaster->RemoveObserver(_observer);
	}

	_gameMaster = gameMaster;

	if (_gameMaster != nullptr)
	{
		if (_observer == nullptr)
		{
			_observer = Allocate<PGameMasterActorObserver>(this);
		}
		_gameMaster->AddObserver(_observer);
	}

	// BeginPlay 전이면 BeginPlay 에서 붙인다 (엔트리 액터가 규칙을 다 등록한 뒤라야 클라가 받은 시작을 적용할 수 있다).
	if (HasBegunPlay() == true)
	{
		bindSession();
	}
}

PSharedPtr<PGameMaster> JGGameMasterActor::GetGameMaster() const
{
	return _gameMaster;
}

PSharedPtr<PGameMaster> JGGameMasterActor::GetOrCreateGameMaster()
{
	if (_gameMaster == nullptr)
	{
		SetGameMaster(Allocate<PGameMaster>());
	}
	return _gameMaster;
}

void JGGameMasterActor::RegisterCue(PSharedPtr<JGGameplayCue> prototype)
{
	if (prototype == nullptr)
	{
		return;
	}
	_cuePrototypes.push_back(prototype);
}

int32 JGGameMasterActor::RegisterCuesFromReflection()
{
	HList<PSharedPtr<JGClass>> classes;
	PGameMaster::CollectDerivedClasses(StaticClass<JGGameplayCue>(), classes);

	int32 count = 0;
	for (const PSharedPtr<JGClass>& classObject : classes)
	{
		PSharedPtr<JGGameplayCue> prototype = RawDynamicCast<JGGameplayCue>(AllocateByClass(classObject));
		if (prototype == nullptr)
		{
			continue;
		}
		RegisterCue(prototype);
		++count;
	}
	return count;
}

bool JGGameMasterActor::StartGame(uint64 seed)
{
	if (_gameMaster == nullptr)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Error, "GameMasterActor::StartGame: no gameMaster");
		return false;
	}

	// 엔트리 액터(OnEnterWorld)에서 부르면 아직 BeginPlay 전이다. 규칙 등록이 끝났으니 여기서 붙인다.
	bindSession();
	PSharedPtr<PGameplaySession> session = GetSession();
	if (session != nullptr)
	{
		return session->StartGame(seed);
	}
	if (findGameInstanceSession() != nullptr)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Error, "GameMasterActor::StartGame: the session is bound to another gameMaster");
		return false;
	}
	return _gameMaster->Start(seed);
}

PSharedPtr<PGameplaySession> JGGameMasterActor::GetSession() const
{
	PSharedPtr<PGameplaySession> session = findGameInstanceSession();
	if (session == nullptr || _gameMaster == nullptr || session->GetGameMaster() != _gameMaster)
	{
		return nullptr;
	}
	return session;
}

EGameMasterActorSubmit JGGameMasterActor::Submit(const HGameplayCommand& command, PString* outReason)
{
	if (_gameMaster == nullptr)
	{
		if (outReason != nullptr)
		{
			*outReason = "no gameMaster";
		}
		return EGameMasterActorSubmit::Rejected;
	}

	if (IsBusy() == true)
	{
		switch (_inputPolicy)
		{
		case EGameplayInputPolicy::Block:
			if (outReason != nullptr)
			{
				*outReason = "presentation busy";
			}
			return EGameMasterActorSubmit::Rejected;

		case EGameplayInputPolicy::Buffer:
			_bufferedCommands.push_back(command);
			return EGameMasterActorSubmit::Buffered;

		case EGameplayInputPolicy::SkipAndExecute:
			SkipAll();
			break;
		}
	}

	return executeNow(command, outReason);
}

void JGGameMasterActor::SetInputPolicy(EGameplayInputPolicy policy)
{
	_inputPolicy = policy;
}

EGameplayInputPolicy JGGameMasterActor::GetInputPolicy() const
{
	return _inputPolicy;
}

bool JGGameMasterActor::IsBusy() const
{
	return _activeCue != nullptr || _pendingEvents.empty() == false;
}

int32 JGGameMasterActor::PendingEventCount() const
{
	return (int32)_pendingEvents.size();
}

void JGGameMasterActor::SkipAll()
{
	if (_activeCue != nullptr)
	{
		_activeCue->Skip();
		finishActiveCue();
	}

	while (_pendingEvents.empty() == false)
	{
		HGameplayEvent event = _pendingEvents.front();
		_pendingEvents.pop_front();
		applyBindingBeforeCue(event);

		for (const PSharedPtr<JGGameplayCue>& prototype : _cuePrototypes)
		{
			if (prototype->Accepts(event) == false)
			{
				continue;
			}
			PSharedPtr<JGGameplayCue> cue = prototype->CreateInstance();
			if (cue != nullptr)
			{
				cue->Begin(event, *this);
				cue->Skip();
			}
			break;
		}

		applyBindingAfterCue(event);
	}
}

PSharedPtr<JGActor> JGGameMasterActor::FindActor(const HGameplayEntityId& id) const
{
	auto iter = _actorsByEntity.find(id.ToKey());
	if (iter == _actorsByEntity.end())
	{
		return nullptr;
	}
	return iter->second.Pin();
}

void JGGameMasterActor::BindActor(const HGameplayEntityId& id, PSharedPtr<JGActor> actor)
{
	if (id.IsValid() == false || actor == nullptr)
	{
		return;
	}
	_actorsByEntity[id.ToKey()] = actor;
}

void JGGameMasterActor::UnbindActor(const HGameplayEntityId& id)
{
	_actorsByEntity.erase(id.ToKey());
}

void JGGameMasterActor::RebuildBindings()
{
	// 기존 액터 전부 제거
	for (HPair<const uint64, PWeakPtr<JGActor>>& entry : _actorsByEntity)
	{
		PSharedPtr<JGActor> actor = entry.second.Pin();
		if (actor != nullptr)
		{
			actor->Destroy();
		}
	}
	_actorsByEntity.clear();
	_pendingEvents.clear();
	_activeCue.Reset();
	_activeEvent = HGameplayEvent();

	if (_gameMaster == nullptr)
	{
		return;
	}

	// 살아 있는 엔티티마다 액터 생성 (연출 없음)
	HList<HGameplayEntityId> alive;
	_gameMaster->GetState().Entities.CollectAlive(alive);
	for (const HGameplayEntityId& id : alive)
	{
		HGameplayEvent synthetic(PName(HGameplayBuiltin::EventEntitySpawned), id, HGameplayEntityId::None());
		PSharedPtr<JGGameplayEntityActor> actor = SpawnActorForEntity(id, synthetic);
		if (actor != nullptr)
		{
			actor->SetEntityId(id);
			BindActor(id, actor);
		}
	}
}

void JGGameMasterActor::SetBoardLayout(const HGameplayBoardLayout& layout)
{
	_boardLayout = layout;
}

const HGameplayBoardLayout& JGGameMasterActor::GetBoardLayout() const
{
	return _boardLayout;
}

void JGGameMasterActor::OnBeginPlay()
{
	// 클라는 붙는 순간 보관돼 있던 시작 · 문서가 적용된다 (상태 교체 → 바인딩 재구성).
	bindSession();

	if (_gameMaster != nullptr && _gameMaster->IsStarted() == true && _actorsByEntity.empty() == true)
	{
		RebuildBindings();
	}
}

void JGGameMasterActor::OnTick(float32 deltaSeconds)
{
	if (_activeCue != nullptr)
	{
		_activeCue->Tick(deltaSeconds);
		if (_activeCue->IsDone() == false)
		{
			return;
		}
		finishActiveCue();
	}

	// 큐가 시작되지 않은 이벤트(맡는 큐 없음)와 즉시 완료되는 큐는 같은 프레임에 계속 넘어간다.
	// 진행 중인 큐가 생기면 루프 조건이 false 가 되어 멈춘다.
	while (_activeCue == nullptr && _pendingEvents.empty() == false)
	{
		startNextCue();
		if (_activeCue != nullptr && _activeCue->IsDone() == true)
		{
			finishActiveCue();
		}
	}

	if (IsBusy() == false)
	{
		flushBufferedCommands();
	}
}

void JGGameMasterActor::OnEndPlay()
{
	unbindSession();

	if (_gameMaster != nullptr && _observer != nullptr)
	{
		_gameMaster->RemoveObserver(_observer);
	}
	if (_observer != nullptr)
	{
		_observer->Clear();
	}
	_activeCue.Reset();
	_activeEvent = HGameplayEvent();
	_pendingEvents.clear();
	_bufferedCommands.clear();
}

PSharedPtr<JGGameplayEntityActor> JGGameMasterActor::SpawnActorForEntity(const HGameplayEntityId& id, const HGameplayEvent& event)
{
	PSharedPtr<PWorld> world = GetWorld();
	if (world == nullptr)
	{
		return nullptr;
	}
	return world->SpawnActor<JGGameplayEntityActor>(PName(PString::Format("Entity_%s", id.ToString())));
}

void JGGameMasterActor::DestroyActorForEntity(const HGameplayEntityId& id, PSharedPtr<JGActor> actor)
{
	if (actor != nullptr)
	{
		actor->Destroy();
	}
}

void JGGameMasterActor::onGameplayEvents(const HList<HGameplayEvent>& events)
{
	for (const HGameplayEvent& event : events)
	{
		_pendingEvents.push_back(event);
	}
}

void JGGameMasterActor::onGameplayStateReplaced()
{
	RebuildBindings();
}

void JGGameMasterActor::applyBindingBeforeCue(const HGameplayEvent& event)
{
	if (event.Kind != PName(HGameplayBuiltin::EventEntitySpawned))
	{
		return;
	}
	if (FindActor(event.Subject) != nullptr)
	{
		return;
	}

	PSharedPtr<JGGameplayEntityActor> actor = SpawnActorForEntity(event.Subject, event);
	if (actor != nullptr)
	{
		actor->SetEntityId(event.Subject);
		BindActor(event.Subject, actor);
	}
}

void JGGameMasterActor::applyBindingAfterCue(const HGameplayEvent& event)
{
	if (event.Kind != PName(HGameplayBuiltin::EventEntityDestroyed))
	{
		return;
	}

	PSharedPtr<JGActor> actor = FindActor(event.Subject);
	UnbindActor(event.Subject);
	DestroyActorForEntity(event.Subject, actor);
}

bool JGGameMasterActor::startNextCue()
{
	if (_pendingEvents.empty() == true)
	{
		return false;
	}

	HGameplayEvent event = _pendingEvents.front();
	_pendingEvents.pop_front();

	applyBindingBeforeCue(event);

	for (const PSharedPtr<JGGameplayCue>& prototype : _cuePrototypes)
	{
		if (prototype->Accepts(event) == false)
		{
			continue;
		}
		PSharedPtr<JGGameplayCue> cue = prototype->CreateInstance();
		if (cue == nullptr)
		{
			continue;
		}
		cue->Begin(event, *this);
		_activeCue   = cue;
		_activeEvent = event;
		return true;
	}

	// 맡는 큐가 없는 이벤트는 연출 없이 지나간다.
	applyBindingAfterCue(event);
	return true;
}

void JGGameMasterActor::finishActiveCue()
{
	_activeCue.Reset();
	HGameplayEvent event = _activeEvent;
	_activeEvent = HGameplayEvent();
	applyBindingAfterCue(event);
}

void JGGameMasterActor::flushBufferedCommands()
{
	while (_bufferedCommands.empty() == false && IsBusy() == false)
	{
		HGameplayCommand command = _bufferedCommands.front();
		_bufferedCommands.pop_front();

		PString reason;
		EGameMasterActorSubmit result = executeNow(command, &reason);
		if (result == EGameMasterActorSubmit::Rejected)
		{
			JG_LOG(GameFrameWorks, ELogLevel::Warning, "GameMasterActor: buffered command rejected: %s", reason);
		}
	}
}

EGameMasterActorSubmit JGGameMasterActor::executeNow(const HGameplayCommand& command, PString* outReason)
{
	// 이벤트는 어느 경로든 관찰자(onGameplayEvents)로 큐에 들어간다.
	// 게임 인스턴스 월드: 세션이 권한 경로의 유일한 입구다 (싱글은 Standalone 세션).
	if (findGameInstanceSession() != nullptr)
	{
		bindSession();
		PSharedPtr<PGameplaySession> session = GetSession();
		if (session == nullptr)
		{
			if (outReason != nullptr)
			{
				*outReason = "gameMaster is not bound to the session";
			}
			return EGameMasterActorSubmit::Rejected;
		}

		switch (session->SubmitLocal(command, outReason))
		{
		case EGameplaySessionSubmit::Executed:
			return EGameMasterActorSubmit::Executed;
		case EGameplaySessionSubmit::PendingChoice:
			return EGameMasterActorSubmit::PendingChoice;
		case EGameplaySessionSubmit::Sent:
		case EGameplaySessionSubmit::Queued:
			return EGameMasterActorSubmit::Sent;
		case EGameplaySessionSubmit::Rejected:
		default:
			return EGameMasterActorSubmit::Rejected;
		}
	}

	// 게임 인스턴스 밖 월드(자체 테스트가 만든 월드): 세션 없이 바로 낸다.
	HList<HGameplayEvent> events;
	EGameplaySubmitResult result = _gameMaster->Submit(command, events, outReason);

	switch (result)
	{
	case EGameplaySubmitResult::Executed:
		return EGameMasterActorSubmit::Executed;
	case EGameplaySubmitResult::PendingChoice:
		return EGameMasterActorSubmit::PendingChoice;
	case EGameplaySubmitResult::Rejected:
	default:
		return EGameMasterActorSubmit::Rejected;
	}
}

PSharedPtr<PGameplaySession> JGGameMasterActor::findGameInstanceSession() const
{
	if (JGGameInstance::HasInstance() == false)
	{
		return nullptr;
	}

	JGGameInstance&    gameInstance = JGGameInstance::Get();
	PSharedPtr<PWorld> world        = GetWorld();
	if (world == nullptr || gameInstance.GetWorld() != world)
	{
		return nullptr;
	}
	return gameInstance.GetSession();
}

// 세션이 비어 있을 때만 붙는다. 다른 GameMaster 가 붙어 있으면 빼앗지 않는다 (세션은 한 번에 하나만 붙는다).
void JGGameMasterActor::bindSession()
{
	PSharedPtr<PGameplaySession> session = findGameInstanceSession();
	if (session == nullptr || _gameMaster == nullptr || session->GetGameMaster() != nullptr)
	{
		return;
	}
	session->BindGameMaster(_gameMaster);
}

void JGGameMasterActor::unbindSession()
{
	PSharedPtr<PGameplaySession> session = GetSession();
	if (session != nullptr)
	{
		session->UnbindGameMaster();
	}
}
