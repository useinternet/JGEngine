#include "PCH/PCH.h"
#include "SelfTest/GameplayNetSelfTest.h"
#include "Network/Transport/NetLoopbackTransport.h"
#include "Network/Transport/NetTcpTransport.h"
#include "Network/Session/GameplayHostSession.h"
#include "Network/Session/GameplayClientSession.h"
#include "GameMaster/GameMaster.h"
#include "GameMaster/Agents/GameplayAgent.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Actors/GameMasterActor.h"
#include "Actors/GameplayControllerActor.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include <iostream>
#include <chrono>
#include <thread>

// ---- 테스트 전용 최소 규칙 ---------------------------------------------------------
// 턴 순서(NetUnits)의 유닛은 죽지 않는다(체력 0 이면 30 으로). 소환 · 파괴는 토큰(NetTokens)이 맡아 엔티티 번호 재사용을 만든다.
// 난수(피해량 · 섞기) · 선택 대기(NetPick) · 트리거(NetThorns) · 호스트가 돌리는 적 AI 를 모두 지나가게 한다.

namespace
{
	const char* NetUnitComponent = "NetUnit";
	const char* NetZoneUnits     = "NetUnits";
	const char* NetZoneTokens    = "NetTokens";
	const char* NetEventDamaged  = "NetDamaged";
	constexpr int32   NetMaxTokensPerController = 2;
	constexpr uint64  NetEnemyAgentSeed         = 777;
	constexpr float32 NetTestDeltaSeconds       = 1.0f / 60.0f;

	struct HNetTestUnit : public IJsonable
	{
		int32 Hp         = 0;
		int32 Controller = 0;
		bool  bToken     = false;

	protected:
		virtual void WriteJson(PJsonData& json) const override
		{
			json.AddMember("Hp", Hp);
			json.AddMember("Controller", Controller);
			json.AddMember("Token", bToken);
		}
		virtual void ReadJson(const PJsonData& json) override
		{
			json.GetData("Hp", &Hp);
			json.GetData("Controller", &Controller);
			json.GetData("Token", &bToken);
		}
	};

	bool checkActorTurn(const HGameplayState& state, const HGameplayCommand& command, PString* outReason)
	{
		if (state.Turn.CanAct() == false || state.Turn.IsActorTurn(command.Actor) == false)
		{
			if (outReason != nullptr)
			{
				*outReason = "not your turn";
			}
			return false;
		}
		return true;
	}

	int32 countTokens(const HGameplayState& state, int32 controller)
	{
		int32 count = 0;
		state.Each<HNetTestUnit>([&](const HGameplayEntityId&, const HNetTestUnit& unit)
		{
			if (unit.bToken == true && unit.Controller == controller)
			{
				++count;
			}
		});
		return count;
	}

