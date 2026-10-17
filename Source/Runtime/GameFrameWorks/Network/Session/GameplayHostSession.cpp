#include "PCH/PCH.h"
#include "Network/Session/GameplayHostSession.h"

namespace
{
	const char* LocalToken = "local";

	void setReason(PString* outReason, const PString& reason)
	{
		if (outReason != nullptr)
		{
			*outReason = reason;
		}
	}
}

PGameplayHostSession::~PGameplayHostSession()
{
	if (_transport != nullptr)
	{
		_transport->Shutdown();
	}
}

PSharedPtr<PGameplayHostSession> PGameplayHostSession::CreateStandalone(const HGameplaySessionConfig& config)
{
	PSharedPtr<PGameplayHostSession> session = Allocate<PGameplayHostSession>();
	session->_mode   = EGameplayNetMode::Standalone;
	session->_state  = EGameplaySessionState::Ready;
	session->_config = config;

	HGameplayPlayerSlot local;
	local.Slot            = 0;
	local.bAllControllers = true;
	local.bConnected      = true;
	local.Name            = config.PlayerName;
	local.Token           = LocalToken;
	session->_slots.push_back(local);
	session->_localSlot = 0;
	return session;
}

PSharedPtr<PGameplayHostSession> PGameplayHostSession::CreateListenServer(PSharedPtr<INetTransport> transport, uint16 port, const HGameplaySessionConfig& config)
{
	if (transport == nullptr || transport->Listen(port) == false)
	{
		JG_LOG(Network, ELogLevel::Error, "PGameplayHostSession: cannot listen on port %u", (uint32)port);
		return nullptr;
	}

	PSharedPtr<PGameplayHostSession> session = Allocate<PGameplayHostSession>();
	session->_mode      = EGameplayNetMode::ListenServer;
	session->_state     = EGameplaySessionState::Ready;
	session->_config    = config;
	session->_transport = transport;

	int32 slotCount = config.MaxPlayers < 1 ? 1 : config.MaxPlayers;
	for (int32 i = 0; i < slotCount; ++i)
	{
		HGameplayPlayerSlot slot;
		slot.Slot = i;
		slot.Controllers.push_back(i);
		session->_slots.push_back(slot);
	}

	if (config.bLocalPlayer == true)
	{
		HGameplayPlayerSlot& local = session->_slots[0];
		local.bConnected = true;
		local.Name       = config.PlayerName;
		local.Token      = LocalToken;
		session->_localSlot = 0;
	}

	JG_LOG(Network, ELogLevel::Info, "PGameplayHostSession: listen server on port %u, %d slots", (uint32)port, slotCount);
	return session;
}

EGameplaySessionSubmit PGameplayHostSession::SubmitLocal(const HGameplayCommand& command, PString* outReason)
{
	adoptStartedGameMaster();
	if (_state != EGameplaySessionState::Playing)
	{
		setReason(outReason, "game not started");
		return EGameplaySessionSubmit::Rejected;
	}

	const HGameplayPlayerSlot* local = FindSlot(_localSlot);
	if (local == nullptr)
	{
		setReason(outReason, "no local player");
		return EGameplaySessionSubmit::Rejected;
	}
	if (slotControls(*local, command.Actor) == false)
	{
		setReason(outReason, "not your actor");
		return EGameplaySessionSubmit::Rejected;
	}

	return executeAuthoritative(command, _localSlot, 0, outReason);
}

