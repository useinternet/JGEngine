#include "PCH/PCH.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Actors/GameEntryActor.h"
#include "Network/Session/GameplayHostSession.h"
#include "Network/Session/GameplayClientSession.h"
#include "Network/Transport/NetTcpTransport.h"
#include "UI/GameUIManager.h"

JGGameInstance* JGGameInstance::s_instance = nullptr;

namespace
{
	constexpr uint16  DefaultSessionPort    = 47771;
	constexpr float32 SessionFlushSeconds   = 2.0f;

	// 프로세스 실행 인자에서 -name 또는 -name=값 을 찾는다. 따옴표로 묶은 토큰은 한 토큰이다.
	bool findLaunchArgument(const char* name, PString* outValue)
	{
		const char* commandLine = ::GetCommandLineA();
		if (commandLine == nullptr)
		{
			return false;
		}

		const HRawString key  = HRawString("-") + name;
		const HRawString text = commandLine;
		size_t index = 0;
		while (index < text.size())
		{
			while (index < text.size() && (text[index] == ' ' || text[index] == '\t'))
			{
				++index;
			}

			HRawString token;
			bool bQuoted = false;
			while (index < text.size() && (bQuoted == true || (text[index] != ' ' && text[index] != '\t')))
			{
				if (text[index] == '"')
				{
					bQuoted = !bQuoted;
				}
				else
				{
					token.push_back(text[index]);
				}
				++index;
			}

			if (token == key)
			{
				if (outValue != nullptr)
				{
					*outValue = PString();
				}
				return true;
			}
			if (token.size() > key.size() && token.compare(0, key.size(), key) == 0 && token[key.size()] == '=')
			{
				if (outValue != nullptr)
				{
					*outValue = PString(token.substr(key.size() + 1).c_str());
				}
				return true;
			}
		}
		return false;
	}

	bool parsePort(const HRawString& text, uint16* outPort)
	{
		if (text.empty() == true || text.size() > 5)
		{
			return false;
		}
		int32 value = 0;
		for (char c : text)
		{
			if (c < '0' || c > '9')
			{
				return false;
			}
			value = value * 10 + (c - '0');
		}
		if (value <= 0 || value > 65535)
		{
			return false;
		}
		*outPort = (uint16)value;
		return true;
	}
}

bool JGGameInstance::HasInstance()
{
	return s_instance != nullptr;
}

JGGameInstance& JGGameInstance::Get()
{
	JG_CHECK(s_instance != nullptr);
	return *s_instance;
}

void JGGameInstance::SetEntryClass(PSharedPtr<JGClass> entryClass)
{
	_entryClass = entryClass;
}

PSharedPtr<JGClass> JGGameInstance::GetEntryClass() const
{
	return _entryClass;
}

PSharedPtr<PWorld> JGGameInstance::LoadWorld(const PName& name)
{
	if (_world != nullptr)
	{
		UnloadWorld();
	}

	PSharedPtr<PWorld> world = (name == NAME_NONE) ? Allocate<PWorld>() : Allocate<PWorld>(name);
	_world = world;

	// 이동 알림은 엔트리 액터보다 먼저 나간다. 엔트리 액터가 시작하면 그 알림(StartGame)이 이동 알림 뒤에 도착해야 클라가 새 월드에서 받는다.
	PSharedPtr<PGameplayHostSession> host = RawDynamicCast<PGameplayHostSession>(_session);
	if (host != nullptr)
	{
		host->NotifyTravel(world->GetName());
	}

	if (_entryClass != nullptr)
	{
		PSharedPtr<JGActor> actor = world->SpawnActorByClass(_entryClass, PName("GameEntry"));
		PSharedPtr<JGGameEntryActor> entry = RawDynamicCast<JGGameEntryActor>(actor);
		if (entry != nullptr)
		{
			entry->OnEnterWorld();
		}
		else
		{
			JG_LOG(GameFrameWorks, ELogLevel::Warning, "JGGameInstance: entry class is not a JGGameEntryActor");
		}
	}

	OnWorldReady(world);
	OnWorldLoaded.BroadCast(world);

	world->BeginPlay();
	JG_LOG(GameFrameWorks, ELogLevel::Info, "JGGameInstance: world loaded (%s)", world->GetName().ToString());
	return world;
}

