#include "PCH/PCH.h"
#include "Network/Session/GameplayClientSession.h"

namespace
{
	void setReason(PString* outReason, const PString& reason)
	{
		if (outReason != nullptr)
		{
			*outReason = reason;
		}
	}

	const char* disconnectText(ENetDisconnectReason reason)
	{
		switch (reason)
		{
		case ENetDisconnectReason::Closed:  return "host closed the connection";
		case ENetDisconnectReason::Refused: return "connection refused";
		case ENetDisconnectReason::Error:   return "connection lost";
		default:                            return "disconnected";
		}
	}
}

PGameplayClientSession::~PGameplayClientSession()
{
	if (_transport != nullptr)
	{
		_transport->Shutdown();
	}
}

PSharedPtr<PGameplayClientSession> PGameplayClientSession::Create(PSharedPtr<INetTransport> transport, const PString& address, uint16 port, const HGameplaySessionConfig& config)
{
	if (transport == nullptr)
	{
		return nullptr;
	}

	PSharedPtr<PGameplayClientSession> session = Allocate<PGameplayClientSession>();
	session->_mode      = EGameplayNetMode::Client;
	session->_state     = EGameplaySessionState::Connecting;
	session->_config    = config;
	session->_transport = transport;
	session->_address   = address;
	session->_port      = port;
	session->_token     = HGuid::New().ToString();

	session->_hostPeer = transport->Connect(address, port);
	if (session->_hostPeer == NetPeerNone)
	{
		session->setClosed(PString::Format("cannot connect to %s:%u", address, (uint32)port));
	}
	return session;
}

EGameplaySessionSubmit PGameplayClientSession::SubmitLocal(const HGameplayCommand& command, PString* outReason)
{
	if (_state != EGameplaySessionState::Playing || _gameMaster == nullptr)
	{
		setReason(outReason, "not playing");
		return EGameplaySessionSubmit::Rejected;
	}
	if (IsLocallyControlled(command.Actor) == false)
	{
		setReason(outReason, "not your actor");
		return EGameplaySessionSubmit::Rejected;
	}

	// 한 번에 하나만 보낸다. 앞선 명령이 복제본에 반영되기 전의 상태로 다음 명령을 검증하지 않기 위해서다.
	if (_bInFlight == true || _localQueue.empty() == false)
	{
		_localQueue.push_back(command);
		return EGameplaySessionSubmit::Queued;
	}

	if (sendLocal(command, outReason) == false)
	{
		return EGameplaySessionSubmit::Rejected;
	}
	return EGameplaySessionSubmit::Sent;
}

bool PGameplayClientSession::StartGame(uint64 seed)
{
	JG_LOG(Network, ELogLevel::Warning, "PGameplayClientSession::StartGame: only the host starts the game");
	return false;
}

void PGameplayClientSession::Tick(float32 deltaSeconds)
{
	_time += deltaSeconds;
	if (_transport == nullptr)
	{
		return;
	}

	HList<HNetEvent> events;
	_transport->Poll(events);
	if (_state == EGameplaySessionState::Closed)
	{
		return;
	}

	for (const HNetEvent& event : events)
	{
		handleEvent(event);
		if (_state == EGameplaySessionState::Closed)
		{
			return;
		}
	}

	if (_state != EGameplaySessionState::Connecting)
	{
		_pingTimer += deltaSeconds;
		if (_pingTimer >= _config.PingInterval)
		{
			_pingTimer = 0.0f;
			HList<uint8> ping;
			HGameplayNetCodec::Encode(EGameplayNetMessage::Ping, HRawString(), false, ping);
			send(ping);
		}
		if (_time - _lastHeard > _config.TimeoutSeconds)
		{
			fail("host timed out");
			return;
		}
	}

	if (_state == EGameplaySessionState::Playing && _bInFlight == false)
	{
		sendNextQueued();
	}
}

void PGameplayClientSession::Close(const PString& reason)
{
	if (_state == EGameplaySessionState::Closed)
	{
		return;
	}
	if (_transport != nullptr && _hostPeer != NetPeerNone)
	{
		_transport->Close(_hostPeer);
	}
	_hostPeer = NetPeerNone;
	setClosed(reason);
}

bool PGameplayClientSession::HasPendingSends() const
{
	return _transport != nullptr && _transport->HasPendingSends();
}