bool PGameplayHostSession::StartGame(uint64 seed)
{
	if (_gameMaster == nullptr || _state == EGameplaySessionState::Closed)
	{
		JG_LOG(Network, ELogLevel::Error, "PGameplayHostSession::StartGame: no gameMaster bound");
		return false;
	}
	if (_gameMaster->Start(seed) == false)
	{
		return false;
	}
	_state          = EGameplaySessionState::Playing;
	_bGameAnnounced = true;

	if (_mode == EGameplayNetMode::ListenServer)
	{
		HGameplayNetStartGame start;
		start.Seed         = seed;
		start.Fingerprint  = _gameMaster->RulesFingerprint();
		start.InitialState = _gameMaster->GetInitialState().ToJsonString();

		HList<uint8> bytes;
		HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::StartGame, start, bytes);
		broadcast(bytes);
	}

	// 판이 시작되면 준비 표시를 지운다. 판이 끝나 대기방으로 돌아오면 다시 정한다.
	if (clearReady() == true)
	{
		broadcastSlots();
	}
	return true;
}

void PGameplayHostSession::Tick(float32 deltaSeconds)
{
	_time += deltaSeconds;
	adoptStartedGameMaster();

	if (_transport != nullptr)
	{
		// 닫은 뒤에도 Poll 은 돌려야 남은 데이터가 나가고 연결이 정리된다.
		HList<HNetEvent> events;
		_transport->Poll(events);

		if (_state != EGameplaySessionState::Closed)
		{
			for (const HNetEvent& event : events)
			{
				handleEvent(event);
			}
			updateTimers(deltaSeconds);
		}
	}

	if (_state == EGameplaySessionState::Playing)
	{
		driveAgents();
	}
}

void PGameplayHostSession::Close(const PString& reason)
{
	if (_state == EGameplaySessionState::Closed)
	{
		return;
	}

	if (_transport != nullptr)
	{
		for (HGameplayPlayerSlot& slot : _slots)
		{
			if (slot.bConnected == true && slot.Slot != _localSlot && slot.Peer != NetPeerNone)
			{
				_transport->Close(slot.Peer);
				slot.bConnected = false;
				slot.Peer       = NetPeerNone;
			}
		}
		for (const HPendingPeer& pending : _handshaking)
		{
			_transport->Close(pending.Peer);
		}
		_handshaking.clear();
	}
	setClosed(reason);
}

bool PGameplayHostSession::HasPendingSends() const
{
	return _transport != nullptr && _transport->HasPendingSends();
}

bool PGameplayHostSession::SetLocalReady(bool bReady)
{
	HGameplayPlayerSlot* local = findSlotMutable(_localSlot);
	if (_state != EGameplaySessionState::Ready || local == nullptr)
	{
		return false;
	}
	if (local->bReady != bReady)
	{
		local->bReady = bReady;
		broadcastSlots();
	}
	return true;
}

bool PGameplayHostSession::SetSessionData(const PName& key, const PString& value)
{
	if (key == NAME_NONE)
	{
		JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession::SetSessionData: empty key");
		return false;
	}

	// 크기 = 키 + 값 바이트 합. 같은 키는 새 값으로 바꿔 센다.
	const PString keyText = key.ToString();
	int64 total = (int64)keyText.GetRawString().size() + (int64)value.GetRawString().size();
	for (const HGameplaySessionDataEntry& entry : _sessionData)
	{
		if (entry.Key != key)
		{
			total += (int64)entry.Key.ToString().GetRawString().size() + (int64)entry.Value.GetRawString().size();
		}
	}
	if (total > (int64)_config.MaxSessionDataBytes)
	{
		JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession::SetSessionData: %s would make the session data %lld bytes (limit %d)", keyText, total, _config.MaxSessionDataBytes);
		return false;
	}

	// 참가자에게 먼저 보내고 알린다. 알림을 받은 화면 코드가 다른 키를 바꿔도 보낸 순서가 표를 바꾼 순서와 같다.
	if (putSessionData(key, value) == true)
	{
		sendSessionData(key, value, false);
		OnSessionDataChanged.BroadCast(key);
	}
	return true;
}

bool PGameplayHostSession::RemoveSessionData(const PName& key)
{
	if (dropSessionData(key) == false)
	{
		return false;
	}
	sendSessionData(key, PString(), true);
	OnSessionDataChanged.BroadCast(key);
	return true;
}

