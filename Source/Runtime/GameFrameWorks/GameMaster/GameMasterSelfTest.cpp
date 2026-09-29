#include "PCH/PCH.h"
#include "GameMaster/GameMasterSelfTest.h"
#include "GameMaster/GameMaster.h"
#include "GameMaster/Agents/GameplayAgent.h"
#include <iostream>

// ---- 테스트 전용 최소 규칙 ---------------------------------------------------------
// 리플렉션 등록 없이 수동 등록한다 (자체 테스트가 리플렉션 유무에 좌우되지 않도록).

namespace
{
	const char* TestValueName        = "TestValue";
	const char* TestZoneUnits        = "Units";
	const char* TestCommandAct       = "TestAct";
	const char* TestCommandPickAct   = "TestPickAct";
	const char* TestEffectChange     = "TestChange";
	const char* TestEffectHeal       = "TestHeal";
	const char* TestEffectPick       = "TestPick";
	const char* TestEventChanged     = "TestValueChanged";

	struct HGameplayTestValue : public IJsonable
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

	class PTestActHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(TestCommandAct);
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (state.Turn.IsActorTurn(command.Actor) == false || state.Turn.CanAct() == false)
			{
				if (outReason != nullptr)
				{
					*outReason = "not your turn";
				}
				return false;
			}
			HGameplayEntityId target = command.Target();
			if (state.IsAlive(target) == false || state.Has<HGameplayTestValue>(target) == false || target == command.Actor)
			{
				if (outReason != nullptr)
				{
					*outReason = "invalid target";
				}
				return false;
			}
			return true;
		}

		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayEffectRequest request(PName(TestEffectChange), command.Actor, command.Target());
			request.Params = command.Params;
			ctx.Enqueue(request);
		}

		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			state.Each<HGameplayTestValue>([&](const HGameplayEntityId& id, const HGameplayTestValue&)
			{
				if (id == actor)
				{
					return;
				}
				HGameplayCommand command(GetKind(), actor);
				command.Targets.push_back(id);
				command.Params.push_back(3);
				outCommands.push_back(command);
			});
		}
	};

	class PTestChangeEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(TestEffectChange);
		}

		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayTestValue* value = ctx.State.Find<HGameplayTestValue>(request.Target);
			if (value == nullptr)
			{
				return;
			}

			int32 before = value->Value;
			int32 amount = ctx.Compute(PName(TestValueName), request.Subject, request.Target, request.Param(0, 0));
			int32 after  = before - amount;
			if (after < 0)
			{
				after = 0;
			}
			value->Value = after;

			HGameplayEvent changed(PName(TestEventChanged), request.Subject, request.Target);
			changed.Before = before;
			changed.After  = after;
			changed.Amount = amount;
			ctx.Emit(changed);

			if (after == 0)
			{
				HGameplayEffectRequest destroy(PName(HGameplayBuiltin::EffectDestroyEntity), request.Subject, request.Target);
				ctx.Enqueue(destroy);
			}
		}
	};

	class PTestHealEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(TestEffectHeal);
		}

		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayTestValue* value = ctx.State.Find<HGameplayTestValue>(request.Target);
			if (value == nullptr)
			{
				return;
			}
			int32 before = value->Value;
			value->Value = before + request.Param(0, 0);

			HGameplayEvent changed(PName(TestEventChanged), request.Subject, request.Target);
			changed.Before = before;
			changed.After  = value->Value;
			changed.Amount = -request.Param(0, 0);
			ctx.Emit(changed);
		}
	};

	// 값이 0 이 된 이벤트에 반응해 공격자를 1 회복시킨다. (트리거 연쇄 · 깊이 검사용)
	class PTestOnZeroTrigger : public JGGameplayTrigger
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestOnZero");
		}

		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			return event.Kind == PName(TestEventChanged) && event.After == 0 && event.Amount > 0 && state.IsAlive(event.Subject) == true;
		}

		virtual void React(HGameplayContext& ctx, const HGameplayEvent& event) override
		{
			HGameplayEffectRequest heal(PName(TestEffectHeal), event.Subject, event.Subject);
			heal.Params.push_back(1);
			ctx.Enqueue(heal);
		}
	};

	// Add 단계에서 +2. (수치 파이프라인 검사용)
	class PTestBonusModifier : public JGGameplayModifier
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestBonus");
		}
		virtual PName GetValueKind() const override
		{
			return PName(TestValueName);
		}
		virtual PName GetStage() const override
		{
			return PName("Add");
		}
		virtual bool Applies(const HGameplayContext& ctx, const HGameplayValueQuery& query) const override
		{
			return true;
		}
		virtual int32 Apply(const HGameplayContext& ctx, const HGameplayValueQuery& query, int32 value) const override
		{
			return value + 2;
		}
	};

	// 선택 대기 재진입 검사용. 자기 외 유닛 하나를 고르게 하고 고른 대상에 1 피해.
	class PTestPickEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(TestEffectPick);
		}

		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			if (request.HasChoice() == false)
			{
				HGameplayChoice choice;
				choice.Chooser = request.Subject;
				choice.Kind    = PName("TestPick");
				choice.Min     = 1;
				choice.Max     = 1;

				const HGameplayZone* zone = ctx.State.FindZone(PName(TestZoneUnits));
				if (zone != nullptr)
				{
					for (const HGameplayEntityId& id : zone->Entities)
					{
						if (id != request.Subject && ctx.State.IsAlive(id) == true)
						{
							choice.Candidates.push_back(id);
						}
					}
				}
				ctx.RequestChoice(choice);
				return;
			}

			HGameplayEffectRequest change(PName(TestEffectChange), request.Subject, request.Choice[0]);
			change.Params.push_back(1);
			ctx.Enqueue(change);
		}
	};

	class PTestPickHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(TestCommandPickAct);
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (state.Turn.IsActorTurn(command.Actor) == false || state.Turn.CanAct() == false)
			{
				if (outReason != nullptr)
				{
					*outReason = "not your turn";
				}
				return false;
			}
			return true;
		}

		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayEffectRequest pick(PName(TestEffectPick), command.Actor, HGameplayEntityId::None());
			ctx.Enqueue(pick);
		}

		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			outCommands.push_back(HGameplayCommand(GetKind(), actor));
		}
	};

	class PTestEvaluator : public IGameplayEvaluator
	{
	public:
		virtual int32 Evaluate(const HGameplayState& state, const HGameplayEntityId& actor) const override
		{
			int32 score = 0;
			state.Each<HGameplayTestValue>([&](const HGameplayEntityId& id, const HGameplayTestValue& value)
			{
				if (id == actor)
				{
					score += value.Value;
				}
				else
				{
					score -= value.Value;
				}
			});
			return score;
		}
	};

	// ---- 시나리오 ----

	void setupGameMaster(PGameMaster& sim)
	{
		sim.RegisterComponent<HGameplayTestValue>(PName(TestValueName));
		sim.RegisterZone(PName(TestZoneUnits));

		HList<PName> stages;
		stages.push_back(PName("Base"));
		stages.push_back(PName("Add"));
		stages.push_back(PName("Multiply"));
		stages.push_back(PName("Clamp"));
		sim.DefineValueStages(PName(TestValueName), stages);

		sim.RegisterHandler(Allocate<PTestActHandler>());
		sim.RegisterHandler(Allocate<PTestPickHandler>());
		sim.RegisterEffect(Allocate<PTestChangeEffect>());
		sim.RegisterEffect(Allocate<PTestHealEffect>());
		sim.RegisterEffect(Allocate<PTestPickEffect>());
		sim.RegisterTrigger(Allocate<PTestOnZeroTrigger>());
		sim.RegisterModifier(Allocate<PTestBonusModifier>());

		sim.SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName(TestZoneUnits)));
		sim.SetBoard(EGameplayBoardKind::None);

		HGameplayState& initial = sim.EditInitialState();
		for (int32 i = 0; i < 3; ++i)
		{
			HGameplayEntityId id = initial.CreateEntity();
			HGameplayTestValue value;
			value.Value = 10;
			initial.Add<HGameplayTestValue>(id, value);
			initial.Zone(PName(TestZoneUnits)).PushBack(id);
		}
	}

	HList<HGameplayEntityId> unitsOf(const PGameMaster& sim)
	{
		HList<HGameplayEntityId> ids;
		const HGameplayZone* zone = sim.GetState().FindZone(PName(TestZoneUnits));
		if (zone != nullptr)
		{
			zone->CollectTo(ids);
		}
		return ids;
	}

	int32 valueOf(const PGameMaster& sim, const HGameplayEntityId& id)
	{
		const HGameplayTestValue* value = sim.GetState().Find<HGameplayTestValue>(id);
		if (value == nullptr)
		{
			return INDEX_NONE;
		}
		return value->Value;
	}

	bool hasEvent(const HList<HGameplayEvent>& events, const char* kind)
	{
		PName name(kind);
		for (const HGameplayEvent& event : events)
		{
			if (event.Kind == name)
			{
				return true;
			}
		}
		return false;
	}

	struct HCheck
	{
		int32 Failures = 0;
		int32 Passed   = 0;

		void operator()(bool bCondition, const PString& what)
		{
			if (bCondition == true)
			{
				++Passed;
				std::cout << "  [PASS] " << what.GetRawString() << std::endl;
				return;
			}
			++Failures;
			std::cout << "  [FAIL] " << what.GetRawString() << std::endl;
			JG_LOG(GameMasterSelfTest, ELogLevel::Error, "[FAIL] %s", what);
		}
	};
}