bool PGameplayClientSession::Reconnect()
{
	if (_state != EGameplaySessionState::Closed || _transport == nullptr)
	{
		return false;
	}

	_hostPeer = _transport->Connect(_address, _port);
	if (_hostPeer == NetPeerNone)
	{
		return false;
	}

	_state       = EGameplaySessionState::Connecting;
	_closeReason = PString();
	_bInFlight   = false;
	_localQueue.clear();
	_held.clear();
	_lastHeard = _time;
	_pingTimer = 0.0f;
	JG_LOG(Network, ELogLevel::Info, "PGameplayClientSession: reconnecting to %s:%u", _address, (uint32)_port);
	return true;
}

int32 PGameplayClientSession::GetDesyncCount() const
{
	return _desyncCount;
}

uint32 PGameplayClientSession::GetLastAppliedSeq() const
{
	return _lastAppliedSeq;
}

bool PGameplayClientSession::HasPendingLocalCommand() const
{
	return _bInFlight == true || _localQueue.empty() == false;
}

uint64 PGameplayClientSession::GetHostFingerprint() const
{
	return _hostFingerprint;
}

void PGameplayClientSession::onBound()
{
	HDeque<HHeldMessage> held;
	held.swap(_held);
	for (const HHeldMessage& message : held)
	{
		handleMessage(message.Type, message.Text);
		if (_state == EGameplaySessionState::Closed)
		{
			return;
		}
	}
}

void PGameplayClientSession::onUnbound()
{
	// 떼어진 GameMaster 의 게임은 끝났다 (월드 이동 · 세션 교체). 보낸 명령의 답은 이동 알림 전에 왔으므로 기다리지 않는다.
	_bInFlight = false;
	_localQueue.clear();
	if (_state == EGameplaySessionState::Playing || _state == EGameplaySessionState::Resyncing)
	{
		_state = EGameplaySessionState::Ready;
	}
}

// ---- 메시지 -------------------------------------------------------------------

void PGameplayClientSession::handleEvent(const HNetEvent& event)
{
	if (event.Peer != _hostPeer)
	{
		return;
	}

	switch (event.Type)
	{
	case ENetEventType::Connected:
	{
		_lastHeard = _time;
		sendHello();
		break;
	}
	case ENetEventType::Disconnected:
	{
		_hostPeer = NetPeerNone;
		setClosed(disconnectText(event.Reason));
		break;
	}
	case ENetEventType::Message:
	{
		_lastHeard = _time;
		EGameplayNetMessage type = EGameplayNetMessage::None;
		HRawString text;
		if (HGameplayNetCodec::Decode(event.Data, &type, &text) == false)
		{
			JG_LOG(Network, ELogLevel::Warning, "PGameplayClientSession: undecodable message");
			break;
		}
		handleMessage(type, text);
		break;
	}
	}
}

void PGameplayClientSession::handleMessage(EGameplayNetMessage type, const HRawString& text)
{
	if (needsGameMaster(type) == true && _gameMaster == nullptr)
	{
		HHeldMessage held;
		held.Type = type;
		held.Text = text;
		_held.push_back(held);
		return;
	}

	switch (type)
	{
	case EGameplayNetMessage::Welcome:
	{
		HGameplayNetWelcome welcome;
		if (HGameplayNetCodec::DecodeMessage(text, &welcome) == false)
		{
			fail("bad welcome");
			return;
		}
		_localSlot       = welcome.Slot;
		_slots           = welcome.Slots;
		_hostFingerprint = welcome.Fingerprint;
		if (_state == EGameplaySessionState::Connecting)
		{
			_state = EGameplaySessionState::Ready;
		}
		JG_LOG(Network, ELogLevel::Info, "PGameplayClientSession: joined as slot %d (game %s)", _localSlot, welcome.bStarted ? "in progress" : "not started");
		OnSlotsChanged.BroadCast();
		if (welcome.World != NAME_NONE)
		{
			OnTravelRequested.BroadCast(welcome.World);
		}
		break;
	}
	case EGameplayNetMessage::Refuse:
	{
		HGameplayNetRefuse refused;
		HGameplayNetCodec::DecodeMessage(text, &refused);
		fail(PString::Format("refused: %s", refused.Reason));
		break;
	}
	case EGameplayNetMessage::SlotChanged:
	{
		HGameplayNetSlots slots;
		if (HGameplayNetCodec::DecodeMessage(text, &slots) == true)
		{
			_slots = slots.Slots;
			OnSlotsChanged.BroadCast();
		}
		break;
	}
	case EGameplayNetMessage::Travel:
	{
		HGameplayNetTravel travel;
		if (HGameplayNetCodec::DecodeMessage(text, &travel) == true)
		{
			// 뒤따르는 게임 메시지(새 게임의 시작 · 승인)는 새 월드의 GameMaster 몫이다. 떼어 두면 붙을 때까지 보관된다.
			UnbindGameMaster();
			OnTravelRequested.BroadCast(travel.World);
		}
		break;
	}
	case EGameplayNetMessage::CommandRejected:
	{
		HGameplayNetCommandRejected rejected;
		if (HGameplayNetCodec::DecodeMessage(text, &rejected) == false)
		{
			break;
		}
		if (_bInFlight == true && rejected.ClientSeq == _inFlightSeq)
		{
			_bInFlight = false;
			HGameplayCommand command = _inFlightCommand;
			OnLocalCommandRejected.BroadCast(command, rejected.Reason);
		}
		break;
	}
	case EGameplayNetMessage::StartGame:
	{
		applyStartGame(text);
		break;
	}
	case EGameplayNetMessage::CommandAccepted:
	{
		applyAccepted(text);
		break;
	}
	case EGameplayNetMessage::Document:
	{
		applyDocument(text);
		break;
	}
	case EGameplayNetMessage::Ping:
	{
		break;
	}
	default:
	{
		JG_LOG(Network, ELogLevel::Warning, "PGameplayClientSession: unexpected %s", HGameplayNetCodec::ToString(type));
		break;
	}
	}
}

