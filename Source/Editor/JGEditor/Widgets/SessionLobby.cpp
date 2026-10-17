#include "PCH/PCH.h"
#include "Widgets/SessionLobby.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Network/Session/GameplayHostSession.h"
#include "Network/Session/GameplayClientSession.h"
#include "Network/Transport/NetTcpTransport.h"
#include "GameMaster/GameMaster.h"
#include "GUI.h"

namespace
{
	constexpr float32 LobbyFieldWidth = 140.0f;   // 100%(96 DPI) 기준 — 쓸 때 화면 배율을 곱한다(D-22 ②)

	const char* netModeText(EGameplayNetMode mode)
	{
		switch (mode)
		{
		case EGameplayNetMode::Standalone:   return "Standalone";
		case EGameplayNetMode::ListenServer: return "ListenServer";
		case EGameplayNetMode::Client:       return "Client";
		default:                             return "?";
		}
	}

	const char* sessionStateText(EGameplaySessionState state)
	{
		switch (state)
		{
		case EGameplaySessionState::Connecting: return "Connecting";
		case EGameplaySessionState::Ready:      return "Ready";
		case EGameplaySessionState::Playing:    return "Playing";
		case EGameplaySessionState::Resyncing:  return "Resyncing";
		case EGameplaySessionState::Closed:     return "Closed";
		default:                                return "?";
		}
	}

	PString controllersText(const HGameplayPlayerSlot& slot)
	{
		if (slot.bAllControllers == true)
		{
			return "all";
		}
		PString text;
		for (int32 controller : slot.Controllers)
		{
			if (text.Empty() == false)
			{
				text += ", ";
			}
			text += PString::Format("%d", controller);
		}
		if (text.Empty() == true)
		{
			text = "-";
		}
		return text;
	}

	void labeledField(const char* label, const char* id, const char* hint, PString& value)
	{
		HGUI::AlignTextToFramePadding();
		HGUI::Text(label);
		HGUI::SameLine();
		HGUI::InputTextWithHint(id, hint, value, LobbyFieldWidth * HGUI::GetDpiScale());
	}

	// 쓸 수 없는 버튼도 같은 자리에 그린다(흐리게). 눌렸는지만 돌려준다.
	bool actionButton(const char* label, bool bAvailable)
	{
		if (bAvailable == false)
		{
			HGUI::PushStyleColor(EGUIColor::Text, HGUI::DisplayColor(0x808080));
		}
		const bool bPressed = HGUI::Button(label);
		if (bAvailable == false)
		{
			HGUI::PopStyleColor();
		}
		return bPressed;
	}

	HGameplaySessionConfig makeConfig(const PString& playerName)
	{
		HGameplaySessionConfig config;
		config.PlayerName = playerName.Empty() == true ? PString("Player") : playerName;
		return config;
	}

	// 긴 값(캠페인 기록 JSON 등)은 앞부분만 보인다. UTF-8 글자 중간에서 자르지 않는다.
	PString shortText(const PString& text, size_t maxBytes)
	{
		const HRawString& raw = text.GetRawString();
		if (raw.size() <= maxBytes)
		{
			return text;
		}
		size_t cut = maxBytes;
		while (cut > 0 && (((uint8)raw[cut]) & 0xC0) == 0x80)
		{
			--cut;
		}
		return PString((raw.substr(0, cut) + "...").c_str());
	}
}

PString JGSessionLobby::GetTitleName() const
{
	return "Session Lobby";
}

void JGSessionLobby::OnGenerateGUI()
{
	if (JGGameInstance::HasInstance() == false)
	{
		HGUI::Text("GameFrameWorks is not running");
		return;
	}

	PSharedPtr<PGameplaySession> session = JGGameInstance::Get().GetSession();
	if (session == nullptr)
	{
		HGUI::Text("The game instance has no session");
		return;
	}

	drawActions(*session);
	drawStatus(*session);
	drawSlots(*session);
	drawSessionData(*session);
}