	class PNetStrikeHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetStrike");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (checkActorTurn(state, command, outReason) == false)
			{
				return false;
			}
			HGameplayEntityId target = command.Target();
			if (state.IsAlive(target) == false || state.Has<HNetTestUnit>(target) == false || target == command.Actor)
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
			ctx.Enqueue(HGameplayEffectRequest(PName("NetDamage"), command.Actor, command.Target()));
		}
		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			state.Each<HNetTestUnit>([&](const HGameplayEntityId& id, const HNetTestUnit&)
			{
				if (id == actor)
				{
					return;
				}
				HGameplayCommand command(GetKind(), actor);
				command.Targets.push_back(id);
				outCommands.push_back(command);
			});
		}
	};

	class PNetSummonHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetSummon");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (checkActorTurn(state, command, outReason) == false)
			{
				return false;
			}
			const HNetTestUnit* unit = state.Find<HNetTestUnit>(command.Actor);
			if (unit == nullptr || countTokens(state, unit->Controller) >= NetMaxTokensPerController)
			{
				if (outReason != nullptr)
				{
					*outReason = "token limit";
				}
				return false;
			}
			return true;
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			ctx.Enqueue(HGameplayEffectRequest(PName("NetSummonToken"), command.Actor, HGameplayEntityId::None()));
		}
		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			outCommands.push_back(HGameplayCommand(GetKind(), actor));
		}
	};

	class PNetPickHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetPick");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return checkActorTurn(state, command, outReason);
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			ctx.Enqueue(HGameplayEffectRequest(PName("NetPickDamage"), command.Actor, HGameplayEntityId::None()));
		}
		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			outCommands.push_back(HGameplayCommand(GetKind(), actor));
		}
	};

	class PNetShuffleHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetShuffle");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (checkActorTurn(state, command, outReason) == false)
			{
				return false;
			}
			const HGameplayZone* tokens = state.FindZone(PName(NetZoneTokens));
			if (tokens == nullptr || tokens->Count() < 2)
			{
				if (outReason != nullptr)
				{
					*outReason = "nothing to shuffle";
				}
				return false;
			}
			return true;
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			ctx.Enqueue(HGameplayEffectRequest(PName("NetShuffleTokens"), command.Actor, HGameplayEntityId::None()));
		}
		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			outCommands.push_back(HGameplayCommand(GetKind(), actor));
		}
	};

	// 난수 피해. 토큰은 0 이하에서 파괴, 유닛은 30 으로 되돌린다 (턴 순서가 흔들리지 않게).
	class PNetDamageEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetDamage");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HNetTestUnit* unit = ctx.State.Find<HNetTestUnit>(request.Target);
			if (unit == nullptr)
			{
				return;
			}

			int32 before = unit->Hp;
			int32 amount = ctx.Rng(EGameplayRandomStream::Action).Range(1, 3);
			unit->Hp -= amount;

			bool bDestroy = false;
			if (unit->Hp <= 0)
			{
				if (unit->bToken == true)
				{
					bDestroy = true;
				}
				else
				{
					unit->Hp = 30;
				}
			}
			int32 after = unit->Hp;

			HGameplayEvent damaged(PName(NetEventDamaged), request.Subject, request.Target);
			damaged.Before = before;
			damaged.After  = after;
			damaged.Amount = amount;
			ctx.Emit(damaged);

			if (bDestroy == true)
			{
				ctx.Enqueue(HGameplayEffectRequest(PName(HGameplayBuiltin::EffectDestroyEntity), request.Subject, request.Target));
			}
		}
	};

	class PNetSummonTokenEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetSummonToken");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			const HNetTestUnit* owner = ctx.State.Find<HNetTestUnit>(request.Subject);
			if (owner == nullptr)
			{
				return;
			}
			int32 controller = owner->Controller;

			HGameplayEntityId id = ctx.SpawnEntity(PName(NetZoneTokens));
			HNetTestUnit token;
			token.Hp         = 3;
			token.Controller = controller;
			token.bToken     = true;
			ctx.State.Add<HNetTestUnit>(id, token);
		}
	};

	// 선택 대기: 행동자가 자기 외 대상 하나를 고른다 → 그 대상에 난수 피해.
	class PNetPickDamageEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetPickDamage");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			if (request.HasChoice() == false)
			{
				HGameplayChoice choice;
				choice.Chooser = request.Subject;
				choice.Kind    = PName("NetPick");
				choice.Min     = 1;
				choice.Max     = 1;
				ctx.State.Each<HNetTestUnit>([&](const HGameplayEntityId& id, const HNetTestUnit&)
				{
					if (id != request.Subject)
					{
						choice.Candidates.push_back(id);
					}
				});
				if (choice.Candidates.empty() == true)
				{
					return;
				}
				ctx.RequestChoice(choice);
				return;
			}

			ctx.Enqueue(HGameplayEffectRequest(PName("NetDamage"), request.Subject, request.Choice[0]));
		}
	};

	class PNetShuffleTokensEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetShuffleTokens");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			ctx.State.Zone(PName(NetZoneTokens)).Shuffle(ctx.Rng(EGameplayRandomStream::Shuffle));
		}
	};

	class PNetHealEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetHeal");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			HNetTestUnit* unit = ctx.State.Find<HNetTestUnit>(request.Target);
			if (unit != nullptr)
			{
				unit->Hp += request.Param(0, 0);
			}
		}
	};

	// 유닛이 3 피해를 받으면 1 회복 (트리거 경로).
	class PNetThornsTrigger : public JGGameplayTrigger
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetThorns");
		}
		virtual bool Matches(const HGameplayState& state, const HGameplayEvent& event) const override
		{
			if (event.Kind != PName(NetEventDamaged) || event.Amount != 3 || state.IsAlive(event.Target) == false)
			{
				return false;
			}
			const HNetTestUnit* unit = state.Find<HNetTestUnit>(event.Target);
			return unit != nullptr && unit->bToken == false;
		}
		virtual void React(HGameplayTriggerContext& ctx, const HGameplayEvent& event) override
		{
			HGameplayEffectRequest heal(PName("NetHeal"), event.Target, event.Target);
			heal.Params.push_back(1);
			ctx.Enqueue(heal);
		}
	};

	// 지문 불일치 검사용: 호스트에 없는 명령.
	class PNetExtraHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetExtra");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return false;
		}
	};

	// 모든 기계가 같은 코드로 규칙을 등록한다. 조작 주체 = 유닛의 Controller. 적(Controller == playerCount)은 AI.
	void setupNetRules(PGameMaster& gameMaster, int32 playerCount)
	{
		gameMaster.RegisterComponent<HNetTestUnit>(PName(NetUnitComponent));
		gameMaster.RegisterZone(PName(NetZoneUnits));
		gameMaster.RegisterZone(PName(NetZoneTokens));

		gameMaster.RegisterHandler(Allocate<PNetStrikeHandler>());
		gameMaster.RegisterHandler(Allocate<PNetSummonHandler>());
		gameMaster.RegisterHandler(Allocate<PNetPickHandler>());
		gameMaster.RegisterHandler(Allocate<PNetShuffleHandler>());
		gameMaster.RegisterEffect(Allocate<PNetDamageEffect>());
		gameMaster.RegisterEffect(Allocate<PNetSummonTokenEffect>());
		gameMaster.RegisterEffect(Allocate<PNetPickDamageEffect>());
		gameMaster.RegisterEffect(Allocate<PNetShuffleTokensEffect>());
		gameMaster.RegisterEffect(Allocate<PNetHealEffect>());
		gameMaster.RegisterTrigger(Allocate<PNetThornsTrigger>());

		gameMaster.SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName(NetZoneUnits)));
		gameMaster.SetBoard(EGameplayBoardKind::None);
		gameMaster.SetSnapshotLimit(8);
		gameMaster.SetTeamOfActorFunction([](const HGameplayState& state, const HGameplayEntityId& actor) -> int32
		{
			const HNetTestUnit* unit = state.Find<HNetTestUnit>(actor);
			if (unit == nullptr)
			{
				return INDEX_NONE;
			}
			return unit->Controller;
		});
		gameMaster.SetAgent(playerCount, Allocate<PGameplayRandomAgent>(NetEnemyAgentSeed));
	}

	// 권한 쪽만 만든다. 클라는 StartGame 으로 받는다.
	void buildNetInitialState(PGameMaster& gameMaster, int32 playerCount)
	{
		HGameplayState& initial = gameMaster.EditInitialState();
		for (int32 controller = 0; controller <= playerCount; ++controller)
		{
			HGameplayEntityId id = initial.CreateEntity();
			HNetTestUnit unit;
			unit.Hp         = controller == playerCount ? 40 : 30;
			unit.Controller = controller;
			initial.Add<HNetTestUnit>(id, unit);
			initial.Zone(PName(NetZoneUnits)).PushBack(id);
		}
	}

	HGameplayEntityId unitOfController(const HGameplayState& state, int32 controller)
	{
		HGameplayEntityId found;
		state.Each<HNetTestUnit>([&](const HGameplayEntityId& id, const HNetTestUnit& unit)
		{
			if (unit.bToken == false && unit.Controller == controller && found.IsValid() == false)
			{
				found = id;
			}
		});
		return found;
	}

	int32 countCommands(const PGameMaster& gameMaster, const char* kind)
	{
		PName name(kind);
		int32 count = 0;
		for (const HGameplayCommand& command : gameMaster.GetCommandLog().Commands)
		{
			if (command.Kind == name)
			{
				++count;
			}
		}
		return count;
	}

	int32 countCommandsBy(const PGameMaster& gameMaster, const HGameplayEntityId& actor)
	{
		int32 count = 0;
		for (const HGameplayCommand& command : gameMaster.GetCommandLog().Commands)
		{
			if (command.Actor == actor)
			{
				++count;
			}
		}
		return count;
	}

	struct HCheck
	{
		int32 Failures = 0;
		int32 Passed   = 0;
		int32 Skipped  = 0;

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
			JG_LOG(NetSelfTest, ELogLevel::Error, "[FAIL] %s", what);
		}
	};

	void info(const PString& text)
	{
		std::cout << "  [INFO] " << text.GetRawString() << std::endl;
	}

	// 참가자 한 명: 세션 + 복제(또는 권한) GameMaster + 입력을 흉내 내는 무작위 에이전트.
	struct HNetParticipant
	{
		PSharedPtr<PGameplayHostSession>   Host;
		PSharedPtr<PGameplayClientSession> Client;
		PSharedPtr<PGameMaster>            GameMaster;
		PSharedPtr<PGameplayRandomAgent>   Agent;
		PSharedPtr<PNetLoopbackTransport>  Loopback;
		int32                              Submitted = 0;

		PGameplaySession* Session() const
		{
			if (Host != nullptr)
			{
				return Host.GetRawPointer();
			}
			return Client.GetRawPointer();
		}

		void Tick(float32 deltaSeconds)
		{
			PGameplaySession* session = Session();
			if (session != nullptr)
			{
				session->Tick(deltaSeconds);
			}
		}
	};

	// 로컬 플레이어 입력 한 번. 지금 입력을 받는 행동자가 내 것이면 에이전트가 고른 명령을 세션에 넣는다.
	bool driveInput(HNetParticipant& participant)
	{
		PGameplaySession* session = participant.Session();
		if (session == nullptr || participant.GameMaster == nullptr || session->GetState() != EGameplaySessionState::Playing)
		{
			return false;
		}
		if (participant.Client != nullptr && participant.Client->HasPendingLocalCommand() == true)
		{
			return false;
		}

		const HGameplayState& state = participant.GameMaster->GetState();
		if (state.Turn.IsFinished() == true)
		{
			return false;
		}
		HGameplayEntityId actor = session->GetInputActor();
		if (actor.IsValid() == false || session->IsLocallyControlled(actor) == false)
		{
			return false;
		}

		HGameplayCommand command;
		if (state.Choice.bPending == true)
		{
			HList<HGameplayEntityId> selection;
			if (participant.Agent->ChooseOption(*participant.GameMaster, state.Choice, selection) == false)
			{
				return false;
			}
			command = HGameplayCommand(PName(HGameplayBuiltin::CommandResolveChoice), state.Choice.Chooser);
			command.Targets = selection;
		}
		else
		{
			if (participant.Agent->ChooseCommand(*participant.GameMaster, actor, command) == false)
			{
				return false;
			}
		}

		EGameplaySessionSubmit result = session->SubmitLocal(command);
		if (result == EGameplaySessionSubmit::Rejected)
		{
			return false;
		}
		++participant.Submitted;
		return true;
	}

	HNetParticipant makeLoopbackClient(PSharedPtr<PNetLoopbackHub> hub, uint16 port, int32 playerCount, const PString& name, uint64 agentSeed, const PString& dumpDirectory, const PString& reconnectToken = PString())
	{
		HNetParticipant participant;
		participant.GameMaster = Allocate<PGameMaster>();
		setupNetRules(*participant.GameMaster, playerCount);

		HGameplaySessionConfig config;
		config.PlayerName          = name;
		config.DesyncDumpDirectory = dumpDirectory;
		config.ReconnectToken      = reconnectToken;

		participant.Loopback = Allocate<PNetLoopbackTransport>(hub);
		participant.Client   = PGameplayClientSession::Create(participant.Loopback, "loopback", port, config);
		participant.Client->BindGameMaster(participant.GameMaster);
		participant.Agent = Allocate<PGameplayRandomAgent>(agentSeed);
		return participant;
	}

	void tickAll(HNetParticipant& host, HList<HNetParticipant>& clients, int32 ticks)
	{
		for (int32 i = 0; i < ticks; ++i)
		{
			host.Tick(NetTestDeltaSeconds);
			for (HNetParticipant& client : clients)
			{
				client.Tick(NetTestDeltaSeconds);
			}
		}
	}

	// 명령 순번이 target 에 이를 때까지 모두가 입력한다.
	void playUntil(HNetParticipant& host, HList<HNetParticipant>& clients, uint32 targetSeq, int32 maxTicks)
	{
		for (int32 tick = 0; tick < maxTicks; ++tick)
		{
			if (host.GameMaster->GetState().Sequence >= targetSeq)
			{
				return;
			}
			host.Tick(NetTestDeltaSeconds);
			driveInput(host);
			for (HNetParticipant& client : clients)
			{
				client.Tick(NetTestDeltaSeconds);
				driveInput(client);
			}
		}
	}

	bool allClientsIn(HList<HNetParticipant>& clients, EGameplaySessionState state)
	{
		for (HNetParticipant& client : clients)
		{
			if (client.Client->GetState() != state)
			{
				return false;
			}
		}
		return true;
	}

	// ---- 가짜 클라: 호스트의 검사를 세션 없이 직접 두드린다 ----
	struct HRawPeer
	{
		PSharedPtr<PNetLoopbackTransport> Transport;
		HNetPeerId                        Peer      = NetPeerNone;
		bool                              bConnected = false;
		bool                              bClosed    = false;
		HList<HPair<EGameplayNetMessage, HRawString>> Received;

		void Poll()
		{
			HList<HNetEvent> events;
			Transport->Poll(events);
			for (const HNetEvent& event : events)
			{
				if (event.Type == ENetEventType::Connected)
				{
					bConnected = true;
				}
				else if (event.Type == ENetEventType::Disconnected)
				{
					bClosed = true;
				}
				else
				{
					EGameplayNetMessage type = EGameplayNetMessage::None;
					HRawString text;
					if (HGameplayNetCodec::Decode(event.Data, &type, &text) == true)
					{
						Received.push_back(HPair<EGameplayNetMessage, HRawString>(type, text));
					}
				}
			}
		}

		bool Find(EGameplayNetMessage type, HRawString* outText) const
		{
			for (const HPair<EGameplayNetMessage, HRawString>& message : Received)
			{
				if (message.first == type)
				{
					if (outText != nullptr)
					{
						*outText = message.second;
					}
					return true;
				}
			}
			return false;
		}

		template<class T>
		void Send(EGameplayNetMessage type, const T& message)
		{
			HList<uint8> bytes;
			HGameplayNetCodec::EncodeMessage(type, message, bytes);
			Transport->Send(Peer, bytes);
		}
	};

	// ---- 전송 ----------------------------------------------------------------------

	HList<uint8> makeBytes(uint32 size, uint32 salt)
	{
		HList<uint8> bytes(size);
		for (uint32 i = 0; i < size; ++i)
		{
			bytes[i] = (uint8)((i * 31u + salt) & 0xff);
		}
		return bytes;
	}

	void runTransportTests(HCheck& check)
	{
		std::cout << "-- transport: loopback" << std::endl;
		{
			PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();
			PSharedPtr<PNetLoopbackTransport> host   = Allocate<PNetLoopbackTransport>(hub);
			PSharedPtr<PNetLoopbackTransport> client = Allocate<PNetLoopbackTransport>(hub);
			check(host->Listen(9001) == true, "loopback listen");

			HNetPeerId clientPeer = client->Connect("loopback", 9001);
			HList<HNetEvent> hostEvents;
			HList<HNetEvent> clientEvents;
			host->Poll(hostEvents);
			client->Poll(clientEvents);
			check(hostEvents.size() == 1 && hostEvents[0].Type == ENetEventType::Connected, "loopback host sees Connected");
			check(clientEvents.size() == 1 && clientEvents[0].Type == ENetEventType::Connected && clientEvents[0].Peer == clientPeer, "loopback client sees Connected");
			HNetPeerId hostPeer = hostEvents.empty() == false ? hostEvents[0].Peer : NetPeerNone;

			HList<uint8> big = makeBytes(1024u * 1024u, 7);
			client->Send(clientPeer, makeBytes(16, 1));
			host->Send(hostPeer, big);
			hostEvents.clear();
			clientEvents.clear();
			host->Poll(hostEvents);
			client->Poll(clientEvents);
			check(hostEvents.size() == 1 && hostEvents[0].Data == makeBytes(16, 1), "loopback message arrives intact");
			check(clientEvents.size() == 1 && clientEvents[0].Data == big, "loopback 1 MB message arrives intact");

			hub->SetDelayPolls(2);
			client->Send(clientPeer, makeBytes(8, 2));
			hostEvents.clear();
			host->Poll(hostEvents);
			bool bNotYet = hostEvents.empty();
			host->Poll(hostEvents);
			host->Poll(hostEvents);
			check(bNotYet == true && hostEvents.size() == 1, "loopback delay holds the message for 2 polls");
			hub->SetDelayPolls(0);

			client->Send(clientPeer, makeBytes(4, 3));
			client->Close(clientPeer);
			hostEvents.clear();
			host->Poll(hostEvents);
			check(hostEvents.size() == 2 && hostEvents[0].Type == ENetEventType::Message && hostEvents[1].Type == ENetEventType::Disconnected && hostEvents[1].Reason == ENetDisconnectReason::Closed, "loopback Close delivers pending data before Disconnected(Closed)");

			PSharedPtr<PNetLoopbackTransport> other = Allocate<PNetLoopbackTransport>(hub);
			HNetPeerId otherPeer = other->Connect("loopback", 9001);
			hostEvents.clear();
			host->Poll(hostEvents);
			HNetPeerId hostSideOther = hostEvents.empty() == false ? hostEvents[0].Peer : NetPeerNone;
			hub->Break(host->GetEndpoint(), hostSideOther);
			hostEvents.clear();
			HList<HNetEvent> otherEvents;
			host->Poll(hostEvents);
			other->Poll(otherEvents);
			bool bOtherBroken = false;
			for (const HNetEvent& event : otherEvents)
			{
				if (event.Type == ENetEventType::Disconnected && event.Peer == otherPeer && event.Reason == ENetDisconnectReason::Error)
				{
					bOtherBroken = true;
				}
			}
			check(hostEvents.size() == 1 && hostEvents[0].Reason == ENetDisconnectReason::Error && bOtherBroken, "loopback Break gives Disconnected(Error) on both sides");

			PSharedPtr<PNetLoopbackTransport> nowhere = Allocate<PNetLoopbackTransport>(hub);
			nowhere->Connect("loopback", 9999);
			HList<HNetEvent> nowhereEvents;
			nowhere->Poll(nowhereEvents);
			check(nowhereEvents.size() == 1 && nowhereEvents[0].Reason == ENetDisconnectReason::Refused, "loopback connect to a closed port is refused");
		}

		std::cout << "-- transport: TCP (127.0.0.1, same process)" << std::endl;
		{
			// 포트를 박아 두면 같은 기계의 다른 프로그램과 부딪힌다(10-02 다른 도구가 47790 을 쓰고 있어 bind 실패).
			// 0 번으로 열어 운영체제가 고른 빈 포트를 쓴다.
			PSharedPtr<PNetTcpTransport> host   = Allocate<PNetTcpTransport>();
			PSharedPtr<PNetTcpTransport> client = Allocate<PNetTcpTransport>();
			host->SetListenAddress("127.0.0.1");
			const bool   bListening = host->Listen(0);
			const uint16 port       = host->GetListenPort();
			check(bListening == true && port != 0, PString::Format("tcp listen on 127.0.0.1:%u (port chosen by the OS)", (uint32)port));

			HNetPeerId clientPeer = client->Connect("127.0.0.1", port);
			check(clientPeer != NetPeerNone, "tcp connect starts");

			HNetPeerId hostPeer = NetPeerNone;
			bool bClientConnected = false;
			HList<uint8> bigReceived;
			HList<uint8> smallReceived;
			HList<uint8> big = makeBytes(1024u * 1024u, 11);
			bool bSentBig = false;

			auto begin = std::chrono::steady_clock::now();
			while (std::chrono::steady_clock::now() - begin < std::chrono::seconds(10))
			{
				HList<HNetEvent> hostEvents;
				HList<HNetEvent> clientEvents;
				host->Poll(hostEvents);
				client->Poll(clientEvents);
				for (const HNetEvent& event : hostEvents)
				{
					if (event.Type == ENetEventType::Connected)
					{
						hostPeer = event.Peer;
					}
					else if (event.Type == ENetEventType::Message)
					{
						smallReceived = event.Data;
					}
				}
				for (const HNetEvent& event : clientEvents)
				{
					if (event.Type == ENetEventType::Connected)
					{
						bClientConnected = true;
						client->Send(clientPeer, makeBytes(32, 5));
					}
					else if (event.Type == ENetEventType::Message)
					{
						bigReceived = event.Data;
					}
				}
				if (hostPeer != NetPeerNone && bSentBig == false)
				{
					host->Send(hostPeer, big);
					bSentBig = true;
				}
				if (bigReceived.empty() == false && smallReceived.empty() == false)
				{
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			check(bClientConnected == true && hostPeer != NetPeerNone, "tcp both sides see Connected");
			check(smallReceived == makeBytes(32, 5), "tcp small message arrives intact");
			check(bigReceived == big, PString::Format("tcp 1 MB message arrives intact (%u bytes)", (uint32)bigReceived.size()));

			// 클라가 닫으면 호스트는 Disconnected(Closed).
			client->Close(clientPeer);
			bool bHostSawClose = false;
			begin = std::chrono::steady_clock::now();
			while (std::chrono::steady_clock::now() - begin < std::chrono::seconds(5) && bHostSawClose == false)
			{
				HList<HNetEvent> hostEvents;
				HList<HNetEvent> clientEvents;
				host->Poll(hostEvents);
				client->Poll(clientEvents);
				for (const HNetEvent& event : hostEvents)
				{
					if (event.Type == ENetEventType::Disconnected && event.Peer == hostPeer && event.Reason == ENetDisconnectReason::Closed)
					{
						bHostSawClose = true;
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			check(bHostSawClose == true, "tcp close is seen as Disconnected(Closed)");

			// 아무도 듣지 않는 포트: 빈 포트를 하나 받아 바로 닫은 것.
			PSharedPtr<PNetTcpTransport> released = Allocate<PNetTcpTransport>();
			released->SetListenAddress("127.0.0.1");
			released->Listen(0);
			const uint16 closedPort = released->GetListenPort();
			released->Shutdown();
			PSharedPtr<PNetTcpTransport> nowhere = Allocate<PNetTcpTransport>();
			HNetPeerId nowherePeer = nowhere->Connect("127.0.0.1", closedPort);
			bool bRefused = false;
			begin = std::chrono::steady_clock::now();
			while (std::chrono::steady_clock::now() - begin < std::chrono::seconds(10) && bRefused == false)
			{
				HList<HNetEvent> events;
				nowhere->Poll(events);
				for (const HNetEvent& event : events)
				{
					if (event.Type == ENetEventType::Disconnected && event.Peer == nowherePeer && event.Reason == ENetDisconnectReason::Refused)
					{
						bRefused = true;
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}
			check(bRefused == true, "tcp connect to a closed port is refused");
		}

		std::cout << "-- codec" << std::endl;
		{
			HGameplayNetCommandAccepted accepted;
			accepted.Seq       = 42;
			accepted.Slot      = 2;
			accepted.ClientSeq = 7;
			accepted.Checksum  = 0xF00DCAFEBEEF1234ull;
			accepted.Command   = HGameplayCommand(PName("NetStrike"), HGameplayEntityId(3, 1));
			accepted.Command.Targets.push_back(HGameplayEntityId(4, 2));

			HList<uint8> bytes;
			HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::CommandAccepted, accepted, bytes);
			EGameplayNetMessage type = EGameplayNetMessage::None;
			HRawString text;
			HGameplayNetCodec::Decode(bytes, &type, &text);
			HGameplayNetCommandAccepted decoded;
			bool bDecoded = HGameplayNetCodec::DecodeMessage(text, &decoded);
			check(bDecoded == true && type == EGameplayNetMessage::CommandAccepted && decoded.Seq == 42 && decoded.Slot == 2 && decoded.ClientSeq == 7
				&& decoded.Checksum == accepted.Checksum && decoded.Command.Kind == PName("NetStrike") && decoded.Command.Target() == HGameplayEntityId(4, 2),
				"codec round trip keeps every field (uint64 checksum included)");

			HRawString documentText;
			for (int32 i = 0; i < 2000; ++i)
			{
				documentText += "{\"Hp\":30,\"Controller\":1},";
			}
			HList<uint8> compressed;
			HGameplayNetCodec::Encode(EGameplayNetMessage::Document, documentText, true, compressed);
			HRawString inflated;
			bool bInflated = HGameplayNetCodec::Decode(compressed, &type, &inflated);
			check(bInflated == true && inflated == documentText && compressed.size() < documentText.size() / 4,
				PString::Format("compressed document round trip (%u -> %u bytes)", (uint32)documentText.size(), (uint32)compressed.size()));

			HList<uint8> garbage;
			garbage.push_back(200);
			garbage.push_back(0);
			check(HGameplayNetCodec::Decode(garbage, &type, &text) == false, "codec rejects an unknown message type");

			// 프로토콜 2 (D-38): Welcome 의 세션 공유 데이터 표 · 슬롯 준비 · SessionData · SetReady
			HGameplayNetWelcome welcome;
			welcome.Slot = 1;
			HGameplayPlayerSlot readySlot;
			readySlot.Slot   = 1;
			readySlot.Name   = "Ready";
			readySlot.bReady = true;
			welcome.Slots.push_back(readySlot);
			HGameplaySessionDataEntry entry;
			entry.Key   = PName("campaign");
			entry.Value = "{\"next\":2,\"name\":\"한글\"}";
			welcome.SessionData.push_back(entry);
			HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::Welcome, welcome, bytes);
			HGameplayNetCodec::Decode(bytes, &type, &text);
			HGameplayNetWelcome welcomeBack;
			bool bWelcome = HGameplayNetCodec::DecodeMessage(text, &welcomeBack);
			check(bWelcome == true && welcomeBack.World == NAME_NONE && welcomeBack.Slots.size() == 1 && welcomeBack.Slots[0].bReady == true
				&& welcomeBack.SessionData.size() == 1 && welcomeBack.SessionData[0].Key == PName("campaign") && welcomeBack.SessionData[0].Value == entry.Value,
				"codec round trip: Welcome keeps the session data table and slot Ready");

			HGameplayNetSessionData change;
			change.Key      = PName("scenario");
			change.bRemoved = true;
			HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::SessionData, change, bytes);
			HGameplayNetCodec::Decode(bytes, &type, &text);
			HGameplayNetSessionData changeBack;
			bool bChange = HGameplayNetCodec::DecodeMessage(text, &changeBack);
			check(bChange == true && type == EGameplayNetMessage::SessionData && changeBack.Key == PName("scenario") && changeBack.bRemoved == true,
				"codec round trip: SessionData (key, removed)");

			HGameplayNetSetReady ready;
			ready.bReady = true;
			HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::SetReady, ready, bytes);
			HGameplayNetCodec::Decode(bytes, &type, &text);
			HGameplayNetSetReady readyBack;
			check(HGameplayNetCodec::DecodeMessage(text, &readyBack) == true && type == EGameplayNetMessage::SetReady && readyBack.bReady == true, "codec round trip: SetReady");
		}
	}

	// ---- 세션: 핸드셰이크 검사 -------------------------------------------------------------

	void runHandshakeTests(HCheck& check)
	{
		std::cout << "-- session: handshake" << std::endl;

		const uint16 port    = 7102;
		const int32  players = 1;
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();

		HNetParticipant host;
		host.GameMaster = Allocate<PGameMaster>();
		setupNetRules(*host.GameMaster, players);
		buildNetInitialState(*host.GameMaster, players);
		HGameplaySessionConfig config;
		config.MaxPlayers          = 2;
		config.DesyncDumpDirectory = "";
		host.Host = PGameplayHostSession::CreateListenServer(Allocate<PNetLoopbackTransport>(hub), port, config);
		host.Host->BindGameMaster(host.GameMaster);

		// 프로토콜이 다른 클라 → 거부
		HRawPeer oldClient;
		oldClient.Transport = Allocate<PNetLoopbackTransport>(hub);
		oldClient.Peer      = oldClient.Transport->Connect("loopback", port);
		host.Tick(NetTestDeltaSeconds);
		oldClient.Poll();
		HGameplayNetHello badHello;
		badHello.Protocol = HGameplayNetProtocol::Version + 100;
		badHello.Schema   = HGameplayState::SchemaVersion;
		badHello.Name     = "Old";
		badHello.Token    = "old-token";
		oldClient.Send(EGameplayNetMessage::Hello, badHello);
		host.Tick(NetTestDeltaSeconds);
		oldClient.Poll();
		HRawString refuseText;
		HGameplayNetRefuse refused;
		bool bRefused = oldClient.Find(EGameplayNetMessage::Refuse, &refuseText) && HGameplayNetCodec::DecodeMessage(refuseText, &refused);
		check(bRefused == true && refused.Reason.Contains("protocol") == true && oldClient.bClosed == true, PString::Format("different protocol is refused and closed (%s)", refused.Reason));

		// 규칙이 다른 클라 → 입장은 되고, 시작 때 지문 불일치로 스스로 닫는다
		HList<HNetParticipant> clients;
		clients.push_back(makeLoopbackClient(hub, port, players, "Mismatch", 11, ""));
		clients[0].GameMaster->RegisterHandler(Allocate<PNetExtraHandler>());
		tickAll(host, clients, 5);
		check(clients[0].Client->GetState() == EGameplaySessionState::Ready && clients[0].Client->GetLocalSlot() == 1, "client with different rules joins slot 1 before start");

		// 자리가 없으면 거부
		HList<HNetParticipant> overflow;
		overflow.push_back(makeLoopbackClient(hub, port, players, "Overflow", 12, ""));
		for (int32 i = 0; i < 5; ++i)
		{
			host.Tick(NetTestDeltaSeconds);
			overflow[0].Tick(NetTestDeltaSeconds);
		}
		check(overflow[0].Client->GetState() == EGameplaySessionState::Closed && overflow[0].Client->GetCloseReason().Contains("session full") == true,
			PString::Format("full session refuses a new client (%s)", overflow[0].Client->GetCloseReason()));

		check(host.Host->StartGame(99) == true, "host starts the game");
		tickAll(host, clients, 5);
		check(clients[0].Client->GetState() == EGameplaySessionState::Closed && clients[0].Client->GetCloseReason().Contains("fingerprint") == true,
			PString::Format("client with different rules closes on StartGame (%s)", clients[0].Client->GetCloseReason()));
		tickAll(host, clients, 3);
		check(host.Host->GetConnectedPeerCount() == 0, "host sees the mismatched client leave");
	}

	// ---- 세션: 1,000 명령 복제 · 소유 검사 · 체크섬 비용 ------------------------------------------

	void runSessionTests(HCheck& check)
	{
		std::cout << "-- session: 1 host + 3 clients, 1000 commands" << std::endl;

		const uint16 port      = 7101;
		const int32  players   = 4;
		const uint64 seed      = 4242;
		const uint32 targetSeq = 1000;
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();

		HNetParticipant host;
		host.GameMaster = Allocate<PGameMaster>();
		setupNetRules(*host.GameMaster, players);
		buildNetInitialState(*host.GameMaster, players);
		host.Agent = Allocate<PGameplayRandomAgent>((uint64)101);

		HGameplaySessionConfig hostConfig;
		hostConfig.PlayerName          = "Host";
		hostConfig.MaxPlayers          = players + 1;   // 마지막 자리는 가짜 클라용
		hostConfig.DesyncDumpDirectory = "";
		host.Loopback = Allocate<PNetLoopbackTransport>(hub);
		host.Host     = PGameplayHostSession::CreateListenServer(host.Loopback, port, hostConfig);
		check(host.Host != nullptr, "listen server created");
		host.Host->BindGameMaster(host.GameMaster);
		check(host.GameMaster->IsUndoEnabled() == false, "binding a network session disables local Undo");

		HList<HNetParticipant> clients;
		for (int32 i = 0; i < 3; ++i)
		{
			clients.push_back(makeLoopbackClient(hub, port, players, PString::Format("Client%d", i + 1), (uint64)(201 + i), ""));
		}

		for (int32 i = 0; i < 20 && allClientsIn(clients, EGameplaySessionState::Ready) == false; ++i)
		{
			tickAll(host, clients, 1);
		}
		check(allClientsIn(clients, EGameplaySessionState::Ready) == true && host.Host->GetConnectedPeerCount() == 3, "3 clients joined");
		bool bSlotsInOrder = clients[0].Client->GetLocalSlot() == 1 && clients[1].Client->GetLocalSlot() == 2 && clients[2].Client->GetLocalSlot() == 3;
		check(bSlotsInOrder == true, "clients got slots 1, 2, 3");

		// 가짜 클라: 남의 행동자로 명령을 보낸다.
		HRawPeer forger;
		forger.Transport = Allocate<PNetLoopbackTransport>(hub);
		forger.Peer      = forger.Transport->Connect("loopback", port);
		host.Tick(NetTestDeltaSeconds);
		forger.Poll();
		HGameplayNetHello hello;
		hello.Protocol = HGameplayNetProtocol::Version;
		hello.Schema   = HGameplayState::SchemaVersion;
		hello.Name     = "Forger";
		hello.Token    = "forger-token";
		forger.Send(EGameplayNetMessage::Hello, hello);
		host.Tick(NetTestDeltaSeconds);
		forger.Poll();
		check(forger.Find(EGameplayNetMessage::Welcome, nullptr) == true, "raw client handshake gets Welcome");

		check(host.Host->StartGame(seed) == true, "host StartGame");
		tickAll(host, clients, 3);
		check(allClientsIn(clients, EGameplaySessionState::Playing) == true, "all clients started from StartGame");
		bool bSameStart = true;
		for (HNetParticipant& client : clients)
		{
			if (client.GameMaster->Checksum() != host.GameMaster->Checksum())
			{
				bSameStart = false;
			}
		}
		check(bSameStart == true, "every replica starts with the host checksum");

		HGameplayNetCommandRequest forged;
		forged.ClientSeq = 77;
		forged.Command   = HGameplayCommand(PName("NetStrike"), unitOfController(host.GameMaster->GetState(), 1));
		forged.Command.Targets.push_back(unitOfController(host.GameMaster->GetState(), players));
		uint32 seqBeforeForge = host.GameMaster->GetState().Sequence;
		forger.Send(EGameplayNetMessage::CommandRequest, forged);
		host.Tick(NetTestDeltaSeconds);
		forger.Poll();
		HRawString rejectedText;
		HGameplayNetCommandRejected rejected;
		bool bRejected = forger.Find(EGameplayNetMessage::CommandRejected, &rejectedText) && HGameplayNetCodec::DecodeMessage(rejectedText, &rejected);
		check(bRejected == true && rejected.ClientSeq == 77 && rejected.Reason == PString("not your actor") && host.GameMaster->GetState().Sequence == seqBeforeForge,
			PString::Format("host rejects a command for another slot's actor (%s)", rejected.Reason));
		forger.Transport->Shutdown();

		auto begin = std::chrono::steady_clock::now();
		playUntil(host, clients, targetSeq, 200000);
		tickAll(host, clients, 30);
		float64 seconds = std::chrono::duration<float64>(std::chrono::steady_clock::now() - begin).count();

		uint32 hostSeq      = host.GameMaster->GetState().Sequence;
		uint64 hostChecksum = host.GameMaster->Checksum();
		check(hostSeq >= targetSeq, PString::Format("host reached %u commands (%u)", targetSeq, hostSeq));
		info(PString::Format("played %u commands in %.2f s (debug build, 4 replicas in one process)", hostSeq, seconds));

		for (HNetParticipant& client : clients)
		{
			int32 slot = client.Client->GetLocalSlot();
			check(client.Client->GetDesyncCount() == 0, PString::Format("slot %d: no desync over every accepted command", slot));
			check(client.Client->GetLastAppliedSeq() == hostSeq && client.GameMaster->GetState().Sequence == hostSeq, PString::Format("slot %d applied every command (%u)", slot, client.Client->GetLastAppliedSeq()));
			check(client.GameMaster->Checksum() == hostChecksum, PString::Format("slot %d final checksum equals host (%llu)", slot, hostChecksum));
			check(client.Submitted > 0, PString::Format("slot %d sent %d commands of its own", slot, client.Submitted));
		}

		HGameplayEntityId enemy = unitOfController(host.GameMaster->GetState(), players);
		int32 enemyCommands = countCommandsBy(*host.GameMaster, enemy);
		check(enemyCommands > 0, PString::Format("host AI played the enemy (%d commands)", enemyCommands));
		check(countCommands(*host.GameMaster, HGameplayBuiltin::CommandResolveChoice) > 0, PString::Format("pending choices were resolved over the network (%d)", countCommands(*host.GameMaster, HGameplayBuiltin::CommandResolveChoice)));
		check(countCommands(*host.GameMaster, "NetSummon") > 0 && countCommands(*host.GameMaster, "NetShuffle") > 0, "spawn and shuffle commands were played");
		check(host.Submitted > 0, PString::Format("host player sent %d commands", host.Submitted));

		// 체크섬 비용 (명령마다 호스트와 클라가 한 번씩 계산한다)
		const int32 iterations = 200;
		auto measure = [&](const PGameMaster& gameMaster) -> float64
		{
			auto start = std::chrono::steady_clock::now();
			uint64 sink = 0;
			for (int32 i = 0; i < iterations; ++i)
			{
				sink ^= gameMaster.Checksum();
			}
			float64 elapsed = std::chrono::duration<float64, std::milli>(std::chrono::steady_clock::now() - start).count();
			if (sink == 1)
			{
				std::cout << "";
			}
			return elapsed / iterations;
		};
		float64 smallCost = measure(*host.GameMaster);

		PSharedPtr<PGameMaster> large = Allocate<PGameMaster>();
		setupNetRules(*large, 300);
		buildNetInitialState(*large, 300);
		large->Start(1);
		float64 largeCost = measure(*large);
		int32 smallEntities = 0;
		host.GameMaster->GetState().Each<HNetTestUnit>([&](const HGameplayEntityId&, const HNetTestUnit&)
		{
			++smallEntities;
		});
		info(PString::Format("checksum cost: %d entities %.3f ms, 301 entities %.3f ms (debug build)", smallEntities, smallCost, largeCost));
		check(largeCost > 0.0, "checksum cost measured");

		host.Host->Close("test done");
		for (HNetParticipant& client : clients)
		{
			client.Client->Close("test done");
		}
		tickAll(host, clients, 2);
	}

	// ---- 입장 · 불일치 복구 · 재접속 ---------------------------------------------------------

	void runRecoveryTests(HCheck& check)
	{
		std::cout << "-- recovery: join in progress, desync, reconnect" << std::endl;

		const uint16  port          = 7103;
		const int32   players       = 4;
		const uint64  seed          = 9090;
		const PString dumpDirectory = "NetSelfTestDesync";
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();

		std::error_code error;
		fs::remove_all(fs::path(dumpDirectory.GetRawString()), error);

		HNetParticipant host;
		host.GameMaster = Allocate<PGameMaster>();
		setupNetRules(*host.GameMaster, players);
		buildNetInitialState(*host.GameMaster, players);
		// 아직 오지 않은 플레이어 3 자리는 호스트 AI 가 대신한다.
		host.GameMaster->SetAgent(3, Allocate<PGameplayRandomAgent>((uint64)303));
		host.Agent = Allocate<PGameplayRandomAgent>((uint64)301);

		HGameplaySessionConfig hostConfig;
		hostConfig.PlayerName          = "Host";
		hostConfig.MaxPlayers          = players;
		hostConfig.DesyncDumpDirectory = "";
		host.Loopback = Allocate<PNetLoopbackTransport>(hub);
		host.Host     = PGameplayHostSession::CreateListenServer(host.Loopback, port, hostConfig);
		host.Host->BindGameMaster(host.GameMaster);

		// 끊긴 자리는 호스트 AI 가 이어받는다 (게임이 정하는 정책을 테스트가 흉내 낸다).
		int32 disconnectedSlot = INDEX_NONE;
		host.Host->OnPeerDisconnected.AddLambda([&](int32 slot)
		{
			disconnectedSlot = slot;
			host.GameMaster->SetAgent(slot, Allocate<PGameplayRandomAgent>((uint64)(400 + slot)));
		});

		HList<HNetParticipant> clients;
		clients.push_back(makeLoopbackClient(hub, port, players, "Client1", 311, dumpDirectory));
		clients.push_back(makeLoopbackClient(hub, port, players, "Client2", 312, dumpDirectory));
		for (int32 i = 0; i < 20 && allClientsIn(clients, EGameplaySessionState::Ready) == false; ++i)
		{
			tickAll(host, clients, 1);
		}
		host.Host->StartGame(seed);
		tickAll(host, clients, 3);
		playUntil(host, clients, 500, 100000);
		check(host.GameMaster->GetState().Sequence >= 500, PString::Format("played to seq %u before the late join", host.GameMaster->GetState().Sequence));

		// 진행 중 입장
		clients.push_back(makeLoopbackClient(hub, port, players, "Client3", 313, dumpDirectory));
		HNetParticipant& late = clients[2];
		bool bAgentRemoved = false;
		for (int32 i = 0; i < 20 && late.Client->GetState() != EGameplaySessionState::Playing; ++i)
		{
			tickAll(host, clients, 1);
			const HGameplayPlayerSlot* slot3 = host.Host->FindSlot(3);
			if (bAgentRemoved == false && slot3 != nullptr && slot3->bConnected == true)
			{
				host.GameMaster->SetAgent(3, nullptr);
				bAgentRemoved = true;
			}
		}
		check(late.Client->GetState() == EGameplaySessionState::Playing && late.Client->GetLocalSlot() == 3, "late client joins slot 3 and plays from the document");
		check(late.GameMaster->Checksum() == host.GameMaster->Checksum() && late.Client->GetLastAppliedSeq() == host.GameMaster->GetState().Sequence,
			PString::Format("late client matches the host at seq %u", host.GameMaster->GetState().Sequence));

		uint32 joinSeq = host.GameMaster->GetState().Sequence;
		playUntil(host, clients, joinSeq + 100, 100000);
		check(late.Submitted > 0 && late.Client->GetDesyncCount() == 0, PString::Format("late client keeps up and sends its own commands (%d)", late.Submitted));

		// 복제본을 고의로 바꾼다 → 다음 승인에서 검출 · 덤프 · 문서로 복구
		HNetParticipant& tampered = clients[0];
		HGameplayState broken = tampered.GameMaster->Snapshot();
		broken.EachMutable<HNetTestUnit>([&](const HGameplayEntityId&, HNetTestUnit& unit)
		{
			unit.Hp += 5;
		});
		tampered.GameMaster->Restore(broken);
		uint32 tamperSeq = host.GameMaster->GetState().Sequence;
		playUntil(host, clients, tamperSeq + 60, 100000);
		tickAll(host, clients, 5);
		check(tampered.Client->GetDesyncCount() == 1, PString::Format("tampered replica is detected once (%d)", tampered.Client->GetDesyncCount()));
		check(tampered.GameMaster->Checksum() == host.GameMaster->Checksum() && tampered.Client->GetState() == EGameplaySessionState::Playing, "tampered replica recovered from the document");

		int32 localDumps = 0;
		int32 hostDumps  = 0;
		if (fs::exists(fs::path(dumpDirectory.GetRawString())) == true)
		{
			for (const fs::directory_entry& entry : fs::directory_iterator(fs::path(dumpDirectory.GetRawString())))
			{
				HRawString name = entry.path().filename().string();
				if (name.find("_local.json") != HRawString::npos)
				{
					++localDumps;
				}
				if (name.find("_host.json") != HRawString::npos)
				{
					++hostDumps;
				}
			}
		}
		check(localDumps == 1 && hostDumps == 1, PString::Format("desync wrote a local and a host dump (%d, %d)", localDumps, hostDumps));

		// 연결 유실 → 호스트 AI 가 이어받음 → 같은 토큰으로 재접속
		HNetParticipant& dropped = clients[1];
		const HGameplayPlayerSlot* slot2 = host.Host->FindSlot(2);
		HNetPeerId hostSidePeer = slot2 != nullptr ? slot2->Peer : NetPeerNone;
		hub->Break(host.Loopback->GetEndpoint(), hostSidePeer);
		tickAll(host, clients, 2);
		check(dropped.Client->GetState() == EGameplaySessionState::Closed && disconnectedSlot == 2, PString::Format("dropped client closes and the host reports slot %d", disconnectedSlot));

		uint32 dropSeq = host.GameMaster->GetState().Sequence;
		playUntil(host, clients, dropSeq + 60, 100000);
		check(host.GameMaster->GetState().Sequence >= dropSeq + 60, "game goes on with the host AI in the dropped slot");

		check(dropped.Client->Reconnect() == true, "reconnect starts");
		bool bAgentReturned = false;
		for (int32 i = 0; i < 20 && dropped.Client->GetState() != EGameplaySessionState::Playing; ++i)
		{
			tickAll(host, clients, 1);
			const HGameplayPlayerSlot* slot = host.Host->FindSlot(2);
			if (bAgentReturned == false && slot != nullptr && slot->bConnected == true)
			{
				host.GameMaster->SetAgent(2, nullptr);
				bAgentReturned = true;
			}
		}
		check(dropped.Client->GetState() == EGameplaySessionState::Playing && dropped.Client->GetLocalSlot() == 2, "reconnected client gets slot 2 back");
		check(dropped.GameMaster->Checksum() == host.GameMaster->Checksum(), "reconnected client matches the host");

		playUntil(host, clients, 1000, 200000);
		tickAll(host, clients, 30);
		uint64 hostChecksum = host.GameMaster->Checksum();
		uint32 hostSeq      = host.GameMaster->GetState().Sequence;
		bool bAllMatch = true;
		for (HNetParticipant& client : clients)
		{
			if (client.GameMaster->Checksum() != hostChecksum || client.Client->GetLastAppliedSeq() != hostSeq)
			{
				bAllMatch = false;
			}
		}
		check(bAllMatch == true, PString::Format("all replicas match the host at seq %u after join, desync and reconnect", hostSeq));
		check(clients[1].Client->GetDesyncCount() == 0 && clients[2].Client->GetDesyncCount() == 0, "no other desync");

		host.Host->Close("test done");
		for (HNetParticipant& client : clients)
		{
			client.Client->Close("test done");
		}
		tickAll(host, clients, 2);
		fs::remove_all(fs::path(dumpDirectory.GetRawString()), error);
	}

	// ---- 대기방: 세션 공유 데이터 · 시작 전 나간 자리 비우기 · 준비 표시 (대기방 기능 묶음 D-38) ------------------

	// 세션 공유 데이터 알림을 받은 순서대로 모은다.
	struct HKeyLog
	{
		HList<PName> Keys;

		int32 Count(const char* key) const
		{
			PName name(key);
			int32 count = 0;
			for (const PName& entry : Keys)
			{
				if (entry == name)
				{
					++count;
				}
			}
			return count;
		}
	};

	void recordKeys(PGameplaySession& session, HKeyLog& log)
	{
		session.OnSessionDataChanged.AddLambda([&log](const PName& key)
		{
			log.Keys.push_back(key);
		});
	}

	bool dataIs(const PGameplaySession& session, const char* key, const PString& value)
	{
		const PString* found = session.FindSessionData(PName(key));
		return found != nullptr && *found == value;
	}

	bool dataMissing(const PGameplaySession& session, const char* key)
	{
		return session.FindSessionData(PName(key)) == nullptr;
	}

	// 이 세션이 보는 슬롯 표에서 그 자리가 비었는가 (이름 없음 · 접속 안 됨 · 준비 아님).
	bool seatLooksFree(const PGameplaySession& session, int32 slot)
	{
		const HGameplayPlayerSlot* entry = session.FindSlot(slot);
		return entry != nullptr && entry->bConnected == false && entry->Name.Empty() == true && entry->bReady == false;
	}

	bool seatIsReady(const PGameplaySession& session, int32 slot)
	{
		const HGameplayPlayerSlot* entry = session.FindSlot(slot);
		return entry != nullptr && entry->bReady == true;
	}

	bool seatHasName(const PGameplaySession& session, int32 slot, const PString& name)
	{
		const HGameplayPlayerSlot* entry = session.FindSlot(slot);
		return entry != nullptr && entry->Name == name;
	}

	// 대기방 호스트: GameMaster 는 붙었지만 시작 전(Ready).
	HNetParticipant makeLobbyHost(PSharedPtr<PNetLoopbackHub> hub, uint16 port, int32 players, uint64 agentSeed)
	{
		HNetParticipant host;
		host.GameMaster = Allocate<PGameMaster>();
		setupNetRules(*host.GameMaster, players);
		buildNetInitialState(*host.GameMaster, players);
		host.Agent = Allocate<PGameplayRandomAgent>(agentSeed);

		HGameplaySessionConfig config;
		config.PlayerName          = "Host";
		config.MaxPlayers          = players;
		config.DesyncDumpDirectory = "";
		host.Loopback = Allocate<PNetLoopbackTransport>(hub);
		host.Host     = PGameplayHostSession::CreateListenServer(host.Loopback, port, config);
		host.Host->BindGameMaster(host.GameMaster);
		return host;
	}

	// 마지막 참가자가 Ready(또는 Playing)가 될 때까지 모두를 돌린다.
	void tickUntilJoined(HNetParticipant& host, HList<HNetParticipant>& clients, EGameplaySessionState state)
	{
		for (int32 i = 0; i < 30 && clients.back().Client->GetState() != state; ++i)
		{
			tickAll(host, clients, 1);
		}
	}

	void closeAll(HNetParticipant& host, HList<HNetParticipant>& clients)
	{
		host.Host->Close("test done");
		for (HNetParticipant& client : clients)
		{
			client.Client->Close("test done");
		}
		tickAll(host, clients, 2);
	}

	void runLobbyDataTests(HCheck& check)
	{
		std::cout << "-- lobby: session data (room info)" << std::endl;

		const uint16 port    = 7104;
		const int32  players = 4;
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();
		HNetParticipant host = makeLobbyHost(hub, port, players, 601);
		HKeyLog hostKeys;
		recordKeys(*host.Host, hostKeys);

		const PString campaign1 = "{\"campaign\":\"c1\",\"next\":1,\"note\":\"한글 기록\"}";
		check(host.Host->SetSessionData(PName("campaign"), campaign1) == true && dataIs(*host.Host, "campaign", campaign1) == true && hostKeys.Count("campaign") == 1,
			"host sets session data before anyone joins and reads it back (one notice)");
		check(host.Host->SetSessionData(PName("campaign"), campaign1) == true && hostKeys.Count("campaign") == 1, "setting the same value again changes nothing (no notice)");
		check(host.Host->SetSessionData(PName(), "x") == false && host.Host->GetSessionData().size() == 1, "an empty key is refused");
		HRawString bigText(70u * 1024u, 'x');
		check(host.Host->SetSessionData(PName("big"), PString(bigText.c_str())) == false && dataMissing(*host.Host, "big") == true && host.Host->GetSessionData().size() == 1,
			"a value over the 64 KB limit is refused and nothing changes");

		// 지금 참가자는 Welcome 으로 받는다
		HList<HNetParticipant> clients;
		clients.push_back(makeLoopbackClient(hub, port, players, "A", 611, ""));
		HKeyLog aKeys;
		recordKeys(*clients[0].Client, aKeys);
		tickUntilJoined(host, clients, EGameplaySessionState::Ready);
		check(dataIs(*clients[0].Client, "campaign", campaign1) == true && aKeys.Count("campaign") == 1, "client A gets the data in Welcome (one notice)");

		// 바꾸기 · 더하기 · 지우기
		const PString campaign2 = "{\"campaign\":\"c1\",\"next\":2}";
		host.Host->SetSessionData(PName("campaign"), campaign2);
		host.Host->SetSessionData(PName("scenario"), "s2");
		tickAll(host, clients, 2);
		check(dataIs(*clients[0].Client, "campaign", campaign2) == true && dataIs(*clients[0].Client, "scenario", "s2") == true && aKeys.Count("campaign") == 2 && aKeys.Count("scenario") == 1,
			"client A sees a changed and an added key, one notice each");
		check(hostKeys.Count("campaign") == 2 && hostKeys.Count("scenario") == 1, "the host gets the same notices for its own changes");
		check(host.Host->RemoveSessionData(PName("scenario")) == true && host.Host->RemoveSessionData(PName("scenario")) == false, "remove reports true once, then false");
		tickAll(host, clients, 2);
		check(dataMissing(*clients[0].Client, "scenario") == true && aKeys.Count("scenario") == 2, "the removal reaches client A with a notice");

		// 나중 참가자는 지금 표만 받는다
		host.Host->SetSessionData(PName("difficulty"), "hard");
		clients.push_back(makeLoopbackClient(hub, port, players, "B", 612, ""));
		HKeyLog bKeys;
		recordKeys(*clients[1].Client, bKeys);
		tickUntilJoined(host, clients, EGameplaySessionState::Ready);
		check(clients[1].Client->GetSessionData().size() == 2 && dataIs(*clients[1].Client, "campaign", campaign2) == true && dataIs(*clients[1].Client, "difficulty", "hard") == true
			&& dataMissing(*clients[1].Client, "scenario") == true && bKeys.Keys.size() == 2, "late client B gets only the current table");

		// 끊긴 동안은 마지막 값을 읽고, 재접속 Welcome 으로 따라잡는다 (판 시작 전이라 빈 자리를 새로 받는다)
		const HGameplayPlayerSlot* seatA = host.Host->FindSlot(clients[0].Client->GetLocalSlot());
		hub->Break(host.Loopback->GetEndpoint(), seatA != nullptr ? seatA->Peer : NetPeerNone);
		tickAll(host, clients, 2);
		const PString campaign3 = "{\"campaign\":\"c1\",\"next\":3}";
		host.Host->SetSessionData(PName("campaign"), campaign3);
		host.Host->RemoveSessionData(PName("difficulty"));
		tickAll(host, clients, 2);
		check(clients[0].Client->GetState() == EGameplaySessionState::Closed && dataIs(*clients[0].Client, "campaign", campaign2) == true && dataIs(*clients[0].Client, "difficulty", "hard") == true,
			"a disconnected client keeps reading its last values");
		int32 noticesBefore = (int32)aKeys.Keys.size();
		check(clients[0].Client->Reconnect() == true, "client A reconnects");
		for (int32 i = 0; i < 30 && clients[0].Client->GetState() != EGameplaySessionState::Ready; ++i)
		{
			tickAll(host, clients, 1);
		}
		check(clients[0].Client->GetState() == EGameplaySessionState::Ready && dataIs(*clients[0].Client, "campaign", campaign3) == true && dataMissing(*clients[0].Client, "difficulty") == true
			&& (int32)aKeys.Keys.size() == noticesBefore + 2, "reconnected client A catches up from Welcome (changed and removed keys noticed)");

		// 판 진행 중에도 보낸다. 아직 오지 않은 자리(조작 주체 3)는 호스트 AI 가 맡는다
		host.GameMaster->SetAgent(3, Allocate<PGameplayRandomAgent>((uint64)603));
		check(host.Host->StartGame(6060) == true, "host starts the game");
		tickAll(host, clients, 3);
		playUntil(host, clients, 60, 20000);
		host.Host->SetSessionData(PName("status"), "playing");
		playUntil(host, clients, 120, 20000);
		tickAll(host, clients, 3);
		check(dataIs(*clients[0].Client, "status", "playing") == true && dataIs(*clients[1].Client, "status", "playing") == true && host.GameMaster->GetState().Sequence >= 120,
			"data set during the game reaches every client");

		// 진행 중 입장: Welcome(데이터) → 문서
		clients.push_back(makeLoopbackClient(hub, port, players, "C", 613, ""));
		HKeyLog cKeys;
		recordKeys(*clients[2].Client, cKeys);
		bool bAgentRemoved = false;
		for (int32 i = 0; i < 30 && clients[2].Client->GetState() != EGameplaySessionState::Playing; ++i)
		{
			tickAll(host, clients, 1);
			const HGameplayPlayerSlot* seat3 = host.Host->FindSlot(3);
			if (bAgentRemoved == false && seat3 != nullptr && seat3->bConnected == true)
			{
				host.GameMaster->SetAgent(3, nullptr);
				bAgentRemoved = true;
			}
		}
		check(clients[2].Client->GetState() == EGameplaySessionState::Playing && dataIs(*clients[2].Client, "campaign", campaign3) == true && dataIs(*clients[2].Client, "status", "playing") == true
			&& cKeys.Keys.size() == 2 && clients[2].GameMaster->Checksum() == host.GameMaster->Checksum(), "a client joining during the game gets the data and then the document");

		uint32 joinSeq = host.GameMaster->GetState().Sequence;
		host.Host->SetSessionData(PName("status"), "late join done");
		playUntil(host, clients, joinSeq + 100, 40000);
		tickAll(host, clients, 20);
		bool bSame = true;
		for (HNetParticipant& client : clients)
		{
			if (client.Client->GetDesyncCount() != 0 || client.GameMaster->Checksum() != host.GameMaster->Checksum() || dataIs(*client.Client, "status", "late join done") == false)
			{
				bSame = false;
			}
		}
		check(bSame == true, PString::Format("session data never touches the game: no desync, same checksum and data on every replica (seq %u)", host.GameMaster->GetState().Sequence));

		closeAll(host, clients);
	}

	void runLobbySeatTests(HCheck& check)
	{
		std::cout << "-- lobby: seats of players who leave before the game" << std::endl;

		const uint16 port    = 7105;
		const int32  players = 4;   // 4명 방 = 호스트 + 원격 3 자리
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();
		HNetParticipant host = makeLobbyHost(hub, port, players, 621);

		HList<HNetParticipant> clients;
		clients.push_back(makeLoopbackClient(hub, port, players, "Stay", 622, ""));
		tickUntilJoined(host, clients, EGameplaySessionState::Ready);
		check(clients[0].Client->GetLocalSlot() == 1, "client Stay joins slot 1 and stays");

		// 새 사람 4명이 차례로 들어왔다 나간다(매번 새 토큰). 4명 방의 원격 자리는 3개다
		bool bJoined = true;
		bool bListed = true;
		bool bFreed  = true;
		for (int32 i = 0; i < 4; ++i)
		{
			PString name = PString::Format("Visitor%d", i + 1);
			clients.push_back(makeLoopbackClient(hub, port, players, name, (uint64)(630 + i), ""));
			tickUntilJoined(host, clients, EGameplaySessionState::Ready);
			int32 seat = clients.back().Client->GetLocalSlot();
			if (clients.back().Client->GetState() != EGameplaySessionState::Ready || seat <= 1)
			{
				bJoined = false;
			}
			if (seatHasName(*host.Host, seat, name) == false || seatHasName(*clients[0].Client, seat, name) == false)
			{
				bListed = false;
			}

			clients.back().Client->Close("leaving the lobby");
			tickAll(host, clients, 3);
			const HGameplayPlayerSlot* hostSeat = host.Host->FindSlot(seat);
			if (hostSeat == nullptr || hostSeat->IsFree() == false || seatLooksFree(*host.Host, seat) == false || seatLooksFree(*clients[0].Client, seat) == false)
			{
				bFreed = false;
			}
			clients.pop_back();
		}
		check(bJoined == true, "4 visitors joined one after another (each with a new token)");
		check(bListed == true, "each visitor was listed by name on the host and on the remaining client");
		check(bFreed == true, "each visitor's seat was freed when they left before the game (host and remaining client)");

		// 그래도 새 사람 둘이 들어온다 → 방이 다시 찬다
		clients.push_back(makeLoopbackClient(hub, port, players, "New1", 640, ""));
		clients.push_back(makeLoopbackClient(hub, port, players, "New2", 641, ""));
		for (int32 i = 0; i < 30 && allClientsIn(clients, EGameplaySessionState::Ready) == false; ++i)
		{
			tickAll(host, clients, 1);
		}
		check(allClientsIn(clients, EGameplaySessionState::Ready) == true && host.Host->GetConnectedPeerCount() == 3 && clients[1].Client->GetLocalSlot() != clients[2].Client->GetLocalSlot(),
			"after 4 visitors came and went, two new clients still get the free seats (room of 4 full again)");
		HList<HNetParticipant> overflow;
		overflow.push_back(makeLoopbackClient(hub, port, players, "Overflow", 642, ""));
		for (int32 i = 0; i < 5; ++i)
		{
			host.Tick(NetTestDeltaSeconds);
			overflow[0].Tick(NetTestDeltaSeconds);
		}
		check(overflow[0].Client->GetState() == EGameplaySessionState::Closed && overflow[0].Client->GetCloseReason().Contains("session full") == true,
			PString::Format("a fourth remote client is refused while 3 are in the room (%s)", overflow[0].Client->GetCloseReason()));

		// 판 중에 나간 자리는 재접속용으로 남는다
		check(host.Host->StartGame(7070) == true, "host starts the game");
		tickAll(host, clients, 3);
		int32 leaverSeat = clients[2].Client->GetLocalSlot();
		clients[2].Client->Close("leaving during the game");
		tickAll(host, clients, 3);
		const HGameplayPlayerSlot* kept = host.Host->FindSlot(leaverSeat);
		check(kept != nullptr && kept->bConnected == false && kept->Name == PString("New2") && kept->IsFree() == false && seatHasName(*clients[0].Client, leaverSeat, "New2") == true,
			"a seat left during the game is kept for reconnecting (name stays, not connected)");
		check(clients[2].Client->Reconnect() == true, "the player who left reconnects during the game");
		for (int32 i = 0; i < 30 && clients[2].Client->GetState() != EGameplaySessionState::Playing; ++i)
		{
			tickAll(host, clients, 1);
		}
		check(clients[2].Client->GetState() == EGameplaySessionState::Playing && clients[2].Client->GetLocalSlot() == leaverSeat, "the same token gets the same seat back during the game");

		// 다시 나간 뒤 판이 끝나 대기방으로 돌아오면(월드를 내려 GameMaster 가 떨어짐) 그 자리를 비운다
		clients[2].Client->Close("leaving during the game");
		tickAll(host, clients, 3);
		check(seatHasName(*host.Host, leaverSeat, "New2") == true, "left again during the game: the seat is still kept");
		host.Host->UnbindGameMaster();
		tickAll(host, clients, 3);
		const HGameplayPlayerSlot* released = host.Host->FindSlot(leaverSeat);
		check(host.Host->GetState() == EGameplaySessionState::Ready && released != nullptr && released->IsFree() == true && seatLooksFree(*host.Host, leaverSeat) == true
			&& seatLooksFree(*clients[0].Client, leaverSeat) == true, "back in the lobby: the seat of the player who left during the game is freed and broadcast");

		closeAll(host, clients);
		overflow[0].Client->Close("test done");
	}

	void runLobbyReadyTests(HCheck& check)
	{
		std::cout << "-- lobby: ready marks" << std::endl;

		const uint16 port    = 7106;
		const int32  players = 4;
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();
		HNetParticipant host = makeLobbyHost(hub, port, players, 651);

		HList<HNetParticipant> clients;
		clients.push_back(makeLoopbackClient(hub, port, players, "A", 652, ""));
		clients.push_back(makeLoopbackClient(hub, port, players, "B", 653, ""));
		for (int32 i = 0; i < 30 && allClientsIn(clients, EGameplaySessionState::Ready) == false; ++i)
		{
			tickAll(host, clients, 1);
		}
		const int32 seatA = clients[0].Client->GetLocalSlot();
		const int32 seatB = clients[1].Client->GetLocalSlot();

		check(clients[0].Client->SetLocalReady(true) == true, "client A sets ready before the game");
		tickAll(host, clients, 3);
		check(seatIsReady(*host.Host, seatA) == true && seatIsReady(*clients[1].Client, seatA) == true && clients[0].Client->IsLocalReady() == true,
			"A's ready shows on the host, on B and back on A");
		check(host.Host->SetLocalReady(true) == true, "the host sets its own ready");
		tickAll(host, clients, 2);
		check(host.Host->IsLocalReady() == true && seatIsReady(*clients[0].Client, 0) == true && seatIsReady(*clients[1].Client, 0) == true, "the host's ready shows on every client");
		clients[0].Client->SetLocalReady(false);
		tickAll(host, clients, 3);
		check(seatIsReady(*host.Host, seatA) == false && seatIsReady(*clients[1].Client, seatA) == false && clients[0].Client->IsLocalReady() == false, "A clears ready and everyone sees it");

		// 준비한 채 나가면 준비와 자리가 함께 지워진다
		clients[1].Client->SetLocalReady(true);
		tickAll(host, clients, 3);
		bool bWasReady = seatIsReady(*host.Host, seatB);
		clients[1].Client->Close("leaving the lobby");
		tickAll(host, clients, 3);
		check(bWasReady == true && seatLooksFree(*host.Host, seatB) == true && seatLooksFree(*clients[0].Client, seatB) == true, "a ready player who leaves before the game: ready and seat cleared");

		// 판이 시작되면 모든 준비가 지워지고, 판 중에는 정할 수 없다
		clients[0].Client->SetLocalReady(true);
		tickAll(host, clients, 3);
		bool bBothReady = seatIsReady(*host.Host, 0) == true && seatIsReady(*host.Host, seatA) == true;
		host.GameMaster->SetAgent(2, Allocate<PGameplayRandomAgent>((uint64)654));
		host.GameMaster->SetAgent(3, Allocate<PGameplayRandomAgent>((uint64)655));
		check(host.Host->StartGame(8080) == true, "host starts the game");
		tickAll(host, clients, 3);
		check(bBothReady == true && seatIsReady(*host.Host, 0) == false && seatIsReady(*host.Host, seatA) == false && seatIsReady(*clients[0].Client, 0) == false
			&& clients[0].Client->IsLocalReady() == false, "StartGame clears every ready mark (host and client)");
		check(clients[0].Client->SetLocalReady(true) == false && host.Host->SetLocalReady(true) == false, "ready cannot be set during the game");

		// 판 중에 온 SetReady 는 버린다 (게임 상태도 그대로)
		HRawPeer raw;
		raw.Transport = Allocate<PNetLoopbackTransport>(hub);
		raw.Peer      = raw.Transport->Connect("loopback", port);
		host.Tick(NetTestDeltaSeconds);
		raw.Poll();
		HGameplayNetHello hello;
		hello.Protocol = HGameplayNetProtocol::Version;
		hello.Schema   = HGameplayState::SchemaVersion;
		hello.Name     = "Raw";
		hello.Token    = "raw-token";
		raw.Send(EGameplayNetMessage::Hello, hello);
		host.Tick(NetTestDeltaSeconds);
		raw.Poll();
		HRawString welcomeText;
		HGameplayNetWelcome welcome;
		bool bWelcome = raw.Find(EGameplayNetMessage::Welcome, &welcomeText) && HGameplayNetCodec::DecodeMessage(welcomeText, &welcome);
		uint64 checksumBefore = host.GameMaster->Checksum();
		HGameplayNetSetReady lateReady;
		lateReady.bReady = true;
		raw.Send(EGameplayNetMessage::SetReady, lateReady);
		host.Tick(NetTestDeltaSeconds);
		check(bWelcome == true && seatIsReady(*host.Host, welcome.Slot) == false && host.GameMaster->Checksum() == checksumBefore, "a SetReady that arrives during the game is ignored");
		raw.Transport->Shutdown();

		// 옛 빌드(프로토콜 1)는 입장 때 거절된다
		HRawPeer oldBuild;
		oldBuild.Transport = Allocate<PNetLoopbackTransport>(hub);
		oldBuild.Peer      = oldBuild.Transport->Connect("loopback", port);
		host.Tick(NetTestDeltaSeconds);
		oldBuild.Poll();
		HGameplayNetHello oldHello;
		oldHello.Protocol = 1;
		oldHello.Schema   = HGameplayState::SchemaVersion;
		oldHello.Name     = "OldBuild";
		oldHello.Token    = "old-build-token";
		oldBuild.Send(EGameplayNetMessage::Hello, oldHello);
		host.Tick(NetTestDeltaSeconds);
		oldBuild.Poll();
		HRawString refuseText;
		HGameplayNetRefuse refused;
		bool bRefused = oldBuild.Find(EGameplayNetMessage::Refuse, &refuseText) && HGameplayNetCodec::DecodeMessage(refuseText, &refused);
		check(HGameplayNetProtocol::Version == 2 && bRefused == true && refused.Reason == PString("protocol 1 != 2") && oldBuild.bClosed == true,
			PString::Format("a protocol 1 build is refused at Hello (%s)", refused.Reason));
		oldBuild.Transport->Shutdown();

		closeAll(host, clients);
	}

	// ---- 창을 다시 띄운 참가자: 저장한 토큰으로 새 세션 (ER-026) ----------------------------------------

	void runRejoinTests(HCheck& check)
	{
		std::cout << "-- recovery: a new session rejoins with the saved reconnect token (full room, during the game)" << std::endl;

		const uint16 port    = 7107;
		const int32  players = 4;   // 4명 방 = 호스트 + 원격 3 자리
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();
		HNetParticipant host = makeLobbyHost(hub, port, players, 701);

		HList<HNetParticipant> clients;
		clients.push_back(makeLoopbackClient(hub, port, players, "A", 711, ""));
		clients.push_back(makeLoopbackClient(hub, port, players, "B", 712, ""));
		clients.push_back(makeLoopbackClient(hub, port, players, "C", 713, ""));
		for (int32 i = 0; i < 30 && allClientsIn(clients, EGameplaySessionState::Ready) == false; ++i)
		{
			tickAll(host, clients, 1);
		}
		const PString tokenA = clients[0].Client->GetReconnectToken();
		const PString tokenB = clients[1].Client->GetReconnectToken();
		check(allClientsIn(clients, EGameplaySessionState::Ready) == true && host.Host->GetConnectedPeerCount() == 3, "room of 4 is full (3 clients)");
		check(tokenB.Empty() == false && tokenA != tokenB, "each session without a configured token makes its own reconnect token");

		check(host.Host->StartGame(7171) == true, "host starts the game");
		tickAll(host, clients, 3);
		playUntil(host, clients, 60, 20000);

		// B 가 창을 닫는다: 세션을 닫고 버린다. 판 중이라 자리는 B 의 토큰을 기다린다
		const int32 seatB = clients[1].Client->GetLocalSlot();
		clients[1].Client->Close("window closed");
		tickAll(host, clients, 3);
		HNetParticipant closedB = clients[1];
		clients.erase(clients.begin() + 1);
		const HGameplayPlayerSlot* waiting = host.Host->FindSlot(seatB);
		check(waiting != nullptr && waiting->bConnected == false && waiting->Name == PString("B") && waiting->IsFree() == false, "B's seat waits for its token during the game");

		// 토큰이 없는 새 사람은 꽉 찬 방에 못 들어온다
		HList<HNetParticipant> stranger;
		stranger.push_back(makeLoopbackClient(hub, port, players, "Stranger", 714, ""));
		for (int32 i = 0; i < 5; ++i)
		{
			host.Tick(NetTestDeltaSeconds);
			stranger[0].Tick(NetTestDeltaSeconds);
		}
		check(stranger[0].Client->GetState() == EGameplaySessionState::Closed && stranger[0].Client->GetCloseReason().Contains("session full") == true,
			PString::Format("a new player without the token is refused (%s)", stranger[0].Client->GetCloseReason()));

		// 창을 다시 띄운 B: 저장한 토큰으로 새 세션 · 새 GameMaster(새 프로세스와 같다)
		uint32 seqBefore = host.GameMaster->GetState().Sequence;
		playUntil(host, clients, seqBefore + 20, 2000);   // B 의 차례가 오면 멈춘다(B 를 기다린다)
		clients.push_back(makeLoopbackClient(hub, port, players, "B", 715, "", tokenB));
		for (int32 i = 0; i < 30 && clients.back().Client->GetState() != EGameplaySessionState::Playing; ++i)
		{
			tickAll(host, clients, 1);
		}
		HNetParticipant& rejoined = clients.back();
		check(rejoined.Client->GetReconnectToken() == tokenB, "the new session uses the configured reconnect token");
		check(rejoined.Client->GetState() == EGameplaySessionState::Playing && rejoined.Client->GetLocalSlot() == seatB && rejoined.GameMaster->Checksum() == host.GameMaster->Checksum(),
			PString::Format("the new session gets seat %d back in the full room and plays on from the document (seq %u)", seatB, host.GameMaster->GetState().Sequence));

		uint32 rejoinSeq = host.GameMaster->GetState().Sequence;
		playUntil(host, clients, rejoinSeq + 100, 40000);
		tickAll(host, clients, 20);
		bool bSame = true;
		for (HNetParticipant& client : clients)
		{
			if (client.Client->GetDesyncCount() != 0 || client.GameMaster->Checksum() != host.GameMaster->Checksum())
			{
				bSame = false;
			}
		}
		check(bSame == true && rejoined.Submitted > 0, PString::Format("everyone matches after the rejoin and the rejoined player sends its own commands (%d)", rejoined.Submitted));

		// 두 창이 한 토큰을 같이 쓰면: 나중 창이 자리를 이어받고 앞 창은 끊긴다
		HList<HNetParticipant> twin;
		twin.push_back(makeLoopbackClient(hub, port, players, "B-twin", 716, "", tokenB));
		for (int32 i = 0; i < 30 && twin[0].Client->GetState() != EGameplaySessionState::Playing; ++i)
		{
			tickAll(host, clients, 1);
			twin[0].Tick(NetTestDeltaSeconds);
		}
		tickAll(host, clients, 3);
		check(twin[0].Client->GetState() == EGameplaySessionState::Playing && twin[0].Client->GetLocalSlot() == seatB && clients.back().Client->GetState() == EGameplaySessionState::Closed,
			PString::Format("two windows with one token: the later one takes seat %d and the earlier one is disconnected (%s)", seatB, clients.back().Client->GetCloseReason()));

		// 쓸 수 없는 토큰은 새 토큰으로 바뀐다(새 사람으로 들어간다)
		HGameplaySessionConfig spaced;
		spaced.ReconnectToken = "has space";
		PSharedPtr<PGameplayClientSession> spacedSession = PGameplayClientSession::Create(Allocate<PNetLoopbackTransport>(hub), "loopback", port, spaced);
		HGameplaySessionConfig longer;
		longer.ReconnectToken = PString(HRawString(200, 'x').c_str());
		PSharedPtr<PGameplayClientSession> longSession = PGameplayClientSession::Create(Allocate<PNetLoopbackTransport>(hub), "loopback", port, longer);
		check(spacedSession->GetReconnectToken() != spaced.ReconnectToken && spacedSession->GetReconnectToken().Empty() == false
			&& longSession->GetReconnectToken() != longer.ReconnectToken && longSession->GetReconnectToken().Empty() == false,
			"a configured token with a space or over 128 bytes is replaced by a new token");
		spacedSession->Close("test done");
		longSession->Close("test done");

		twin[0].Client->Close("test done");
		stranger[0].Client->Close("test done");
		closedB.Client->Close("test done");
		closeAll(host, clients);
	}

	// ---- 규칙 효과 안 상태 되돌리기 (D-39, ER-027) -------------------------------------------------
	// 되감기는 명령 실행의 일부다. 모든 기계가 같은 명령을 돌리므로 같은 결과여야 하고, 지점 · 세대 바닥은 상태 안이라 입장 · 재동기 문서에 실려 간다.
	// 이 절의 명령은 이 절에서만 등록한다 — 다른 절의 규칙 · 체크섬은 그대로다.

	const char* NetRewindPoint = "NetTurn";

	bool hasRewindPoint(const HGameplayState& state, PString* outReason)
	{
		if (state.FindRewindPoint(PName(NetRewindPoint)) == nullptr)
		{
			if (outReason != nullptr)
			{
				*outReason = "no rewind point";
			}
			return false;
		}
		return true;
	}

	// 되감은 뒤 남길 변화: 행동자 유닛 Hp -1 (1 아래로는 내리지 않는다). 되감기 전에 받은 포인터를 쓰지 않도록 다시 찾는다.
	void payRewindCost(HGameplayContext& ctx, const HGameplayEntityId& actor)
	{
		HNetTestUnit* unit = ctx.State.Find<HNetTestUnit>(actor);
		if (unit != nullptr && unit->Hp > 1)
		{
			unit->Hp -= 1;
		}
	}

	// 되감기 명령의 후보를 에이전트에게 내놓을지. 각본 판은 시험이 손으로 내고(false), 무작위 판은 에이전트가 고른다(true).
	class PNetRewindCommandHandler : public JGGameplayCommandHandler
	{
	protected:
		bool _bOfferToAgents = false;

	public:
		explicit PNetRewindCommandHandler(bool bOfferToAgents)
			: _bOfferToAgents(bOfferToAgents)
		{
		}
	};

	// 지금 상태를 지점으로 남긴다(같은 이름이면 새 것으로).
	class PNetMarkHandler : public PNetRewindCommandHandler
	{
	public:
		explicit PNetMarkHandler(bool bOfferToAgents)
			: PNetRewindCommandHandler(bOfferToAgents)
		{
		}
		virtual PName GetKind() const override
		{
			return PName("NetMark");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return checkActorTurn(state, command, outReason);
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			ctx.MarkRewindPoint(PName(NetRewindPoint));
		}
		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			if (_bOfferToAgents == true)
			{
				outCommands.push_back(HGameplayCommand(GetKind(), actor));
			}
		}
	};

	// 지점으로 되감고 남길 변화를 적용한다. Params[0] = EGameplayRewindRandom, Params[1] = 0 핸들러에서 바로 · 1 효과를 넣어 효과에서.
	class PNetRewindHandler : public PNetRewindCommandHandler
	{
	public:
		explicit PNetRewindHandler(bool bOfferToAgents)
			: PNetRewindCommandHandler(bOfferToAgents)
		{
		}
		virtual PName GetKind() const override
		{
			return PName("NetRewind");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (checkActorTurn(state, command, outReason) == false)
			{
				return false;
			}
			return hasRewindPoint(state, outReason);
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			if (command.Param(1, 0) == 0)
			{
				if (ctx.RewindTo(PName(NetRewindPoint), (EGameplayRewindRandom)command.Param(0, 0)) == true)
				{
					payRewindCost(ctx, command.Actor);
				}
				return;
			}
			HGameplayEffectRequest rewind(PName("NetRewindNow"), command.Actor, HGameplayEntityId::None());
			rewind.Params.push_back(command.Param(0, 0));
			ctx.Enqueue(rewind);
		}
		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			if (_bOfferToAgents == false || hasRewindPoint(state, nullptr) == false)
			{
				return;
			}
			// 후보는 하나만 — 난수 방식 · 부르는 곳을 순번으로 돌려 쓴다(상태만 보고 정하므로 결정론).
			HGameplayCommand command(GetKind(), actor);
			command.Params.push_back((int32)(state.Sequence % 2));
			command.Params.push_back((int32)((state.Sequence / 2) % 2));
			outCommands.push_back(command);
		}
	};

	class PNetClearHandler : public PNetRewindCommandHandler
	{
	public:
		explicit PNetClearHandler(bool bOfferToAgents)
			: PNetRewindCommandHandler(bOfferToAgents)
		{
		}
		virtual PName GetKind() const override
		{
			return PName("NetClear");
		}
		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (checkActorTurn(state, command, outReason) == false)
			{
				return false;
			}
			return hasRewindPoint(state, outReason);
		}
		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			ctx.ClearRewindPoint(PName(NetRewindPoint));
		}
		virtual void Enumerate(const HGameplayState& state, const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const override
		{
			if (_bOfferToAgents == true && hasRewindPoint(state, nullptr) == true)
			{
				outCommands.push_back(HGameplayCommand(GetKind(), actor));
			}
		}
	};

	// 효과 안에서 되감기 (NetRewind Params[1] == 1).
	class PNetRewindEffect : public JGGameplayEffect
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("NetRewindNow");
		}
		virtual void Resolve(HGameplayContext& ctx, const HGameplayEffectRequest& request) override
		{
			if (ctx.RewindTo(PName(NetRewindPoint), (EGameplayRewindRandom)request.Param(0, 0)) == true)
			{
				payRewindCost(ctx, request.Subject);
			}
		}
	};

	void addRewindRules(PGameMaster& gameMaster, bool bOfferToAgents)
	{
		gameMaster.RegisterHandler(Allocate<PNetMarkHandler>(bOfferToAgents));
		gameMaster.RegisterHandler(Allocate<PNetRewindHandler>(bOfferToAgents));
		gameMaster.RegisterHandler(Allocate<PNetClearHandler>(bOfferToAgents));
		gameMaster.RegisterEffect(Allocate<PNetRewindEffect>());
	}

	HNetParticipant makeRewindClient(PSharedPtr<PNetLoopbackHub> hub, uint16 port, int32 players, const PString& name, uint64 agentSeed, bool bOfferToAgents)
	{
		HNetParticipant participant = makeLoopbackClient(hub, port, players, name, agentSeed, "");
		addRewindRules(*participant.GameMaster, bOfferToAgents);
		return participant;
	}

	HGameplayCommand rewindCommand(const HGameplayEntityId& actor, EGameplayRewindRandom random, bool bFromEffect)
	{
		HGameplayCommand command(PName("NetRewind"), actor);
		command.Params.push_back((int32)random);
		command.Params.push_back(bFromEffect == true ? 1 : 0);
		return command;
	}

	HGameplayCommand strikeCommand(const HGameplayEntityId& actor, const HGameplayEntityId& target)
	{
		HGameplayCommand command(PName("NetStrike"), actor);
		command.Targets.push_back(target);
		return command;
	}

	int32 hpOf(const PGameMaster& gameMaster, const HGameplayEntityId& id)
	{
		const HNetTestUnit* unit = gameMaster.GetState().Find<HNetTestUnit>(id);
		return unit != nullptr ? unit->Hp : INDEX_NONE;
	}

	HList<HGameplayEntityId> tokensOf(const HGameplayState& state, int32 controller)
	{
		HList<HGameplayEntityId> tokens;
		state.Each<HNetTestUnit>([&](const HGameplayEntityId& id, const HNetTestUnit& unit)
		{
			if (unit.bToken == true && unit.Controller == controller)
			{
				tokens.push_back(id);
			}
		});
		return tokens;
	}

	// 소환 명령 하나 뒤에 새로 생긴 토큰.
	HGameplayEntityId newToken(const HGameplayState& state, int32 controller, const HList<HGameplayEntityId>& before)
	{
		for (const HGameplayEntityId& id : tokensOf(state, controller))
		{
			if (std::find(before.begin(), before.end(), id) == before.end())
			{
				return id;
			}
		}
		return HGameplayEntityId::None();
	}

	// 모든 참가자가 방장 순번까지 적용할 때까지 돈다.
	void settleAll(HNetParticipant& host, HList<HNetParticipant>& clients)
	{
		for (int32 i = 0; i < 60; ++i)
		{
			tickAll(host, clients, 1);
			bool bApplied = true;
			for (HNetParticipant& client : clients)
			{
				if (client.Client->HasPendingLocalCommand() == true || client.Client->GetLastAppliedSeq() != host.GameMaster->GetState().Sequence)
				{
					bApplied = false;
				}
			}
			if (bApplied == true)
			{
				return;
			}
		}
	}

	// 모든 참가자가 방장과 같은 판인지: 마지막 적용 순번 · 상태 순번 · 체크섬.
	bool replicasMatch(HNetParticipant& host, HList<HNetParticipant>& clients)
	{
		const uint32 seq      = host.GameMaster->GetState().Sequence;
		const uint64 checksum = host.GameMaster->Checksum();
		for (HNetParticipant& client : clients)
		{
			if (client.Client->GetLastAppliedSeq() != seq || client.GameMaster->GetState().Sequence != seq || client.GameMaster->Checksum() != checksum)
			{
				return false;
			}
		}
		return true;
	}

	int32 totalDesyncs(HList<HNetParticipant>& clients)
	{
		int32 count = 0;
		for (HNetParticipant& client : clients)
		{
			count += client.Client->GetDesyncCount();
		}
		return count;
	}

	// 지점이 모두에게 있는지(bExpected == true) 또는 모두에게 없는지.
	bool everyoneHasPoint(HNetParticipant& host, HList<HNetParticipant>& clients, bool bExpected)
	{
		if ((host.GameMaster->GetState().FindRewindPoint(PName(NetRewindPoint)) != nullptr) != bExpected)
		{
			return false;
		}
		for (HNetParticipant& client : clients)
		{
			if ((client.GameMaster->GetState().FindRewindPoint(PName(NetRewindPoint)) != nullptr) != bExpected)
			{
				return false;
			}
		}
		return true;
	}

	// 엔티티가 모두에게 살아 있는지(bExpected == true) 또는 모두에게 죽은 번호인지.
	bool aliveOnAll(HNetParticipant& host, HList<HNetParticipant>& clients, const HGameplayEntityId& id, bool bExpected)
	{
		if (host.GameMaster->GetState().IsAlive(id) != bExpected)
		{
			return false;
		}
		for (HNetParticipant& client : clients)
		{
			if (client.GameMaster->GetState().IsAlive(id) != bExpected)
			{
				return false;
			}
		}
		return true;
	}

	// actor 의 차례가 될 때까지 나머지가 입력한다(clients[scripted] 는 입력하지 않는다). 모두가 따라오면 멈춘다.
	bool waitForTurn(HNetParticipant& host, HList<HNetParticipant>& clients, int32 scripted, const HGameplayEntityId& actor, int32 maxTicks)
	{
		for (int32 tick = 0; tick < maxTicks; ++tick)
		{
			if (host.Host->GetInputActor() == actor && host.GameMaster->GetState().Choice.bPending == false && replicasMatch(host, clients) == true)
			{
				return true;
			}
			host.Tick(NetTestDeltaSeconds);
			driveInput(host);
			for (int32 i = 0; i < (int32)clients.size(); ++i)
			{
				clients[i].Tick(NetTestDeltaSeconds);
				if (i != scripted)
				{
					driveInput(clients[i]);
				}
			}
		}
		return false;
	}

	// 나중 참가자가 Playing 이 될 때까지 돌리고, 자리가 이어지면 그 자리를 대신하던 호스트 AI 를 뗀다.
	void joinLate(HNetParticipant& host, HList<HNetParticipant>& clients, int32 slot)
	{
		bool bAgentRemoved = false;
		for (int32 i = 0; i < 30 && clients.back().Client->GetState() != EGameplaySessionState::Playing; ++i)
		{
			tickAll(host, clients, 1);
			const HGameplayPlayerSlot* entry = host.Host->FindSlot(slot);
			if (bAgentRemoved == false && entry != nullptr && entry->bConnected == true)
			{
				host.GameMaster->SetAgent(slot, nullptr);
				bAgentRemoved = true;
			}
		}
	}

	// 각본 판: A(slot 1)가 자기 차례 안에서 표시 → 소환 · 공격 → 되감기(+ 줄에 선 공격) → 나중 참가자 입장 → 소환 → 다시 되감기 → 소환 → 재동기 → 지우기.
	void runRewindScriptTests(HCheck& check)
	{
		std::cout << "-- rewind: a rule effect rewinds to a marked point on every machine (scripted, join after the rewind)" << std::endl;

		const uint16 port    = 7108;
		const int32  players = 4;   // 호스트 + 원격 3 자리, 적(조작 주체 4)은 호스트 AI
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();
		const PName point(NetRewindPoint);

		HNetParticipant host = makeLobbyHost(hub, port, players, 801);
		addRewindRules(*host.GameMaster, false);
		host.GameMaster->SetAgent(3, Allocate<PGameplayRandomAgent>((uint64)803));   // 나중 참가자 C 가 오기 전 자리 3

		HList<HNetParticipant> clients;
		clients.push_back(makeRewindClient(hub, port, players, "A", 811, false));
		clients.push_back(makeRewindClient(hub, port, players, "B", 812, false));
		tickUntilJoined(host, clients, EGameplaySessionState::Ready);
		check(host.Host->StartGame(8181) == true, "host starts the game");
		tickAll(host, clients, 3);

		int32 rejectedA = 0;
		PString rejectedReason;
		clients[0].Client->OnLocalCommandRejected.AddLambda([&](const HGameplayCommand& command, const PString& reason)
		{
			++rejectedA;
			rejectedReason = reason;
		});

		const HGameplayEntityId actorA = unitOfController(host.GameMaster->GetState(), 1);
		const HGameplayEntityId enemy  = unitOfController(host.GameMaster->GetState(), players);
		const bool bTurnA = waitForTurn(host, clients, 0, actorA, 20000);
		check(bTurnA == true, PString::Format("A's unit gets the turn (seq %u)", host.GameMaster->GetState().Sequence));

		// 1. 표시
		EGameplaySessionSubmit marked = clients[0].Client->SubmitLocal(HGameplayCommand(PName("NetMark"), actorA));
		settleAll(host, clients);
		check(marked == EGameplaySessionSubmit::Sent && everyoneHasPoint(host, clients, true) == true && replicasMatch(host, clients) == true,
			PString::Format("A marks the point: every machine has it and matches (seq %u)", host.GameMaster->GetState().Sequence));
		const int32 hpAtMark    = hpOf(*host.GameMaster, actorA);
		const int32 enemyAtMark = hpOf(*host.GameMaster, enemy);
		const int32 hpAfterCost = hpAtMark > 1 ? hpAtMark - 1 : hpAtMark;

		// 2. 지점 뒤: 토큰 소환 · 공격
		HList<HGameplayEntityId> before = tokensOf(host.GameMaster->GetState(), 1);
		clients[0].Client->SubmitLocal(HGameplayCommand(PName("NetSummon"), actorA));
		settleAll(host, clients);
		const HGameplayEntityId tokenT = newToken(host.GameMaster->GetState(), 1, before);
		clients[0].Client->SubmitLocal(strikeCommand(actorA, enemy));
		settleAll(host, clients);
		check(tokenT.IsValid() == true && aliveOnAll(host, clients, tokenT, true) == true && replicasMatch(host, clients) == true,
			PString::Format("after the point A summons token %s and strikes the enemy (Hp %d -> %d); every machine matches", tokenT.ToString(), enemyAtMark, hpOf(*host.GameMaster, enemy)));

		// 3. 효과 안 되감기(난수 이어 감) + 그 뒤에 줄을 선 토큰 공격
		const uint32 seqBeforeRewind = host.GameMaster->GetState().Sequence;
		EGameplaySessionSubmit rewound = clients[0].Client->SubmitLocal(rewindCommand(actorA, EGameplayRewindRandom::KeepCurrent, true));
		EGameplaySessionSubmit queued  = clients[0].Client->SubmitLocal(strikeCommand(actorA, tokenT));
		settleAll(host, clients);
		check(rewound == EGameplaySessionSubmit::Sent && queued == EGameplaySessionSubmit::Queued, "A sends a rewind (in an effect, keep random) and queues a strike on the token behind it");
		check(host.GameMaster->GetState().Sequence == seqBeforeRewind + 1 && replicasMatch(host, clients) == true,
			PString::Format("the rewind is one more command on every machine: sequence %u -> %u, not rewound", seqBeforeRewind, host.GameMaster->GetState().Sequence));
		check(rejectedA == 1 && rejectedReason == PString("invalid target"),
			PString::Format("the queued strike is validated again after the rewind and rejected locally (%d, %s)", rejectedA, rejectedReason));
		check(aliveOnAll(host, clients, tokenT, false) == true, PString::Format("token %s made after the point is a dead id on every machine", tokenT.ToString()));
		check(hpOf(*host.GameMaster, actorA) == hpAfterCost && hpOf(*host.GameMaster, enemy) == enemyAtMark,
			PString::Format("state is the point plus the kept change (A Hp %d -> %d, enemy Hp back to %d)", hpAtMark, hpOf(*host.GameMaster, actorA), enemyAtMark));
		check(everyoneHasPoint(host, clients, true) == true, "the point stays after the rewind on every machine");

		// 4. 되감은 직후 나중 참가자 C 가 문서로 들어온다 — 지점 · 세대 바닥이 문서에 실려 와야 한다
		PString documentWithPoint;
		host.GameMaster->ExportDocument(&documentWithPoint);
		clients.push_back(makeRewindClient(hub, port, players, "C", 813, false));
		joinLate(host, clients, 3);
		HNetParticipant& lateC = clients.back();
		check(lateC.Client->GetState() == EGameplaySessionState::Playing && lateC.Client->GetLocalSlot() == 3 && replicasMatch(host, clients) == true,
			PString::Format("C joins right after the rewind and matches from the document (seq %u, document %llu bytes)", host.GameMaster->GetState().Sequence, (uint64)documentWithPoint.Size()));
		check(lateC.GameMaster->GetState().FindRewindPoint(point) != nullptr && lateC.GameMaster->GetState().IsAlive(tokenT) == false,
			"C's replica has the point and sees the token as a dead id");

		// 5. 되감은 뒤 소환: 죽은 번호를 다시 내지 않는다(C 도 같은 번호 — 세대 바닥이 문서로 왔다)
		before = tokensOf(host.GameMaster->GetState(), 1);
		clients[0].Client->SubmitLocal(HGameplayCommand(PName("NetSummon"), actorA));
		settleAll(host, clients);
		const HGameplayEntityId tokenT2 = newToken(host.GameMaster->GetState(), 1, before);
		check(tokenT2.IsValid() == true && (tokenT2 == tokenT) == false && aliveOnAll(host, clients, tokenT2, true) == true && aliveOnAll(host, clients, tokenT, false) == true,
			PString::Format("the next token is %s, not the dead %s, on every machine including C", tokenT2.ToString(), tokenT.ToString()));
		check(replicasMatch(host, clients) == true && totalDesyncs(clients) == 0, "every machine matches after the spawn, no desync");

		// 6. 핸들러 안 되감기(난수 지점 것으로) — C 도 같은 지점으로
		clients[0].Client->SubmitLocal(rewindCommand(actorA, EGameplayRewindRandom::RestorePoint, false));
		settleAll(host, clients);
		check(replicasMatch(host, clients) == true && aliveOnAll(host, clients, tokenT2, false) == true && hpOf(*host.GameMaster, actorA) == hpAfterCost && hpOf(*host.GameMaster, enemy) == enemyAtMark,
			PString::Format("A rewinds again (in the handler, restore random): every machine including C matches and token %s is dead", tokenT2.ToString()));

		// 7. 다시 소환
		before = tokensOf(host.GameMaster->GetState(), 1);
		clients[0].Client->SubmitLocal(HGameplayCommand(PName("NetSummon"), actorA));
		settleAll(host, clients);
		const HGameplayEntityId tokenT3 = newToken(host.GameMaster->GetState(), 1, before);
		check(tokenT3.IsValid() == true && (tokenT3 == tokenT) == false && (tokenT3 == tokenT2) == false && replicasMatch(host, clients) == true && totalDesyncs(clients) == 0,
			PString::Format("the token after the second rewind is %s (neither dead id) on every machine", tokenT3.ToString()));

		// 8. B 의 복제본을 고의로 바꾼다 → 다음 승인에서 검출 → 지점이 든 재동기 문서로 복구
		HGameplayState broken = clients[1].GameMaster->Snapshot();
		broken.EachMutable<HNetTestUnit>([&](const HGameplayEntityId&, HNetTestUnit& unit)
		{
			unit.Hp += 5;
		});
		clients[1].GameMaster->Restore(broken);
		clients[0].Client->SubmitLocal(strikeCommand(actorA, enemy));
		settleAll(host, clients);
		tickAll(host, clients, 5);
		check(clients[1].Client->GetDesyncCount() == 1 && replicasMatch(host, clients) == true && clients[1].GameMaster->GetState().FindRewindPoint(point) != nullptr,
			PString::Format("B's tampered replica is caught once and recovers from a resync document that carries the point (%d)", clients[1].Client->GetDesyncCount()));

		// 9. 지우기 → 그 뒤 되감기는 로컬 검증에서 거절
		clients[0].Client->SubmitLocal(HGameplayCommand(PName("NetClear"), actorA));
		settleAll(host, clients);
		check(everyoneHasPoint(host, clients, false) == true && replicasMatch(host, clients) == true, "A clears the point on every machine");
		PString documentWithoutPoint;
		host.GameMaster->ExportDocument(&documentWithoutPoint);
		info(PString::Format("join document %llu bytes with the point, %llu bytes after clearing it", (uint64)documentWithPoint.Size(), (uint64)documentWithoutPoint.Size()));
		PString reason;
		const uint32 seqBeforeMissing = host.GameMaster->GetState().Sequence;
		EGameplaySessionSubmit missing = clients[0].Client->SubmitLocal(rewindCommand(actorA, EGameplayRewindRandom::KeepCurrent, false), &reason);
		settleAll(host, clients);
		check(missing == EGameplaySessionSubmit::Rejected && reason == PString("no rewind point") && host.GameMaster->GetState().Sequence == seqBeforeMissing,
			PString::Format("a rewind without a point is rejected locally (%s)", reason));

		// 10. 모두가 조금 더 둔다
		const uint32 seqEnd = host.GameMaster->GetState().Sequence + 80;
		playUntil(host, clients, seqEnd, 40000);
		settleAll(host, clients);
		check(host.GameMaster->GetState().Sequence >= seqEnd && replicasMatch(host, clients) == true && clients[0].Client->GetDesyncCount() == 0 && clients[2].Client->GetDesyncCount() == 0,
			PString::Format("everyone plays on to seq %u and matches (no other desync)", host.GameMaster->GetState().Sequence));

		closeAll(host, clients);
	}

	// 무작위 판: 에이전트(참가자 · 방장 · 방장 AI)가 표시 · 되감기 · 지우기를 다른 명령과 함께 고른다. 판 도중 참가자 C 가 들어온다.
	void runRewindPlayTests(HCheck& check)
	{
		std::cout << "-- rewind: random play with marks, rewinds and clears (1 host + 3 clients, join in progress)" << std::endl;

		const uint16 port    = 7109;
		const int32  players = 4;
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();

		HNetParticipant host = makeLobbyHost(hub, port, players, 901);
		addRewindRules(*host.GameMaster, true);
		host.GameMaster->SetAgent(3, Allocate<PGameplayRandomAgent>((uint64)903));

		HList<HNetParticipant> clients;
		clients.push_back(makeRewindClient(hub, port, players, "A", 911, true));
		clients.push_back(makeRewindClient(hub, port, players, "B", 912, true));
		tickUntilJoined(host, clients, EGameplaySessionState::Ready);
		check(host.Host->StartGame(9191) == true, "host starts the game");
		tickAll(host, clients, 3);

		// 순번 300 뒤 지점이 있는 때에 C 가 들어온다(지점 · 세대 바닥이 든 입장 문서).
		playUntil(host, clients, 300, 100000);
		for (int32 i = 0; i < 2000 && host.GameMaster->GetState().FindRewindPoint(PName(NetRewindPoint)) == nullptr; ++i)
		{
			playUntil(host, clients, host.GameMaster->GetState().Sequence + 1, 1000);
		}
		const bool bPointAtJoin = host.GameMaster->GetState().FindRewindPoint(PName(NetRewindPoint)) != nullptr;
		const uint32 joinSeq    = host.GameMaster->GetState().Sequence;
		clients.push_back(makeRewindClient(hub, port, players, "C", 913, true));
		joinLate(host, clients, 3);
		check(bPointAtJoin == true && clients.back().Client->GetState() == EGameplaySessionState::Playing && replicasMatch(host, clients) == true,
			PString::Format("C starts joining at seq %u while a point exists and matches once in (seq %u)", joinSeq, host.GameMaster->GetState().Sequence));

		playUntil(host, clients, 600, 200000);
		settleAll(host, clients);
		tickAll(host, clients, 10);

		const int32 marks   = countCommands(*host.GameMaster, "NetMark");
		const int32 rewinds = countCommands(*host.GameMaster, "NetRewind");
		const int32 clears  = countCommands(*host.GameMaster, "NetClear");
		check(host.GameMaster->GetState().Sequence >= 600, PString::Format("host reached seq %u", host.GameMaster->GetState().Sequence));
		check(marks > 0 && rewinds > 0 && clears > 0, PString::Format("players and AI marked %d, rewound %d and cleared %d times", marks, rewinds, clears));
		check(replicasMatch(host, clients) == true && totalDesyncs(clients) == 0,
			PString::Format("every replica matches the host at seq %u with no desync (checksum %llu)", host.GameMaster->GetState().Sequence, host.GameMaster->Checksum()));
		check(clients.back().Submitted > 0, PString::Format("C sends its own commands after joining (%d)", clients.back().Submitted));

		closeAll(host, clients);
	}

	// ---- 2 프로세스 대기방 확인 (세션 공유 데이터 · 준비) ----------------------------------------

	const char* ProcessSeedKey     = "net.test.seed";
	const char* ProcessProgressKey = "net.test.progress";

	// 세션 공유 데이터 요약: 키 이름 순으로 "키=값;" 을 FNV-1a. 프로세스끼리 같은 표인지 비교한다.
	uint64 sessionDataHash(const PGameplaySession& session)
	{
		HList<HPair<HRawString, HRawString>> entries;
		for (const HGameplaySessionDataEntry& entry : session.GetSessionData())
		{
			entries.push_back(HPair<HRawString, HRawString>(entry.Key.ToString().GetRawString(), entry.Value.GetRawString()));
		}
		std::sort(entries.begin(), entries.end());

		uint64 hash = 1469598103934665603ull;
		auto mix = [&hash](const HRawString& text)
		{
			for (char c : text)
			{
				hash ^= (uint8)c;
				hash *= 1099511628211ull;
			}
		};
		for (const HPair<HRawString, HRawString>& entry : entries)
		{
			mix(entry.first);
			mix("=");
			mix(entry.second);
			mix(";");
		}
		return hash;
	}

	int32 readySeatCount(const PGameplaySession& session)
	{
		int32 count = 0;
		for (const HGameplayPlayerSlot& slot : session.GetSlots())
		{
			if (slot.bReady == true)
			{
				++count;
			}
		}
		return count;
	}

	// 대기방: 원격 자리 clientCount 개가 모두 접속해 준비를 켰는가. 먼저 들어왔다 나간 클라의 자리는 비워져 있어야 한다.
	bool lobbyReady(const PGameplayHostSession& host, int32 clientCount)
	{
		int32 ready = 0;
		for (const HGameplayPlayerSlot& slot : host.GetSlots())
		{
			if (slot.Slot != host.GetLocalSlot() && slot.bConnected == true && slot.bReady == true)
			{
				++ready;
			}
		}
		return host.GetConnectedPeerCount() == clientCount && ready == clientCount;
	}

	float32 elapsedSince(std::chrono::steady_clock::time_point& last)
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		float32 seconds = std::chrono::duration<float32>(now - last).count();
		last = now;
		if (seconds > 0.25f)
		{
			seconds = 0.25f;
		}
		return seconds;
	}

	// ---- 월드 경유 (게임 인스턴스 · GameMasterActor · 컨트롤러) --------------------------------

	// 게임 인스턴스 월드의 참가자. 엔트리 액터가 하는 일을 OnWorldLoaded 에서 한다 (테스트 규칙은 리플렉션 클래스가 아니다).
	struct HNetWorldParticipant
	{
		PWeakPtr<JGGameMasterActor>         GameMasterActor;
		PWeakPtr<JGGameplayControllerActor> Controller;
		PSharedPtr<PGameMaster>             GameMaster;
		PSharedPtr<PGameplayRandomAgent>    Agent;
		int32                               PlayerCount  = 0;
		uint64                              Seed         = 0;
		int32                               WorldsLoaded = 0;
		int32                               Submitted    = 0;
		int32                               Executed     = 0;
		int32                               Sent         = 0;
	};

	// 엔트리 액터 규약: 규칙 등록은 모든 기계, 초기 상태 구성과 시작은 권한 쪽만.
	void enterNetWorld(PSharedPtr<PWorld> world, HNetWorldParticipant& participant)
	{
		PSharedPtr<JGGameMasterActor>         gameMasterActor = world->SpawnActor<JGGameMasterActor>(PName("GameMaster"));
		PSharedPtr<JGGameplayControllerActor> controller      = world->SpawnActor<JGGameplayControllerActor>(PName("Controller"));
		controller->SetGameMasterActor(gameMasterActor);

		PSharedPtr<PGameMaster> gameMaster = Allocate<PGameMaster>();
		setupNetRules(*gameMaster, participant.PlayerCount);
		gameMasterActor->SetGameMaster(gameMaster);

		participant.GameMasterActor = gameMasterActor;
		participant.Controller      = controller;
		participant.GameMaster      = gameMaster;
		++participant.WorldsLoaded;

		if (JGGameInstance::Get().IsAuthority() == true)
		{
			buildNetInitialState(*gameMaster, participant.PlayerCount);
			gameMasterActor->StartGame(participant.Seed + (uint64)participant.WorldsLoaded);
		}
	}

	// 로컬 플레이어 입력 한 번을 게임과 같은 경로로 넣는다: 컨트롤러 → GameMasterActor(입력 정책) → 세션.
	bool driveController(HNetWorldParticipant& participant)
	{
		PSharedPtr<JGGameMasterActor>         gameMasterActor = participant.GameMasterActor.Pin();
		PSharedPtr<JGGameplayControllerActor> controller      = participant.Controller.Pin();
		if (gameMasterActor == nullptr || controller == nullptr || participant.GameMaster == nullptr || participant.GameMaster->IsStarted() == false)
		{
			return false;
		}

		PSharedPtr<PGameplaySession> session = gameMasterActor->GetSession();
		if (session == nullptr || session->GetState() != EGameplaySessionState::Playing)
		{
			return false;
		}
		PSharedPtr<PGameplayClientSession> client = RawDynamicCast<PGameplayClientSession>(session);
		if (client != nullptr && client->HasPendingLocalCommand() == true)
		{
			return false;
		}
		// 연출 중이면 기다린다 (Buffer 정책이 쌓아 두면 같은 입력을 두 번 넣게 된다).
		if (gameMasterActor->IsBusy() == true)
		{
			return false;
		}

		const HGameplayState& state = participant.GameMaster->GetState();
		if (state.Turn.IsFinished() == true)
		{
			return false;
		}
		HGameplayEntityId actor = controller->GetInputActor();
		if (actor.IsValid() == false || controller->IsLocallyControlled(actor) == false)
		{
			return false;
		}

		EGameMasterActorSubmit result = EGameMasterActorSubmit::Rejected;
		PString reason;
		if (state.Choice.bPending == true)
		{
			HList<HGameplayEntityId> selection;
			if (participant.Agent->ChooseOption(*participant.GameMaster, state.Choice, selection) == false)
			{
				return false;
			}
			result = controller->ResolveChoice(selection, &reason);
		}
		else
		{
			HGameplayCommand command;
			if (participant.Agent->ChooseCommand(*participant.GameMaster, actor, command) == false)
			{
				return false;
			}
			if (controller->BeginCommand(command.Kind, command.Actor) == false)
			{
				return false;
			}
			for (const HGameplayEntityId& target : command.Targets)
			{
				controller->AddTarget(target);
			}
			for (int32 value : command.Params)
			{
				controller->AddParam(value);
			}
			for (const HGameplayCoord& coord : command.Path)
			{
				controller->AddPathCoord(coord);
			}
			result = controller->Commit(&reason);
		}

		if (result == EGameMasterActorSubmit::Rejected)
		{
			return false;
		}
		++participant.Submitted;
		if (result == EGameMasterActorSubmit::Sent)
		{
			++participant.Sent;
		}
		else
		{
			++participant.Executed;
		}
		return true;
	}

	bool sameGame(const PGameMaster& lhs, const PGameMaster& rhs)
	{
		return lhs.GetState().Sequence == rhs.GetState().Sequence && lhs.Checksum() == rhs.Checksum();
	}

	// 살아 있는 엔티티마다 액터가 묶여 있는가 (연출 큐가 빈 뒤).
	bool allEntitiesBound(const HNetWorldParticipant& participant)
	{
		PSharedPtr<JGGameMasterActor> gameMasterActor = participant.GameMasterActor.Pin();
		if (gameMasterActor == nullptr || participant.GameMaster == nullptr)
		{
			return false;
		}
		HList<HGameplayEntityId> alive;
		participant.GameMaster->GetState().Entities.CollectAlive(alive);
		for (const HGameplayEntityId& id : alive)
		{
			if (gameMasterActor->FindActor(id) == nullptr)
			{
				return false;
			}
		}
		return alive.empty() == false;
	}

	void tickWorld(JGGameInstance& gameInstance, HList<HNetParticipant>& peers, int32 ticks)
	{
		for (int32 i = 0; i < ticks; ++i)
		{
			gameInstance.Tick(NetTestDeltaSeconds);
			for (HNetParticipant& peer : peers)
			{
				peer.Tick(NetTestDeltaSeconds);
			}
		}
	}

	// 기준 GameMaster 의 순번이 target 에 이를 때까지 월드 참가자(컨트롤러)와 커널 참가자들이 입력한다.
	void playWorldUntil(JGGameInstance& gameInstance, HNetWorldParticipant& local, HList<HNetParticipant>& peers, PSharedPtr<PGameMaster> reference, uint32 targetSeq, int32 maxTicks)
	{
		for (int32 tick = 0; tick < maxTicks; ++tick)
		{
			if (reference == nullptr || reference->GetState().Sequence >= targetSeq)
			{
				return;
			}
			gameInstance.Tick(NetTestDeltaSeconds);
			driveController(local);
			for (HNetParticipant& peer : peers)
			{
				peer.Tick(NetTestDeltaSeconds);
				driveInput(peer);
			}
		}
	}

	// 게임 인스턴스가 리슨 서버: 로비 입장 → 월드 로드(이동 알림 · 시작) → 컨트롤러 입력 → 월드 이동 → 떠나기.
	void runWorldHostTests(HCheck& check, JGGameInstance& gameInstance)
	{
		constexpr int32  players = 3;
		constexpr uint16 port    = 7401;
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();

		HGameplaySessionConfig config;
		config.PlayerName          = "WorldHost";
		config.MaxPlayers          = players;
		config.DesyncDumpDirectory = "";
		PSharedPtr<PGameplayHostSession> host = gameInstance.HostSession(port, config, Allocate<PNetLoopbackTransport>(hub));
		check(host != nullptr && gameInstance.GetSession() == host && gameInstance.IsAuthority() == true, "game instance hosts a listen server");
		if (host == nullptr)
		{
			return;
		}

		HList<HNetParticipant> clients;
		clients.push_back(makeLoopbackClient(hub, port, players, "WorldA", 611, ""));
		clients.push_back(makeLoopbackClient(hub, port, players, "WorldB", 612, ""));
		int32 travels[2] = { 0, 0 };
		for (int32 i = 0; i < 2; ++i)
		{
			HNetParticipant* client = &clients[i];
			int32*           count  = &travels[i];
			client->Client->OnTravelRequested.AddLambda([client, count](const PName&)
			{
				// 커널 클라가 이동을 따라간다: 새 GameMaster(규칙만)를 붙이면 뒤따르는 시작이 적용된다.
				client->GameMaster = Allocate<PGameMaster>();
				setupNetRules(*client->GameMaster, players);
				client->Client->BindGameMaster(client->GameMaster);
				++(*count);
			});
		}

		tickWorld(gameInstance, clients, 30);
		check(allClientsIn(clients, EGameplaySessionState::Ready) == true && host->GetConnectedPeerCount() == 2, "two clients joined the lobby before any world was loaded");

		HNetWorldParticipant local;
		local.PlayerCount = players;
		local.Seed        = 4240;
		local.Agent       = Allocate<PGameplayRandomAgent>(613);
		HDelegateHandle loaded = gameInstance.OnWorldLoaded.AddLambda([&local](PSharedPtr<PWorld> world)
		{
			enterNetWorld(world, local);
		});

		gameInstance.LoadWorld(PName("NetWorldA"));
		PSharedPtr<JGGameMasterActor>         gameMasterActor = local.GameMasterActor.Pin();
		PSharedPtr<JGGameplayControllerActor> controller      = local.Controller.Pin();
		check(local.GameMaster != nullptr && local.GameMaster->IsStarted() == true && host->GetState() == EGameplaySessionState::Playing, "entry StartGame started the game through the session");
		check(gameMasterActor != nullptr && gameMasterActor->GetSession() == host, "GameMasterActor bound its GameMaster to the game instance session");
		check(local.GameMaster != nullptr && local.GameMaster->IsUndoEnabled() == false, "local undo is off while bound to a listen server");

		const HGameplayState& state = local.GameMaster->GetState();
		check(controller != nullptr && controller->IsLocallyControlled(unitOfController(state, 0)) == true, "controller owns the local slot's actor");
		check(controller != nullptr && controller->BeginCommand(PName("NetStrike"), unitOfController(state, 1)) == false, "controller refuses a remote player's actor");
		check(controller != nullptr && controller->BeginCommand(PName("NetStrike"), unitOfController(state, players)) == false && controller->IsDrafting() == false, "controller refuses the AI's actor");

		tickWorld(gameInstance, clients, 10);
		check(travels[0] == 1 && travels[1] == 1 && allClientsIn(clients, EGameplaySessionState::Playing) == true,
			PString::Format("clients followed the Travel and received StartGame (travels %d / %d, states %d / %d)", travels[0], travels[1], (int32)clients[0].Client->GetState(), (int32)clients[1].Client->GetState()));

		playWorldUntil(gameInstance, local, clients, local.GameMaster, 300, 20000);
		tickWorld(gameInstance, clients, 10);
		bool bSame = true;
		for (HNetParticipant& client : clients)
		{
			bSame = bSame && sameGame(*client.GameMaster, *local.GameMaster) == true && client.Client->GetDesyncCount() == 0;
		}
		check(local.GameMaster->GetState().Sequence >= 300 && bSame == true, PString::Format("world host and clients agree at seq %u, desync 0", local.GameMaster->GetState().Sequence));
		check(local.Executed > 0 && local.Sent == 0 && clients[0].Submitted > 0 && clients[1].Submitted > 0,
			PString::Format("inputs: host controller %d executed, clients %d / %d", local.Executed, clients[0].Submitted, clients[1].Submitted));
		check(countCommandsBy(*local.GameMaster, unitOfController(local.GameMaster->GetState(), players)) > 0, "host session drove the AI in the world");
		check(allEntitiesBound(local) == true, "every alive entity has an actor in the host world");

		// 월드 이동: 호스트가 다른 월드를 로드하면 새 게임이 시작되고 클라가 따라온다.
		PSharedPtr<PGameMaster> firstGame = local.GameMaster;
		gameInstance.LoadWorld(PName("NetWorldB"));
		check(local.WorldsLoaded == 2 && local.GameMaster != firstGame && local.GameMaster->IsStarted() == true, "second world started a new game");
		tickWorld(gameInstance, clients, 10);
		check(travels[0] == 2 && travels[1] == 2 && allClientsIn(clients, EGameplaySessionState::Playing) == true,
			PString::Format("clients followed the second Travel (travels %d / %d)", travels[0], travels[1]));

		playWorldUntil(gameInstance, local, clients, local.GameMaster, 150, 20000);
		tickWorld(gameInstance, clients, 10);
		bSame = true;
		for (HNetParticipant& client : clients)
		{
			bSame = bSame && sameGame(*client.GameMaster, *local.GameMaster) == true && client.Client->GetDesyncCount() == 0;
		}
		check(local.GameMaster->GetState().Sequence >= 150 && bSame == true, PString::Format("after travel all agree at seq %u, desync 0", local.GameMaster->GetState().Sequence));

		gameInstance.OnWorldLoaded.Remove(loaded);
		gameInstance.UnloadWorld();
		check(host->GetGameMaster() == nullptr, "unloading the world unbinds the GameMaster");
		gameInstance.LeaveSession();
		tickWorld(gameInstance, clients, 10);
		check(gameInstance.GetSession() != nullptr && gameInstance.GetSession()->GetMode() == EGameplayNetMode::Standalone && allClientsIn(clients, EGameplaySessionState::Closed) == true,
			"LeaveSession returns to standalone and closes the clients");
	}

	// 게임 인스턴스가 클라: 진행 중 입장(Welcome 의 월드 로드 · 문서) → 컨트롤러 입력(Sent) → 호스트 이동 → 호스트 종료 → 싱글로 이어받기.
	void runWorldClientTests(HCheck& check, JGGameInstance& gameInstance)
	{
		constexpr int32  players = 2;
		constexpr uint16 port    = 7402;
		PSharedPtr<PNetLoopbackHub> hub = Allocate<PNetLoopbackHub>();

		HNetParticipant host;
		host.GameMaster = Allocate<PGameMaster>();
		setupNetRules(*host.GameMaster, players);
		buildNetInitialState(*host.GameMaster, players);
		host.Agent = Allocate<PGameplayRandomAgent>(621);
		HGameplaySessionConfig hostConfig;
		hostConfig.PlayerName          = "KernelHost";
		hostConfig.MaxPlayers          = players;
		hostConfig.DesyncDumpDirectory = "";
		host.Loopback = Allocate<PNetLoopbackTransport>(hub);
		host.Host     = PGameplayHostSession::CreateListenServer(host.Loopback, port, hostConfig);
		host.Host->BindGameMaster(host.GameMaster);
		host.Host->NotifyTravel(PName("NetWorldC"));
		host.Host->StartGame(99);

		HList<HNetParticipant> peers;
		peers.push_back(host);
		HNetParticipant& hostRef = peers[0];
		for (int32 tick = 0; tick < 50; ++tick)
		{
			hostRef.Tick(NetTestDeltaSeconds);
			driveInput(hostRef);
		}
		uint32 seqBeforeJoin = hostRef.GameMaster->GetState().Sequence;

		HNetWorldParticipant local;
		local.PlayerCount = players;
		local.Agent       = Allocate<PGameplayRandomAgent>(622);
		HDelegateHandle loaded = gameInstance.OnWorldLoaded.AddLambda([&local](PSharedPtr<PWorld> world)
		{
			enterNetWorld(world, local);
		});

		HGameplaySessionConfig config;
		config.PlayerName          = "WorldClient";
		config.DesyncDumpDirectory = "";
		PSharedPtr<PGameplayClientSession> client = gameInstance.JoinSession("loopback", port, config, Allocate<PNetLoopbackTransport>(hub));
		check(client != nullptr && gameInstance.IsAuthority() == false, "joined game instance is not the authority");
		if (client == nullptr)
		{
			gameInstance.OnWorldLoaded.Remove(loaded);
			return;
		}

		tickWorld(gameInstance, peers, 30);
		check(gameInstance.GetWorld() != nullptr && gameInstance.GetWorld()->GetName() == PName("NetWorldC"), "client loaded the host's world named in Welcome");
		check(client->GetState() == EGameplaySessionState::Playing && local.GameMaster != nullptr && sameGame(*local.GameMaster, *hostRef.GameMaster) == true,
			PString::Format("mid-game join: document applied to the world's GameMaster at seq %u (host was at %u before the join)", local.GameMaster != nullptr ? local.GameMaster->GetState().Sequence : 0, seqBeforeJoin));

		PSharedPtr<JGGameMasterActor>         gameMasterActor = local.GameMasterActor.Pin();
		PSharedPtr<JGGameplayControllerActor> controller      = local.Controller.Pin();
		check(gameMasterActor != nullptr && gameMasterActor->GetSession() == client, "client GameMasterActor bound at BeginPlay");
		check(allEntitiesBound(local) == true, "client world rebuilt entity actors from the document");
		check(controller != nullptr && controller->BeginCommand(PName("NetStrike"), unitOfController(local.GameMaster->GetState(), 0)) == false, "client controller refuses the host's actor");
		check(controller != nullptr && controller->IsLocallyControlled(unitOfController(local.GameMaster->GetState(), 1)) == true, "client controller owns its slot's actor");
		check(gameMasterActor != nullptr && gameMasterActor->StartGame(5) == false, "StartGame is refused on a client");

		playWorldUntil(gameInstance, local, peers, hostRef.GameMaster, seqBeforeJoin + 200, 20000);
		tickWorld(gameInstance, peers, 10);
		check(sameGame(*local.GameMaster, *hostRef.GameMaster) == true && client->GetDesyncCount() == 0 && local.Sent > 0 && local.Executed == 0,
			PString::Format("client controller commands were sent (%d) and replicated, seq %u, desync 0", local.Sent, local.GameMaster->GetState().Sequence));

		// 호스트 이동: 이동 알림 → 새 게임 시작. 클라 게임 인스턴스가 월드를 바꾸고 새 GameMaster 가 보관된 시작을 받는다.
		PSharedPtr<PGameMaster> firstGame = local.GameMaster;
		hostRef.Host->UnbindGameMaster();
		hostRef.Host->NotifyTravel(PName("NetWorldD"));
		hostRef.GameMaster = Allocate<PGameMaster>();
		setupNetRules(*hostRef.GameMaster, players);
		buildNetInitialState(*hostRef.GameMaster, players);
		hostRef.Host->BindGameMaster(hostRef.GameMaster);
		hostRef.Host->StartGame(123);
		tickWorld(gameInstance, peers, 10);
		check(gameInstance.GetWorld() != nullptr && gameInstance.GetWorld()->GetName() == PName("NetWorldD") && local.WorldsLoaded == 2 && local.GameMaster != firstGame,
			"client followed the host's Travel into a new world");
		check(client->GetState() == EGameplaySessionState::Playing && sameGame(*local.GameMaster, *hostRef.GameMaster) == true, "held StartGame applied to the new world's GameMaster");

		playWorldUntil(gameInstance, local, peers, hostRef.GameMaster, 100, 20000);
		tickWorld(gameInstance, peers, 10);
		check(sameGame(*local.GameMaster, *hostRef.GameMaster) == true && client->GetDesyncCount() == 0, PString::Format("after travel client agrees at seq %u", local.GameMaster->GetState().Sequence));

		// 호스트가 닫으면 클라 세션은 Closed. 월드는 그대로 남는다. 싱글로 돌아가면 그 게임을 이어받는다.
		hostRef.Host->Close("host left");
		tickWorld(gameInstance, peers, 10);
		check(client->GetState() == EGameplaySessionState::Closed && gameInstance.HasWorld() == true, "host closed: client session closed, world kept");

		uint32 seqBeforeLeave = local.GameMaster->GetState().Sequence;
		gameInstance.LeaveSession();
		PSharedPtr<PGameplaySession> standalone = gameInstance.GetSession();
		gameMasterActor = local.GameMasterActor.Pin();   // 이동 뒤 새 월드의 액터
		check(standalone != nullptr && standalone->GetMode() == EGameplayNetMode::Standalone && gameMasterActor != nullptr && gameMasterActor->GetSession() == standalone,
			"LeaveSession rebinds the world's GameMaster to a standalone session");
		HList<HNetParticipant> none;
		playWorldUntil(gameInstance, local, none, local.GameMaster, seqBeforeLeave + 20, 5000);
		check(standalone != nullptr && standalone->GetState() == EGameplaySessionState::Playing && local.GameMaster->GetState().Sequence >= seqBeforeLeave + 20 && local.Executed > 0,
			PString::Format("standalone adopted the started game and plays on (seq %u -> %u)", seqBeforeLeave, local.GameMaster->GetState().Sequence));
		check(local.GameMaster->IsUndoEnabled() == true, "local undo is back in standalone");

		gameInstance.OnWorldLoaded.Remove(loaded);
		gameInstance.UnloadWorld();
	}

	void runWorldTests(HCheck& check)
	{
		if (JGGameInstance::HasInstance() == false)
		{
			check(false, "world tests need the game instance (connect GameFrameWork)");
			return;
		}

		// 게임이 월드를 띄운 프로세스(프로젝트 모드 에디터)에서는 건드리지 않는다.
		JGGameInstance& gameInstance = JGGameInstance::Get();
		PSharedPtr<PGameplaySession> session = gameInstance.GetSession();
		if (gameInstance.HasWorld() == true || session == nullptr || session->GetMode() != EGameplayNetMode::Standalone)
		{
			info("world section skipped: the game instance already has a world or a network session");
			++check.Skipped;
			return;
		}

		runWorldHostTests(check, gameInstance);
		runWorldClientTests(check, gameInstance);

		PSharedPtr<PGameplaySession> after = gameInstance.GetSession();
		check(gameInstance.HasWorld() == false && after != nullptr && after->GetMode() == EGameplayNetMode::Standalone, "game instance left standalone with no world");
	}
}

