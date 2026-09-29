#include "PCH/PCH.h"
#include "GameMaster/GameMaster.h"
#include "GameMaster/Services/GameplaySerializer.h"

PGameMaster::PGameMaster()
{
	SetBoard(EGameplayBoardKind::None);
}

// ---- 설정 --------------------------------------------------------------------

void PGameMaster::RegisterZone(const PName& zoneName)
{
	_initialState.Zone(zoneName);
	_state.Zone(zoneName);
}

void PGameMaster::DefineValueStages(const PName& valueKind, const HList<PName>& stages)
{
	_engine.ValuePipeline.DefineStages(valueKind, stages);
}

bool PGameMaster::RegisterHandler(PSharedPtr<JGGameplayCommandHandler> handler)
{
	return _engine.Handlers.Register(handler);
}

bool PGameMaster::RegisterEffect(PSharedPtr<JGGameplayEffect> effect)
{
	return _engine.Effects.Register(effect);
}

bool PGameMaster::RegisterTrigger(PSharedPtr<JGGameplayTrigger> trigger)
{
	return _engine.Triggers.Register(trigger);
}

bool PGameMaster::RegisterModifier(PSharedPtr<JGGameplayModifier> modifier)
{
	return _engine.Modifiers.Register(modifier);
}

void PGameMaster::SetOrderPolicy(PSharedPtr<IGameplayOrderPolicy> policy)
{
	_engine.OrderPolicy = policy;
}

void PGameMaster::SetBoard(EGameplayBoardKind kind, int32 width, int32 height)
{
	SetBoard(IGameplayBoard::Create(kind), width, height);
}

void PGameMaster::SetBoard(PSharedPtr<IGameplayBoard> board, int32 width, int32 height)
{
	if (board == nullptr)
	{
		board = IGameplayBoard::Create(EGameplayBoardKind::None);
	}

	_board        = board;
	_engine.Board = board;

	_initialState.Board.Reset(board->GetKind(), width, height);
	_state.Board.Reset(board->GetKind(), width, height);
}

void PGameMaster::SetMigrator(PSharedPtr<IGameplayMigrator> migrator)
{
	_migrator = migrator;
}

void PGameMaster::SetSnapshotLimit(int32 limit)
{
	_snapshots.SetLimit(limit);
}

void PGameMaster::SetAgent(int32 team, PSharedPtr<IGameplayAgent> agent)
{
	for (HPair<int32, PSharedPtr<IGameplayAgent>>& entry : _agentsByTeam)
	{
		if (entry.first == team)
		{
			entry.second = agent;
			return;
		}
	}
	_agentsByTeam.push_back(HPair<int32, PSharedPtr<IGameplayAgent>>(team, agent));
}

void PGameMaster::SetTeamOfActorFunction(const std::function<int32(const HGameplayState&, const HGameplayEntityId&)>& teamOfActor)
{
	_teamOfActor = teamOfActor;
}

PSharedPtr<IGameplayAgent> PGameMaster::FindAgent(int32 team) const
{
	for (const HPair<int32, PSharedPtr<IGameplayAgent>>& entry : _agentsByTeam)
	{
		if (entry.first == team)
		{
			return entry.second;
		}
	}
	return nullptr;
}

PSharedPtr<IGameplayAgent> PGameMaster::FindAgentForActor(const HGameplayEntityId& actor) const
{
	if (_teamOfActor == nullptr)
	{
		// 팀 함수가 없으면 팀 0 의 에이전트가 전부를 맡는다.
		return FindAgent(0);
	}
	return FindAgent(_teamOfActor(_state, actor));
}

void PGameMaster::AddObserver(PSharedPtr<IGameplayObserver> observer)
{
	if (observer == nullptr)
	{
		return;
	}
	_observers.push_back(observer);
}