void JGSessionLobby::drawActions(PGameplaySession& session)
{
	const bool bStandalone = session.GetMode() == EGameplayNetMode::Standalone;
	const bool bCanReload  = session.IsAuthority() == true && JGGameInstance::Get().HasWorld() == true;
	const bool bCanReady   = bStandalone == false && session.GetState() == EGameplaySessionState::Ready && session.GetLocalSlot() != INDEX_NONE;

	if (actionButton("Host", bStandalone) == true)
	{
		if (bStandalone == true)
		{
			host();
		}
		else
		{
			_lastResult = "Leave the current network session first";
		}
	}
	HGUI::SameLine();
	if (actionButton("Join", bStandalone) == true)
	{
		if (bStandalone == true)
		{
			join();
		}
		else
		{
			_lastResult = "Leave the current network session first";
		}
	}
	HGUI::SameLine();
	if (actionButton("Leave", bStandalone == false) == true)
	{
		if (bStandalone == false)
		{
			leave();
		}
		else
		{
			_lastResult = "Already standalone";
		}
	}
	HGUI::SameLine();
	if (actionButton("Reload World", bCanReload) == true)
	{
		if (bCanReload == true)
		{
			reloadWorld();
		}
		else
		{
			_lastResult = "Only the authority (host or standalone) with a loaded world can start a new match";
		}
	}
	HGUI::SameLine();
	if (actionButton(session.IsLocalReady() == true ? "Not Ready" : "Ready", bCanReady) == true)
	{
		if (bCanReady == true)
		{
			toggleReady(session);
		}
		else
		{
			_lastResult = "Ready is for the lobby: a network session before the game starts";
		}
	}

	labeledField("Name", "SessionLobbyName", "player name", _playerName);
	HGUI::SameLine();
	labeledField("Port", "SessionLobbyPort", "47771", _portText);
	HGUI::SameLine();
	labeledField("Address", "SessionLobbyAddress", "host address", _address);
	HGUI::Checkbox("Host on this machine only (127.0.0.1)", _bLocalOnly);
	drawDataActions(session);

	if (_lastResult.Empty() == false)
	{
		HGUI::Text(PString::Format("Last action: %s", _lastResult));
	}
	else
	{
		HGUI::Text("Host: listen on Port | Join: connect to Address:Port | Reload World: new match with the current players");
	}
	HGUI::Separator();
}

// 세션 공유 데이터 쓰기 줄. 권한 쪽(호스트 · 싱글)만 쓸 수 있고, 참가자는 아래 표로 읽기만 한다.
void JGSessionLobby::drawDataActions(PGameplaySession& session)
{
	PGameplayHostSession* host = dynamic_cast<PGameplayHostSession*>(&session);
	labeledField("Data key", "SessionLobbyDataKey", "key", _dataKey);
	HGUI::SameLine();
	HGUI::AlignTextToFramePadding();
	HGUI::Text("Value");
	HGUI::SameLine();
	HGUI::InputTextWithHint("SessionLobbyDataValue", "text or JSON", _dataValue, LobbyFieldWidth * 2.0f * HGUI::GetDpiScale());
	HGUI::SameLine();
	if (actionButton("Set Data", host != nullptr) == true)
	{
		if (host != nullptr)
		{
			setData(*host);
		}
		else
		{
			_lastResult = "Only the host sets session data (clients read it)";
		}
	}
	HGUI::SameLine();
	if (actionButton("Remove Data", host != nullptr) == true)
	{
		if (host != nullptr)
		{
			removeData(*host);
		}
		else
		{
			_lastResult = "Only the host removes session data (clients read it)";
		}
	}
}