HSelfTestResult PGameplayNetSelfTest::Run(const PString& which)
{
	HCheck check;
	std::cout << "== Network self test (" << which.GetRawString() << ") ==" << std::endl;

	bool bAll = which.Empty() == true || which == PString("all");
	if (bAll == true || which == PString("transport"))
	{
		runTransportTests(check);
	}
	if (bAll == true || which == PString("session"))
	{
		runHandshakeTests(check);
		runSessionTests(check);
	}
	if (bAll == true || which == PString("recovery"))
	{
		runRecoveryTests(check);
		runRejoinTests(check);
	}
	if (bAll == true || which == PString("lobby"))
	{
		runLobbyDataTests(check);
		runLobbySeatTests(check);
		runLobbyReadyTests(check);
	}
	if (bAll == true || which == PString("rewind"))
	{
		runRewindScriptTests(check);
		runRewindPlayTests(check);
	}
	if (bAll == true || which == PString("world"))
	{
		runWorldTests(check);
	}

	std::cout << "== Network self test: " << check.Passed << " passed, " << check.Failures << " failed ==" << std::endl;
	JG_LOG(NetSelfTest, ELogLevel::Info, "Network self test: %d passed, %d failed", check.Passed, check.Failures);
	return { check.Passed, check.Failures, check.Skipped };
}

