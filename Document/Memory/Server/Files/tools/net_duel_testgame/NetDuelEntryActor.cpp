#include "PCH/PCH.h"
#include "NetDuelEntryActor.h"
#include "Core/World.h"
#include "Core/GameInstance.h"
#include "Actors/GameMasterActor.h"
#include "Actors/GameplayControllerActor.h"
#include "GameMaster/GameMaster.h"
#include "GameMaster/Agents/GameplayAgent.h"
#include "Network/Session/GameplaySession.h"
#include "Network/Session/GameplayClientSession.h"

// Server Phase 5-6 검증용 게임 코드 (엔진 밖, 스크래치패드 프로젝트). 게임 콘텐츠가 아니라 검증 장치다.
// 2인 결투: 팀마다 유닛 2(체력 6). 공격은 난수 피해 1~3, 체력 0 이면 파괴, 한 팀이 전멸하면 끝난다.
// 멀티플레이 규약: 규칙 등록은 모든 기계, 초기 상태와 시작은 권한 쪽만. 로컬 플레이어의 입력은 스크립트 액터가 컨트롤러로 넣는다.
// 실행: 호스트 JGLauncher.exe -host -bind=127.0.0.1 · 참가자 JGLauncher.exe -join=127.0.0.1 -name=Guest
namespace
{
	const char* DuelUnitsZone   = "DuelUnits";
	const char* DuelUnitName    = "DuelUnit";
	const char* DuelAttackKind  = "DuelAttack";
	const char* DuelDamageKind  = "DuelDamage";
	const char* DuelDamagedKind = "DuelDamaged";
	constexpr int32   DuelUnitsPerTeam = 2;
	constexpr int32   DuelUnitHp       = 6;
	constexpr uint64  DuelSeed         = 20260930;
	constexpr float32 DuelInputDelay   = 0.1f;

	struct HNetDuelUnit : public IJsonable
	{
		int32 Hp   = 0;
		int32 Team = 0;

	protected:
		virtual void WriteJson(PJsonData& json) const override
		{
			json.AddMember("Hp", Hp);
			json.AddMember("Team", Team);
		}

		virtual void ReadJson(const PJsonData& json) override
		{
			json.GetData("Hp", &Hp);
			json.GetData("Team", &Team);
		}
	};

	class PNetDuelAttackHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(DuelAttackKind);
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (state.Turn.CanAct() == false || state.Turn.IsActorTurn(command.Actor) == false)
			{
				if (outReason != nullptr)
				{
					*outReason = "not your turn";
				}
				return false;
			}

