#pragma once
#include "GameMaster/GameMasterDefines.h"
#include "GameMaster/State/GameplayState.h"
#include "GameMaster/Messages/GameplayCommand.h"
#include "GameMaster/Messages/GameplayEvent.h"
#include "GameMaster/Rules/GameplayRuleEngine.h"
#include "GameMaster/Boards/GameplayBoard.h"
#include "GameMaster/Services/GameplaySnapshotStack.h"
#include "GameMaster/Services/GameplayCommandLog.h"
#include "GameMaster/Services/GameplayObserver.h"
#include "GameMaster/Agents/GameplayAgent.h"

// GameMaster 파사드. 게임 모듈이 보는 단일 입구.
//   설정(StartupModule) → Start(seed) → Submit / EnumerateLegal / GetState → Undo · Save · Load · Replay · Checksum
// 프레젠테이션 · AI · 리플레이 · 테스트가 전부 같은 Submit 을 쓴다.
class GAMEFRAMEWORKS_API PGameMaster : public IMemoryObject
{
	HGameplayState         _initialState;   // Start 시점의 상태. 리플레이 기준
	HGameplayState         _state;
	PGameplayRuleEngine    _engine;
	PGameplaySnapshotStack _snapshots;
	PGameplayCommandLog    _log;
	PSharedPtr<IGameplayBoard>    _board;
	PSharedPtr<IGameplayMigrator> _migrator;

	HList<PWeakPtr<IGameplayObserver>>              _observers;
	HList<HPair<int32, PSharedPtr<IGameplayAgent>>> _agentsByTeam;
	std::function<int32(const HGameplayState&, const HGameplayEntityId&)> _teamOfActor;

	bool _bStarted     = false;
	bool _bUndoEnabled = true;

public:
	// Submit 결과와 같은 이벤트 목록을 구독으로도 받을 수 있다.
	HMulticastDelegate<const HList<HGameplayEvent>&> OnEvents;

public:
	PGameMaster();
	virtual ~PGameMaster() = default;

	// ---- 설정. Start 전에 한 번. 플레이 중 변경 금지 (결정론) ----

	template<class T>
	void RegisterComponent(const PName& typeName)
	{
		_initialState.RegisterTable<T>(typeName);
		_state.RegisterTable<T>(typeName);
	}

	template<class T>
	void RegisterComponent()
	{
		RegisterComponent<T>(JGType::GenerateType<T>().GetName());
	}

	void RegisterZone(const PName& zoneName);
	void DefineValueStages(const PName& valueKind, const HList<PName>& stages);

	bool RegisterHandler(PSharedPtr<JGGameplayCommandHandler> handler);
	bool RegisterEffect(PSharedPtr<JGGameplayEffect> effect);
	bool RegisterTrigger(PSharedPtr<JGGameplayTrigger> trigger);
	bool RegisterModifier(PSharedPtr<JGGameplayModifier> modifier);

	// T 의 모든 파생 클래스(리플렉션 등록된 것)를 하나씩 만들어 등록한다. 등록 수를 돌려준다.
	template<class T>
	int32 RegisterAllFromReflection()
	{
		HList<PSharedPtr<JGClass>> classes;
		CollectDerivedClasses(StaticClass<T>(), classes);

		int32 count = 0;
		for (const PSharedPtr<JGClass>& classObject : classes)
		{
			PSharedPtr<JGObject> object = AllocateByClass(classObject);
			if (object == nullptr)
			{
				continue;
			}
			PSharedPtr<T> typed = Cast<T>(object);
			if (typed == nullptr)
			{
				continue;
			}
			if (registerTyped(typed) == true)
			{
				++count;
			}
		}
		return count;
	}

	void SetOrderPolicy(PSharedPtr<IGameplayOrderPolicy> policy);
	// 게임 흐름(턴 · 페이즈 구조). null 이면 기본 흐름(라운드 → 순서 → 행동자마다 차례, PGameplayRoundTurnFlow). IGameplayFlow 참고.
	void                      SetFlow(PSharedPtr<IGameplayFlow> flow);
	PSharedPtr<IGameplayFlow> GetFlow() const;
	void SetBoard(EGameplayBoardKind kind, int32 width = 0, int32 height = 0);
	void SetBoard(PSharedPtr<IGameplayBoard> board, int32 width = 0, int32 height = 0);
	void SetMigrator(PSharedPtr<IGameplayMigrator> migrator);
	void SetSnapshotLimit(int32 limit);