int32 PGameplayNetSelfTest::RunHostProcess(const PString& bindAddress, uint16 port, int32 clientCount, int32 commandCount, uint64 seed, int32 rejoinGraceSeconds)
{
	int32 players = clientCount + 1;

	HNetParticipant host;
	host.GameMaster = Allocate<PGameMaster>();
	setupNetRules(*host.GameMaster, players);
	buildNetInitialState(*host.GameMaster, players);
	host.Agent = Allocate<PGameplayRandomAgent>(seed + 1);

	HGameplaySessionConfig config;
	config.PlayerName          = "Host";
	config.MaxPlayers          = players;
	config.DesyncDumpDirectory = "";
	PSharedPtr<PNetTcpTransport> transport = Allocate<PNetTcpTransport>();
	transport->SetListenAddress(bindAddress);
	host.Host = PGameplayHostSession::CreateListenServer(transport, port, config);
	if (host.Host == nullptr)
	{
		std::cout << "net.host: cannot listen on " << bindAddress.GetRawString() << ":" << port << std::endl;
		return 2;
	}
	host.Host->BindGameMaster(host.GameMaster);
	host.Host->SetSessionData(PName(ProcessSeedKey), PString::Format("%llu", seed));
	std::cout << "net.host: listening on port " << port << ", waiting for " << clientCount << " clients (connected and ready)" << std::endl;

	// 대기방: 클라 모두가 들어와 준비를 켤 때까지. 먼저 들어왔다 나간 클라의 자리는 비워져 있어야 모두 들어온다.
	std::chrono::steady_clock::time_point last  = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point begin = last;
	while (lobbyReady(*host.Host, clientCount) == false)
	{
		host.Tick(elapsedSince(last));
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(60))
		{
			std::cout << "net.host: timed out waiting for clients (connected " << host.Host->GetConnectedPeerCount() << ", ready " << readySeatCount(*host.Host) << ")" << std::endl;
			return 3;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	host.Host->StartGame(seed);
	begin = std::chrono::steady_clock::now();
	bool bProgressSent = false;
	bool bAbsent       = false;
	int32 rejoins      = 0;
	std::chrono::steady_clock::time_point absentSince = begin;
	while (host.GameMaster->GetState().Sequence < (uint32)commandCount)
	{
		host.Tick(elapsedSince(last));
		driveInput(host);
		if (bProgressSent == false && host.GameMaster->GetState().Sequence >= (uint32)(commandCount / 2))
		{
			host.Host->SetSessionData(PName(ProcessProgressKey), PString::Format("half at %u", host.GameMaster->GetState().Sequence));
			bProgressSent = true;
		}
		if (host.Host->GetConnectedPeerCount() < clientCount)
		{
			// 창을 다시 띄워 같은 토큰으로 돌아오는 클라를 기다린다(ER-026). 그 클라의 차례가 오면 판은 멈춰 기다린다.
			if (rejoinGraceSeconds <= 0)
			{
				std::cout << "net.host: a client left early" << std::endl;
				return 4;
			}
			if (bAbsent == false)
			{
				bAbsent     = true;
				absentSince = std::chrono::steady_clock::now();
				std::cout << "net.host: a client left at seq " << host.GameMaster->GetState().Sequence << ", waiting " << rejoinGraceSeconds << " s for it to rejoin" << std::endl;
			}
			else if (std::chrono::steady_clock::now() - absentSince > std::chrono::seconds(rejoinGraceSeconds))
			{
				std::cout << "net.host: the client did not rejoin" << std::endl;
				return 4;
			}
		}
		else if (bAbsent == true)
		{
			bAbsent = false;
			++rejoins;
			std::cout << "net.host: the client rejoined at seq " << host.GameMaster->GetState().Sequence << std::endl;
		}
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(300))
		{
			std::cout << "net.host: timed out at seq " << host.GameMaster->GetState().Sequence << std::endl;
			return 5;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(200));
	}

	float64 seconds = std::chrono::duration<float64>(std::chrono::steady_clock::now() - begin).count();
	std::cout << PString::Format("net.host: seq=%u checksum=%llu seconds=%.2f data=%llu keys=%d ready=%d rejoins=%d", host.GameMaster->GetState().Sequence, host.GameMaster->Checksum(), seconds,
		sessionDataHash(*host.Host), (int32)host.Host->GetSessionData().size(), readySeatCount(*host.Host), rejoins).GetRawString() << std::endl;

	host.Host->Close("done");
	begin = std::chrono::steady_clock::now();
	while (host.Host->HasPendingSends() == true && std::chrono::steady_clock::now() - begin < std::chrono::seconds(10))
	{
		host.Tick(elapsedSince(last));
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return 0;
}

int32 PGameplayNetSelfTest::RunJoinProcess(const PString& address, uint16 port, int32 playerCount, uint64 agentSeed, const PString& name, const HNetJoinProcessOptions& options)
{
	HNetParticipant client;
	client.GameMaster = Allocate<PGameMaster>();
	setupNetRules(*client.GameMaster, playerCount);
	client.Agent = Allocate<PGameplayRandomAgent>(agentSeed);

	HGameplaySessionConfig config;
	config.PlayerName          = name;
	config.DesyncDumpDirectory = "NetDesync";
	config.ReconnectToken      = options.ReconnectToken;

	std::chrono::steady_clock::time_point last  = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point begin = last;

	// 호스트가 아직 안 떴으면 거부된다. 잠시 뒤 다시 시도한다.
	while (true)
	{
		client.Client = PGameplayClientSession::Create(Allocate<PNetTcpTransport>(), address, port, config);
		client.Client->BindGameMaster(client.GameMaster);
		while (client.Client->GetState() == EGameplaySessionState::Connecting)
		{
			client.Tick(elapsedSince(last));
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		if (client.Client->GetState() != EGameplaySessionState::Closed)
		{
			break;
		}
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(30))
		{
			std::cout << "net.join: cannot join " << address.GetRawString() << ":" << port << " (" << client.Client->GetCloseReason().GetRawString() << ")" << std::endl;
			return 2;
		}
		client.Client->UnbindGameMaster();
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	// 대기방에 들렀다 나가는 클라(판 시작 전 나간 자리 비우기 확인): Welcome 을 받으면 바로 닫는다.
	if (options.bLeaveAfterWelcome == true)
	{
		std::cout << PString::Format("net.join: name=%s slot=%d left after welcome (data keys %d)", name, client.Client->GetLocalSlot(), (int32)client.Client->GetSessionData().size()).GetRawString() << std::endl;
		client.Client->Close("left the lobby");
		begin = std::chrono::steady_clock::now();
		while (client.Client->HasPendingSends() == true && std::chrono::steady_clock::now() - begin < std::chrono::seconds(10))
		{
			client.Tick(elapsedSince(last));
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return 0;
	}

	// 판 시작 전이면 준비를 켠다. 호스트는 모두가 준비하면 시작한다.
	client.Client->SetLocalReady(true);

	begin = std::chrono::steady_clock::now();
	while (client.Client->GetState() != EGameplaySessionState::Closed)
	{
		client.Tick(elapsedSince(last));
		driveInput(client);

		// 판 도중 창을 닫는 클라(ER-026 확인): 이 순번까지 적용하면 세션을 닫고 끝낸다. 같은 토큰으로 새 프로세스가 돌아온다.
		if (options.LeaveAtSeq > 0 && client.Client->GetState() == EGameplaySessionState::Playing && client.Client->GetLastAppliedSeq() >= options.LeaveAtSeq)
		{
			std::cout << PString::Format("net.join: name=%s slot=%d left at seq=%u token=%s", name, client.Client->GetLocalSlot(), client.Client->GetLastAppliedSeq(), client.Client->GetReconnectToken()).GetRawString() << std::endl;
			client.Client->Close("window closed");
			std::chrono::steady_clock::time_point closing = std::chrono::steady_clock::now();
			while (client.Client->HasPendingSends() == true && std::chrono::steady_clock::now() - closing < std::chrono::seconds(10))
			{
				client.Tick(elapsedSince(last));
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			return 0;
		}

		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(300))
		{
			std::cout << "net.join: timed out" << std::endl;
			return 3;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(200));
	}

	std::cout << PString::Format("net.join: name=%s slot=%d seq=%u checksum=%llu desyncs=%d sent=%d data=%llu keys=%d ready=%d reason=%s",
		name, client.Client->GetLocalSlot(), client.Client->GetLastAppliedSeq(), client.GameMaster->Checksum(),
		client.Client->GetDesyncCount(), client.Submitted, sessionDataHash(*client.Client), (int32)client.Client->GetSessionData().size(),
		readySeatCount(*client.Client), client.Client->GetCloseReason()).GetRawString() << std::endl;

	bool bStarted = client.GameMaster->IsStarted();
	return (bStarted == true && client.Client->GetDesyncCount() == 0) ? 0 : 1;
}

int32 PGameplayNetSelfTest::RunWorldHostProcess(const PString& bindAddress, uint16 port, int32 clientCount, int32 commandCount, uint64 seed)
{
	if (JGGameInstance::HasInstance() == false)
	{
		std::cout << "net.host: no game instance" << std::endl;
		return 2;
	}
	JGGameInstance& gameInstance = JGGameInstance::Get();
	int32 players = clientCount + 1;

	HGameplaySessionConfig config;
	config.PlayerName          = "Host";
	config.MaxPlayers          = players;
	config.DesyncDumpDirectory = "";
	PSharedPtr<PNetTcpTransport> transport = Allocate<PNetTcpTransport>();
	transport->SetListenAddress(bindAddress);
	PSharedPtr<PGameplayHostSession> host = gameInstance.HostSession(port, config, transport);
	if (host == nullptr)
	{
		std::cout << "net.host: cannot listen on " << bindAddress.GetRawString() << ":" << port << std::endl;
		return 2;
	}

	HNetWorldParticipant local;
	local.PlayerCount = players;
	local.Seed        = seed;
	local.Agent       = Allocate<PGameplayRandomAgent>(seed + 1);
	HDelegateHandle loaded = gameInstance.OnWorldLoaded.AddLambda([&local](PSharedPtr<PWorld> world)
	{
		enterNetWorld(world, local);
	});
	host->SetSessionData(PName(ProcessSeedKey), PString::Format("%llu", seed));
	std::cout << "net.host: world mode, listening on port " << port << ", waiting for " << clientCount << " clients (connected and ready)" << std::endl;

	std::chrono::steady_clock::time_point last  = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point begin = last;
	int32 exitCode = 0;
	while (lobbyReady(*host, clientCount) == false)
	{
		gameInstance.Tick(elapsedSince(last));
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(60))
		{
			std::cout << "net.host: timed out waiting for clients (connected " << host->GetConnectedPeerCount() << ", ready " << readySeatCount(*host) << ")" << std::endl;
			exitCode = 3;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// 로비 → 월드 1 → 절반 진행 → 월드 2 로 이동(새 게임) → 나머지 진행. 클라는 이동 알림을 따라온다.
	uint32 firstHalf  = (uint32)(commandCount / 2 > 0 ? commandCount / 2 : 1);
	uint32 secondHalf = (uint32)(commandCount - (int32)firstHalf > 0 ? commandCount - (int32)firstHalf : 1);
	const char* worlds[2]  = { "NetWorld1", "NetWorld2" };
	uint32      targets[2] = { firstHalf, secondHalf };
	begin = std::chrono::steady_clock::now();
	for (int32 index = 0; index < 2 && exitCode == 0; ++index)
	{
		if (index == 1)
		{
			// 세션 공유 데이터는 월드 이동에도 남는다 — 이동 전에 바꾼 값을 끝에 모두가 같게 가져야 한다.
			host->SetSessionData(PName(ProcessProgressKey), PString::Format("half at %u", local.GameMaster != nullptr ? local.GameMaster->GetState().Sequence : 0));
		}
		gameInstance.LoadWorld(PName(worlds[index]));
		while (exitCode == 0 && local.GameMaster != nullptr && local.GameMaster->GetState().Sequence < targets[index])
		{
			gameInstance.Tick(elapsedSince(last));
			driveController(local);
			if (host->GetConnectedPeerCount() < clientCount)
			{
				std::cout << "net.host: a client left early" << std::endl;
				exitCode = 4;
			}
			if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(300))
			{
				std::cout << "net.host: timed out at seq " << local.GameMaster->GetState().Sequence << std::endl;
				exitCode = 5;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(200));
		}
	}

	if (exitCode == 0 && local.GameMaster != nullptr)
	{
		float64 seconds = std::chrono::duration<float64>(std::chrono::steady_clock::now() - begin).count();
		std::cout << PString::Format("net.host: world=%s worlds=%d seq=%u checksum=%llu seconds=%.2f executed=%d data=%llu keys=%d ready=%d",
			gameInstance.GetWorld()->GetName().ToString(), local.WorldsLoaded, local.GameMaster->GetState().Sequence, local.GameMaster->Checksum(), seconds, local.Executed,
			sessionDataHash(*host), (int32)host->GetSessionData().size(), readySeatCount(*host)).GetRawString() << std::endl;
	}

	gameInstance.OnWorldLoaded.Remove(loaded);
	gameInstance.UnloadWorld();
	gameInstance.LeaveSession();
	return exitCode;
}

int32 PGameplayNetSelfTest::RunWorldJoinProcess(const PString& address, uint16 port, int32 playerCount, uint64 agentSeed, const PString& name)
{
	if (JGGameInstance::HasInstance() == false)
	{
		std::cout << "net.join: no game instance" << std::endl;
		return 2;
	}
	JGGameInstance& gameInstance = JGGameInstance::Get();

	HNetWorldParticipant local;
	local.PlayerCount = playerCount;
	local.Agent       = Allocate<PGameplayRandomAgent>(agentSeed);
	HDelegateHandle loaded = gameInstance.OnWorldLoaded.AddLambda([&local](PSharedPtr<PWorld> world)
	{
		enterNetWorld(world, local);
	});

	HGameplaySessionConfig config;
	config.PlayerName          = name;
	config.DesyncDumpDirectory = "NetDesync";

	std::chrono::steady_clock::time_point last  = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point begin = last;
	int32 exitCode = 0;

	// 호스트가 아직 안 떴으면 거부된다. 잠시 뒤 다시 시도한다.
	PSharedPtr<PGameplayClientSession> client;
	while (true)
	{
		client = gameInstance.JoinSession(address, port, config);
		while (client->GetState() == EGameplaySessionState::Connecting)
		{
			gameInstance.Tick(elapsedSince(last));
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		if (client->GetState() != EGameplaySessionState::Closed)
		{
			break;
		}
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(30))
		{
			std::cout << "net.join: cannot join " << address.GetRawString() << ":" << port << " (" << client->GetCloseReason().GetRawString() << ")" << std::endl;
			exitCode = 2;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	// 판 시작 전이면 준비를 켠다. 호스트는 모두가 준비하면 첫 월드를 로드한다.
	if (exitCode == 0)
	{
		client->SetLocalReady(true);
	}

	begin = std::chrono::steady_clock::now();
	while (exitCode == 0 && client->GetState() != EGameplaySessionState::Closed)
	{
		gameInstance.Tick(elapsedSince(last));
		driveController(local);
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(300))
		{
			std::cout << "net.join: timed out" << std::endl;
			exitCode = 3;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(200));
	}

	if (exitCode == 0)
	{
		PSharedPtr<PWorld> world = gameInstance.GetWorld();
		std::cout << PString::Format("net.join: name=%s slot=%d world=%s worlds=%d seq=%u checksum=%llu desyncs=%d sent=%d data=%llu keys=%d ready=%d reason=%s",
			name, client->GetLocalSlot(), world != nullptr ? world->GetName().ToString() : PString("none"), local.WorldsLoaded,
			local.GameMaster != nullptr ? local.GameMaster->GetState().Sequence : 0, local.GameMaster != nullptr ? local.GameMaster->Checksum() : 0,
			client->GetDesyncCount(), local.Sent, sessionDataHash(*client), (int32)client->GetSessionData().size(), readySeatCount(*client), client->GetCloseReason()).GetRawString() << std::endl;

		bool bStarted = local.GameMaster != nullptr && local.GameMaster->IsStarted() == true;
		exitCode = (bStarted == true && client->GetDesyncCount() == 0 && local.WorldsLoaded == 2) ? 0 : 1;
	}

	gameInstance.OnWorldLoaded.Remove(loaded);
	gameInstance.UnloadWorld();
	gameInstance.LeaveSession();
	return exitCode;
}