			const HNetDuelUnit* attacker = state.Find<HNetDuelUnit>(command.Actor);
			const HNetDuelUnit* target   = state.Find<HNetDuelUnit>(command.Target());
			if (attacker == nullptr || target == nullptr || state.IsAlive(command.Target()) == false || attacker->Team == target->Team)
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
			ctx.Enqueue(HGameplayEffectRequest(PName(DuelDamageKind), command.Actor, command.Target()));
		}

		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			const HNetDuelUnit* attacker = state.Find<HNetDuelUnit>(actor);
			if (attacker == nullptr)
			{
				return;
			}
			state.Each<HNetDuelUnit>([&](const HGameplayEntityId& id, const HNetDuelUnit& unit)
			{
				if (unit.Team == attacker->Team)
				{
					return;
				}
				HGameplayCommand command(GetKind(), actor);
				command.Targets.push_back(id);
				outCommands.push_back(command);
			});
		}
	};

	class PNetDuelDamageEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName(DuelDamageKind);
		}

		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HNetDuelUnit* unit = ctx.State.Find<HNetDuelUnit>(request.Target);
			if (unit == nullptr)
			{
				return;
			}

			const int32 before = unit->Hp;
			const int32 amount = ctx.Rng(EGameplayRandomStream::Action).Range(1, 3);
			unit->Hp -= amount;
			const int32 team = unit->Team;

			HGameplayEvent damaged(PName(DuelDamagedKind), request.Subject, request.Target);
			damaged.Before = before;
			damaged.After  = unit->Hp;
			damaged.Amount = amount;
			ctx.Emit(damaged);

			if (unit->Hp > 0)
			{
				return;
			}

			// 이 유닛이 쓰러지면 그 팀에 남는 유닛이 있는가. 없으면 공격한 팀의 승리로 끝낸다.
			int32 remaining = 0;
			ctx.State.Each<HNetDuelUnit>([&](const HGameplayEntityId& id, const HNetDuelUnit& other)
			{
				if (other.Team == team && id != request.Target && other.Hp > 0)
				{
					++remaining;
				}
			});

			ctx.Enqueue(HGameplayEffectRequest(PName(HGameplayBuiltin::EffectDestroyEntity), request.Subject, request.Target));
			if (remaining == 0)
			{
				ctx.FinishGame(team == 0 ? 1 : 0);
			}
		}
	};

	void registerDuelRules(PGameMaster& gameMaster)
	{
		gameMaster.RegisterComponent<HNetDuelUnit>(PName(DuelUnitName));
		gameMaster.RegisterZone(PName(DuelUnitsZone));
		gameMaster.RegisterHandler(Allocate<PNetDuelAttackHandler>());
		gameMaster.RegisterEffect(Allocate<PNetDuelDamageEffect>());
		gameMaster.SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName(DuelUnitsZone)));
		gameMaster.SetTeamOfActorFunction([](const HGameplayState& state, const HGameplayEntityId& actor) -> int32
		{
			const HNetDuelUnit* unit = state.Find<HNetDuelUnit>(actor);
			if (unit == nullptr)
			{
				return INDEX_NONE;
			}
			return unit->Team;
		});
	}

	// 권한 쪽만: 유닛을 팀 번갈아 턴 순서에 넣는다 (팀 0 · 1 · 0 · 1).
	void buildDuelInitialState(PGameMaster& gameMaster)
	{
		HGameplayState& initial = gameMaster.EditInitialState();
		for (int32 i = 0; i < DuelUnitsPerTeam * 2; ++i)
		{
			const HGameplayEntityId id = initial.CreateEntity();
			HNetDuelUnit unit;
			unit.Hp   = DuelUnitHp;
			unit.Team = i % 2;
			initial.Add<HNetDuelUnit>(id, unit);
			initial.Zone(PName(DuelUnitsZone)).PushBack(id);
		}
	}

	const char* netModeName(EGameplayNetMode mode)
	{
		switch (mode)
		{
		case EGameplayNetMode::Standalone:   return "Standalone";
		case EGameplayNetMode::ListenServer: return "ListenServer";
		case EGameplayNetMode::Client:       return "Client";
		default:                             return "?";
		}
	}

	// 로컬 플레이어 대신 두는 검증 스크립트: 내 차례면 0.1 초마다 무작위 합법 명령을 컨트롤러로 넣는다. 끝나면 결과를 한 번 남긴다.
	class PNetDuelScriptActor : public JGActor
	{
	public:
		PWeakPtr<JGGameMasterActor>         GameMasterActor;
		PWeakPtr<JGGameplayControllerActor> Controller;
		PSharedPtr<PGameplayRandomAgent>    Agent;

	private:
		float32 _delay     = DuelInputDelay;
		int32   _submitted = 0;
		bool    _bReported = false;

	protected:
		virtual void OnTick(float32 deltaSeconds) override
		{
			PSharedPtr<JGGameMasterActor>         gameMasterActor = GameMasterActor.Pin();
			PSharedPtr<JGGameplayControllerActor> controller      = Controller.Pin();
			PSharedPtr<PGameMaster>               gameMaster      = gameMasterActor != nullptr ? gameMasterActor->GetGameMaster() : nullptr;
			PSharedPtr<PGameplaySession>          session         = gameMasterActor != nullptr ? gameMasterActor->GetSession() : nullptr;
			if (gameMaster == nullptr || controller == nullptr || session == nullptr || gameMaster->IsStarted() == false)
			{
				return;
			}

			if (gameMaster->GetState().Turn.IsFinished() == true)
			{
				report(*gameMaster, *session);
				return;
			}

			_delay -= deltaSeconds;
			if (_delay > 0.0f || session->GetState() != EGameplaySessionState::Playing || gameMasterActor->IsBusy() == true)
			{
				return;
			}
			PSharedPtr<PGameplayClientSession> client = RawDynamicCast<PGameplayClientSession>(session);
			if (client != nullptr && client->HasPendingLocalCommand() == true)
			{
				return;
			}

			const HGameplayEntityId actor = controller->GetInputActor();
			if (actor.IsValid() == false || controller->IsLocallyControlled(actor) == false)
			{
				return;
			}

			HGameplayCommand command;
			if (Agent->ChooseCommand(*gameMaster, actor, command) == false || controller->BeginCommand(command.Kind, command.Actor) == false)
			{
				return;
			}
			for (const HGameplayEntityId& target : command.Targets)
			{
				controller->AddTarget(target);
			}

			PString reason;
			const EGameMasterActorSubmit result = controller->Commit(&reason);
			if (result == EGameMasterActorSubmit::Rejected)
			{
				JG_LOG(NetDuel, ELogLevel::Warning, "NetDuel script: %s rejected: %s", command.ToString(), reason);
			}
			else
			{
				++_submitted;
			}
			_delay = DuelInputDelay;
		}

	private:
		void report(PGameMaster& gameMaster, PGameplaySession& session)
		{
			if (_bReported == true)
			{
				return;
			}
			_bReported = true;

			// 이긴 팀 = 유닛이 남은 팀
			int32 winner = INDEX_NONE;
			gameMaster.GetState().Each<HNetDuelUnit>([&](const HGameplayEntityId&, const HNetDuelUnit& unit)
			{
				if (unit.Hp > 0)
				{
					winner = unit.Team;
				}
			});

			PGameplayClientSession* client = dynamic_cast<PGameplayClientSession*>(&session);
			JG_LOG(NetDuel, ELogLevel::Info, "NetDuel: finished mode=%s slot=%d seq=%u checksum=%llu winner=%d submitted=%d desyncs=%d",
				PString(netModeName(session.GetMode())), session.GetLocalSlot(), gameMaster.GetState().Sequence, gameMaster.Checksum(), winner, _submitted,
				client != nullptr ? client->GetDesyncCount() : 0);
		}
	};
}