	// 에이전트. 팀 번호는 게임이 정하고, 행동자 → 팀 함수를 주면 러너가 자동으로 고른다.
	void SetAgent(int32 team, PSharedPtr<IGameplayAgent> agent);
	void SetTeamOfActorFunction(const std::function<int32(const HGameplayState&, const HGameplayEntityId&)>& teamOfActor);
	PSharedPtr<IGameplayAgent> FindAgent(int32 team) const;
	PSharedPtr<IGameplayAgent> FindAgentForActor(const HGameplayEntityId& actor) const;
	// 행동자의 팀(조작 주체) 번호. 팀 함수가 없으면 0 (FindAgentForActor 와 같은 규칙).
	int32 TeamOfActor(const HGameplayEntityId& actor) const;

	void AddObserver(PSharedPtr<IGameplayObserver> observer);
	void RemoveObserver(PSharedPtr<IGameplayObserver> observer);

	// Start 전에 초기 상태를 구성한다 (엔티티 · 컴포넌트 · 영역 · 보드 배치).
	HGameplayState& EditInitialState();

	// 시드를 심고 초기 상태를 복사한 뒤 페이즈 기계를 시작한다. 시작 이벤트는 로그에 남지 않는다 (리플레이가 다시 만든다).
	bool Start(uint64 seed, HList<HGameplayEvent>* outEvents = nullptr);
	bool IsStarted() const;

	// ---- 플레이 ----

	EGameplaySubmitResult Submit(const HGameplayCommand& command, HList<HGameplayEvent>& outEvents, PString* outReason = nullptr);
	const HGameplayState&  GetState() const;
	void                     EnumerateLegal(const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const;
	bool                     Validate(const HGameplayCommand& command, PString* outReason) const;
	const HGameplayChoice* GetPendingChoice() const;

	// ---- 되돌리기 · 저장 · 검증 ----

	bool             Undo();
	int32            UndoCount() const;
	HGameplayState Snapshot() const;
	void             Restore(const HGameplayState& state);
	// 네트워크 세션이 붙어 있는 동안 세션이 끈다. 한 기계만 되돌리면 즉시 어긋나기 때문이다.
	void SetUndoEnabled(bool bEnabled);
	bool IsUndoEnabled() const;

	bool Save(const PString& path) const;
	bool Load(const PString& path);
	// 저장 문서(초기 상태 + 현재 상태 + 명령 로그)를 파일 없이 텍스트로. 입장 · 재동기에 쓴다.
	bool ExportDocument(PString* outText) const;
	bool ImportDocument(const PString& text);
	bool Replay(const HList<HGameplayCommand>& commands, HList<HGameplayEvent>& outEvents);
	uint64 Checksum() const;
	// 등록된 규칙의 지문 (명령 · 효과 · 트리거 · 수정자 종류, 컴포넌트 테이블, 수치 단계, 보드, 흐름 이름, 스키마).
	// 기계마다 같은 규칙 세트인지 확인한다. 등록 순서와 무관하다.
	uint64 RulesFingerprint() const;
	const PGameplayCommandLog& GetCommandLog() const;
	const HGameplayState&      GetInitialState() const;

	// 상태 복사본에 명령을 적용한다. 자기 상태는 바꾸지 않는다 (에이전트 · 분석용).
	EGameplaySubmitResult Simulate(const HGameplayState& from, const HGameplayCommand& command, HGameplayState& outState, HList<HGameplayEvent>& outEvents);

	PGameplayRuleEngine&       GetEngine();
	const PGameplayRuleEngine& GetEngine() const;
	const IGameplayBoard*      GetBoard() const;

	// base 의 파생 클래스를 (중간 클래스 포함) 전부 모은다.
	static void CollectDerivedClasses(PSharedPtr<JGClass> base, HList<PSharedPtr<JGClass>>& outClasses);

private:
	bool registerTyped(PSharedPtr<JGGameplayCommandHandler> handler);
	bool registerTyped(PSharedPtr<JGGameplayEffect> effect);
	bool registerTyped(PSharedPtr<JGGameplayTrigger> trigger);
	bool registerTyped(PSharedPtr<JGGameplayModifier> modifier);

	void notifyEvents(const HList<HGameplayEvent>& events);
	void notifyStateReplaced();
};
