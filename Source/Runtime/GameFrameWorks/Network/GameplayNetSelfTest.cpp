#include "PCH/PCH.h"
#include "Network/GameplayNetSelfTest.h"
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

	HNetParticipant makeLoopbackClient(PSharedPtr<PNetLoopbackHub> hub, uint16 port, int32 playerCount, const PString& name, uint64 agentSeed, const PString& dumpDirectory)
	{
		HNetParticipant participant;
		participant.GameMaster = Allocate<PGameMaster>();
		setupNetRules(*participant.GameMaster, playerCount);

		HGameplaySessionConfig config;
		config.PlayerName          = name;
		config.DesyncDumpDirectory = dumpDirectory;

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
			const uint16 port = 47790;
			PSharedPtr<PNetTcpTransport> host   = Allocate<PNetTcpTransport>();
			PSharedPtr<PNetTcpTransport> client = Allocate<PNetTcpTransport>();
			host->SetListenAddress("127.0.0.1");
			check(host->Listen(port) == true, PString::Format("tcp listen on 127.0.0.1:%u", (uint32)port));

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

			// 아무도 듣지 않는 포트.
			PSharedPtr<PNetTcpTransport> nowhere = Allocate<PNetTcpTransport>();
			HNetPeerId nowherePeer = nowhere->Connect("127.0.0.1", 47791);
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
			check(false, "world tests need the game instance (connect GameFrameWorks)");
			return;
		}

		// 게임이 월드를 띄운 프로세스(프로젝트 모드 에디터)에서는 건드리지 않는다.
		JGGameInstance& gameInstance = JGGameInstance::Get();
		PSharedPtr<PGameplaySession> session = gameInstance.GetSession();
		if (gameInstance.HasWorld() == true || session == nullptr || session->GetMode() != EGameplayNetMode::Standalone)
		{
			info("world section skipped: the game instance already has a world or a network session");
			return;
		}

		runWorldHostTests(check, gameInstance);
		runWorldClientTests(check, gameInstance);

		PSharedPtr<PGameplaySession> after = gameInstance.GetSession();
		check(gameInstance.HasWorld() == false && after != nullptr && after->GetMode() == EGameplayNetMode::Standalone, "game instance left standalone with no world");
	}
}

int32 PGameplayNetSelfTest::Run(const PString& which)
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
	}
	if (bAll == true || which == PString("world"))
	{
		runWorldTests(check);
	}

	std::cout << "== Network self test: " << check.Passed << " passed, " << check.Failures << " failed ==" << std::endl;
	JG_LOG(NetSelfTest, ELogLevel::Info, "Network self test: %d passed, %d failed", check.Passed, check.Failures);
	return check.Failures;
}

int32 PGameplayNetSelfTest::RunHostProcess(const PString& bindAddress, uint16 port, int32 clientCount, int32 commandCount, uint64 seed)
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
	std::cout << "net.host: listening on port " << port << ", waiting for " << clientCount << " clients" << std::endl;

	std::chrono::steady_clock::time_point last  = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point begin = last;
	while (host.Host->GetConnectedPeerCount() < clientCount)
	{
		host.Tick(elapsedSince(last));
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(60))
		{
			std::cout << "net.host: timed out waiting for clients (" << host.Host->GetConnectedPeerCount() << ")" << std::endl;
			return 3;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	host.Host->StartGame(seed);
	begin = std::chrono::steady_clock::now();
	while (host.GameMaster->GetState().Sequence < (uint32)commandCount)
	{
		host.Tick(elapsedSince(last));
		driveInput(host);
		if (host.Host->GetConnectedPeerCount() < clientCount)
		{
			std::cout << "net.host: a client left early" << std::endl;
			return 4;
		}
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(300))
		{
			std::cout << "net.host: timed out at seq " << host.GameMaster->GetState().Sequence << std::endl;
			return 5;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(200));
	}

	float64 seconds = std::chrono::duration<float64>(std::chrono::steady_clock::now() - begin).count();
	std::cout << PString::Format("net.host: seq=%u checksum=%llu seconds=%.2f", host.GameMaster->GetState().Sequence, host.GameMaster->Checksum(), seconds).GetRawString() << std::endl;

	host.Host->Close("done");
	begin = std::chrono::steady_clock::now();
	while (host.Host->HasPendingSends() == true && std::chrono::steady_clock::now() - begin < std::chrono::seconds(10))
	{
		host.Tick(elapsedSince(last));
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return 0;
}