void JGGameInstance::UnloadWorld()
{
	if (_world == nullptr)
	{
		return;
	}

	PSharedPtr<PWorld> world = _world;

	PSharedPtr<JGGameEntryActor> entry = GetEntryActor();
	if (entry != nullptr)
	{
		entry->OnExitWorld();
	}

	OnWorldUnloading.BroadCast(world);
	OnWorldUnload(world);

	world->EndPlay();
	_world.Reset();
	JG_LOG(GameFrameWorks, ELogLevel::Info, "JGGameInstance: world unloaded (%s)", world->GetName().ToString());
}

PSharedPtr<PWorld> JGGameInstance::GetWorld() const
{
	return _world;
}

bool JGGameInstance::HasWorld() const
{
	return _world != nullptr;
}

PSharedPtr<JGGameEntryActor> JGGameInstance::GetEntryActor() const
{
	if (_world == nullptr)
	{
		return nullptr;
	}

	HList<PSharedPtr<JGGameEntryActor>> entries;
	_world->FindActors<JGGameEntryActor>(entries);
	if (entries.empty() == true)
	{
		return nullptr;
	}
	return entries[0];
}

PSharedPtr<PGameplaySession> JGGameInstance::GetSession() const
{
	return _session;
}

PSharedPtr<PGameUIManager> JGGameInstance::GetUI() const
{
	return _ui;
}

bool JGGameInstance::IsAuthority() const
{
	return _session == nullptr || _session->IsAuthority() == true;
}

PSharedPtr<PGameplayHostSession> JGGameInstance::HostSession(uint16 port, const HGameplaySessionConfig& config, PSharedPtr<INetTransport> transport)
{
	if (transport == nullptr)
	{
		transport = Allocate<PNetTcpTransport>();
	}

	PSharedPtr<PGameplayHostSession> session = PGameplayHostSession::CreateListenServer(transport, port, config);
	if (session == nullptr)
	{
		return nullptr;
	}
	replaceSession(session);
	return session;
}

PSharedPtr<PGameplayClientSession> JGGameInstance::JoinSession(const PString& address, uint16 port, const HGameplaySessionConfig& config, PSharedPtr<INetTransport> transport)
{
	if (transport == nullptr)
	{
		transport = Allocate<PNetTcpTransport>();
	}

	PSharedPtr<PGameplayClientSession> session = PGameplayClientSession::Create(transport, address, port, config);
	if (session == nullptr)
	{
		return nullptr;
	}
	replaceSession(session);
	return session;
}

void JGGameInstance::LeaveSession()
{
	HGameplaySessionConfig config;
	if (_session != nullptr && _session->GetLocalSlot() != INDEX_NONE)
	{
		const HGameplayPlayerSlot* local = _session->FindSlot(_session->GetLocalSlot());
		if (local != nullptr && local->Name.Empty() == false)
		{
			config.PlayerName = local->Name;
		}
	}
	replaceSession(PGameplayHostSession::CreateStandalone(config));
}

void JGGameInstance::Tick(float32 deltaSeconds)
{
	if (_session != nullptr)
	{
		_session->Tick(deltaSeconds);
	}

	// 세션 틱 안(메시지 처리 중)에서 월드를 바꾸지 않는다. 이동 요청은 틱이 끝난 뒤 처리한다.
	if (_bTravelPending == true)
	{
		_bTravelPending = false;
		JG_LOG(GameFrameWorks, ELogLevel::Info, "JGGameInstance: following the host to world %s", _pendingTravel.ToString());
		LoadWorld(_pendingTravel);
	}

	if (_world != nullptr && _world->HasBegun() == true)
	{
		_world->Tick(deltaSeconds);
	}

	// 화면은 월드 뒤에 갱신한다(이번 틱의 월드 상태를 본다).
	if (_ui != nullptr)
	{
		_ui->Update(deltaSeconds);
	}
}

