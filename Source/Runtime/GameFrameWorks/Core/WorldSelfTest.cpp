#include "PCH/PCH.h"
#include "Core/WorldSelfTest.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Actors/GameMasterActor.h"
#include "Actors/GameplayEntityActor.h"
#include "GameMaster/GameMaster.h"
#include <iostream>

namespace
{
	struct HWorldCheck
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
			JG_LOG(WorldSelfTest, ELogLevel::Error, "[FAIL] %s", what);
		}
	};

	// 대상 엔티티를 파괴하는 최소 명령. 엔티티 파괴가 액터 파괴로 이어지는지 보기 위한 것.
	class PWorldTestKillHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("WorldTestKill");
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (state.IsAlive(command.Target()) == false)
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
			HGameplayEffectRequest destroy(PName(HGameplayBuiltin::EffectDestroyEntity), command.Actor, command.Target());
			ctx.Enqueue(destroy);
		}
	};

	// 엔티티를 만들고 같은 명령 안에서 곧바로 파괴한다. 한 프레임에 EntitySpawned · EntityDestroyed 가 함께 온다 (R15).
	class PWorldTestSpawnAndKillHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("WorldTestSpawnAndKill");
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return true;
		}

		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayEntityId id = ctx.SpawnEntity(PName("Units"));
			ctx.DestroyEntity(id);
		}
	};

	// 틱 안에서 액터를 스폰하고 같은 틱에 파괴한다 (R11).
	class PWorldTestSpawnerActor : public JGActor
	{
	public:
		PSharedPtr<JGActor> Spawned;

	protected:
		virtual void OnTick(float32 deltaSeconds) override
		{
			if (Spawned != nullptr)
			{
				return;
			}
			Spawned = GetWorld()->SpawnActor<JGActor>(PName("WorldTestZombie"));
			Spawned->Destroy();
		}
	};

	// 파괴 이벤트의 큐를 물어보는 순간 그 엔티티의 액터를 찾을 수 있는지 센다 (R13).
	class PWorldTestDeathCue : public JGGameplayCue
	{
	public:
		JGGameMasterActor* Owner = nullptr;
		mutable int32 Asked = 0;
		mutable int32 Found = 0;

		virtual bool Accepts(const HGameplayEvent& event) const override
		{
			if (event.Kind != PName(HGameplayBuiltin::EventEntityDestroyed))
			{
				return false;
			}
			++Asked;
			if (Owner != nullptr && Owner->FindActor(event.Subject) != nullptr)
			{
				++Found;
			}
			return true;
		}
	};

	int32 countEntityActors(PSharedPtr<PWorld> world)
	{
		HList<PSharedPtr<JGGameplayEntityActor>> actors;
		world->FindActors<JGGameplayEntityActor>(actors);
		return (int32)actors.size();
	}

	void tickFrames(PSharedPtr<PWorld> world, int32 frames)
	{
		for (int32 i = 0; i < frames; ++i)
		{
			world->Tick(1.0f / 60.0f);
		}
	}
}