void JGSessionLobby::drawStatus(PGameplaySession& session)
{
	PSharedPtr<PWorld> world = JGGameInstance::Get().GetWorld();
	HGUI::Text(PString::Format("Mode %s | State %s | Local slot %d | World %s",
		PString(netModeText(session.GetMode())), PString(sessionStateText(session.GetState())), session.GetLocalSlot(),
		world != nullptr ? world->GetName().ToString() : PString("none")));

	PSharedPtr<PGameMaster> gameMaster = session.GetGameMaster();
	if (gameMaster == nullptr)
	{
		HGUI::Text("GameMaster: not bound");
	}
	else
	{
		HGUI::Text(PString::Format("GameMaster: %s | Sequence %u | Checksum %llu",
			PString(gameMaster->IsStarted() == true ? "started" : "not started"), gameMaster->GetState().Sequence, gameMaster->Checksum()));
	}

	if (const PGameplayHostSession* host = dynamic_cast<const PGameplayHostSession*>(&session))
	{
		HGUI::Text(PString::Format("Connected peers %d", host->GetConnectedPeerCount()));
	}
	else if (const PGameplayClientSession* client = dynamic_cast<const PGameplayClientSession*>(&session))
	{
		HGUI::Text(PString::Format("Last applied sequence %u | Desyncs %d", client->GetLastAppliedSeq(), client->GetDesyncCount()));
	}

	if (session.GetState() == EGameplaySessionState::Closed)
	{
		HGUI::Text(PString::Format("Closed: %s", session.GetCloseReason()), HGUI::DisplayColor(0xE0A040));
	}
	HGUI::Separator();
}

void JGSessionLobby::drawSlots(PGameplaySession& session)
{
	const HList<HGameplayPlayerSlot>& slots = session.GetSlots();
	if (slots.empty() == true)
	{
		HGUI::Text("No slots yet");
		return;
	}

	if (HGUI::BeginTable("SessionLobbySlots", 5) == true)
	{
		HGUI::TableSetupColumn("Slot", 70.0f * HGUI::GetDpiScale());
		HGUI::TableSetupColumn("Name");
		HGUI::TableSetupColumn("Connected", 90.0f * HGUI::GetDpiScale());
		HGUI::TableSetupColumn("Ready", 70.0f * HGUI::GetDpiScale());
		HGUI::TableSetupColumn("Controls", 120.0f * HGUI::GetDpiScale());
		HGUI::TableHeadersRow();

		for (const HGameplayPlayerSlot& slot : slots)
		{
			HGUI::TableNextRow();
			HGUI::TableNextColumn();
			HGUI::Text(slot.Slot == session.GetLocalSlot() ? PString::Format("%d (you)", slot.Slot) : PString::Format("%d", slot.Slot));
			HGUI::TableNextColumn();
			HGUI::Text(slot.Name.Empty() == true ? PString("-") : slot.Name);
			HGUI::TableNextColumn();
			HGUI::Text(slot.bConnected == true ? "yes" : "no");
			HGUI::TableNextColumn();
			HGUI::Text(slot.bReady == true ? "ready" : "-");
			HGUI::TableNextColumn();
			HGUI::Text(controllersText(slot));
		}
		HGUI::EndTable();
	}
}

void JGSessionLobby::drawSessionData(PGameplaySession& session)
{
	HGUI::Separator();
	const HList<HGameplaySessionDataEntry>& entries = session.GetSessionData();
	HGUI::Text(PString::Format("Session data: %d keys (the host sets them, everyone reads them)", (int32)entries.size()));
	if (entries.empty() == true)
	{
		return;
	}

	if (HGUI::BeginTable("SessionLobbyData", 2) == true)
	{
		HGUI::TableSetupColumn("Key", 160.0f * HGUI::GetDpiScale());
		HGUI::TableSetupColumn("Value");
		HGUI::TableHeadersRow();

		for (const HGameplaySessionDataEntry& entry : entries)
		{
			HGUI::TableNextRow();
			HGUI::TableNextColumn();
			HGUI::Text(entry.Key.ToString());
			HGUI::TableNextColumn();
			HGUI::Text(shortText(entry.Value, 160));
		}
		HGUI::EndTable();
	}
}

