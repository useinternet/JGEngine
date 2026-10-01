#include "PCH/PCH.h"
#include "GameMaster/GameMasterSelfTest.h"
#include "GameMaster/GameMaster.h"
#include "GameMaster/Agents/GameplayAgent.h"
#include <iostream>
#include <type_traits>
#include <utility>

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

		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
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

	// ---- 리뷰 회귀용 (R1–R3 죽은 ID, R7 불러오기 후 트리거 순서) ----

	const char* TestZoneGrave = "Grave";

	// 새 엔티티를 Units 에 만들고 좌표 · 값을 준다.
	class PTestSpawnValueEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestSpawnValue");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayEntityId id = ctx.SpawnEntity(PName(TestZoneUnits));
			ctx.State.Board.SetPosition(id, request.Coord);
			HGameplayTestValue value;
			value.Value = request.Param(0, 0);
			ctx.State.Add<HGameplayTestValue>(id, value);
		}
	};

	// 대상에게 값 컴포넌트를 준다 (대상이 죽었으면 상태가 거부해야 한다).
	class PTestAddValueEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestAddValue");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayTestValue value;
			value.Value = request.Param(0, 0);
			ctx.State.Add<HGameplayTestValue>(request.Target, value);
		}
	};

	// "죽인 뒤 밀치기" 처럼 대상이 이미 죽은 뒤에 도착하는 효과들을 한 명령으로 재현한다.
	//   대상 파괴 → 새 엔티티(같은 번호 재사용) → 죽은 대상 좌표 이동 · 값 부여 · 묘지 이동
	class PTestStaleOpHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestStaleOp");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return command.Targets.empty() == false;
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayEntityId victim = command.Target();

			ctx.Enqueue(HGameplayEffectRequest(PName(HGameplayBuiltin::EffectDestroyEntity), command.Actor, victim));

			HGameplayEffectRequest spawn(PName("TestSpawnValue"), command.Actor, HGameplayEntityId::None());
			spawn.Coord = HGameplayCoord(2, 2);
			spawn.Params.push_back(50);
			ctx.Enqueue(spawn);

			HGameplayEffectRequest knockback(PName(HGameplayBuiltin::EffectSetBoardPosition), command.Actor, victim);
			knockback.Coord = HGameplayCoord(3, 3);
			ctx.Enqueue(knockback);

			HGameplayEffectRequest status(PName("TestAddValue"), command.Actor, victim);
			status.Params.push_back(7);
			ctx.Enqueue(status);

			HGameplayEffectRequest grave(PName(HGameplayBuiltin::EffectMoveToZone), command.Actor, victim);
			grave.Tag = PName(TestZoneGrave);
			ctx.Enqueue(grave);
		}
	};

	class PTestDoubleEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestDouble");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayTestValue* value = ctx.State.Find<HGameplayTestValue>(request.Target);
			if (value != nullptr)
			{
				value->Value *= 2;
			}
		}
	};

	class PTestIncrementEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestIncrement");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayTestValue* value = ctx.State.Find<HGameplayTestValue>(request.Target);
			if (value != nullptr)
			{
				value->Value += 1;
			}
		}
	};

	// 같은 우선순위 트리거 둘. 반응 순서(= 레지스트리 순서)에 따라 결과가 달라진다: (v + 1) * 2 vs v * 2 + 1.
	class PTestOrderTrigger : public JGGameplayTrigger
	{
		PName _kind;
		PName _effect;

	public:
		PTestOrderTrigger() = default;
		PTestOrderTrigger(const PName& kind, const PName& effect)
			: _kind(kind)
			, _effect(effect)
		{
		}
		virtual PName GetKind() const override
		{
			return _kind;
		}
		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			return event.Kind == PName(TestEventChanged) && event.Amount > 0 && state.IsAlive(event.Target) == true;
		}
		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
		{
			ctx.Enqueue(HGameplayEffectRequest(_effect, event.Subject, event.Target));
		}
	};

	// ---- 리뷰 재현 회귀용 규칙 (Phase 1: R4 · R5 · R6 · R8 · R16 · R17) ----
	// R 번호는 Document/GameFrameWorks_설계코드리뷰_2026-09-28.md. R1–R3 · R7 은 9 · 10 절이 본다.

	const char* TestEventProbed   = "TestProbed";
	const char* TestEventAnswered = "TestAnswered";

	// 테스트가 채운 효과 요청을 그대로 넣는 명령.
	class PTestScriptHandler : public JGGameplayCommandHandler
	{
	public:
		HList<HGameplayEffectRequest> Script;

		virtual PName GetKind() const override
		{
			return PName("TestScript");
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

	// 핸들러 안에서 선택을 요청하는 명령 (R5). 효과 밖의 요청이라 거부돼야 한다.
	class PTestAskInHandler : public JGGameplayCommandHandler
	{
	public:
		bool bAccepted = true;

		virtual PName GetKind() const override
		{
			return PName("TestAskInHandler");
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return true;
		}

		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayChoice choice;
			choice.Chooser = command.Actor;
			choice.Kind    = PName("TestAskInHandler");
			choice.Candidates.push_back(command.Actor);
			bAccepted = ctx.RequestChoice(choice);
		}
	};

	// 대상의 값을 Param(0) 으로 둔다. 없으면 붙인다 (R6b).
	class PTestSetValueEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestSetValue");
		}

		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayTestValue value;
			value.Value = request.Param(0, 0);
			ctx.State.Add<HGameplayTestValue>(request.Target, value);
		}
	};

	// 해결 순간의 현재 행동자 · 페이즈를 이벤트로 남긴다 (R6a).
	class PTestProbeEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestProbe");
		}

		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HGameplayEvent probed(PName(TestEventProbed), ctx.State.Turn.CurrentActor, request.Target);
			probed.Amount = (int32)ctx.State.Turn.Phase;
			ctx.Emit(probed);
		}
	};

	// 대상 하나를 후보로 선택을 요청하고, 선택이 오면 TestAnswered 를 낸다 (R4 · 전이 도중 선택).
	class PTestAskEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("TestAsk");
		}

		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			if (request.HasChoice() == true)
			{
				HGameplayEvent answered(PName(TestEventAnswered), request.Subject, request.Target);
				ctx.Emit(answered);
				return;
			}

			HGameplayChoice choice;
			choice.Chooser = request.Subject;
			choice.Kind    = PName("TestAsk");
			choice.Candidates.push_back(request.Target);
			ctx.RequestChoice(choice);
		}
	};

	// EventKind 이벤트에 반응해 EffectKind 효과를 넣는 트리거 (R4 · R6 · R8).
	// OnlySubject 를 주면 그 주체의 이벤트에만, Target 을 주면 그 대상에게 넣는다. 없으면 이벤트의 대상(없으면 주체).
	class PTestReactTrigger : public JGGameplayTrigger
	{
	public:
		PName               Kind;
		PName               EventKind;
		PName               EffectKind;
		HGameplayEntityId OnlySubject;
		HGameplayEntityId Target;
		int32               Param = 0;

		virtual PName GetKind() const override
		{
			return Kind;
		}

		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			if (event.Kind != EventKind)
			{
				return false;
			}
			return OnlySubject.IsValid() == false || event.Subject == OnlySubject;
		}

		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
		{
			HGameplayEntityId target = Target;
			if (target.IsValid() == false)
			{
				target = event.Target.IsValid() == true ? event.Target : event.Subject;
			}
			HGameplayEffectRequest request(EffectKind, event.Subject, target);
			request.Params.push_back(Param);
			ctx.Enqueue(request);
		}
	};

	PSharedPtr<PTestReactTrigger> makeReactTrigger(const char* kind, const char* eventKind, const char* effectKind, int32 param = 0)
	{
		PSharedPtr<PTestReactTrigger> trigger = Allocate<PTestReactTrigger>();
		trigger->Kind       = PName(kind);
		trigger->EventKind  = PName(eventKind);
		trigger->EffectKind = PName(effectKind);
		trigger->Param      = param;
		return trigger;
	}

	// R17: const 로 받은 상태로는 컴포넌트를 바꿀 수 없다 (컴파일 시점 검사).
	static_assert(std::is_same<decltype(std::declval<const HGameplayState&>().Find<HGameplayTestValue>(HGameplayEntityId())), const HGameplayTestValue*>::value, "R17: Find on a const state must return const T*");
	static_assert(std::is_same<decltype(std::declval<const HGameplayState&>().Get<HGameplayTestValue>(HGameplayEntityId())), const HGameplayTestValue&>::value, "R17: Get on a const state must return const T&");
	static_assert(std::is_same<decltype(std::declval<const HGameplayState&>().Table<HGameplayTestValue>()), const HGameplayComponentTable<HGameplayTestValue>*>::value, "R17: Table on a const state must return a const table");

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

	// 회귀 시나리오용 최소 설정: 값 컴포넌트 · Units 영역 · Units 순서, 값 value 인 유닛 count 개.
	HList<HGameplayEntityId> setupRegressionGameMaster(PGameMaster& sim, int32 count, int32 value)
	{
		sim.RegisterComponent<HGameplayTestValue>(PName(TestValueName));
		sim.RegisterZone(PName(TestZoneUnits));
		sim.SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName(TestZoneUnits)));

		HList<HGameplayEntityId> units;
		HGameplayState& initial = sim.EditInitialState();
		for (int32 i = 0; i < count; ++i)
		{
			HGameplayEntityId id = initial.CreateEntity();
			HGameplayTestValue component;
			component.Value = value;
			initial.Add<HGameplayTestValue>(id, component);
			initial.Zone(PName(TestZoneUnits)).PushBack(id);
			units.push_back(id);
		}
		return units;
	}

	int32 findEventIndex(const HList<HGameplayEvent>& events, const char* kind, const HGameplayEntityId& subject = HGameplayEntityId::None())
	{
		PName name(kind);
		int32 count = (int32)events.size();
		for (int32 i = 0; i < count; ++i)
		{
			if (events[i].Kind == name && (subject.IsValid() == false || events[i].Subject == subject))
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	// 리뷰 재현 항목이 다시 생기지 않는지 본다 (GFW TODO Phase 1 완료 기준). R1–R3 · R7 은 9 · 10 절.
	void runReviewRegressions(HCheck& check, uint64 seed)
	{
		// R4: 트리거가 넣은 효과가 선택을 요청하면 그 효과가 재진입한다 (해결 중이던 피해 효과가 다시 돌지 않는다)
		{
			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			HList<HGameplayEntityId> units = setupRegressionGameMaster(*sim, 2, 10);
			PSharedPtr<PTestScriptHandler> script = Allocate<PTestScriptHandler>();
			sim->RegisterHandler(script);
			sim->RegisterEffect(Allocate<PTestChangeEffect>());
			sim->RegisterEffect(Allocate<PTestAskEffect>());
			sim->RegisterTrigger(makeReactTrigger("TestAskOnChange", TestEventChanged, "TestAsk"));
			sim->Start(seed);

			HGameplayEffectRequest damage(PName(TestEffectChange), units[0], units[1]);
			damage.Params.push_back(3);
			script->Script.push_back(damage);

			HList<HGameplayEvent> events;
			PString reason;
			EGameplaySubmitResult first = sim->Submit(HGameplayCommand(PName("TestScript"), units[0]), events, &reason);
			int32 afterDamage = valueOf(*sim, units[1]);

			HGameplayCommand resolve(PName(HGameplayBuiltin::CommandResolveChoice), units[0]);
			resolve.Targets.push_back(units[1]);
			events.clear();
			EGameplaySubmitResult second = sim->Submit(resolve, events, &reason);
			bool bResults = first == EGameplaySubmitResult::PendingChoice && second == EGameplaySubmitResult::Executed && hasEvent(events, TestEventAnswered);
			check(bResults == true && afterDamage == 7 && valueOf(*sim, units[1]) == 7,
				PString::Format("R4 a choice asked by a trigger's effect re-enters that effect only (value %d -> %d after resolve)", afterDamage, valueOf(*sim, units[1])));
		}

		// R5: 효과 밖(핸들러)의 선택 요청은 오류 로그와 함께 거부된다
		{
			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			HList<HGameplayEntityId> units = setupRegressionGameMaster(*sim, 2, 10);
			PSharedPtr<PTestAskInHandler> ask = Allocate<PTestAskInHandler>();
			sim->RegisterHandler(ask);
			sim->Start(seed);

			std::cout << "  (the next RequestChoice error log is expected)" << std::endl;
			HList<HGameplayEvent> events;
			PString reason;
			EGameplaySubmitResult result = sim->Submit(HGameplayCommand(PName("TestAskInHandler"), units[0]), events, &reason);
			check(ask->bAccepted == false && result == EGameplaySubmitResult::Executed && sim->GetPendingChoice() == nullptr, "R5 RequestChoice outside an effect is rejected (returns false, nothing pending)");
		}

		// R6a: 턴 종료 효과는 다음 행동자가 정해지기 전, 끝나는 턴의 TurnEnd 에서 해결된다
		{
			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			HList<HGameplayEntityId> units = setupRegressionGameMaster(*sim, 2, 10);
			sim->RegisterEffect(Allocate<PTestProbeEffect>());
			sim->RegisterTrigger(makeReactTrigger("TestProbeOnTurnEnded", HGameplayBuiltin::EventTurnEnded, "TestProbe"));
			sim->Start(seed);

			HList<HGameplayEvent> events;
			PString reason;
			sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), units[0]), events, &reason);
			int32 probedAt  = findEventIndex(events, TestEventProbed);
			int32 startedAt = findEventIndex(events, HGameplayBuiltin::EventTurnStarted);
			bool bInEndingTurn = probedAt != INDEX_NONE && events[probedAt].Subject == units[0] && events[probedAt].Amount == (int32)EGameplayPhase::TurnEnd;
			check(bInEndingTurn == true && startedAt != INDEX_NONE && probedAt < startedAt,
				PString::Format("R6a TurnEnded effect resolves in the ending turn (probe event %d, next TurnStarted %d)", probedAt, startedAt));
		}

		// R6b: 라운드 시작 효과가 바꾼 값이 이번 라운드 행동 순서에 반영된다
		{
			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			HList<HGameplayEntityId> units = setupRegressionGameMaster(*sim, 2, 1);
			sim->EditInitialState().Get<HGameplayTestValue>(units[1]).Value = 2;
			sim->RegisterEffect(Allocate<PTestSetValueEffect>());
			PSharedPtr<PTestReactTrigger> initiative = makeReactTrigger("TestInitiative", HGameplayBuiltin::EventRoundStarted, "TestSetValue", 3);
			initiative->Target = units[0];
			sim->RegisterTrigger(initiative);
			PGameplayKeyedOrderPolicy::HKeyFunction key = [](const HGameplayState& state, const HGameplayEntityId& id)
			{
				const HGameplayTestValue* value = state.Find<HGameplayTestValue>(id);
				return value != nullptr ? value->Value : 0;
			};
			sim->SetOrderPolicy(Allocate<PGameplayKeyedOrderPolicy>(PName(TestZoneUnits), key, true));
			sim->Start(seed);

			const HGameplayTurnState& turn = sim->GetState().Turn;
			check(turn.Order.size() == 2 && turn.Order[0] == units[1] && turn.CurrentActor == units[1],
				PString::Format("R6b RoundStarted effect changes this round's order (first actor %s, expected %s)", turn.CurrentActor.ToString(), units[1].ToString()));
		}

		// 턴 시작 효과가 턴을 넘기면(기절 등) 그 행동자는 행동하지 않고 다음 행동자로 간다
		{
			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			HList<HGameplayEntityId> units = setupRegressionGameMaster(*sim, 2, 10);
			PSharedPtr<PTestReactTrigger> stun = makeReactTrigger("TestStun", HGameplayBuiltin::EventTurnStarted, HGameplayBuiltin::EffectEndTurn);
			stun->OnlySubject = units[0];
			sim->RegisterTrigger(stun);

			HList<HGameplayEvent> startEvents;
			sim->Start(seed, &startEvents);
			const HGameplayTurnState& turn = sim->GetState().Turn;
			check(turn.CurrentActor == units[1] && turn.Phase == EGameplayPhase::TurnMain && findEventIndex(startEvents, HGameplayBuiltin::EventTurnEnded, units[0]) != INDEX_NONE,
				"EndTurn effect from a TurnStarted trigger skips that actor's turn");
		}

		// 전이 도중(TurnEnd)의 선택 대기: 예약된 다음 단계가 상태에 남아 저장 · 불러오기 뒤에도 이어진다
		{
			auto setup = [](PGameMaster& sim) -> HList<HGameplayEntityId>
			{
				HList<HGameplayEntityId> units = setupRegressionGameMaster(sim, 2, 10);
				sim.RegisterEffect(Allocate<PTestAskEffect>());
				PSharedPtr<PTestReactTrigger> ask = makeReactTrigger("TestAskOnTurnEnded", HGameplayBuiltin::EventTurnEnded, "TestAsk");
				ask->OnlySubject = units[0];
				sim.RegisterTrigger(ask);
				return units;
			};

			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			HList<HGameplayEntityId> units = setup(*sim);
			sim->Start(seed);

			HList<HGameplayEvent> events;
			PString reason;
			EGameplaySubmitResult ended = sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), units[0]), events, &reason);
			const HGameplayTurnState& turn = sim->GetState().Turn;
			check(ended == EGameplaySubmitResult::PendingChoice && turn.Phase == EGameplayPhase::TurnEnd && turn.CurrentActor == units[0] && turn.PendingStep == EGameplayPhaseStep::NextTurn,
				"a choice from a TurnEnded effect waits inside the ending turn");

			PString document;
			bool bExported = sim->ExportDocument(&document);
			PSharedPtr<PGameMaster> loaded = Allocate<PGameMaster>();
			setup(*loaded);
			bool bImported = loaded->ImportDocument(document);

			HGameplayCommand resolve(PName(HGameplayBuiltin::CommandResolveChoice), units[0]);
			resolve.Targets.push_back(units[0]);
			HList<HGameplayEvent> resolvedEvents;
			HList<HGameplayEvent> loadedEvents;
			EGameplaySubmitResult resolved = sim->Submit(resolve, resolvedEvents, &reason);
			EGameplaySubmitResult loadedResolved = loaded->Submit(resolve, loadedEvents, &reason);
			check(resolved == EGameplaySubmitResult::Executed && sim->GetState().Turn.CurrentActor == units[1] && sim->GetState().Turn.Phase == EGameplayPhase::TurnMain,
				"after that choice the next actor's turn begins");
			check(bExported == true && bImported == true && loadedResolved == EGameplaySubmitResult::Executed && loaded->Checksum() == sim->Checksum(),
				"a document saved during the transition resumes to the same state");
		}

		// R8: 트리거 연쇄는 효과 깊이 한도에서 멈춘다 (React 는 Emit 을 부를 수 없어 재귀가 없다)
		{
			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			HList<HGameplayEntityId> units = setupRegressionGameMaster(*sim, 1, 0);
			PSharedPtr<PTestScriptHandler> script = Allocate<PTestScriptHandler>();
			sim->RegisterHandler(script);
			sim->RegisterTrigger(makeReactTrigger("TestSpawnChain", HGameplayBuiltin::EventEntitySpawned, HGameplayBuiltin::EffectSpawnEntity));
			sim->Start(seed);

			script->Script.push_back(HGameplayEffectRequest(PName(HGameplayBuiltin::EffectSpawnEntity), units[0], HGameplayEntityId::None()));
			HList<HGameplayEvent> events;
			PString reason;
			sim->Submit(HGameplayCommand(PName("TestScript"), units[0]), events, &reason);

			int32 spawned = 0;
			for (const HGameplayEvent& event : events)
			{
				if (event.Kind == PName(HGameplayBuiltin::EventEntitySpawned))
				{
					++spawned;
				}
			}
			check(spawned == HGameplayLimits::MaxEffectDepth + 1 && hasEvent(events, HGameplayBuiltin::EventEffectDepthExceeded),
				PString::Format("R8 a trigger chain stops at MaxEffectDepth with EffectDepthExceeded (%d spawns)", spawned));
		}

		// R16 · R10: 스냅샷 한도(기본 256)를 넘겨 진행해도 한도만큼만 보관하고 되돌리기가 된다
		{
			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			setupRegressionGameMaster(*sim, 2, 1);
			sim->Start(seed);

			const int32 commands = 300;
			int32  executed   = 0;
			uint64 beforeLast = 0;
			for (int32 i = 0; i < commands; ++i)
			{
				if (i == commands - 1)
				{
					beforeLast = sim->Checksum();
				}
				HList<HGameplayEvent> events;
				PString reason;
				HGameplayEntityId actor = sim->GetState().Turn.CurrentActor;
				if (sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), actor), events, &reason) == EGameplaySubmitResult::Executed)
				{
					++executed;
				}
			}
			int32 atLimit = sim->UndoCount();
			bool bUndone = sim->Undo();
			check(executed == commands && atLimit == 256 && bUndone == true && sim->Checksum() == beforeLast,
				PString::Format("R16 %d commands past the snapshot limit keep %d snapshots and undo still works", executed, atLimit));
		}

		check(true, "R17 const HGameplayState gives only const components (static_assert above)");
	}

	// ---- 13. 게임 흐름 (ER-009) ----------------------------------------------------------

	const char* TestFlowName       = "TestPhases";
	const char* TestCommandReady   = "TestReady";
	const char* TestCommandKillAll = "TestKillAll";
	const char* TestEventAutoStep  = "TestAutoStep";
	const char* TestStepSetup      = "Setup";
	const char* TestStepAuto       = "Auto";
	const char* TestStepAct        = "Act";
	const char* TestStepNextActor  = "NextActor";   // 예약 칸(NextStep)에만 쓰는 이름
	const char* TestStepUpkeep     = "Upkeep";
	constexpr int32 TestFlowFinishCode = 7;

	void collectAliveUnits(const HGameplayState& state, HList<HGameplayEntityId>& outUnits)
	{
		const HGameplayZone* zone = state.FindZone(PName(TestZoneUnits));
		if (zone == nullptr)
		{
			return;
		}
		for (const HGameplayEntityId& id : zone->Entities)
		{
			if (state.IsAlive(id) == true)
			{
				outUnits.push_back(id);
			}
		}
	}

	// 검증용 게임 흐름: 준비(전원 동시 입력) → 라운드마다 [자동 → 행동(흐름이 고른 역순으로 한 명씩) → 정리], FinishAfterRound 에서 끝.
	// 기본 흐름과 다른 모양(여러 행동자 · 자동 단계 · 흐름이 고른 순서 · 탈락 처리 · 흐름이 정하는 끝)을 한 번에 본다.
	class PTestPhasesFlow : public IGameplayFlow
	{
	public:
		int32 FinishAfterRound = 2;
		bool  bRunaway         = false;   // 대기 없이 단계를 계속 잇는 고장 난 흐름 (한도 검사용)

		virtual PName GetName() const override
		{
			return PName(TestFlowName);
		}

		virtual void Start(HGameplayContext& ctx) override
		{
			HGameplayTurnState& turn = ctx.State.Turn;
			turn.Round     = 0;
			turn.TurnCount = 0;
			ctx.SetStep(PName(TestStepSetup));

			HList<HGameplayEntityId> units;
			collectAliveUnits(ctx.State, units);
			ctx.SetActors(units);
		}

		virtual bool RunPendingStep(HGameplayContext& ctx) override
		{
			HGameplayTurnState& turn = ctx.State.Turn;
			if (bRunaway == true)
			{
				turn.NextStep = PName("Loop");
				return true;
			}

			// 행동 단계의 행동자가 죽었으면 다음 행동자로 (탈락 처리는 흐름이 정한다)
			if (turn.Step == PName(TestStepAct) && turn.NextStep == NAME_NONE && turn.Actors.empty() == false && ctx.State.IsAlive(turn.Actors[0]) == false)
			{
				turn.NextStep = PName(TestStepNextActor);
			}
			// 준비 단계: 모두 준비하면(입력 행동자가 비면) 자동 단계로
			if (turn.Step == PName(TestStepSetup) && turn.Actors.empty() == true && turn.NextStep == NAME_NONE)
			{
				turn.NextStep = PName(TestStepAuto);
			}

			const PName next = turn.NextStep;
			if (next == NAME_NONE)
			{
				return false;
			}
			turn.NextStep = PName();

			if (next == PName(TestStepAuto))
			{
				++turn.Round;
				ctx.SetStep(next);
				ctx.SetActors(HList<HGameplayEntityId>());

				HList<HGameplayEntityId> units;
				collectAliveUnits(ctx.State, units);
				HGameplayEvent autoStep(PName(TestEventAutoStep), units.empty() == true ? HGameplayEntityId::None() : units[0], HGameplayEntityId::None());
				autoStep.Amount = turn.Round;
				ctx.Emit(autoStep);

				turn.NextStep = PName(TestStepAct);
				return true;
			}

			if (next == PName(TestStepAct))
			{
				ctx.SetStep(next);

				// 흐름이 고른 순서: 살아 있는 유닛을 역순으로
				HList<HGameplayEntityId> units;
				collectAliveUnits(ctx.State, units);
				turn.Order.clear();
				for (int32 i = (int32)units.size() - 1; i >= 0; --i)
				{
					turn.Order.push_back(units[i]);
				}
				turn.OrderIndex = INDEX_NONE;
				turn.NextStep   = PName(TestStepNextActor);
				return true;
			}

			if (next == PName(TestStepNextActor))
			{
				int32 index = turn.OrderIndex + 1;
				while (index < (int32)turn.Order.size() && ctx.State.IsAlive(turn.Order[index]) == false)
				{
					++index;
				}
				if (index >= (int32)turn.Order.size())
				{
					ctx.SetActors(HList<HGameplayEntityId>());
					turn.NextStep = PName(TestStepUpkeep);
					return true;
				}

				turn.OrderIndex = index;
				++turn.TurnCount;
				HList<HGameplayEntityId> actor;
				actor.push_back(turn.Order[index]);
				ctx.SetActors(actor);
				return true;
			}

			if (next == PName(TestStepUpkeep))
			{
				ctx.SetStep(next);
				if (turn.Round >= FinishAfterRound)
				{
					ctx.FinishGame(TestFlowFinishCode);
					return true;
				}
				turn.NextStep = PName(TestStepAuto);
				return true;
			}

			return false;
		}

		virtual bool CanEndTurn(const HGameplayState& state, const HGameplayEntityId& actor, PString* outReason) const override
		{
			if (state.Turn.Step != PName(TestStepAct) || state.Turn.IsActor(actor) == false)
			{
				if (outReason != nullptr)
				{
					*outReason = "not acting now (test flow)";
				}
				return false;
			}
			return true;
		}

		virtual void EndTurn(HGameplayContext& ctx, const HGameplayEntityId& actor) override
		{
			ctx.SetActors(HList<HGameplayEntityId>());
			ctx.State.Turn.NextStep = PName(TestStepNextActor);
		}
	};

	// 준비 단계 명령: 입력 행동자가 내면 그 행동자가 입력 목록에서 빠진다.
	class PTestReadyHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(TestCommandReady);
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (state.Turn.Step != PName(TestStepSetup) || state.IsInputActor(command.Actor) == false)
			{
				if (outReason != nullptr)
				{
					*outReason = "not an input actor in setup";
				}
				return false;
			}
			return true;
		}

		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HList<HGameplayEntityId> remaining;
			for (const HGameplayEntityId& actor : ctx.State.Turn.Actors)
			{
				if (actor != command.Actor)
				{
					remaining.push_back(actor);
				}
			}
			ctx.SetActors(remaining);
		}
	};

	// 모든 유닛을 없앤다 (빈 행동자에서 흐름이 어떻게 끝나는지 보기 위한 것).
	class PTestKillAllHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(TestCommandKillAll);
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (state.IsInputActor(command.Actor) == false)
			{
				if (outReason != nullptr)
				{
					*outReason = "not an input actor";
				}
				return false;
			}
			return true;
		}

		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HList<HGameplayEntityId> units;
			collectAliveUnits(ctx.State, units);
			for (const HGameplayEntityId& unit : units)
			{
				HGameplayEffectRequest kill(PName(TestEffectChange), command.Actor, unit);
				kill.Params.push_back(1000);
				ctx.Enqueue(kill);
			}
		}
	};

	int32 findStepIndex(const HList<HGameplayEvent>& events, const char* step)
	{
		PName kind(HGameplayBuiltin::EventStepChanged);
		PName name(step);
		int32 count = (int32)events.size();
		for (int32 i = 0; i < count; ++i)
		{
			if (events[i].Kind == kind && events[i].Tag == name)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	HList<HGameplayEntityId> setupFlowGameMaster(PGameMaster& sim, PSharedPtr<IGameplayFlow> flow)
	{
		HList<HGameplayEntityId> units = setupRegressionGameMaster(sim, 3, 10);
		sim.RegisterHandler(Allocate<PTestReadyHandler>());
		sim.RegisterHandler(Allocate<PTestKillAllHandler>());
		sim.RegisterEffect(Allocate<PTestChangeEffect>());
		sim.RegisterTrigger(makeReactTrigger("TestAutoStepReact", TestEventAutoStep, TestEffectChange, 1));
		sim.SetFlow(flow);
		return units;
	}

	void runFlowChecks(HCheck& check, uint64 seed)
	{
		// 기본 흐름도 공용 칸을 채운다
		{
			PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
			setupGameMaster(*sim);
			sim->Start(seed);
			const HGameplayState&     state = sim->GetState();
			const HGameplayTurnState& turn  = state.Turn;
			check(turn.Phase == EGameplayPhase::TurnMain && turn.Step == PName("TurnMain") && turn.Actors.size() == 1 && turn.Actors[0] == turn.CurrentActor
				&& state.FirstInputActor() == turn.CurrentActor,
				"13 the default flow fills the common fields at TurnMain (Step TurnMain, Actors = [current actor])");
		}

		PSharedPtr<PTestPhasesFlow> flow = Allocate<PTestPhasesFlow>();
		PSharedPtr<PGameMaster>     sim  = Allocate<PGameMaster>();
		HList<HGameplayEntityId>    units = setupFlowGameMaster(*sim, flow);
		const HGameplayState&       state = sim->GetState();

		HList<HGameplayEvent> startEvents;
		sim->Start(seed, &startEvents);
		HList<HGameplayEntityId> inputActors;
		state.CollectInputActors(inputActors);
		check(state.Turn.Phase == EGameplayPhase::Flow && state.Turn.Step == PName(TestStepSetup) && inputActors.size() == 3,
			"13 a game flow starts in its own setup step with all three units as input actors (Phase Flow)");
		check(hasEvent(startEvents, HGameplayBuiltin::EventGameStarted) == true && findStepIndex(startEvents, TestStepSetup) != INDEX_NONE
			&& hasEvent(startEvents, HGameplayBuiltin::EventRoundStarted) == false && hasEvent(startEvents, HGameplayBuiltin::EventTurnStarted) == false,
			"13 the engine emits GameStarted and the flow its StepChanged; no built-in round or turn events");

		PSharedPtr<PGameMaster> defaultFlowSim = Allocate<PGameMaster>();
		setupFlowGameMaster(*defaultFlowSim, nullptr);
		check(defaultFlowSim->RulesFingerprint() != sim->RulesFingerprint(), "13 the rules fingerprint includes the flow name");

		// 준비 단계: EndTurn 은 흐름이 정하고(거절), 준비 명령은 아무 순서로, 이미 준비한 유닛은 다시 못 낸다
		HList<HGameplayEvent> events;
		PString reason;
		EGameplaySubmitResult endTurnInSetup = sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), units[0]), events, &reason);
		check(endTurnInSetup == EGameplaySubmitResult::Rejected && reason == PString("not acting now (test flow)"),
			PString::Format("13 the flow decides EndTurn: rejected in setup with the flow's reason (%s)", reason));

		EGameplaySubmitResult ready2 = sim->Submit(HGameplayCommand(PName(TestCommandReady), units[2]), events, &reason);
		check(ready2 == EGameplaySubmitResult::Executed && state.Turn.Actors.size() == 2 && state.Turn.IsActor(units[2]) == false,
			"13 setup takes input actors in any order (units[2] first) and drops the one who readied");
		check(sim->Submit(HGameplayCommand(PName(TestCommandReady), units[2]), events, &reason) == EGameplaySubmitResult::Rejected,
			"13 a unit that already readied is no longer an input actor");

		sim->Submit(HGameplayCommand(PName(TestCommandReady), units[0]), events, &reason);
		events.clear();
		sim->Submit(HGameplayCommand(PName(TestCommandReady), units[1]), events, &reason);
		const int32 changedAt = findEventIndex(events, TestEventChanged);
		const int32 actAt     = findStepIndex(events, TestStepAct);
		check(state.Turn.Step == PName(TestStepAct) && state.Turn.Round == 1 && state.Turn.Actors.size() == 1 && state.Turn.Actors[0] == units[2],
			"13 after setup the automatic step runs without input and the flow waits on the first actor it chose (reverse order: units[2])");
		check(changedAt != INDEX_NONE && actAt != INDEX_NONE && changedAt < actAt,
			PString::Format("13 effects raised by the automatic step resolve before the next step starts (change %d < step %d)", changedAt, actAt));

		// 행동 단계: 지금 행동자가 아니면 EndTurn 거절, 맞으면 흐름이 고른 다음 행동자. 되돌리기는 흐름 칸까지 되돌린다
		check(sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), units[0]), events, &reason) == EGameplaySubmitResult::Rejected,
			"13 EndTurn by a unit that is not acting is rejected");
		const uint64             beforeEnd     = sim->Checksum();
		const PName              beforeStep    = state.Turn.Step;
		const HList<HGameplayEntityId> beforeActors = state.Turn.Actors;
		const PName              beforeNext    = state.Turn.NextStep;
		EGameplaySubmitResult ended = sim->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), units[2]), events, &reason);
		check(ended == EGameplaySubmitResult::Executed && state.Turn.Actors.size() == 1 && state.Turn.Actors[0] == units[1],
			"13 EndTurn hands the turn to the next actor the flow chose (units[1])");
		check(sim->Undo() == true && sim->Checksum() == beforeEnd && state.Turn.Step == beforeStep && state.Turn.Actors == beforeActors && state.Turn.NextStep == beforeNext,
			"13 Undo restores the flow's common fields (Step, Actors, NextStep)");

		// AI 러너는 흐름의 입력 행동자를 본다
		sim->SetAgent(0, Allocate<PGameplayRandomAgent>((uint64)5));
		HGameplayCommand chosen;
		check(PGameplayAgentRunner::Choose(*sim, chosen) == true && chosen.Actor == units[2],
			"13 the agent runner chooses a command for the flow's input actor");

		// 유닛이 모두 사라져도 엔진이 끝내지 않는다. 흐름이 자기 규칙으로 라운드를 이어 가다 끝을 정한다
		events.clear();
		EGameplaySubmitResult killed = sim->Submit(HGameplayCommand(PName(TestCommandKillAll), units[2]), events, &reason);
		const int32 finishedAt = findEventIndex(events, HGameplayBuiltin::EventGameFinished);
		check(killed == EGameplaySubmitResult::Executed && state.Turn.IsFinished() == true && state.Turn.Round == 2
			&& finishedAt != INDEX_NONE && events[finishedAt].Amount == TestFlowFinishCode && state.Turn.Actors.empty() == true,
			PString::Format("13 with every unit gone the flow runs on to its own end (round %d, result %d) instead of a built-in empty-order finish",
				state.Turn.Round, finishedAt != INDEX_NONE ? events[finishedAt].Amount : -1));

		// 저장 문서 · 리플레이가 흐름 상태까지 같은 체크섬을 낸다
		PString document;
		sim->ExportDocument(&document);
		PSharedPtr<PGameMaster> loaded = Allocate<PGameMaster>();
		setupFlowGameMaster(*loaded, Allocate<PTestPhasesFlow>());
		check(loaded->ImportDocument(document) == true && loaded->Checksum() == sim->Checksum() && loaded->GetState().Turn.Step == state.Turn.Step,
			"13 a saved document of a flow game loads to the same checksum");

		PSharedPtr<PGameMaster> replayed = Allocate<PGameMaster>();
		setupFlowGameMaster(*replayed, Allocate<PTestPhasesFlow>());
		replayed->Start(seed);
		HList<HGameplayEvent> replayEvents;
		check(replayed->Replay(sim->GetCommandLog().Commands, replayEvents) == true && replayed->Checksum() == sim->Checksum(),
			"13 replaying the command log of a flow game gives the same checksum");

		// 같은 GameMaster 로 다음 판: Start 를 다시 부르면 흐름이 처음부터
		sim->Start(seed + 1);
		check(state.Turn.Step == PName(TestStepSetup) && state.Turn.Round == 0 && state.Turn.Actors.size() == 3 && state.Sequence == 0 && state.Turn.IsFinished() == false,
			"13 Start again on the same GameMaster begins a new game from the flow's first step");

		// 대기 없이 단계를 잇는 흐름은 한도에서 멈춘다 (의도한 [error] 로그 1줄)
		{
			PSharedPtr<PTestPhasesFlow> runaway = Allocate<PTestPhasesFlow>();
			runaway->bRunaway = true;
			PSharedPtr<PGameMaster> broken = Allocate<PGameMaster>();
			setupFlowGameMaster(*broken, runaway);
			HList<HGameplayEvent> brokenEvents;
			broken->Start(seed, &brokenEvents);
			const int32 limitAt = findEventIndex(brokenEvents, HGameplayBuiltin::EventFlowStepLimitExceeded);
			check(limitAt != INDEX_NONE && brokenEvents[limitAt].Amount == HGameplayLimits::MaxFlowStepsPerCommand,
				"13 a flow that never waits stops at the step limit (FlowStepLimitExceeded) instead of hanging");
		}
	}
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

	// 9. 죽은 ID 가드 (리뷰 C1 · R1–R3) -------------------------------------------------
	{
		PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
		sim->RegisterComponent<HGameplayTestValue>(PName(TestValueName));
		sim->RegisterZone(PName(TestZoneUnits));
		sim->RegisterZone(PName(TestZoneGrave));
		sim->RegisterHandler(Allocate<PTestStaleOpHandler>());
		sim->RegisterEffect(Allocate<PTestSpawnValueEffect>());
		sim->RegisterEffect(Allocate<PTestAddValueEffect>());
		sim->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName(TestZoneUnits)));
		sim->SetBoard(EGameplayBoardKind::Square, 5, 5);

		HGameplayState& initial = sim->EditInitialState();
		HGameplayEntityId a = initial.CreateEntity();
		HGameplayEntityId b = initial.CreateEntity();
		HGameplayTestValue va;
		va.Value = 10;
		HGameplayTestValue vb;
		vb.Value = 20;
		initial.Add<HGameplayTestValue>(a, va);
		initial.Add<HGameplayTestValue>(b, vb);
		initial.Zone(PName(TestZoneUnits)).PushBack(a);
		initial.Zone(PName(TestZoneUnits)).PushBack(b);
		initial.Board.SetPosition(a, HGameplayCoord(0, 0));
		initial.Board.SetPosition(b, HGameplayCoord(1, 1));
		sim->Start(seed);

		HGameplayCommand stale(PName("TestStaleOp"), a);
		stale.Targets.push_back(b);
		HList<HGameplayEvent> events;
		PString reason;
		check(sim->Submit(stale, events, &reason) == EGameplaySubmitResult::Executed, PString::Format("stale-target command executed (%s)", reason));

		const HGameplayState& state = sim->GetState();
		HGameplayEntityId spawned;
		for (const HGameplayEvent& event : events)
		{
			if (event.Kind == PName(HGameplayBuiltin::EventEntitySpawned))
			{
				spawned = event.Subject;
			}
		}
		check(spawned.Index == b.Index && spawned.Generation == b.Generation + 1, "new entity reuses the dead entity's index (the case being guarded)");

		const HGameplayCoord* spawnedPosition = state.Board.FindPosition(spawned);
		check(spawnedPosition != nullptr && *spawnedPosition == HGameplayCoord(2, 2), "R1: SetBoardPosition on a dead id keeps the new entity's position");
		check(state.Board.OccupantAt(HGameplayCoord(3, 3)).IsValid() == false, "R1: no phantom occupant left by the dead id");
		check(valueOf(*sim, spawned) == 50, PString::Format("R2: Add<T> on a dead id keeps the new entity's value 50, got %d", valueOf(*sim, spawned)));

		int32 deadComponents = 0;
		state.Each<HGameplayTestValue>([&](const HGameplayEntityId& id, const HGameplayTestValue&)
		{
			if (state.IsAlive(id) == false)
			{
				++deadComponents;
			}
		});
		check(deadComponents == 0, "R2: no components on dead ids");

		const HGameplayZone* grave = state.FindZone(PName(TestZoneGrave));
		check(grave != nullptr && grave->Contains(b) == false, "R3: MoveToZone does not put the dead id into a zone");
		check(hasEvent(events, HGameplayBuiltin::EventZoneChanged) == false && hasEvent(events, HGameplayBuiltin::EventBoardMoved) == false, "R1/R3: no ZoneChanged / BoardMoved events for the dead id");

		HGameplayState direct = state;
		HGameplayTestValue overwrite;
		overwrite.Value = 99;
		check(direct.Add<HGameplayTestValue>(b, overwrite) == nullptr, "state Add<T> returns nullptr for a dead id");
	}

	// 10. 문서 왕복 · 불러오기 후 트리거 순서 (리뷰 C7 · R7) · 규칙 지문 ---------------------------
	{
		auto setupOrdered = [&](PGameMaster& sim, bool bZFirst)
		{
			setupGameMaster(sim);
			sim.RegisterEffect(Allocate<PTestDoubleEffect>());
			sim.RegisterEffect(Allocate<PTestIncrementEffect>());
			PSharedPtr<PTestOrderTrigger> z = Allocate<PTestOrderTrigger>(PName("TestOrderZ"), PName("TestDouble"));
			PSharedPtr<PTestOrderTrigger> a = Allocate<PTestOrderTrigger>(PName("TestOrderA"), PName("TestIncrement"));
			if (bZFirst == true)
			{
				sim.RegisterTrigger(z);
				sim.RegisterTrigger(a);
			}
			else
			{
				sim.RegisterTrigger(a);
				sim.RegisterTrigger(z);
			}
		};

		PSharedPtr<PGameMaster> original = Allocate<PGameMaster>();
		setupOrdered(*original, true);
		original->Start(seed);

		PString document;
		check(original->ExportDocument(&document) == true && document.Empty() == false, "ExportDocument produces text");

		PSharedPtr<PGameMaster> imported = Allocate<PGameMaster>();
		setupOrdered(*imported, true);
		check(imported->ImportDocument(document) == true, "ImportDocument succeeds without Start");
		check(imported->Checksum() == original->Checksum(), "document round trip keeps the checksum");
		check(imported->IsStarted() == true, "imported gameMaster is started");

		HList<HGameplayEntityId> units = unitsOf(*original);
		HGameplayCommand act(PName(TestCommandAct), units[0]);
		act.Targets.push_back(units[1]);
		act.Params.push_back(3);

		HList<HGameplayEvent> originalEvents;
		HList<HGameplayEvent> importedEvents;
		PString reason;
		original->Submit(act, originalEvents, &reason);
		imported->Submit(act, importedEvents, &reason);
		check(valueOf(*original, units[1]) == 12, PString::Format("sorted trigger order gives (10 - 5 + 1) * 2 = 12, got %d", valueOf(*original, units[1])));
		check(valueOf(*imported, units[1]) == valueOf(*original, units[1]) && imported->Checksum() == original->Checksum(), PString::Format("R7: same trigger order after ImportDocument (%d vs %d)", valueOf(*imported, units[1]), valueOf(*original, units[1])));

		PSharedPtr<PGameMaster> reordered = Allocate<PGameMaster>();
		setupOrdered(*reordered, false);
		check(reordered->RulesFingerprint() == original->RulesFingerprint(), "rules fingerprint does not depend on registration order");

		PSharedPtr<PGameMaster> extra = Allocate<PGameMaster>();
		setupOrdered(*extra, true);
		extra->RegisterHandler(Allocate<PTestStaleOpHandler>());
		check(extra->RulesFingerprint() != original->RulesFingerprint(), "rules fingerprint changes when a handler is added");
	}

	// 11. Undo 가드 · 에이전트 명령 고르기 ------------------------------------------------------
	{
		PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
		setupGameMaster(*sim);
		sim->SetAgent(0, Allocate<PGameplayRandomAgent>((uint64)11));
		sim->Start(seed);

		uint64 beforeChoose = sim->Checksum();
		HGameplayCommand chosen;
		check(PGameplayAgentRunner::Choose(*sim, chosen) == true && chosen.Kind != NAME_NONE, "agent runner Choose returns a command");
		check(sim->Checksum() == beforeChoose && sim->GetState().Sequence == 0, "Choose does not change the state");

		HList<HGameplayEvent> events;
		PString reason;
		sim->Submit(chosen, events, &reason);
		uint64 afterSubmit = sim->Checksum();

		sim->SetUndoEnabled(false);
		check(sim->Undo() == false && sim->Checksum() == afterSubmit, "Undo is refused while disabled");
		sim->SetUndoEnabled(true);
		check(sim->Undo() == true && sim->Checksum() == beforeChoose, "Undo works again when enabled");
	}

	// 12. 리뷰 재현 회귀 (R4 · R5 · R6 · R8 · R16 · R17) --------------------------------------
	runReviewRegressions(check, seed);

	// 13. 게임 흐름 (ER-009): 흐름 교체 · 공용 칸 · 대기 지점 · 단계 수 한도 ----------------------
	runFlowChecks(check, seed);

	std::cout << "== GameMaster self test: " << check.Passed << " passed, " << check.Failures << " failed ==" << std::endl;
	JG_LOG(GameMasterSelfTest, ELogLevel::Info, "GameMaster self test: %d passed, %d failed", check.Passed, check.Failures);
	return check.Failures;
}