void PGameMaster::RemoveObserver(PSharedPtr<IGameplayObserver> observer)
{
	for (auto iter = _observers.begin(); iter != _observers.end();)
	{
		PSharedPtr<IGameplayObserver> pinned = (*iter).Pin();
		if (pinned == nullptr || pinned == observer)
		{
			iter = _observers.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

HGameplayState& PGameMaster::EditInitialState()
{
	return _initialState;
}

bool PGameMaster::Start(uint64 seed, HList<HGameplayEvent>* outEvents)
{
	_engine.Finalize();

	_initialState.SeedAll(seed);
	_initialState.Turn     = HGameplayTurnState();
	_initialState.Choice.Clear();
	_initialState.Sequence = 0;

	_state = _initialState;
	_snapshots.Clear();
	_log.Clear();

	HList<HGameplayEvent> events;
	_engine.Start(_state, events);
	_bStarted = true;

	notifyStateReplaced();
	notifyEvents(events);

	if (outEvents != nullptr)
	{
		outEvents->insert(outEvents->end(), events.begin(), events.end());
	}
	return true;
}

bool PGameMaster::IsStarted() const
{
	return _bStarted;
}

// ---- 플레이 --------------------------------------------------------------------

EGameplaySubmitResult PGameMaster::Submit(const HGameplayCommand& command, HList<HGameplayEvent>& outEvents, PString* outReason)
{
	if (_bStarted == false)
	{
		if (outReason != nullptr)
		{
			*outReason = "gameMaster not started";
		}
		return EGameplaySubmitResult::Rejected;
	}

	if (_engine.Validate(_state, command, outReason) == false)
	{
		return EGameplaySubmitResult::Rejected;
	}

	_snapshots.Push(_state);

	EGameplaySubmitResult result = _engine.Execute(_state, command, outEvents, outReason);
	if (result == EGameplaySubmitResult::Rejected)
	{
		HGameplayState restored;
		if (_snapshots.Pop(restored) == true)
		{
			_state = restored;
		}
		return result;
	}

	_log.Append(command);
	notifyEvents(outEvents);
	return result;
}

const HGameplayState& PGameMaster::GetState() const
{
	return _state;
}

void PGameMaster::EnumerateLegal(const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const
{
	_engine.EnumerateLegal(_state, actor, outCommands);
}

bool PGameMaster::Validate(const HGameplayCommand& command, PString* outReason) const
{
	return _engine.Validate(_state, command, outReason);
}

const HGameplayChoice* PGameMaster::GetPendingChoice() const
{
	if (_state.Choice.bPending == false)
	{
		return nullptr;
	}
	return &_state.Choice;
}

// ---- 되돌리기 · 저장 · 검증 --------------------------------------------------------

bool PGameMaster::Undo()
{
	HGameplayState restored;
	if (_snapshots.Pop(restored) == false)
	{
		return false;
	}
	_state = restored;
	_log.PopLast();
	notifyStateReplaced();
	return true;
}

int32 PGameMaster::UndoCount() const
{
	return _snapshots.Count();
}

HGameplayState PGameMaster::Snapshot() const
{
	return _state;
}

void PGameMaster::Restore(const HGameplayState& state)
{
	_state = state;
	notifyStateReplaced();
}

bool PGameMaster::Save(const PString& path) const
{
	return PGameplaySerializer::SaveToFile(path, _initialState, _state, _log);
}

bool PGameMaster::Load(const PString& path)
{
	HGameplayState initial = _initialState;   // 등록된 테이블을 유지한 채 읽는다
	HGameplayState current = _state;
	PGameplayCommandLog log;

	if (PGameplaySerializer::LoadFromFile(path, initial, current, log, _migrator.GetRawPointer()) == false)
	{
		return false;
	}

	_initialState = initial;
	_state        = current;
	_log          = log;
	_snapshots.Clear();
	_bStarted = _state.Turn.IsStarted();

	notifyStateReplaced();
	return true;
}

bool PGameMaster::Replay(const HList<HGameplayCommand>& commands, HList<HGameplayEvent>& outEvents)
{
	_state = _initialState;
	_snapshots.Clear();
	_log.Clear();

	_engine.Start(_state, outEvents);
	_bStarted = true;

	for (const HGameplayCommand& command : commands)
	{
		HList<HGameplayEvent> events;
		PString reason;
		EGameplaySubmitResult result = _engine.Execute(_state, command, events, &reason);
		if (result == EGameplaySubmitResult::Rejected)
		{
			JG_LOG(GameMaster, ELogLevel::Error, "PGameMaster::Replay: command %d rejected: %s", _log.Count(), reason);
			notifyStateReplaced();
			return false;
		}
		_log.Append(command);
		outEvents.insert(outEvents.end(), events.begin(), events.end());
	}

	notifyStateReplaced();
	return true;
}

uint64 PGameMaster::Checksum() const
{
	return _state.Checksum();
}

const PGameplayCommandLog& PGameMaster::GetCommandLog() const
{
	return _log;
}

const HGameplayState& PGameMaster::GetInitialState() const
{
	return _initialState;
}

EGameplaySubmitResult PGameMaster::Simulate(const HGameplayState& from, const HGameplayCommand& command, HGameplayState& outState, HList<HGameplayEvent>& outEvents)
{
	outState = from;
	return _engine.Execute(outState, command, outEvents, nullptr);
}

PGameplayRuleEngine& PGameMaster::GetEngine()
{
	return _engine;
}

const PGameplayRuleEngine& PGameMaster::GetEngine() const
{
	return _engine;
}

const IGameplayBoard* PGameMaster::GetBoard() const
{
	return _board.GetRawPointer();
}

void PGameMaster::CollectDerivedClasses(PSharedPtr<JGClass> base, HList<PSharedPtr<JGClass>>& outClasses)
{
	if (base == nullptr)
	{
		return;
	}

	HList<PSharedPtr<JGClass>> children = base->GetChildClasses(false);
	for (const PSharedPtr<JGClass>& child : children)
	{
		if (child == nullptr)
		{
			continue;
		}
		outClasses.push_back(child);
		CollectDerivedClasses(child, outClasses);
	}
}

// ---- 내부 ----------------------------------------------------------------------

bool PGameMaster::registerTyped(PSharedPtr<JGGameplayCommandHandler> handler)
{
	return RegisterHandler(handler);
}

bool PGameMaster::registerTyped(PSharedPtr<JGGameplayEffect> effect)
{
	return RegisterEffect(effect);
}

bool PGameMaster::registerTyped(PSharedPtr<JGGameplayTrigger> trigger)
{
	return RegisterTrigger(trigger);
}

bool PGameMaster::registerTyped(PSharedPtr<JGGameplayModifier> modifier)
{
	return RegisterModifier(modifier);
}

void PGameMaster::notifyEvents(const HList<HGameplayEvent>& events)
{
	for (auto iter = _observers.begin(); iter != _observers.end();)
	{
		PSharedPtr<IGameplayObserver> observer = (*iter).Pin();
		if (observer == nullptr)
		{
			iter = _observers.erase(iter);
			continue;
		}
		observer->OnGameplayEvents(_state, events);
		++iter;
	}

	OnEvents.BroadCast(events);
}

void PGameMaster::notifyStateReplaced()
{
	for (auto iter = _observers.begin(); iter != _observers.end();)
	{
		PSharedPtr<IGameplayObserver> observer = (*iter).Pin();
		if (observer == nullptr)
		{
			iter = _observers.erase(iter);
			continue;
		}
		observer->OnGameplayStateReplaced(_state);
		++iter;
	}
}