bool PGameplayClientSession::needsGameMaster(EGameplayNetMessage type) const
{
	return type == EGameplayNetMessage::StartGame || type == EGameplayNetMessage::CommandAccepted || type == EGameplayNetMessage::Document;
}

void PGameplayClientSession::applyStartGame(const HRawString& text)
{
	HGameplayNetStartGame start;
	if (HGameplayNetCodec::DecodeMessage(text, &start) == false)
	{
		fail("bad StartGame");
		return;
	}

	uint64 localFingerprint = _gameMaster->RulesFingerprint();
	if (start.Fingerprint != localFingerprint)
	{
		fail(PString::Format("rules fingerprint mismatch (host %llu, local %llu)", start.Fingerprint, localFingerprint));
		return;
	}

	// 등록된 테이블을 가진 초기 상태 사본에 읽어야 컴포넌트가 채워진다.
	HGameplayState initial = _gameMaster->GetInitialState();
	if (initial.FromJsonString(start.InitialState) == false)
	{
		fail("bad initial state");
		return;
	}
	_gameMaster->EditInitialState() = initial;

	if (_gameMaster->Start(start.Seed) == false)
	{
		fail("start failed");
		return;
	}

	_lastAppliedSeq = _gameMaster->GetState().Sequence;
	_state          = EGameplaySessionState::Playing;
	JG_LOG(Network, ELogLevel::Info, "PGameplayClientSession: game started (seed %llu)", start.Seed);
}

void PGameplayClientSession::applyAccepted(const HRawString& text)
{
	HGameplayNetCommandAccepted accepted;
	if (HGameplayNetCodec::DecodeMessage(text, &accepted) == false)
	{
		JG_LOG(Network, ELogLevel::Warning, "PGameplayClientSession: bad CommandAccepted");
		return;
	}

	if (_bInFlight == true && accepted.Slot == _localSlot && accepted.ClientSeq == _inFlightSeq)
	{
		_bInFlight = false;
	}

	if (_state != EGameplaySessionState::Playing)
	{
		// 재동기 중이면 곧 올 문서가 이 명령까지 담고 있다.
		return;
	}
	if (accepted.Seq <= _lastAppliedSeq)
	{
		// 문서에 이미 들어 있는 명령.
		return;
	}

	HList<HGameplayEvent> events;
	PString reason;
	EGameplaySubmitResult result = _gameMaster->Submit(accepted.Command, events, &reason);
	if (result == EGameplaySubmitResult::Rejected)
	{
		JG_LOG(Network, ELogLevel::Error, "PGameplayClientSession: accepted seq %u rejected locally: %s", accepted.Seq, reason);
		beginResync(accepted.Seq);
		return;
	}

	uint32 localSeq      = _gameMaster->GetState().Sequence;
	uint64 localChecksum = _gameMaster->Checksum();
	if (localSeq != accepted.Seq || localChecksum != accepted.Checksum)
	{
		JG_LOG(Network, ELogLevel::Error, "PGameplayClientSession: desync at seq %u (local seq %u, checksum %llu vs host %llu)", accepted.Seq, localSeq, localChecksum, accepted.Checksum);
		beginResync(accepted.Seq);
		return;
	}

	_lastAppliedSeq = accepted.Seq;
}