void JGNetDuelEntryActor::OnEnterWorld()
{
	PSharedPtr<PWorld> world = GetWorld();

	PSharedPtr<JGGameMasterActor>         gameMasterActor = world->SpawnActor<JGGameMasterActor>(PName("GameMaster"));
	PSharedPtr<JGGameplayControllerActor> controller      = world->SpawnActor<JGGameplayControllerActor>(PName("Controller"));
	controller->SetGameMasterActor(gameMasterActor);

	// 규칙 등록: 모든 기계
	PSharedPtr<PGameMaster> gameMaster = Allocate<PGameMaster>();
	registerDuelRules(*gameMaster);
	gameMasterActor->SetGameMaster(gameMaster);

	// 초기 상태 + 시작: 권한 쪽만 (클라는 호스트가 보낸 상태로 시작한다)
	if (GetGameInstance().IsAuthority() == true)
	{
		buildDuelInitialState(*gameMaster);
		gameMasterActor->StartGame(DuelSeed);
	}

	PSharedPtr<PNetDuelScriptActor> script = world->SpawnActor<PNetDuelScriptActor>(PName("DuelScript"));
	script->GameMasterActor = gameMasterActor;
	script->Controller      = controller;

	PSharedPtr<PGameplaySession> session = GetGameInstance().GetSession();
	const int32 localSlot = session != nullptr ? session->GetLocalSlot() : INDEX_NONE;
	script->Agent = Allocate<PGameplayRandomAgent>(DuelSeed + (uint64)(localSlot + 2));

	JG_LOG(NetDuel, ELogLevel::Info, "NetDuel entry actor entered world: mode=%s authority=%d",
		PString(session != nullptr ? netModeName(session->GetMode()) : "none"), GetGameInstance().IsAuthority() == true ? 1 : 0);
}