void JGGameInstance::init()
{
	if (_bInitialized == true)
	{
		return;
	}
	_bInitialized = true;
	s_instance = this;

	// 게임 인스턴스를 교체할 때는 모듈이 옛 UI 관리자를 넘겨 준다(떠 있는 화면 유지).
	if (_ui == nullptr)
	{
		_ui = Allocate<PGameUIManager>();
	}

	// 모듈이 게임 인스턴스를 교체하면 세션(연결)은 이어받는다 (HGameFrameWorksModule::replaceGameInstance). 옛 월드는 이미 내려갔다.
	if (_session == nullptr)
	{
		createLaunchSession();
	}
	else
	{
		PSharedPtr<PGameplaySession> session = _session;
		_session.Reset();
		session->UnbindGameMaster();
		replaceSession(session);
	}
	OnInit();
}

PSharedPtr<PGameplaySession> JGGameInstance::detachSession()
{
	PSharedPtr<PGameplaySession> session = _session;
	if (session != nullptr)
	{
		session->OnTravelRequested.Remove(_travelHandle);
	}
	_session.Reset();
	_bTravelPending = false;
	return session;
}

PSharedPtr<PGameUIManager> JGGameInstance::detachUI()
{
	PSharedPtr<PGameUIManager> ui = _ui;
	_ui.Reset();
	return ui;
}

void JGGameInstance::shutdown()
{
	if (_bInitialized == false)
	{
		return;
	}
	// 화면이 월드 객체를 보고 있을 수 있으므로 월드보다 먼저 내린다(위젯 OnDeactivated · OnShutdown).
	if (_ui != nullptr)
	{
		_ui->Shutdown();
		_ui.Reset();
	}
	UnloadWorld();
	closeSession("shutdown");
	OnShutdown();
	if (s_instance == this)
	{
		s_instance = nullptr;
	}
	_bInitialized = false;
}

void JGGameInstance::createLaunchSession()
{
	HGameplaySessionConfig config;
	PString name;
	if (findLaunchArgument("name", &name) == true && name.Empty() == false)
	{
		config.PlayerName = name;
	}

	PString value;
	if (findLaunchArgument("join", &value) == true)
	{
		HRawString address = value.GetRawString();
		uint16     port    = DefaultSessionPort;
		size_t     colon   = address.rfind(':');
		if (colon != HRawString::npos)
		{
			if (parsePort(address.substr(colon + 1), &port) == false)
			{
				JG_LOG(GameFrameWorks, ELogLevel::Error, "JGGameInstance: bad -join port in %s", value);
			}
			address = address.substr(0, colon);
		}

		if (address.empty() == true)
		{
			JG_LOG(GameFrameWorks, ELogLevel::Error, "JGGameInstance: -join needs an address (-join=host[:port])");
		}
		else
		{
			// 접속에 실패해도 클라 세션(Closed)으로 남는다. 싱글로 바꾸면 게임이 권한 쪽으로 착각하고 혼자 시작한다.
			PSharedPtr<PGameplayClientSession> session = JoinSession(PString(address.c_str()), port, config);
			if (session != nullptr)
			{
				JG_LOG(GameFrameWorks, ELogLevel::Info, "JGGameInstance: joining %s:%u as %s", PString(address.c_str()), (uint32)port, config.PlayerName);
				if (session->GetState() == EGameplaySessionState::Closed)
				{
					JG_LOG(GameFrameWorks, ELogLevel::Error, "JGGameInstance: join failed: %s", session->GetCloseReason());
				}
				return;
			}
		}
	}
	else if (findLaunchArgument("host", &value) == true)
	{
		uint16 port = DefaultSessionPort;
		if (value.Empty() == false && parsePort(value.GetRawString(), &port) == false)
		{
			JG_LOG(GameFrameWorks, ELogLevel::Error, "JGGameInstance: bad -host port %s, using %u", value, (uint32)DefaultSessionPort);
			port = DefaultSessionPort;
		}

		// 기본은 모든 인터페이스(다른 기계에서 접속). 한 기계 안 검사는 -bind=127.0.0.1 (Windows 방화벽 확인 창이 뜨지 않는다).
		PString bindAddress;
		findLaunchArgument("bind", &bindAddress);
		PSharedPtr<PNetTcpTransport> transport = Allocate<PNetTcpTransport>();
		transport->SetListenAddress(bindAddress);
		if (HostSession(port, config, transport) != nullptr)
		{
			JG_LOG(GameFrameWorks, ELogLevel::Info, "JGGameInstance: hosting on port %u as %s", (uint32)port, config.PlayerName);
			return;
		}
		JG_LOG(GameFrameWorks, ELogLevel::Error, "JGGameInstance: cannot host on port %u, falling back to standalone", (uint32)port);
	}

	replaceSession(PGameplayHostSession::CreateStandalone(config));
}

