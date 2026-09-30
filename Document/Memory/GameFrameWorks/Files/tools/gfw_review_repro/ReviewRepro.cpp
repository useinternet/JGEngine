#include "PCH/PCH.h"
#include <iostream>
#include <chrono>
#include "Core.h"
#include "GameMaster/GameMaster.h"
#include "Core/World.h"
#include "Actors/Actor.h"
#include "Actors/GameplayEntityActor.h"
#include "Actors/GameplayCue.h"
#include "Actors/GameMasterActor.h"

// GameFrameWorks 리뷰(2026-09-28) 재현 코드. JGConsole.exe simreview 로 실행한다.
// 각 항목은 "버그가 재현되면 [REPRODUCED]" 를 찍는다. 실패 수를 돌려주지 않는다 (관찰용).
// 사용법: 이 파일을 Source/Programs/JGConsole/ 에 복사, Main.cpp 에 simreview 분기 추가, PreBuild 후 JGConsole 빌드.
// 2026-09-29 GFW TODO Phase 1 에 맞춰 고친 곳: 트리거 React 는 효과 적재만 되므로(HGameplayTriggerContext) R4 · R8 은
// 같은 상황을 "효과를 넣는 트리거" 로 만든다. const 상태로는 쓸 수 없으므로 R17 은 반환 타입을 컴파일 시점에 본다.
// R5 는 RequestChoice 의 반환값(거부 여부)을 본다.

namespace
{
	const char* ReviewValueName = "ReviewValue";
	const char* ReviewZoneUnits = "Units";
	const char* ReviewZoneGrave = "Graveyard";

	void report(const char* id, bool bReproduced, const PString& detail)
	{
		std::cout << (bReproduced ? "[REPRODUCED]     " : "[NOT REPRODUCED] ") << id << " : " << detail.GetRawString() << std::endl;
	}

	struct HReviewValue : public IJsonable
	{
		int32 Value = 0;

	protected:
		virtual void WriteJson(PJsonData& json) const override
		{
			json.AddMember("Value", Value);
		}
		virtual void ReadJson(const PJsonData& json) override
		{
			json.GetData("Value", &Value);
		}
	};

	int32 valueOf(const HGameplayState& state, const HGameplayEntityId& id)
	{
		const HReviewValue* value = state.Find<HReviewValue>(id);
		if (value == nullptr)
		{
			return INDEX_NONE;
		}
		return value->Value;
	}

	int32 findEvent(const HList<HGameplayEvent>& events, const char* kind, int32 from = 0)
	{
		PName name(kind);
		for (int32 i = from; i < (int32)events.size(); ++i)
		{
			if (events[i].Kind == name)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	// ---- 규칙 ----------------------------------------------------------------------

	// 테스트가 채워 둔 효과 요청 목록을 그대로 큐에 넣는 명령.
	class PReviewScriptHandler : public JGGameplayCommandHandler
	{
	public:
		HList<HGameplayEffectRequest> Script;

		virtual PName GetKind() const override
		{
			return PName("ReviewScript");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return true;
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			for (const HGameplayEffectRequest& request : Script)
			{
				ctx.Enqueue(request);
			}
		}
	};

	// 핸들러 안에서 선택을 요청하는 명령 (R5).
	class PReviewAskInHandler : public JGGameplayCommandHandler
	{
	public:
		HGameplayEntityId Candidate;
		bool bAccepted = false;   // RequestChoice 가 요청을 받았다고 답했나

		virtual PName GetKind() const override
		{
			return PName("ReviewAskInHandler");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return true;
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayChoice choice;
			choice.Chooser = command.Actor;
			choice.Kind    = PName("ReviewPick");
			choice.Candidates.push_back(Candidate);
			bAccepted = ctx.RequestChoice(choice);
		}
	};

	// 이벤트 하나를 내는 명령 (R7).
	class PReviewPingHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewPing");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return true;
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayEvent ping(PName("ReviewPinged"), command.Actor, HGameplayEntityId::None());
			ctx.Emit(ping);
		}
	};