void PGameplayHostSession::ClearSessionData()
{
	HList<PName> keys;
	for (const HGameplaySessionDataEntry& entry : _sessionData)
	{
		keys.push_back(entry.Key);
	}
	for (const PName& key : keys)
	{
		RemoveSessionData(key);
	}
}

void PGameplayHostSession::SetSlotControllers(int32 slot, const HList<int32>& controllers)
{
	HGameplayPlayerSlot* entry = findSlotMutable(slot);
	if (entry == nullptr)
	{
		return;
	}
	entry->Controllers = controllers;
	broadcastSlots();
}

void PGameplayHostSession::NotifyTravel(const PName& world)
{
	_world = world;
	if (_mode != EGameplayNetMode::ListenServer)
	{
		return;
	}

	HGameplayNetTravel travel;
	travel.World = world;
	HList<uint8> bytes;
	HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::Travel, travel, bytes);
	broadcast(bytes);
}

int32 PGameplayHostSession::GetConnectedPeerCount() const
{
	int32 count = 0;
	for (const HGameplayPlayerSlot& slot : _slots)
	{
		if (slot.bConnected == true && slot.Slot != _localSlot)
		{
			++count;
		}
	}
	return count;
}

void PGameplayHostSession::SetMaxAgentStepsPerTick(int32 steps)
{
	_maxAgentStepsPerTick = steps < 0 ? 0 : steps;
}

void PGameplayHostSession::onBound()
{
	_bGameAnnounced = false;
	adoptStartedGameMaster();
}

void PGameplayHostSession::onUnbound()
{
	// 월드가 바뀌는 중이다. 다음 GameMaster 가 붙어 시작할 때까지 명령을 받지 않고, 그사이 입장한 클라에게 문서를 보내지 않는다.
	if (_state == EGameplaySessionState::Playing)
	{
		_state = EGameplaySessionState::Ready;
	}
	_bGameAnnounced = false;

	// 대기방으로 돌아왔다: 판 중에 끊긴 채 남겨 둔 자리(재접속용)를 비운다.
	if (_state == EGameplaySessionState::Ready && releaseDisconnectedSlots() == true)
	{
		broadcastSlots();
	}
}

// 게임이 세션을 거치지 않고 PGameMaster::Start 를 불렀으면(붙기 전에 시작 · 세션 이전 코드) 그 게임을 이어받는다.
// 이미 들어와 있는 클라에게는 지금 상태를 문서로 보낸다 (시작 뒤 명령이 진행됐을 수 있어 StartGame 으로는 부족하다).
void PGameplayHostSession::adoptStartedGameMaster()
{
	if (_bGameAnnounced == true || _gameMaster == nullptr || _gameMaster->IsStarted() == false || _state == EGameplaySessionState::Closed)
	{
		return;
	}

	_bGameAnnounced = true;
	_state          = EGameplaySessionState::Playing;
	JG_LOG(Network, ELogLevel::Info, "PGameplayHostSession: adopted a game started outside the session (seq %u)", _gameMaster->GetState().Sequence);

	if (_mode == EGameplayNetMode::ListenServer)
	{
		for (const HGameplayPlayerSlot& slot : _slots)
		{
			if (slot.bConnected == true && slot.Slot != _localSlot && slot.Peer != NetPeerNone)
			{
				sendDocument(slot.Peer);
			}
		}
	}

	// 판이 시작됐다: 준비 표시를 지운다(StartGame 과 같다).
	if (clearReady() == true)
	{
		broadcastSlots();
	}
}

// ---- 권한 실행 ------------------------------------------------------------------