int32 PGameplayNetSelfTest::RunJoinProcess(const PString& address, uint16 port, int32 playerCount, uint64 agentSeed, const PString& name)
{
	HNetParticipant client;
	client.GameMaster = Allocate<PGameMaster>();
	setupNetRules(*client.GameMaster, playerCount);
	client.Agent = Allocate<PGameplayRandomAgent>(agentSeed);

	HGameplaySessionConfig config;
	config.PlayerName          = name;
	config.DesyncDumpDirectory = "NetDesync";

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

	begin = std::chrono::steady_clock::now();
	while (client.Client->GetState() != EGameplaySessionState::Closed)
	{
		client.Tick(elapsedSince(last));
		driveInput(client);
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(300))
		{
			std::cout << "net.join: timed out" << std::endl;
			return 3;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(200));
	}

	std::cout << PString::Format("net.join: name=%s slot=%d seq=%u checksum=%llu desyncs=%d sent=%d reason=%s",
		name, client.Client->GetLocalSlot(), client.Client->GetLastAppliedSeq(), client.GameMaster->Checksum(),
		client.Client->GetDesyncCount(), client.Submitted, client.Client->GetCloseReason()).GetRawString() << std::endl;

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
	std::cout << "net.host: world mode, listening on port " << port << ", waiting for " << clientCount << " clients" << std::endl;

	std::chrono::steady_clock::time_point last  = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point begin = last;
	int32 exitCode = 0;
	while (host->GetConnectedPeerCount() < clientCount)
	{
		gameInstance.Tick(elapsedSince(last));
		if (std::chrono::steady_clock::now() - begin > std::chrono::seconds(60))
		{
			std::cout << "net.host: timed out waiting for clients (" << host->GetConnectedPeerCount() << ")" << std::endl;
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
		std::cout << PString::Format("net.host: world=%s worlds=%d seq=%u checksum=%llu seconds=%.2f executed=%d",
			gameInstance.GetWorld()->GetName().ToString(), local.WorldsLoaded, local.GameMaster->GetState().Sequence, local.GameMaster->Checksum(), seconds, local.Executed).GetRawString() << std::endl;
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
		std::cout << PString::Format("net.join: name=%s slot=%d world=%s worlds=%d seq=%u checksum=%llu desyncs=%d sent=%d reason=%s",
			name, client->GetLocalSlot(), world != nullptr ? world->GetName().ToString() : PString("none"), local.WorldsLoaded,
			local.GameMaster != nullptr ? local.GameMaster->GetState().Sequence : 0, local.GameMaster != nullptr ? local.GameMaster->Checksum() : 0,
			client->GetDesyncCount(), local.Sent, client->GetCloseReason()).GetRawString() << std::endl;

		bool bStarted = local.GameMaster != nullptr && local.GameMaster->IsStarted() == true;
		exitCode = (bStarted == true && client->GetDesyncCount() == 0 && local.WorldsLoaded == 2) ? 0 : 1;
	}

	gameInstance.OnWorldLoaded.Remove(loaded);
	gameInstance.UnloadWorld();
	gameInstance.LeaveSession();
	return exitCode;
}

namespace
{
	// net.test 는 GameFrameWorks 모듈에 둔다 (모듈의 검사 명령은 그 모듈에 선언한다). net.host · net.join 은 JGConsole(NetCommands.cpp).
	bool executeNetTest(const HConsoleCommandArgs& args)
	{
		PString which = "all";
		if (args.GetPositionalCount() > 0)
		{
			which = args.GetPositional(0);
		}
		int32 failures = PGameplayNetSelfTest::Run(which);
		std::cout << "net.test: " << (failures == 0 ? "OK" : "FAILED") << " (" << failures << " failures)" << std::endl;
		return failures == 0;
	}

	HAutoConsoleCommand NetTestCommand(
		"net.test",
		"net.test [transport|session|recovery|world|all]",
		"Run the listen-server self tests (loopback, same-process TCP, game instance world)",
		&executeNetTest);
}