void JGSessionLobby::host()
{
	uint16 port = 0;
	if (readPort(&port) == false)
	{
		return;
	}

	// 기본은 모든 인터페이스(다른 기계에서 접속). 한 기계 안 시험은 127.0.0.1 로 연다(Windows 방화벽 확인 창이 뜨지 않는다).
	PSharedPtr<PNetTcpTransport> transport = Allocate<PNetTcpTransport>();
	transport->SetListenAddress(_bLocalOnly == true ? PString("127.0.0.1") : PString());
	if (JGGameInstance::Get().HostSession(port, makeConfig(_playerName), transport) != nullptr)
	{
		_lastResult = PString::Format("Hosting on port %u%s", (uint32)port, PString(_bLocalOnly == true ? " (127.0.0.1)" : ""));
	}
	else
	{
		_lastResult = PString::Format("Cannot listen on port %u", (uint32)port);
	}
}

void JGSessionLobby::join()
{
	uint16 port = 0;
	if (readPort(&port) == false)
	{
		return;
	}

	PSharedPtr<PGameplayClientSession> client = JGGameInstance::Get().JoinSession(_address, port, makeConfig(_playerName));
	if (client != nullptr && client->GetState() != EGameplaySessionState::Closed)
	{
		_lastResult = PString::Format("Connecting to %s:%u", _address, (uint32)port);
	}
	else
	{
		_lastResult = PString::Format("Cannot join %s:%u (%s)", _address, (uint32)port, client != nullptr ? client->GetCloseReason() : PString("no session"));
	}
}

void JGSessionLobby::leave()
{
	// 싱글로 돌아간다. 지금 월드의 GameMaster 는 싱글 세션에 다시 붙고, 시작된 게임은 그대로 이어진다.
	JGGameInstance::Get().LeaveSession();
	_lastResult = "Left the session (standalone)";
}

void JGSessionLobby::reloadWorld()
{
	// 같은 월드를 다시 로드한다. 호스트면 참가자에게 이동 알림이 나가고, 엔트리 액터가 권한 쪽에서 새 판을 시작한다.
	JGGameInstance& gameInstance = JGGameInstance::Get();
	const PName worldName = gameInstance.GetWorld()->GetName();
	gameInstance.LoadWorld(worldName);
	_lastResult = PString::Format("Reloaded world %s: new match with the current players", worldName.ToString());
}

void JGSessionLobby::toggleReady(PGameplaySession& session)
{
	// 클라는 호스트로 보내고 슬롯 표로 돌아온다. 표가 바뀌기 전까지 버튼 글자는 그대로다.
	const bool bReady = session.IsLocalReady() == false;
	if (session.SetLocalReady(bReady) == true)
	{
		_lastResult = bReady == true ? "Ready" : "Not ready";
	}
	else
	{
		_lastResult = "Cannot change ready now (only in the lobby before the game starts)";
	}
}

void JGSessionLobby::setData(PGameplayHostSession& host)
{
	if (_dataKey.Empty() == true)
	{
		_lastResult = "Data key is empty";
		return;
	}
	if (host.SetSessionData(PName(_dataKey), _dataValue) == true)
	{
		_lastResult = PString::Format("Session data %s = %s", _dataKey, shortText(_dataValue, 60));
	}
	else
	{
		_lastResult = PString::Format("Session data %s refused (over the size limit, see the log)", _dataKey);
	}
}

void JGSessionLobby::removeData(PGameplayHostSession& host)
{
	if (_dataKey.Empty() == false && host.RemoveSessionData(PName(_dataKey)) == true)
	{
		_lastResult = PString::Format("Session data %s removed", _dataKey);
	}
	else
	{
		_lastResult = PString::Format("No session data %s", _dataKey);
	}
}

bool JGSessionLobby::readPort(uint16* outPort)
{
	const HRawString& text = _portText.GetRawString();
	int32 value = 0;
	bool bValid = text.empty() == false && text.size() <= 5;
	for (char c : text)
	{
		if (c < '0' || c > '9')
		{
			bValid = false;
			break;
		}
		value = value * 10 + (c - '0');
	}
	if (bValid == false || value <= 0 || value > 65535)
	{
		_lastResult = PString::Format("Port must be 1..65535 (got '%s')", _portText);
		return false;
	}
	*outPort = (uint16)value;
	return true;
}