EGameplaySessionSubmit PGameplayHostSession::executeAuthoritative(const HGameplayCommand& command, int32 originSlot, uint32 clientSeq, PString* outReason)
{
	if (_gameMaster == nullptr || _gameMaster->IsStarted() == false)
	{
		setReason(outReason, "game not started");
		return EGameplaySessionSubmit::Rejected;
	}

	HList<HGameplayEvent> events;
	PString reason;
	EGameplaySubmitResult result = _gameMaster->Submit(command, events, &reason);
	if (result == EGameplaySubmitResult::Rejected)
	{
		setReason(outReason, reason);
		return EGameplaySessionSubmit::Rejected;
	}

	if (_mode == EGameplayNetMode::ListenServer)
	{
		HGameplayNetCommandAccepted accepted;
		accepted.Seq       = _gameMaster->GetState().Sequence;
		accepted.Slot      = originSlot;
		accepted.ClientSeq = clientSeq;
		accepted.Checksum  = _gameMaster->Checksum();
		accepted.Command   = command;

		HList<uint8> bytes;
		HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::CommandAccepted, accepted, bytes);
		broadcast(bytes);
	}

	if (result == EGameplaySubmitResult::PendingChoice)
	{
		return EGameplaySessionSubmit::PendingChoice;
	}
	return EGameplaySessionSubmit::Executed;
}

void PGameplayHostSession::driveAgents()
{
	for (int32 step = 0; step < _maxAgentStepsPerTick; ++step)
	{
		if (_gameMaster == nullptr || _gameMaster->IsStarted() == false)
		{
			return;
		}
		if (_gameMaster->GetState().Turn.IsFinished() == true)
		{
			return;
		}

		HGameplayCommand command;
		if (PGameplayAgentRunner::Choose(*_gameMaster, command) == false)
		{
			return;
		}

		PString reason;
		if (executeAuthoritative(command, INDEX_NONE, 0, &reason) == EGameplaySessionSubmit::Rejected)
		{
			JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: agent command %s rejected: %s", command.ToString(), reason);
			return;
		}
	}
}

// ---- 메시지 -------------------------------------------------------------------

void PGameplayHostSession::handleEvent(const HNetEvent& event)
{
	switch (event.Type)
	{
	case ENetEventType::Connected:
	{
		HPendingPeer pending;
		pending.Peer        = event.Peer;
		pending.ConnectedAt = _time;
		_handshaking.push_back(pending);
		break;
	}
	case ENetEventType::Disconnected:
	{
		int32 pendingIndex = findHandshaking(event.Peer);
		if (pendingIndex != INDEX_NONE)
		{
			_handshaking.erase(_handshaking.begin() + pendingIndex);
			break;
		}
		HGameplayPlayerSlot* slot = findSlotByPeer(event.Peer);
		if (slot != nullptr)
		{
			markDisconnected(*slot);
		}
		break;
	}
	case ENetEventType::Message:
	{
		EGameplayNetMessage type = EGameplayNetMessage::None;
		HRawString text;
		if (HGameplayNetCodec::Decode(event.Data, &type, &text) == false)
		{
			JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: undecodable message from peer %u", event.Peer);
			break;
		}

		HGameplayPlayerSlot* slot = findSlotByPeer(event.Peer);
		if (slot == nullptr)
		{
			if (type == EGameplayNetMessage::Hello && findHandshaking(event.Peer) != INDEX_NONE)
			{
				handleHello(event.Peer, text);
			}
			break;
		}

		slot->LastHeard = _time;
		handleSlotMessage(*slot, type, text);
		break;
	}
	}
}