	// 새 엔티티를 만들고 보드 좌표 · 값 컴포넌트를 준다.
	class PReviewSpawnPlaceEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewSpawnPlace");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayEntityId id = ctx.SpawnEntity(PName(ReviewZoneUnits));
			ctx.State.Board.SetPosition(id, request.Coord);
			HReviewValue value;
			value.Value = request.Param(0, 0);
			ctx.State.Add<HReviewValue>(id, value);
		}
	};

	class PReviewAddValueEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewAddValue");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HReviewValue value;
			value.Value = request.Param(0, 0);
			ctx.State.Add<HReviewValue>(request.Target, value);
		}
	};

	class PReviewSetValueEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewSetValue");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HReviewValue* value = ctx.State.Find<HReviewValue>(request.Target);
			if (value != nullptr)
			{
				value->Value = request.Param(0, 0);
			}
		}
	};

	// 선택 재진입을 모르는 평범한 피해 효과. 두 번 불리면 두 번 깎인다.
	class PReviewDamageEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewDamage");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HReviewValue* value = ctx.State.Find<HReviewValue>(request.Target);
			if (value == nullptr)
			{
				return;
			}
			int32 before = value->Value;
			value->Value = before - request.Param(0, 0);

			HGameplayEvent damaged(PName("ReviewDamaged"), request.Subject, request.Target);
			damaged.Before = before;
			damaged.After  = value->Value;
			ctx.Emit(damaged);
		}
	};

	// 해결 시점의 현재 행동자 · 페이즈를 이벤트로 남긴다.
	class PReviewProbeEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewProbe");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayEvent probe(PName("ReviewProbed"), ctx.State.Turn.CurrentActor, request.Target);
			probe.Amount = (int32)ctx.State.Turn.Phase;
			ctx.Emit(probe);
		}
	};

	class PReviewMarkEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewMark");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HReviewValue* value = ctx.State.Find<HReviewValue>(request.Target);
			if (value != nullptr)
			{
				value->Value = value->Value * 10 + request.Param(0, 0);
			}
		}
	};

	// 선택을 요청하고, 선택이 오면 아무것도 하지 않는 효과 (R4: 트리거가 넣는다).
	class PReviewAskEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewAsk");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			if (request.HasChoice() == true)
			{
				return;
			}
			HGameplayChoice choice;
			choice.Chooser = request.Subject;
			choice.Kind    = PName("ReviewReactionPick");
			choice.Candidates.push_back(request.Target);
			ctx.RequestChoice(choice);
		}
	};

	class PReviewSpawnAndKillEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewSpawnAndKill");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayEntityId id = ctx.SpawnEntity(PName(ReviewZoneUnits));
			ctx.DestroyEntity(id);
		}
	};

	// 피해 이벤트에 반응해 선택을 요청하는 트리거 (R4). 선택 요청은 효과를 넣어서 한다.
	class PReviewAskOnDamageTrigger : public JGGameplayTrigger
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewAskOnDamage");
		}
		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			return event.Kind == PName("ReviewDamaged") && event.After > 5;
		}
		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
		{
			ctx.Enqueue(HGameplayEffectRequest(PName("ReviewAsk"), event.Subject, event.Target));
		}
	};

	class PReviewProbeOnTurnEndedTrigger : public JGGameplayTrigger
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("ReviewProbeOnTurnEnded");
		}
		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			return event.Kind == PName(HGameplayBuiltin::EventTurnEnded);
		}
		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
		{
			HGameplayEffectRequest probe(PName("ReviewProbe"), event.Subject, event.Subject);
			ctx.Enqueue(probe);
		}
	};

	class PReviewInitiativeTrigger : public JGGameplayTrigger
	{
	public:
		HGameplayEntityId Target;

		virtual PName GetKind() const override
		{
			return PName("ReviewInitiative");
		}
		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			return event.Kind == PName(HGameplayBuiltin::EventRoundStarted);
		}
		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
		{
			HGameplayEffectRequest set(PName("ReviewSetValue"), HGameplayEntityId::None(), Target);
			set.Params.push_back(3);
			ctx.Enqueue(set);
		}
	};

	// 같은 우선순위 트리거 둘. 종류 이름 순서와 등록 순서를 반대로 둔다 (R7).
	class PReviewMarkTrigger : public JGGameplayTrigger
	{
	public:
		PName Kind;
		int32 Digit = 0;

		virtual PName GetKind() const override
		{
			return Kind;
		}
		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			return event.Kind == PName("ReviewPinged");
		}
		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
		{
			HGameplayEffectRequest mark(PName("ReviewMark"), event.Subject, event.Subject);
			mark.Params.push_back(Digit);
			ctx.Enqueue(mark);
		}
	};

	// 생성 이벤트마다 다시 생성하는 트리거. 스스로 80 번에서 멈춘다 (R8).
	// React 가 SpawnEntity 를 직접 부르던 길은 HGameplayTriggerContext 에 없으므로, 생성 효과를 넣는 연쇄로 같은 상황을 만든다.
	class PReviewRecursiveSpawnTrigger : public JGGameplayTrigger
	{
	public:
		int32 Count = 0;
		int32 Limit = 80;

		virtual PName GetKind() const override
		{
			return PName("ReviewRecursiveSpawn");
		}
		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			return event.Kind == PName(HGameplayBuiltin::EventEntitySpawned) && Count < Limit;
		}
		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
		{
			++Count;
			ctx.Enqueue(HGameplayEffectRequest(PName(HGameplayBuiltin::EffectSpawnEntity), HGameplayEntityId::None(), HGameplayEntityId::None()));
		}
	};

	PSharedPtr<PGameMaster> makeGameMaster(HGameplayEntityId* outA, HGameplayEntityId* outB, int32 valueA, int32 valueB)
	{
		PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
		sim->RegisterComponent<HReviewValue>(PName(ReviewValueName));
		sim->RegisterZone(PName(ReviewZoneUnits));
		sim->RegisterZone(PName(ReviewZoneGrave));
		sim->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName(ReviewZoneUnits)));

		HGameplayState& initial = sim->EditInitialState();
		HGameplayEntityId a = initial.CreateEntity();
		HGameplayEntityId b = initial.CreateEntity();
		HReviewValue va;
		va.Value = valueA;
		HReviewValue vb;
		vb.Value = valueB;
		initial.Add<HReviewValue>(a, va);
		initial.Add<HReviewValue>(b, vb);
		initial.Zone(PName(ReviewZoneUnits)).PushBack(a);
		initial.Zone(PName(ReviewZoneUnits)).PushBack(b);

		*outA = a;
		*outB = b;
		return sim;
	}

	// ---- 시나리오 -------------------------------------------------------------------

	// R1 · R2 · R3: 파괴된 엔티티 ID 로 들어온 내장/게임 효과가 살아 있는 엔티티의 데이터를 덮어쓴다.
	void reproStaleEntityId()
	{
		PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
		sim->RegisterComponent<HReviewValue>(PName(ReviewValueName));
		sim->RegisterZone(PName(ReviewZoneUnits));
		sim->RegisterZone(PName(ReviewZoneGrave));
		sim->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName(ReviewZoneUnits)));
		sim->SetBoard(EGameplayBoardKind::Square, 5, 5);

		PSharedPtr<PReviewScriptHandler> script = Allocate<PReviewScriptHandler>();
		sim->RegisterHandler(script);
		sim->RegisterEffect(Allocate<PReviewSpawnPlaceEffect>());
		sim->RegisterEffect(Allocate<PReviewAddValueEffect>());

		HGameplayState& initial = sim->EditInitialState();
		HGameplayEntityId a = initial.CreateEntity();
		HGameplayEntityId b = initial.CreateEntity();
		HReviewValue va;
		va.Value = 10;
		HReviewValue vb;
		vb.Value = 20;
		initial.Add<HReviewValue>(a, va);
		initial.Add<HReviewValue>(b, vb);
		initial.Zone(PName(ReviewZoneUnits)).PushBack(a);
		initial.Zone(PName(ReviewZoneUnits)).PushBack(b);
		initial.Board.SetPosition(a, HGameplayCoord(0, 0));
		initial.Board.SetPosition(b, HGameplayCoord(1, 1));
		sim->Start(1);

		// B 를 죽이고(인덱스 반환) → 새 엔티티 생성(같은 인덱스 재사용) → 뒤늦게 B 를 넉백 · 상태 부여 · 묘지로
		HGameplayEffectRequest kill(PName(HGameplayBuiltin::EffectDestroyEntity), a, b);
		HGameplayEffectRequest spawn(PName("ReviewSpawnPlace"), a, HGameplayEntityId::None());
		spawn.Coord = HGameplayCoord(2, 2);
		spawn.Params.push_back(50);
		HGameplayEffectRequest knockback(PName(HGameplayBuiltin::EffectSetBoardPosition), a, b);
		knockback.Coord = HGameplayCoord(3, 3);
		HGameplayEffectRequest status(PName("ReviewAddValue"), a, b);
		status.Params.push_back(7);
		HGameplayEffectRequest grave(PName(HGameplayBuiltin::EffectMoveToZone), a, b);
		grave.Tag = PName(ReviewZoneGrave);

		script->Script.push_back(kill);
		script->Script.push_back(spawn);
		script->Script.push_back(knockback);
		script->Script.push_back(status);
		script->Script.push_back(grave);

		HList<HGameplayEvent> events;
		PString reason;
		sim->Submit(HGameplayCommand(PName("ReviewScript"), a), events, &reason);

		const HGameplayState& state = sim->GetState();
		HGameplayEntityId spawned;
		int32 spawnedIndex = findEvent(events, HGameplayBuiltin::EventEntitySpawned);
		if (spawnedIndex != INDEX_NONE)
		{
			spawned = events[spawnedIndex].Subject;
		}

		bool bReused = spawned.Index == b.Index && spawned.Generation == b.Generation + 1;
		const HGameplayCoord* spawnedPos = state.Board.FindPosition(spawned);
		HGameplayEntityId at33 = state.Board.OccupantAt(HGameplayCoord(3, 3));
		HGameplayEntityId at22 = state.Board.OccupantAt(HGameplayCoord(2, 2));

		report("R1 SetBoardPosition(stale id)", spawnedPos == nullptr || at33 == b,
			PString::Format("new entity %s reuses B's index=%s, its position=%s (expected (2, 2)), occupant(3,3)=%s alive=%s, occupant(2,2)=%s",
				spawned.ToString(), bReused ? "yes" : "no", spawnedPos == nullptr ? "LOST" : spawnedPos->ToString(),
				at33.ToString(), at33.IsValid() == false ? "empty" : (state.IsAlive(at33) ? "yes" : "NO (phantom)"), at22.ToString()));

		int32 staleCount = 0;
		state.Each<HReviewValue>([&](const HGameplayEntityId& id, const HReviewValue& value)
		{
			if (state.IsAlive(id) == false)
			{
				++staleCount;
			}
		});
		report("R2 Add<T>(stale id)", valueOf(state, spawned) != 50 || staleCount > 0,
			PString::Format("new entity value=%d (expected 50), components on dead ids in Each<T>=%d", valueOf(state, spawned), staleCount));

		const HGameplayZone* graveZone = state.FindZone(PName(ReviewZoneGrave));
		bool bDeadInGrave = graveZone != nullptr && graveZone->Contains(b);
		int32 destroyedAt = findEvent(events, HGameplayBuiltin::EventEntityDestroyed);
		int32 zoneChangedAt = findEvent(events, HGameplayBuiltin::EventZoneChanged);
		report("R3 MoveToZone(stale id)", bDeadInGrave,
			PString::Format("dead B in Graveyard=%s, ZoneChanged event index %d after EntityDestroyed index %d",
				bDeadInGrave ? "yes" : "no", zoneChangedAt, destroyedAt));
	}

	// R4: 트리거가 요청한 선택이 "해결 중이던 효과" 의 선택으로 저장돼, 선택 후 그 효과가 다시 실행된다.
	void reproTriggerChoice()
	{
		HGameplayEntityId a;
		HGameplayEntityId b;
		PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 10, 10);
		PSharedPtr<PReviewScriptHandler> script = Allocate<PReviewScriptHandler>();
		sim->RegisterHandler(script);
		sim->RegisterEffect(Allocate<PReviewDamageEffect>());
		sim->RegisterEffect(Allocate<PReviewAskEffect>());
		sim->RegisterTrigger(Allocate<PReviewAskOnDamageTrigger>());
		sim->Start(1);

		HGameplayEffectRequest damage(PName("ReviewDamage"), a, b);
		damage.Params.push_back(3);
		script->Script.push_back(damage);

		HList<HGameplayEvent> events;
		PString reason;
		EGameplaySubmitResult first = sim->Submit(HGameplayCommand(PName("ReviewScript"), a), events, &reason);
		int32 afterFirst = valueOf(sim->GetState(), b);

		HGameplayCommand resolve(PName(HGameplayBuiltin::CommandResolveChoice), a);
		resolve.Targets.push_back(b);
		events.clear();
		EGameplaySubmitResult second = sim->Submit(resolve, events, &reason);
		int32 afterResolve = valueOf(sim->GetState(), b);

		report("R4 RequestChoice from trigger", first == EGameplaySubmitResult::PendingChoice && afterResolve != afterFirst,
			PString::Format("submit=%d, B after damage=%d, after ResolveChoice=%d (expected %d: the damage effect ran again), resolve result=%d %s",
				(int32)first, afterFirst, afterResolve, afterFirst, (int32)second, reason));
	}

	// R5: 핸들러 Execute 에서 RequestChoice 를 부르면 조용히 버려진다.
	void reproHandlerChoice()
	{
		HGameplayEntityId a;
		HGameplayEntityId b;
		PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 10, 10);
		PSharedPtr<PReviewAskInHandler> ask = Allocate<PReviewAskInHandler>();
		ask->Candidate = b;
		sim->RegisterHandler(ask);
		sim->Start(1);

		HList<HGameplayEvent> events;
		PString reason;
		EGameplaySubmitResult result = sim->Submit(HGameplayCommand(PName("ReviewAskInHandler"), a), events, &reason);
		report("R5 RequestChoice from handler", result == EGameplaySubmitResult::Executed && sim->GetPendingChoice() == nullptr && ask->bAccepted == true,
			PString::Format("submit result=%d (1=Executed, 2=PendingChoice), pending choice=%s, ChoiceRequested event=%s, RequestChoice returned %s",
				(int32)result, sim->GetPendingChoice() == nullptr ? "none" : "yes",
				findEvent(events, HGameplayBuiltin::EventChoiceRequested) == INDEX_NONE ? "none" : "yes",
				ask->bAccepted ? "accepted (then dropped silently)" : "rejected (error log)"));
	}

	// R6 · R9: 페이즈 전이는 Emit 안에서 동기로 끝나고, 트리거가 넣은 효과는 그 뒤에 해결된다.
	void reproPhaseOrdering()
	{
		HGameplayEntityId a;
		HGameplayEntityId b;
		PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 1, 2);
		sim->RegisterEffect(Allocate<PReviewProbeEffect>());
		sim->RegisterTrigger(Allocate<PReviewProbeOnTurnEndedTrigger>());

		HList<HGameplayEvent> startEvents;
		sim->Start(1, &startEvents);

		HList<HGameplayEvent> events;
		PString reason;
		sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), a), events, &reason);

		int32 probeAt   = findEvent(events, "ReviewProbed");
		int32 startedAt = findEvent(events, HGameplayBuiltin::EventTurnStarted);
		bool bProbeFound = probeAt != INDEX_NONE;
		HGameplayEntityId actorAtResolve = bProbeFound ? events[probeAt].Subject : HGameplayEntityId::None();
		int32 phaseAtResolve = bProbeFound ? events[probeAt].Amount : -1;
		report("R6a end-of-turn effect resolves in next turn", bProbeFound && (actorAtResolve != a || phaseAtResolve != (int32)EGameplayPhase::TurnEnd),
			PString::Format("effect from TurnEnded(A=%s) resolved with CurrentActor=%s phase=%d (TurnEnd=5, TurnMain=4); probe event index %d, next TurnStarted index %d",
				a.ToString(), actorAtResolve.ToString(), phaseAtResolve, probeAt, startedAt));

		// R9
		uint32 startCause = startEvents.empty() ? 999 : startEvents[0].CauseSequence;
		uint32 firstCause = events.empty() ? 999 : events[0].CauseSequence;
		report("R9 CauseSequence of Start == first command", startCause == firstCause,
			PString::Format("Start events cause=%u, first command events cause=%u", startCause, firstCause));

		// R6b: 라운드 시작 트리거가 이니셔티브를 바꿔도 이번 라운드 순서에 반영되지 않는다.
		HGameplayEntityId c;
		HGameplayEntityId d;
		PSharedPtr<PGameMaster> keyed = makeGameMaster(&c, &d, 1, 2);
		keyed->RegisterEffect(Allocate<PReviewSetValueEffect>());
		PSharedPtr<PReviewInitiativeTrigger> initiative = Allocate<PReviewInitiativeTrigger>();
		initiative->Target = c;
		keyed->RegisterTrigger(initiative);
		PGameplayKeyedOrderPolicy::HKeyFunction keyFunction = [](const HGameplayState& state, const HGameplayEntityId& id)
		{
			return valueOf(state, id);
		};
		keyed->SetOrderPolicy(Allocate<PGameplayKeyedOrderPolicy>(PName(ReviewZoneUnits), keyFunction, true));
		keyed->Start(1);

		const HGameplayTurnState& turn = keyed->GetState().Turn;
		bool bStale = turn.Order.size() == 2 && turn.Order[0] == c;
		report("R6b RoundStarted effects resolve after OrderResolve", bStale,
			PString::Format("C initiative set to %d by a RoundStarted trigger (D=%d), round-1 order=[%s, %s] (ascending key expects D first)",
				valueOf(keyed->GetState(), c), valueOf(keyed->GetState(), d),
				turn.Order.size() > 0 ? turn.Order[0].ToString() : PString("-"), turn.Order.size() > 1 ? turn.Order[1].ToString() : PString("-")));
	}

	// R7: Load 는 레지스트리를 정렬(Finalize)하지 않아, 같은 우선순위 트리거 순서가 원래 세션과 달라진다.
	void setupMarkGameMaster(PGameMaster& sim, HGameplayEntityId* outActor)
	{
		sim.RegisterComponent<HReviewValue>(PName(ReviewValueName));
		sim.RegisterZone(PName(ReviewZoneUnits));
		sim.SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName(ReviewZoneUnits)));
		sim.RegisterHandler(Allocate<PReviewPingHandler>());
		sim.RegisterEffect(Allocate<PReviewMarkEffect>());

		PSharedPtr<PReviewMarkTrigger> markB = Allocate<PReviewMarkTrigger>();
		markB->Kind  = PName("ReviewMarkB");
		markB->Digit = 2;
		PSharedPtr<PReviewMarkTrigger> markA = Allocate<PReviewMarkTrigger>();
		markA->Kind  = PName("ReviewMarkA");
		markA->Digit = 1;
		sim.RegisterTrigger(markB);   // 등록은 B 먼저
		sim.RegisterTrigger(markA);

		HGameplayState& initial = sim.EditInitialState();
		HGameplayEntityId actor = initial.CreateEntity();
		initial.Add<HReviewValue>(actor, HReviewValue());
		initial.Zone(PName(ReviewZoneUnits)).PushBack(actor);
		*outActor = actor;
	}

	void reproLoadWithoutFinalize()
	{
		HGameplayEntityId actor;
		PSharedPtr<PGameMaster> original = Allocate<PGameMaster>();
		setupMarkGameMaster(*original, &actor);
		original->Start(1);
		original->Save("ReviewR7.json");

		HList<HGameplayEvent> events;
		PString reason;
		original->Submit(HGameplayCommand(PName("ReviewPing"), actor), events, &reason);
		int32 originalValue = valueOf(original->GetState(), actor);

		HGameplayEntityId loadedActor;
		PSharedPtr<PGameMaster> loaded = Allocate<PGameMaster>();
		setupMarkGameMaster(*loaded, &loadedActor);
		bool bLoaded = loaded->Load("ReviewR7.json");
		events.clear();
		loaded->Submit(HGameplayCommand(PName("ReviewPing"), actor), events, &reason);
		int32 loadedValue = valueOf(loaded->GetState(), actor);

		report("R7 Load skips Finalize", bLoaded && originalValue != loadedValue,
			PString::Format("same command after Start: marks=%d, after Load (no Start): marks=%d, checksum %s",
				originalValue, loadedValue, original->Checksum() == loaded->Checksum() ? "equal" : "DIFFERENT"));
	}

	// R8: React 안의 직접 Emit (SpawnEntity) 연쇄는 깊이 상한에 걸리지 않고 재귀한다.
	void reproEmitRecursion()
	{
		HGameplayEntityId a;
		HGameplayEntityId b;
		PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 1, 1);
		PSharedPtr<PReviewScriptHandler> script = Allocate<PReviewScriptHandler>();
		sim->RegisterHandler(script);
		PSharedPtr<PReviewRecursiveSpawnTrigger> spawner = Allocate<PReviewRecursiveSpawnTrigger>();
		sim->RegisterTrigger(spawner);
		sim->Start(1);

		script->Script.push_back(HGameplayEffectRequest(PName(HGameplayBuiltin::EffectSpawnEntity), a, HGameplayEntityId::None()));
		HList<HGameplayEvent> events;
		PString reason;
		sim->Submit(HGameplayCommand(PName("ReviewScript"), a), events, &reason);

		int32 maxDepth = 0;
		for (const HGameplayEvent& event : events)
		{
			if (event.Depth > maxDepth)
			{
				maxDepth = event.Depth;
			}
		}
		bool bNoGuard = spawner->Count == spawner->Limit && findEvent(events, HGameplayBuiltin::EventEffectDepthExceeded) == INDEX_NONE;
		report("R8 Emit recursion has no depth guard", bNoGuard,
			PString::Format("spawn -> trigger -> spawn chain length=%d (the trigger stops itself at 80), max event depth=%d, MaxEffectDepth=%d, EffectDepthExceeded=%s",
				spawner->Count, maxDepth, HGameplayLimits::MaxEffectDepth,
				findEvent(events, HGameplayBuiltin::EventEffectDepthExceeded) == INDEX_NONE ? "none" : "yes"));
	}

	// R10: 스냅샷 스택이 한도에 닿으면 매 명령마다 앞에서 지우며 상태 (한도 - 1) 개를 깊은 복사한다.
	// 기본 한도 256 은 HEAD 의 고정 풀에서 16B 클래스(디버그 컨테이너 프록시)를 고갈시켜 약 95 명령에서 크래시하므로(R16) 여기서는 40 으로 잰다.
	void reproSnapshotCost()
	{
		HGameplayEntityId a;
		HGameplayEntityId b;
		PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 1, 1);
		sim->SetSnapshotLimit(40);
		HGameplayState& initial = sim->EditInitialState();
		for (int32 i = 0; i < 300; ++i)
		{
			HGameplayEntityId extra = initial.CreateEntity();
			HReviewValue value;
			value.Value = i;
			initial.Add<HReviewValue>(extra, value);
		}
		sim->Start(1);

		auto timeSubmits = [&](int32 count) -> float64
		{
			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			for (int32 i = 0; i < count; ++i)
			{
				HList<HGameplayEvent> events;
				PString reason;
				HGameplayEntityId actor = sim->GetState().Turn.CurrentActor;
				sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), actor), events, &reason);
			}
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			return std::chrono::duration<float64, std::milli>(end - begin).count() / (float64)count;
		};

		float64 early = timeSubmits(20);      // 스냅샷 1..20
		timeSubmits(40);                       // 한도 40 도달
		float64 late = timeSubmits(100);      // 한도에서 매번 erase(begin) → 상태 39 개 복사 대입
		report("R10 snapshot stack at limit", late > early * 3.0,
			PString::Format("avg Submit %.3f ms with <=20 snapshots, %.3f ms at the limit of 40 (x%.1f), state has %u entities",
				early, late, late / early, sim->GetState().Entities.Count()));
	}

	// R17: const HGameplayState& 로 받은 상태를 Get/Find 로 바꿀 수 있다 (명령 없이 체크섬이 바뀐다).
	// 수정 전에는 readOnly.Get<T>(a).Value = 99 가 컴파일되고 값이 바뀌었다. 수정 후에는 그 코드가 컴파일되지 않으므로 반환 타입을 본다.
	void reproConstStateWrite()
	{
		HGameplayEntityId a;
		HGameplayEntityId b;
		PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 10, 20);
		sim->Start(1);

		const HGameplayState& readOnly = sim->GetState();
		constexpr bool bGetWritable  = std::is_const<std::remove_reference_t<decltype(readOnly.Get<HReviewValue>(a))>>::value == false;
		constexpr bool bFindWritable = std::is_const<std::remove_pointer_t<decltype(readOnly.Find<HReviewValue>(b))>>::value == false;

		report("R17 write through const state", bGetWritable || bFindWritable,
			PString::Format("through const HGameplayState&: Get<T> returns %s, Find<T> returns %s",
				bGetWritable ? "T& (writable)" : "const T&", bFindWritable ? "T* (writable)" : "const T*"));
	}

	// ---- 월드 · 액터 · Director --------------------------------------------------------

	class PReviewSpawnerActor : public JGActor
	{
	public:
		PSharedPtr<JGActor> Spawned;
		bool bDone = false;

	protected:
		virtual void OnTick(float32 deltaSeconds) override
		{
			if (bDone == true)
			{
				return;
			}
			bDone = true;
			PSharedPtr<PWorld> world = GetWorld();
			Spawned = world->SpawnActor<JGActor>(PName("ReviewZombie"));
			Spawned->Destroy();
		}
	};

	bool worldContains(PSharedPtr<PWorld> world, PSharedPtr<JGActor> actor)
	{
		for (const PSharedPtr<JGActor>& candidate : world->GetActors())
		{
			if (candidate == actor)
			{
				return true;
			}
		}
		return false;
	}

	// R11: 틱 중에 스폰하고 같은 틱에 파괴한 액터가 다음 틱에 월드에 들어가 BeginPlay 되고 영영 남는다.
	void reproWorldZombie()
	{
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("ReviewWorld"));
		world->BeginPlay();
		PSharedPtr<PReviewSpawnerActor> spawner = world->SpawnActor<PReviewSpawnerActor>(PName("Spawner"));
		world->Tick(1.0f / 60.0f);
		world->Tick(1.0f / 60.0f);
		world->Tick(1.0f / 60.0f);

		PSharedPtr<JGActor> zombie = spawner->Spawned;
		bool bInWorld = zombie != nullptr && worldContains(world, zombie);
		report("R11 spawn+destroy in one tick leaves a zombie", bInWorld && zombie->IsPendingDestroy() == true,
			PString::Format("destroyed actor still in world=%s, HasBegunPlay=%s, IsPendingDestroy=%s, GetWorld()=%s",
				bInWorld ? "yes" : "no", zombie != nullptr && zombie->HasBegunPlay() ? "yes" : "no",
				zombie != nullptr && zombie->IsPendingDestroy() ? "yes" : "no", zombie != nullptr && zombie->GetWorld() == nullptr ? "null" : "set"));
		world->EndPlay();
	}

	// R12: 부모가 있는 액터의 GetWorldPosition 이 행렬의 이동 성분이 아닌 4열을 읽는다.
	void reproWorldPosition()
	{
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("ReviewWorld"));
		PSharedPtr<JGActor> parent = world->SpawnActor<JGActor>(PName("Parent"));
		PSharedPtr<JGActor> child = world->SpawnActor<JGActor>(PName("Child"));
		parent->SetLocalPosition(HVector3(5.0f, 0.0f, 0.0f));
		child->SetLocalPosition(HVector3(1.0f, 2.0f, 3.0f));
		child->AttachTo(parent);

		HVector3 position = child->GetWorldPosition();
		HMatrix matrix = child->GetWorldMatrix();
		report("R12 child GetWorldPosition", position.x != 6.0f || position.y != 2.0f || position.z != 3.0f,
			PString::Format("GetWorldPosition=(%.1f, %.1f, %.1f), expected (6, 2, 3); matrix row 3 = (%.1f, %.1f, %.1f)",
				position.x, position.y, position.z, matrix.Get_C(0, 3), matrix.Get_C(1, 3), matrix.Get_C(2, 3)));
	}

	// R13 · R14: 사망 이벤트의 큐가 불리기 전에 액터가 이미 파괴 · 해제된다. 리플렉션 없는 큐 파생은 기본 큐로 재생된다.
	class PReviewDeathProbeCue : public JGGameplayCue
	{
	public:
		JGGameMasterActor* Director = nullptr;
		int32* OutAsked = nullptr;
		int32* OutFound = nullptr;
		int32* OutBegun = nullptr;

		virtual bool Accepts(const HGameplayEvent& event) const override
		{
			if (event.Kind != PName(HGameplayBuiltin::EventEntityDestroyed))
			{
				return false;
			}
			++(*OutAsked);
			if (Director->FindActor(event.Subject) != nullptr)
			{
				++(*OutFound);
			}
			return true;
		}

		virtual void Begin(const HGameplayEvent& event, JGGameMasterActor& gameMasterActor) override
		{
			if (OutBegun != nullptr)
			{
				++(*OutBegun);
			}
		}
	};

	void tickWorld(PSharedPtr<PWorld> world, int32 frames)
	{
		for (int32 i = 0; i < frames; ++i)
		{
			world->Tick(1.0f / 60.0f);
		}
	}

	void reproDirectorDeathCue()
	{
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("ReviewWorld"));
		world->BeginPlay();
		PSharedPtr<JGGameMasterActor> gameMasterActor = world->SpawnActor<JGGameMasterActor>(PName("Director"));

		HGameplayEntityId a;
		HGameplayEntityId b;
		PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 10, 10);
		PSharedPtr<PReviewScriptHandler> script = Allocate<PReviewScriptHandler>();
		sim->RegisterHandler(script);
		gameMasterActor->SetGameMaster(sim);

		int32 asked = 0;
		int32 found = 0;
		int32 begun = 0;
		PSharedPtr<PReviewDeathProbeCue> cue = Allocate<PReviewDeathProbeCue>();
		cue->Director = gameMasterActor.GetRawPointer();
		cue->OutAsked = &asked;
		cue->OutFound = &found;
		cue->OutBegun = &begun;
		gameMasterActor->RegisterCue(cue);

		sim->Start(1);
		tickWorld(world, 3);
		bool bBound = gameMasterActor->FindActor(b) != nullptr;

		script->Script.push_back(HGameplayEffectRequest(PName(HGameplayBuiltin::EffectDestroyEntity), a, b));
		PString reason;
		gameMasterActor->Submit(HGameplayCommand(PName("ReviewScript"), a), &reason);
		tickWorld(world, 3);

		report("R13 death cue cannot find its actor", bBound && asked > 0 && found == 0,
			PString::Format("B bound before=%s, cue asked for EntityDestroyed %d time(s), actor found at that moment %d time(s)",
				bBound ? "yes" : "no", asked, found));
		report("R14 non-reflected cue plays as base class", asked > 0 && begun == 0,
			PString::Format("prototype accepted %d event(s), derived Begin ran %d time(s) (CreateInstance makes GetClass()=JGGameplayCue)", asked, begun));

		world->EndPlay();
	}

	// R15: Director 가 한 프레임에 EntitySpawned · EntityDestroyed 를 연달아 처리하면 R11 좀비가 생긴다.
	void reproDirectorZombie()
	{
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("ReviewWorld"));
		world->BeginPlay();
		PSharedPtr<JGGameMasterActor> gameMasterActor = world->SpawnActor<JGGameMasterActor>(PName("Director"));

		HGameplayEntityId a;
		HGameplayEntityId b;
		PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 10, 10);
		PSharedPtr<PReviewScriptHandler> script = Allocate<PReviewScriptHandler>();
		sim->RegisterHandler(script);
		sim->RegisterEffect(Allocate<PReviewSpawnAndKillEffect>());
		gameMasterActor->SetGameMaster(sim);
		sim->Start(1);
		tickWorld(world, 3);

		script->Script.push_back(HGameplayEffectRequest(PName("ReviewSpawnAndKill"), a, HGameplayEntityId::None()));
		PString reason;
		gameMasterActor->Submit(HGameplayCommand(PName("ReviewScript"), a), &reason);
		tickWorld(world, 5);

		int32 zombies = 0;
		int32 entityActors = 0;
		for (const PSharedPtr<JGActor>& actor : world->GetActors())
		{
			PSharedPtr<JGGameplayEntityActor> typed = RawDynamicCast<JGGameplayEntityActor>(actor);
			if (typed == nullptr)
			{
				continue;
			}
			++entityActors;
			if (typed->IsPendingDestroy() == true)
			{
				++zombies;
			}
		}
		report("R15 gameMasterActor spawn+destroy in one frame", zombies > 0,
			PString::Format("entity actors in world=%d (alive entities=%u), destroyed-but-present actors=%d",
				entityActors, sim->GetState().Entities.Count(), zombies));
		world->EndPlay();
	}
}