void JGGameInstance::replaceSession(PSharedPtr<PGameplaySession> session)
{
	// 활성 월드의 GameMaster 는 새 세션으로 옮긴다.
	PSharedPtr<PGameMaster> gameMaster;
	if (_session != nullptr)
	{
		gameMaster = _session->GetGameMaster();
		closeSession("session replaced");
	}

	_session        = session;
	_bTravelPending = false;
	if (_session == nullptr)
	{
		return;
	}

	_travelHandle = _session->OnTravelRequested.AddRaw(this, &JGGameInstance::onTravelRequested);
	if (gameMaster != nullptr)
	{
		_session->BindGameMaster(gameMaster);
	}

	// 새 호스트는 지금 월드를 알고 있어야 입장한 클라가 같은 월드를 로드한다.
	PSharedPtr<PGameplayHostSession> host = RawDynamicCast<PGameplayHostSession>(_session);
	if (host != nullptr && _world != nullptr)
	{
		host->NotifyTravel(_world->GetName());
	}
}

void JGGameInstance::closeSession(const PString& reason)
{
	if (_session == nullptr)
	{
		return;
	}

	PSharedPtr<PGameplaySession> session = _session;
	_session.Reset();
	session->OnTravelRequested.Remove(_travelHandle);
	session->UnbindGameMaster();
	session->Close(reason);

	// 정상 종료: 남은 데이터를 보내고 상대가 닫을 때까지 조금 더 돌린다.
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point last  = begin;
	while (session->HasPendingSends() == true)
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (std::chrono::duration<float32>(now - begin).count() > SessionFlushSeconds)
		{
			JG_LOG(GameFrameWorks, ELogLevel::Warning, "JGGameInstance: session closed with unsent data");
			break;
		}
		session->Tick(std::chrono::duration<float32>(now - last).count());
		last = now;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void JGGameInstance::onTravelRequested(const PName& world)
{
	// 클라: 호스트의 월드를 따라간다. 입장할 때 이미 같은 월드에 GameMaster 가 붙어 있으면 그대로 둔다.
	if (_session == nullptr || world == NAME_NONE)
	{
		return;
	}
	if (_world != nullptr && _world->GetName() == world && _session->GetGameMaster() != nullptr)
	{
		return;
	}

	// 로드하기 전까지 온 게임 메시지(시작 · 문서 · 승인)는 세션이 보관했다가 새 월드의 GameMaster 가 붙을 때 적용한다.
	_session->UnbindGameMaster();
	_pendingTravel  = world;
	_bTravelPending = true;
}