void PGameplayHostSession::handleHello(HNetPeerId peer, const HRawString& text)
{
	HGameplayNetHello hello;
	if (HGameplayNetCodec::DecodeMessage(text, &hello) == false)
	{
		refuse(peer, "bad hello");
		return;
	}
	if (hello.Protocol != HGameplayNetProtocol::Version)
	{
		refuse(peer, PString::Format("protocol %u != %u", hello.Protocol, HGameplayNetProtocol::Version));
		return;
	}
	if (hello.Schema != HGameplayState::SchemaVersion)
	{
		refuse(peer, PString::Format("state schema %u != %u", hello.Schema, HGameplayState::SchemaVersion));
		return;
	}
	if (hello.Token.Empty() == true)
	{
		refuse(peer, "no reconnect token");
		return;
	}

	HGameplayPlayerSlot* slot = reserveSlot(hello.Token);
	if (slot == nullptr)
	{
		refuse(peer, "session full");
		return;
	}

	int32 pendingIndex = findHandshaking(peer);
	if (pendingIndex != INDEX_NONE)
	{
		_handshaking.erase(_handshaking.begin() + pendingIndex);
	}

	slot->Peer       = peer;
	slot->bConnected = true;
	slot->Name       = hello.Name;
	slot->Token      = hello.Token;
	slot->LastHeard  = _time;

	HGameplayNetWelcome welcome;
	welcome.Slot        = slot->Slot;
	welcome.Fingerprint = _gameMaster != nullptr ? _gameMaster->RulesFingerprint() : 0;
	welcome.World       = _world;
	welcome.bStarted    = _state == EGameplaySessionState::Playing;
	welcome.Slots       = _slots;
	welcome.SessionData = _sessionData;

	HList<uint8> bytes;
	HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::Welcome, welcome, bytes);
	sendTo(peer, bytes);

	JG_LOG(Network, ELogLevel::Info, "PGameplayHostSession: slot %d joined (%s)", slot->Slot, slot->Name);
	int32 joinedSlot = slot->Slot;
	broadcastSlots();

	if (_state == EGameplaySessionState::Playing)
	{
		// 진행 중 입장 · 재접속: 지금 상태를 문서로 보낸다. 이후 승인은 그 뒤 순서로 도착한다.
		sendDocument(peer);
		JG_LOG(Network, ELogLevel::Info, "PGameplayHostSession: sent document to slot %d at seq %u", joinedSlot, _gameMaster->GetState().Sequence);
	}
}

void PGameplayHostSession::handleSlotMessage(HGameplayPlayerSlot& slot, EGameplayNetMessage type, const HRawString& text)
{
	switch (type)
	{
	case EGameplayNetMessage::CommandRequest:
	{
		HGameplayNetCommandRequest request;
		if (HGameplayNetCodec::DecodeMessage(text, &request) == false)
		{
			JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: bad CommandRequest from slot %d", slot.Slot);
			break;
		}

		PString reason;
		EGameplaySessionSubmit result = EGameplaySessionSubmit::Rejected;
		if (_state != EGameplaySessionState::Playing)
		{
			reason = "game not started";
		}
		else if (slotControls(slot, request.Command.Actor) == false)
		{
			reason = "not your actor";
		}
		else
		{
			result = executeAuthoritative(request.Command, slot.Slot, request.ClientSeq, &reason);
		}

		if (result == EGameplaySessionSubmit::Rejected)
		{
			HGameplayNetCommandRejected rejected;
			rejected.ClientSeq = request.ClientSeq;
			rejected.Reason    = reason;

			HList<uint8> bytes;
			HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::CommandRejected, rejected, bytes);
			sendTo(slot.Peer, bytes);
		}
		break;
	}
	case EGameplayNetMessage::ResyncRequest:
	{
		JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: slot %d requested resync", slot.Slot);
		sendDocument(slot.Peer);
		break;
	}
	case EGameplayNetMessage::SetReady:
	{
		HGameplayNetSetReady request;
		if (HGameplayNetCodec::DecodeMessage(text, &request) == false)
		{
			JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: bad SetReady from slot %d", slot.Slot);
			break;
		}
		// 준비 표시는 대기방(판 시작 전)에서만 받는다. 판 중에 온 것은 버린다.
		if (_state != EGameplaySessionState::Ready || slot.bReady == request.bReady)
		{
			break;
		}
		slot.bReady = request.bReady;
		broadcastSlots();
		break;
	}
	case EGameplayNetMessage::Ping:
	{
		break;
	}
	default:
	{
		JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: unexpected %s from slot %d", HGameplayNetCodec::ToString(type), slot.Slot);
		break;
	}
	}
}