int32 PWorldSelfTest::Run()
{
	HWorldCheck check;
	std::cout << "== World self test ==" << std::endl;

	check(JGGameInstance::HasInstance() == true, "game instance exists after module startup");
	if (JGGameInstance::HasInstance() == false)
	{
		return check.Failures;
	}

	JGGameInstance& gameInstance = JGGameInstance::Get();

	PSharedPtr<PWorld> world = gameInstance.LoadWorld(PName("SelfTestWorld"));
	check(world != nullptr && gameInstance.GetWorld() == world, "LoadWorld creates the active world");
	check(world != nullptr && world->HasBegun() == true, "loaded world has begun play");

	// GameMasterActor + GameMaster
	PSharedPtr<JGGameMasterActor> gameMasterActor = world->SpawnActor<JGGameMasterActor>(PName("GameMasterActor"));
	check(gameMasterActor != nullptr && gameMasterActor->GetWorld() == world, "gameMasterActor spawned into the world");

	PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
	sim->RegisterZone(PName("Units"));
	sim->RegisterHandler(Allocate<PWorldTestKillHandler>());
	sim->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName("Units")));

	HGameplayState& initial = sim->EditInitialState();
	HList<HGameplayEntityId> units;
	for (int32 i = 0; i < 3; ++i)
	{
		HGameplayEntityId id = initial.CreateEntity();
		initial.Zone(PName("Units")).PushBack(id);
		units.push_back(id);
	}

	gameMasterActor->SetGameMaster(sim);
	sim->Start(777);

	tickFrames(world, 5);
	check(gameMasterActor->IsBusy() == false, "gameMasterActor drained the start events");
	check(countEntityActors(world) == 3, PString::Format("3 entity actors bound after start, got %d", countEntityActors(world)));
	check(gameMasterActor->FindActor(units[0]) != nullptr, "entity id resolves to an actor");

	PSharedPtr<JGGameplayEntityActor> bound = RawDynamicCast<JGGameplayEntityActor>(gameMasterActor->FindActor(units[1]));
	check(bound != nullptr && bound->GetEntityId() == units[1], "bound actor carries its entity id");

	// 명령 → 파괴 이벤트 → 액터 제거
	HGameplayCommand kill(PName("WorldTestKill"), units[0]);
	kill.Targets.push_back(units[2]);
	PString reason;
	EGameMasterActorSubmit result = gameMasterActor->Submit(kill, &reason);
	check(result == EGameMasterActorSubmit::Executed, PString::Format("gameMasterActor executed the command (%s)", reason));

	tickFrames(world, 5);
	check(countEntityActors(world) == 2, PString::Format("entity actor removed after EntityDestroyed, got %d", countEntityActors(world)));
	check(gameMasterActor->FindActor(units[2]) == nullptr, "destroyed entity no longer resolves");

	// 연출 중 입력 버퍼링: 이벤트가 큐에 있는 동안 제출하면 Buffered, 틱 후 실행
	HGameplayCommand endTurn(PName(HGameplayBuiltin::CommandEndTurn), units[0]);
	gameMasterActor->SetInputPolicy(EGameplayInputPolicy::Buffer);
	HGameplayCommand kill2(PName("WorldTestKill"), units[0]);
	kill2.Targets.push_back(units[1]);
	gameMasterActor->Submit(kill2, &reason);                                       // 이벤트가 큐에 들어간다 (아직 틱 전)
	EGameMasterActorSubmit buffered = gameMasterActor->Submit(endTurn, &reason);
	check(buffered == EGameMasterActorSubmit::Buffered, "command submitted while busy is buffered");
	tickFrames(world, 5);
	check(sim->GetState().Turn.TurnCount >= 2, "buffered command executed after the presentation drained");

	// 되돌리기 → 상태 교체 → 바인딩 재구성
	int32 before = countEntityActors(world);
	sim->Undo();
	sim->Undo();
	tickFrames(world, 5);
	check(countEntityActors(world) == before + 1, PString::Format("bindings rebuilt after undo (%d -> %d)", before, countEntityActors(world)));

	// 언로드
	gameInstance.UnloadWorld();
	check(gameInstance.GetWorld() == nullptr && world->HasBegun() == false, "UnloadWorld ends play and clears the active world");

	// 리뷰 재현 회귀 (R11 · R12 · R13 · R15). 게임 인스턴스와 상관없는 별도 월드에서 본다.
	{
		PSharedPtr<PWorld> reviewWorld = Allocate<PWorld>(PName("ReviewRegressionWorld"));
		reviewWorld->BeginPlay();

		// R11: 틱 안에서 스폰하고 같은 틱에 파괴한 액터는 월드에 들어가지 않는다
		PSharedPtr<PWorldTestSpawnerActor> spawner = reviewWorld->SpawnActor<PWorldTestSpawnerActor>(PName("Spawner"));
		tickFrames(reviewWorld, 3);
		PSharedPtr<JGActor> zombie = spawner->Spawned;
		bool bZombieInWorld = false;
		for (const PSharedPtr<JGActor>& actor : reviewWorld->GetActors())
		{
			if (actor == zombie)
			{
				bZombieInWorld = true;
			}
		}
		check(zombie != nullptr && bZombieInWorld == false && zombie->HasBegunPlay() == false, "R11 an actor spawned and destroyed in one tick never enters the world");

		// R12: 부모가 있는 액터의 월드 위치
		PSharedPtr<JGActor> parent = reviewWorld->SpawnActor<JGActor>(PName("Parent"));
		PSharedPtr<JGActor> child  = reviewWorld->SpawnActor<JGActor>(PName("Child"));
		parent->SetLocalPosition(HVector3(5.0f, 0.0f, 0.0f));
		child->SetLocalPosition(HVector3(1.0f, 2.0f, 3.0f));
		child->AttachTo(parent);
		HVector3 position = child->GetWorldPosition();
		check(position.x == 6.0f && position.y == 2.0f && position.z == 3.0f, PString::Format("R12 child world position (%.1f, %.1f, %.1f) == (6, 2, 3)", position.x, position.y, position.z));

		// R13 · R15 는 GameMasterActor 로 본다
		PSharedPtr<JGGameMasterActor> reviewMaster = reviewWorld->SpawnActor<JGGameMasterActor>(PName("ReviewGameMasterActor"));
		PSharedPtr<PGameMaster> reviewSim = Allocate<PGameMaster>();
		reviewSim->RegisterZone(PName("Units"));
		reviewSim->RegisterHandler(Allocate<PWorldTestKillHandler>());
		reviewSim->RegisterHandler(Allocate<PWorldTestSpawnAndKillHandler>());
		reviewSim->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName("Units")));

		HList<HGameplayEntityId> reviewUnits;
		HGameplayState& reviewInitial = reviewSim->EditInitialState();
		for (int32 i = 0; i < 2; ++i)
		{
			HGameplayEntityId id = reviewInitial.CreateEntity();
			reviewInitial.Zone(PName("Units")).PushBack(id);
			reviewUnits.push_back(id);
		}

		PSharedPtr<PWorldTestDeathCue> deathCue = Allocate<PWorldTestDeathCue>();
		deathCue->Owner = reviewMaster.GetRawPointer();
		reviewMaster->RegisterCue(deathCue);
		reviewMaster->SetGameMaster(reviewSim);
		reviewSim->Start(777);
		tickFrames(reviewWorld, 3);

		// R13: 사망 큐를 고르는 순간 액터가 아직 있고, 큐가 끝나면 없어진다
		bool bBoundBefore = reviewMaster->FindActor(reviewUnits[1]) != nullptr;
		HGameplayCommand reviewKill(PName("WorldTestKill"), reviewUnits[0]);
		reviewKill.Targets.push_back(reviewUnits[1]);
		PString reviewReason;
		reviewMaster->Submit(reviewKill, &reviewReason);
		tickFrames(reviewWorld, 3);
		check(bBoundBefore == true && deathCue->Asked == 1 && deathCue->Found == 1 && reviewMaster->FindActor(reviewUnits[1]) == nullptr,
			PString::Format("R13 the death cue finds its actor (asked %d, found %d) and the actor goes after the cue", deathCue->Asked, deathCue->Found));

		// R15: 한 프레임에 스폰 · 파괴가 함께 와도 좀비 액터가 남지 않는다
		reviewMaster->Submit(HGameplayCommand(PName("WorldTestSpawnAndKill"), reviewUnits[0]), &reviewReason);
		tickFrames(reviewWorld, 3);
		int32 zombies = 0;
		int32 entityActors = 0;
		for (const PSharedPtr<JGActor>& actor : reviewWorld->GetActors())
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
		check(zombies == 0 && entityActors == (int32)reviewSim->GetState().Entities.Count(),
			PString::Format("R15 spawn + destroy in one frame leaves no zombie (entity actors %d, alive entities %u)", entityActors, reviewSim->GetState().Entities.Count()));

		reviewWorld->EndPlay();
	}

	std::cout << "== World self test: " << check.Passed << " passed, " << check.Failures << " failed ==" << std::endl;
	JG_LOG(WorldSelfTest, ELogLevel::Info, "World self test: %d passed, %d failed", check.Passed, check.Failures);
	return check.Failures;
}