void PGameplayClientSession::applyDocument(const HRawString& text)
{
	uint64 localFingerprint = _gameMaster->RulesFingerprint();
	if (_hostFingerprint != 0 && _hostFingerprint != localFingerprint)
	{
		fail(PString::Format("rules fingerprint mismatch (host %llu, local %llu)", _hostFingerprint, localFingerprint));
		return;
	}

	if (_gameMaster->ImportDocument(PString(text.c_str())) == false)
	{
		fail("bad document");
		return;
	}

	_lastAppliedSeq = _gameMaster->GetState().Sequence;
	if (_bDumpHostOnDocument == true)
	{
		dumpState("host", _gameMaster->GetState());
		_bDumpHostOnDocument = false;
	}
	_state = EGameplaySessionState::Playing;
	JG_LOG(Network, ELogLevel::Info, "PGameplayClientSession: document applied at seq %u", _lastAppliedSeq);
}

// ---- 보내기 --------------------------------------------------------------------

void PGameplayClientSession::sendHello()
{
	HGameplayNetHello hello;
	hello.Protocol = HGameplayNetProtocol::Version;
	hello.Schema   = HGameplayState::SchemaVersion;
	hello.Name     = _config.PlayerName;
	hello.Token    = _token;

	HList<uint8> bytes;
	HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::Hello, hello, bytes);
	send(bytes);
}

bool PGameplayClientSession::sendLocal(const HGameplayCommand& command, PString* outReason)
{
	// 복제본 기준 검증으로 틀린 명령은 왕복 없이 바로 거절한다. 최종 판단은 호스트가 한다.
	if (_gameMaster->Validate(command, outReason) == false)
	{
		return false;
	}

	HGameplayNetCommandRequest request;
	request.ClientSeq = _nextClientSeq++;
	request.Command   = command;

	HList<uint8> bytes;
	HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::CommandRequest, request, bytes);
	send(bytes);

	_bInFlight       = true;
	_inFlightSeq     = request.ClientSeq;
	_inFlightCommand = command;
	return true;
}

void PGameplayClientSession::sendNextQueued()
{
	while (_bInFlight == false && _localQueue.empty() == false && _state == EGameplaySessionState::Playing)
	{
		HGameplayCommand command = _localQueue.front();
		_localQueue.pop_front();

		PString reason;
		if (IsLocallyControlled(command.Actor) == false)
		{
			reason = "not your actor";
		}
		else if (sendLocal(command, &reason) == true)
		{
			continue;
		}
		OnLocalCommandRejected.BroadCast(command, reason);
	}
}

void PGameplayClientSession::beginResync(uint32 seq)
{
	++_desyncCount;
	_desyncSeq = seq;
	JG_LOG(Network, ELogLevel::Warning, "PGameplayClientSession: requesting document after desync at seq %u (count %d)", seq, _desyncCount);

	if (_config.DesyncDumpDirectory.Empty() == false)
	{
		dumpState("local", _gameMaster->GetState());
		_bDumpHostOnDocument = true;
	}

	_state = EGameplaySessionState::Resyncing;

	HGameplayNetResyncRequest request;
	request.Seq = _lastAppliedSeq;
	HList<uint8> bytes;
	HGameplayNetCodec::EncodeMessage(EGameplayNetMessage::ResyncRequest, request, bytes);
	send(bytes);

	OnDesync.BroadCast(seq);
}

void PGameplayClientSession::dumpState(const char* tag, const HGameplayState& state) const
{
	std::error_code error;
	fs::create_directories(fs::path(_config.DesyncDumpDirectory.GetRawString()), error);

	PString path = PString::Format("%s/desync_slot%d_seq%u_%s.json", _config.DesyncDumpDirectory, _localSlot, _desyncSeq, tag);
	if (HFileHelper::WriteAllText(path, state.ToJsonString()) == true)
	{
		JG_LOG(Network, ELogLevel::Warning, "PGameplayClientSession: desync dump %s", path);
	}
}

void PGameplayClientSession::send(const HList<uint8>& bytes)
{
	if (_transport == nullptr || _hostPeer == NetPeerNone)
	{
		return;
	}
	_transport->Send(_hostPeer, bytes);
}

void PGameplayClientSession::fail(const PString& reason)
{
	JG_LOG(Network, ELogLevel::Error, "PGameplayClientSession: %s", reason);
	Close(reason);
}