void PGameplayHostSession::refuse(HNetPeerId peer, const PString& reason)
{
	JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: refused peer %u: %s", peer, reason);

	HGameplayNetRefuse refused;
	refused.Reason = reason;
	HList<uint8> bytes;
	HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::Refuse, refused, bytes);
	sendTo(peer, bytes);

	if (_transport != nullptr)
	{
		_transport->Close(peer);
	}

	int32 pendingIndex = findHandshaking(peer);
	if (pendingIndex != INDEX_NONE)
	{
		_handshaking.erase(_handshaking.begin() + pendingIndex);
	}
}

void PGameplayHostSession::markDisconnected(HGameplayPlayerSlot& slot)
{
	if (slot.bConnected == false)
	{
		return;
	}
	slot.bConnected = false;
	slot.Peer       = NetPeerNone;
	slot.bReady     = false;

	// 판 시작 전(대기방)에 나간 사람의 자리는 바로 비운다 — 목록에서 사라지고 새 사람이 그 자리에 들어온다.
	// 판 중에 나간 사람의 자리는 같은 토큰으로 돌아오도록 남긴다(대기방으로 돌아오면 onUnbound 가 비운다).
	const bool bRelease = _state != EGameplaySessionState::Playing;
	JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: slot %d disconnected%s", slot.Slot, bRelease == true ? " (released: game not started)" : "");
	if (bRelease == true)
	{
		releaseSlot(slot);
	}
	int32 slotIndex = slot.Slot;
	broadcastSlots();
	OnPeerDisconnected.BroadCast(slotIndex);
}

// 빈 자리로 되돌린다: 토큰 · 이름 · 준비를 지운다. 번호와 조작 주체(게임이 SetSlotControllers 로 정한 배정)는 둔다.
void PGameplayHostSession::releaseSlot(HGameplayPlayerSlot& slot)
{
	slot.bConnected = false;
	slot.Peer       = NetPeerNone;
	slot.Token      = PString();
	slot.Name       = PString();
	slot.bReady     = false;
	slot.LastHeard  = 0.0f;
}

bool PGameplayHostSession::releaseDisconnectedSlots()
{
	bool bReleased = false;
	for (HGameplayPlayerSlot& slot : _slots)
	{
		if (slot.Slot == _localSlot || slot.bConnected == true || slot.IsFree() == true)
		{
			continue;
		}
		JG_LOG(Network, ELogLevel::Info, "PGameplayHostSession: released slot %d (%s, left during the game)", slot.Slot, slot.Name);
		releaseSlot(slot);
		bReleased = true;
	}
	return bReleased;
}

bool PGameplayHostSession::clearReady()
{
	bool bCleared = false;
	for (HGameplayPlayerSlot& slot : _slots)
	{
		if (slot.bReady == true)
		{
			slot.bReady = false;
			bCleared    = true;
		}
	}
	return bCleared;
}

void PGameplayHostSession::sendSessionData(const PName& key, const PString& value, bool bRemoved)
{
	if (_mode != EGameplayNetMode::ListenServer)
	{
		return;
	}

	HGameplayNetSessionData change;
	change.Key      = key;
	change.Value    = value;
	change.bRemoved = bRemoved;
	HList<uint8> bytes;
	HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::SessionData, change, bytes);
	broadcast(bytes);
}

void PGameplayHostSession::sendTo(HNetPeerId peer, const HList<uint8>& bytes)
{
	if (_transport == nullptr || peer == NetPeerNone)
	{
		return;
	}
	_transport->Send(peer, bytes);
}

void PGameplayHostSession::broadcast(const HList<uint8>& bytes)
{
	if (_transport == nullptr)
	{
		return;
	}
	for (const HGameplayPlayerSlot& slot : _slots)
	{
		if (slot.bConnected == true && slot.Slot != _localSlot && slot.Peer != NetPeerNone)
		{
			_transport->Send(slot.Peer, bytes);
		}
	}
}