// R16: 기본 스냅샷 한도(256)로 엔티티 2 개짜리 상태를 계속 진행한다. HEAD 의 고정 풀에서는 이 수준에서 크래시한다.
// 프로세스가 죽으므로 별도 명령(simreview256)으로 실행한다.
int32 GameFrameWorksReviewSnapshotLimit()
{
	HGameplayEntityId a;
	HGameplayEntityId b;
	PSharedPtr<PGameMaster> sim = makeGameMaster(&a, &b, 1, 1);
	sim->Start(1);
	for (int32 i = 1; i <= 400; ++i)
	{
		HList<HGameplayEvent> events;
		PString reason;
		HGameplayEntityId actor = sim->GetState().Turn.CurrentActor;
		sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), actor), events, &reason);
		if (i % 25 == 0)
		{
			std::cout << "R16 submitted " << i << " commands, snapshots=" << sim->UndoCount() << std::endl;
		}
	}
	std::cout << "R16 finished 400 commands without crash" << std::endl;
	return 0;
}

int32 GameFrameWorksReviewRepro()
{
	std::cout << "== GameFrameWorks review repro ==" << std::endl;
	reproStaleEntityId();
	reproTriggerChoice();
	reproHandlerChoice();
	reproPhaseOrdering();
	reproLoadWithoutFinalize();
	reproEmitRecursion();
	reproConstStateWrite();
	reproWorldZombie();
	reproWorldPosition();
	reproDirectorDeathCue();
	reproDirectorZombie();
	std::cout << "== done ==" << std::endl;
	return 0;
}

// R10 은 새 프로세스에서 따로 잰다 (simreviewperf). JGConsole 은 GCoreSystem::Update 를 부르지 않아 GC 가 돌지 않으므로,
// 앞 시나리오의 GC 객체가 종료 때까지 남아 HEAD 의 고정 풀을 먼저 채우기 때문이다.
int32 GameFrameWorksReviewSnapshotCost()
{
	reproSnapshotCost();
	return 0;
}