int32 PGameMasterSelfTest::Run()
{
	HCheck check;
	const uint64 seed = 12345;

	std::cout << "== GameMaster self test ==" << std::endl;

	// 1. 기본 시나리오 + 결정론 -------------------------------------------------------
	uint64 checksums[3] = { 0, 0, 0 };
	HList<HGameplayCommand> recordedLog;

	for (int32 run = 0; run < 3; ++run)
	{
		PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
		setupGameMaster(*sim);

		HList<HGameplayEvent> startEvents;
		sim->Start(seed, &startEvents);
		HList<HGameplayEntityId> units = unitsOf(*sim);

		if (run == 0)
		{
			check(units.size() == 3, "initial state has 3 units");
			check(hasEvent(startEvents, HGameplayBuiltin::EventGameStarted), "start emits GameStarted");
			check(hasEvent(startEvents, HGameplayBuiltin::EventTurnStarted), "start emits TurnStarted");
			check(sim->GetState().Turn.Phase == EGameplayPhase::TurnMain, "phase is TurnMain after start");
			check(sim->GetState().Turn.CurrentActor == units[0], "first actor is first unit in zone");

			HList<HGameplayCommand> legal;
			sim->EnumerateLegal(units[0], legal);
			check(legal.size() == 4, PString::Format("EnumerateLegal for first actor = 4 (EndTurn + 2 TestAct + TestPickAct), got %d", (int32)legal.size()));
		}

		HList<HGameplayEvent> events;
		PString reason;

		HGameplayCommand act(PName(TestCommandAct), units[0]);
		act.Targets.push_back(units[1]);
		act.Params.push_back(3);
		EGameplaySubmitResult result = sim->Submit(act, events, &reason);
		if (run == 0)
		{
			check(result == EGameplaySubmitResult::Executed, PString::Format("TestAct executed (%s)", reason));
			check(valueOf(*sim, units[1]) == 5, PString::Format("target value 10 - (3 + 2 bonus) = 5, got %d", valueOf(*sim, units[1])));
			check(hasEvent(events, TestEventChanged), "TestValueChanged emitted");
			check(events.size() > 0 && events[0].CauseSequence == 0, "event cause sequence is 0 for first command");

			HGameplayCommand wrongTurn(PName(TestCommandAct), units[1]);
			wrongTurn.Targets.push_back(units[0]);
			wrongTurn.Params.push_back(1);
			HList<HGameplayEvent> ignored;
			check(sim->Submit(wrongTurn, ignored, &reason) == EGameplaySubmitResult::Rejected, PString::Format("acting out of turn is rejected (%s)", reason));
		}

		events.clear();
		result = sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), units[0]), events, &reason);
		if (run == 0)
		{
			check(result == EGameplaySubmitResult::Executed, "EndTurn executed");
			check(sim->GetState().Turn.CurrentActor == units[1], "current actor advanced to second unit");
			check(hasEvent(events, HGameplayBuiltin::EventTurnEnded) && hasEvent(events, HGameplayBuiltin::EventTurnStarted), "EndTurn emits TurnEnded and TurnStarted");
		}

		events.clear();
		HGameplayCommand kill(PName(TestCommandAct), units[1]);
		kill.Targets.push_back(units[2]);
		kill.Params.push_back(10);
		result = sim->Submit(kill, events, &reason);
		if (run == 0)
		{
			check(result == EGameplaySubmitResult::Executed, "lethal TestAct executed");
			check(sim->GetState().IsAlive(units[2]) == false, "target destroyed at 0");
			check(hasEvent(events, HGameplayBuiltin::EventEntityDestroyed), "EntityDestroyed emitted");
			check(valueOf(*sim, units[1]) == 6, PString::Format("trigger healed attacker 5 -> 6, got %d", valueOf(*sim, units[1])));

			bool bDepthOk = false;
			for (const HGameplayEvent& event : events)
			{
				if (event.Kind == PName(TestEventChanged) && event.Amount < 0)
				{
					bDepthOk = (event.Depth >= 1);
				}
			}
			check(bDepthOk, "trigger reaction event has depth >= 1");
		}

		checksums[run] = sim->Checksum();
		if (run == 0)
		{
			recordedLog = sim->GetCommandLog().Commands;
			check(recordedLog.size() == 3, "command log has 3 commands");

			// 2. 되돌리기 ------------------------------------------------------------
			uint64 before = sim->Checksum();
			HList<HGameplayEvent> undoEvents;
			sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), units[1]), undoEvents, &reason);
			check(sim->Checksum() != before, "state changed after extra EndTurn");
			check(sim->Undo() == true, "Undo succeeds");
			check(sim->Checksum() == before, "checksum restored after Undo");
			check(sim->GetCommandLog().Count() == 3, "command log restored after Undo");

			// 3. 저장 / 로드 ----------------------------------------------------------
			PString savePath = "GameMasterSelfTest_Save.json";
			check(sim->Save(savePath) == true, "Save succeeds");

			PSharedPtr<PGameMaster> loaded = Allocate<PGameMaster>();
			setupGameMaster(*loaded);
			check(loaded->Load(savePath) == true, "Load succeeds");
			check(loaded->Checksum() == before, "loaded checksum equals saved checksum");
			check(loaded->GetCommandLog().Count() == 3, "loaded command log has 3 commands");

			HList<HGameplayEvent> afterLoad;
			check(loaded->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), units[1]), afterLoad, &reason) == EGameplaySubmitResult::Executed, "loaded gameMaster accepts commands");

			// 4. 리플레이 -------------------------------------------------------------
			PSharedPtr<PGameMaster> replayed = Allocate<PGameMaster>();
			setupGameMaster(*replayed);
			replayed->Start(seed);
			HList<HGameplayEvent> replayEvents;
			check(replayed->Replay(recordedLog, replayEvents) == true, "Replay succeeds");
			check(replayed->Checksum() == before, "replayed checksum equals original");
		}
	}

	check(checksums[0] == checksums[1] && checksums[1] == checksums[2], PString::Format("3 runs produce identical checksums (%llu)", checksums[0]));

	// 5. 선택 대기 재진입 ----------------------------------------------------------------
	{
		PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
		setupGameMaster(*sim);
		sim->Start(seed);
		HList<HGameplayEntityId> units = unitsOf(*sim);

		HList<HGameplayEvent> events;
		PString reason;
		EGameplaySubmitResult result = sim->Submit(HGameplayCommand(PName(TestCommandPickAct), units[0]), events, &reason);
		check(result == EGameplaySubmitResult::PendingChoice, "TestPickAct returns PendingChoice");
		check(hasEvent(events, HGameplayBuiltin::EventChoiceRequested), "ChoiceRequested emitted");

		const HGameplayChoice* choice = sim->GetPendingChoice();
		check(choice != nullptr && choice->Candidates.size() == 2, "pending choice has 2 candidates");

		HList<HGameplayEvent> ignored;
		HGameplayCommand act(PName(TestCommandAct), units[0]);
		act.Targets.push_back(units[1]);
		act.Params.push_back(1);
		check(sim->Submit(act, ignored, &reason) == EGameplaySubmitResult::Rejected, "other commands rejected while choice pending");

		HGameplayCommand badResolve(PName(HGameplayBuiltin::CommandResolveChoice), units[0]);
		badResolve.Targets.push_back(units[0]);
		check(sim->Submit(badResolve, ignored, &reason) == EGameplaySubmitResult::Rejected, PString::Format("non-candidate selection rejected (%s)", reason));

		// 대기 중 저장 · 로드
		PString savePath = "GameMasterSelfTest_Pending.json";
		check(sim->Save(savePath) == true, "Save while pending succeeds");
		PSharedPtr<PGameMaster> loaded = Allocate<PGameMaster>();
		setupGameMaster(*loaded);
		check(loaded->Load(savePath) == true && loaded->GetPendingChoice() != nullptr, "loaded gameMaster keeps pending choice");

		HGameplayCommand resolve(PName(HGameplayBuiltin::CommandResolveChoice), units[0]);
		resolve.Targets.push_back(units[1]);
		events.clear();
		reason = PString();
		result = sim->Submit(resolve, events, &reason);
		check(result == EGameplaySubmitResult::Executed, PString::Format("ResolveChoice executed (%s)", reason));
		check(hasEvent(events, HGameplayBuiltin::EventChoiceResolved), "ChoiceResolved emitted");
		check(valueOf(*sim, units[1]) == 7, PString::Format("picked target 10 - (1 + 2 bonus) = 7, got %d", valueOf(*sim, units[1])));
		check(sim->GetPendingChoice() == nullptr, "no pending choice after resolve");

		HList<HGameplayEvent> loadedEvents;
		check(loaded->Submit(resolve, loadedEvents, &reason) == EGameplaySubmitResult::Executed && valueOf(*loaded, units[1]) == 7, "loaded gameMaster resolves the same choice");
	}

	// 6. 에이전트 ------------------------------------------------------------------
	{
		PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
		setupGameMaster(*sim);
		sim->SetAgent(0, Allocate<PGameplayRandomAgent>((uint64)7));
		sim->Start(seed);
		int32 steps = PGameplayAgentRunner::Run(*sim, 40);
		check(steps > 0, PString::Format("random agent played %d steps", steps));
		check(sim->GetState().Sequence == (uint32)steps, "sequence equals steps played");

		PSharedPtr<PGameMaster> greedy = Allocate<PGameMaster>();
		setupGameMaster(*greedy);
		greedy->SetAgent(0, Allocate<PGameplayGreedyAgent>(Allocate<PTestEvaluator>()));
		greedy->Start(seed);
		int32 greedySteps = PGameplayAgentRunner::Run(*greedy, 12);
		check(greedySteps > 0, PString::Format("greedy agent played %d steps", greedySteps));

		// 같은 시드의 무작위 에이전트 두 번 → 같은 체크섬
		PSharedPtr<PGameMaster> again = Allocate<PGameMaster>();
		setupGameMaster(*again);
		again->SetAgent(0, Allocate<PGameplayRandomAgent>((uint64)7));
		again->Start(seed);
		PGameplayAgentRunner::Run(*again, 40);
		check(again->Checksum() == sim->Checksum(), "random agent runs are reproducible");
	}

	// 7. 보드 ----------------------------------------------------------------------
	{
		PSharedPtr<IGameplayBoard> hex = IGameplayBoard::Create(EGameplayBoardKind::Hex);
		check(hex->Distance(HGameplayCoord(0, 0), HGameplayCoord(2, -1)) == 2, "hex distance (0,0)-(2,-1) = 2");
		check(hex->Distance(HGameplayCoord(0, 0), HGameplayCoord(3, 0)) == 3, "hex distance (0,0)-(3,0) = 3");

		HGameplayBoardState square;
		square.Reset(EGameplayBoardKind::Square, 5, 5);
		square.SetBlocked(HGameplayCoord(1, 0), true);
		PSharedPtr<IGameplayBoard> board = IGameplayBoard::Create(EGameplayBoardKind::Square);
		HList<HGameplayCoord> path;
		bool bPathFound = board->FindPath(square, HGameplayCoord(0, 0), HGameplayCoord(2, 0), 10, path);
		check(bPathFound == true && path.size() == 4, PString::Format("square path around a blocked cell has 4 steps, got %d", (int32)path.size()));
		check(path.empty() == false && path.back() == HGameplayCoord(2, 0), "path ends at the goal");

		HGameplayEntityRegistry registry;
		HGameplayEntityId unit = registry.Create();
		check(square.SetPosition(unit, HGameplayCoord(3, 3)) == true, "board SetPosition succeeds");
		check(square.OccupantAt(HGameplayCoord(3, 3)) == unit, "OccupantAt returns the unit");
		check(square.SetPosition(unit, HGameplayCoord(1, 0)) == false, "cannot move onto a blocked cell");

		HList<HGameplayCoord> range;
		board->CoordsInRange(square, HGameplayCoord(2, 2), 1, range);
		check(range.size() == 5, PString::Format("square range 1 around center = 5 cells, got %d", (int32)range.size()));
	}

	// 8. 난수 · 영역 ---------------------------------------------------------------------
	{
		HGameplayRandomStream a;
		HGameplayRandomStream b;
		a.Seed(42, 1);
		b.Seed(42, 1);
		bool bSame = true;
		for (int32 i = 0; i < 100; ++i)
		{
			if (a.Next() != b.Next())
			{
				bSame = false;
			}
		}
		check(bSame, "same seed gives same random sequence");
		check(a.CallCount == 100, "random stream counts calls");

		HGameplayZone zone(PName("Deck"));
		HGameplayEntityRegistry registry;
		for (int32 i = 0; i < 8; ++i)
		{
			zone.PushBack(registry.Create());
		}
		HGameplayRandomStream shuffleRng;
		shuffleRng.Seed(9, 2);
		HGameplayZone copy = zone;
		copy.Shuffle(shuffleRng);
		check(copy.Count() == 8, "shuffle keeps all entities");
		bool bAllPresent = true;
		for (const HGameplayEntityId& id : zone.Entities)
		{
			if (copy.Contains(id) == false)
			{
				bAllPresent = false;
			}
		}
		check(bAllPresent, "shuffle is a permutation");
	}

	std::cout << "== GameMaster self test: " << check.Passed << " passed, " << check.Failures << " failed ==" << std::endl;
	JG_LOG(GameMasterSelfTest, ELogLevel::Info, "GameMaster self test: %d passed, %d failed", check.Passed, check.Failures);
	return check.Failures;
}