void PGameplayHostSession::broadcastSlots()
{
	if (_mode == EGameplayNetMode::ListenServer)
	{
		HGameplayNetSlots slots;
		slots.Slots = _slots;
		HList<uint8> bytes;
		HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::SlotChanged, slots, bytes);
		broadcast(bytes);
	}
	OnSlotsChanged.BroadCast();
}

void PGameplayHostSession::sendDocument(HNetPeerId peer)
{
	if (_gameMaster == nullptr || _gameMaster->IsStarted() == false)
	{
		return;
	}

	PString document;
	if (_gameMaster->ExportDocument(&document) == false)
	{
		JG_LOG(Network, ELogLevel::Error, "PGameplayHostSession: cannot export document");
		return;
	}

	HList<uint8> bytes;
	HGameplayNetCodec::Encode(EGameplayNetMessage::Document, document.GetRawString(), true, bytes);
	sendTo(peer, bytes);
}

void PGameplayHostSession::updateTimers(float32 deltaSeconds)
{
	_pingTimer += deltaSeconds;
	if (_pingTimer >= _config.PingInterval)
	{
		_pingTimer = 0.0f;
		HList<uint8> ping;
		HGameplayNetCodec::Encode(EGameplayNetMessage::Ping, HRawString(), false, ping);
		broadcast(ping);
	}

	for (HGameplayPlayerSlot& slot : _slots)
	{
		if (slot.bConnected == false || slot.Slot == _localSlot)
		{
			continue;
		}
		if (_time - slot.LastHeard > _config.TimeoutSeconds)
		{
			JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: slot %d timed out", slot.Slot);
			if (_transport != nullptr)
			{
				_transport->Close(slot.Peer);
			}
			markDisconnected(slot);
		}
	}

	for (auto iter = _handshaking.begin(); iter != _handshaking.end();)
	{
		if (_time - (*iter).ConnectedAt > _config.TimeoutSeconds)
		{
			if (_transport != nullptr)
			{
				_transport->Close((*iter).Peer);
			}
			iter = _handshaking.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

HGameplayPlayerSlot* PGameplayHostSession::findSlotByPeer(HNetPeerId peer)
{
	if (peer == NetPeerNone)
	{
		return nullptr;
	}
	for (HGameplayPlayerSlot& slot : _slots)
	{
		if (slot.bConnected == true && slot.Peer == peer)
		{
			return &slot;
		}
	}
	return nullptr;
}

int32 PGameplayHostSession::findHandshaking(HNetPeerId peer) const
{
	int32 count = (int32)_handshaking.size();
	for (int32 i = 0; i < count; ++i)
	{
		if (_handshaking[i].Peer == peer)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

HGameplayPlayerSlot* PGameplayHostSession::reserveSlot(const PString& token)
{
	// 재접속: 같은 토큰의 자리. 옛 연결이 아직 살아 있다고 알고 있으면 그 연결을 닫고 이어받는다.
	for (HGameplayPlayerSlot& slot : _slots)
	{
		if (slot.Slot == _localSlot || slot.Token.Empty() == true || slot.Token != token)
		{
			continue;
		}
		if (slot.bConnected == true && slot.Peer != NetPeerNone && _transport != nullptr)
		{
			// 창을 다시 띄워 옛 연결이 아직 끊기지 않았거나, 두 창이 한 토큰을 같이 쓴다(앞 창은 끊긴다 — 창마다 다른 토큰).
			JG_LOG(Network, ELogLevel::Warning, "PGameplayHostSession: slot %d joined again with the same reconnect token, closing its previous connection", slot.Slot);
			_transport->Close(slot.Peer);
		}
		slot.bConnected = false;
		slot.Peer       = NetPeerNone;
		return &slot;
	}

	for (HGameplayPlayerSlot& slot : _slots)
	{
		if (slot.Slot != _localSlot && slot.IsFree() == true)
		{
			return &slot;
		}
	}
	return nullptr;
}
