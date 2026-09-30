#include "PCH/PCH.h"
#include "Network/Transport/NetTcpTransport.h"

// PCH 가 Windows.h 로 Winsock 1.1(winsock.h)을 이미 들여온다. 그 뒤에 winsock2.h 를 넣으면 재정의 오류가 난다
// (Document/Memory/Server/Files/2026-09-29_winsock_include_probe.txt). 여기서 쓰는 함수는 1.1 에 모두 있어 공유 PCH 를 고치지 않는다.
// Windows.h 가 winsock.h 를 들여오지 않는 설정(WIN32_LEAN_AND_MEAN)이면 winsock2.h 를 쓴다.
#if !defined(_WINSOCKAPI_)
#include <winsock2.h>
#endif
#pragma comment(lib, "ws2_32.lib")

#ifndef TCP_NODELAY
#define TCP_NODELAY 0x0001
#endif
#ifndef SD_SEND
#define SD_SEND 0x01   // winsock2.h 에만 있다
#endif

namespace
{
	constexpr uint64 InvalidSocketValue = ~0ull;
	constexpr int32  ReceiveChunkBytes  = 16 * 1024;
	constexpr int32  SendChunkBytes     = 1024 * 1024;
	constexpr uint64 CompactThreshold   = 64 * 1024;

	SOCKET toSocket(uint64 value)
	{
		return (SOCKET)value;
	}

	uint64 fromSocket(SOCKET socketHandle)
	{
		return (uint64)socketHandle;
	}

	bool setNonBlocking(SOCKET socketHandle)
	{
		u_long mode = 1;
		return ioctlsocket(socketHandle, FIONBIO, &mode) == 0;
	}

	void setNoDelay(SOCKET socketHandle)
	{
		// 작은 메시지(명령 · 승인)가 Nagle 로 묶여 늦게 나가지 않게 한다.
		BOOL flag = TRUE;
		setsockopt(socketHandle, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));
	}

	bool resolveIPv4(const PString& address, in_addr* outAddress)
	{
		const char* text = address.GetCStr();
		unsigned long numeric = inet_addr(text);
		if (numeric != INADDR_NONE)
		{
			outAddress->s_addr = numeric;
			return true;
		}

		hostent* host = gethostbyname(text);
		if (host == nullptr || host->h_addrtype != AF_INET || host->h_addr_list == nullptr || host->h_addr_list[0] == nullptr)
		{
			return false;
		}
		memcpy(&outAddress->s_addr, host->h_addr_list[0], sizeof(outAddress->s_addr));
		return true;
	}

	void appendFrame(HList<uint8>& outBytes, const HList<uint8>& data)
	{
		uint32 size = (uint32)data.size();
		outBytes.push_back((uint8)(size & 0xff));
		outBytes.push_back((uint8)((size >> 8) & 0xff));
		outBytes.push_back((uint8)((size >> 16) & 0xff));
		outBytes.push_back((uint8)((size >> 24) & 0xff));
		outBytes.insert(outBytes.end(), data.begin(), data.end());
	}
}

PNetTcpTransport::PNetTcpTransport()
{
}

PNetTcpTransport::~PNetTcpTransport()
{
	Shutdown();
	if (_bWinsockReady == true)
	{
		WSACleanup();
		_bWinsockReady = false;
	}
}

void PNetTcpTransport::SetListenAddress(const PString& address)
{
	_listenAddress = address;
}

bool PNetTcpTransport::Listen(uint16 port)
{
	if (startWinsock() == false)
	{
		return false;
	}

	if (_listenSocket != InvalidSocketValue)
	{
		closesocket(toSocket(_listenSocket));
		_listenSocket = InvalidSocketValue;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: socket failed (WSA %d)", WSAGetLastError());
		return false;
	}

	sockaddr_in address = {};
	address.sin_family      = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port        = htons(port);
	if (_listenAddress.Empty() == false && resolveIPv4(_listenAddress, &address.sin_addr) == false)
	{
		JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: cannot resolve listen address %s", _listenAddress);
		closesocket(listenSocket);
		return false;
	}

	if (bind(listenSocket, (const sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: bind port %u failed (WSA %d)", (uint32)port, WSAGetLastError());
		closesocket(listenSocket);
		return false;
	}
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: listen failed (WSA %d)", WSAGetLastError());
		closesocket(listenSocket);
		return false;
	}
	setNonBlocking(listenSocket);

	_listenSocket = fromSocket(listenSocket);
	JG_LOG(Network, ELogLevel::Info, "PNetTcpTransport: listening on port %u", (uint32)port);
	return true;
}

HNetPeerId PNetTcpTransport::Connect(const PString& address, uint16 port)
{
	if (startWinsock() == false)
	{
		return NetPeerNone;
	}

	sockaddr_in target = {};
	target.sin_family = AF_INET;
	target.sin_port   = htons(port);
	if (resolveIPv4(address, &target.sin_addr) == false)
	{
		JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: cannot resolve %s", address);
		return NetPeerNone;
	}

	SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSocket == INVALID_SOCKET)
	{
		JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: socket failed (WSA %d)", WSAGetLastError());
		return NetPeerNone;
	}
	setNonBlocking(connectSocket);
	setNoDelay(connectSocket);

	if (connect(connectSocket, (const sockaddr*)&target, sizeof(target)) == SOCKET_ERROR)
	{
		int32 error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: connect %s:%u failed (WSA %d)", address, (uint32)port, error);
			closesocket(connectSocket);
			return NetPeerNone;
		}
	}

	// 바로 연결돼도 Poll 의 select 가 쓰기 가능으로 알려 준다. 완료 처리는 한 곳에서 한다.
	HConnection connection;
	connection.Peer        = _nextPeer++;
	connection.Socket      = fromSocket(connectSocket);
	connection.bConnecting = true;
	_connections.push_back(connection);
	return connection.Peer;
}

bool PNetTcpTransport::Send(HNetPeerId peer, const HList<uint8>& data)
{
	if (data.size() > MaxMessageBytes)
	{
		JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: message of %u bytes exceeds the limit", (uint32)data.size());
		return false;
	}

	HConnection* connection = find(peer);
	if (connection == nullptr || connection->bDead == true || connection->bClosing == true)
	{
		return false;
	}

	appendFrame(connection->Outgoing, data);
	if (connection->bConnecting == false)
	{
		if (flush(*connection) == false)
		{
			fail(*connection, ENetDisconnectReason::Error, _deferredEvents);
			return false;
		}
	}
	return true;
}

void PNetTcpTransport::Poll(HList<HNetEvent>& outEvents)
{
	if (_deferredEvents.empty() == false)
	{
		outEvents.insert(outEvents.end(), _deferredEvents.begin(), _deferredEvents.end());
		_deferredEvents.clear();
	}

	acceptPending(outEvents);
	updateConnecting(outEvents);

	for (HConnection& connection : _connections)
	{
		if (connection.bDead == true || connection.bConnecting == true)
		{
			continue;
		}

		if (connection.bClosing == true)
		{
			// 정상 종료: 다 보냄 → 송신 종료 → 상대가 닫을 때까지 받은 것을 버린다.
			// 받지 않은 데이터가 남은 채 닫으면 RST 가 나가 상대가 아직 못 읽은 데이터를 잃을 수 있다.
			if (connection.bShutdownSent == false)
			{
				if (flush(connection) == false)
				{
					closeSocket(connection);
					connection.bDead = true;
					continue;
				}
				if (connection.OutgoingOffset < connection.Outgoing.size())
				{
					continue;
				}
				shutdown(toSocket(connection.Socket), SD_SEND);
				connection.bShutdownSent = true;
			}

			char discard[ReceiveChunkBytes];
			while (true)
			{
				int32 received = recv(toSocket(connection.Socket), discard, ReceiveChunkBytes, 0);
				if (received > 0)
				{
					continue;
				}
				if (received == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
				{
					break;
				}
				closeSocket(connection);
				connection.bDead = true;
				break;
			}
			continue;
		}

		receive(connection, outEvents);
		if (connection.bDead == true)
		{
			continue;
		}
		if (flush(connection) == false)
		{
			fail(connection, ENetDisconnectReason::Error, outEvents);
		}
	}

	removeDead();
}

void PNetTcpTransport::Close(HNetPeerId peer)
{
	HConnection* connection = find(peer);
	if (connection == nullptr || connection->bDead == true)
	{
		return;
	}
	if (connection->bConnecting == true)
	{
		closeSocket(*connection);
		connection->bDead = true;
		return;
	}
	connection->bClosing = true;
}

void PNetTcpTransport::Shutdown()
{
	for (HConnection& connection : _connections)
	{
		closeSocket(connection);
	}
	_connections.clear();
	_deferredEvents.clear();

	if (_listenSocket != InvalidSocketValue)
	{
		closesocket(toSocket(_listenSocket));
		_listenSocket = InvalidSocketValue;
	}
}

bool PNetTcpTransport::HasPendingSends() const
{
	for (const HConnection& connection : _connections)
	{
		if (connection.bDead == true)
		{
			continue;
		}
		if (connection.bClosing == true || connection.OutgoingOffset < connection.Outgoing.size())
		{
			return true;
		}
	}
	return false;
}

bool PNetTcpTransport::startWinsock()
{
	if (_bWinsockReady == true)
	{
		return true;
	}

	// WSAStartup 은 Windows 가 호출 횟수를 센다. 전송 객체마다 한 번 시작하고 소멸자에서 한 번 정리한다.
	WSADATA data;
	int32 result = WSAStartup(MAKEWORD(2, 2), &data);
	if (result != 0)
	{
		JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: WSAStartup failed (%d)", result);
		return false;
	}
	_bWinsockReady = true;
	return true;
}

PNetTcpTransport::HConnection* PNetTcpTransport::find(HNetPeerId peer)
{
	for (HConnection& connection : _connections)
	{
		if (connection.Peer == peer)
		{
			return &connection;
		}
	}
	return nullptr;
}

void PNetTcpTransport::acceptPending(HList<HNetEvent>& outEvents)
{
	if (_listenSocket == InvalidSocketValue)
	{
		return;
	}

	while (true)
	{
		SOCKET accepted = accept(toSocket(_listenSocket), nullptr, nullptr);
		if (accepted == INVALID_SOCKET)
		{
			break;
		}
		setNonBlocking(accepted);
		setNoDelay(accepted);

		HConnection connection;
		connection.Peer   = _nextPeer++;
		connection.Socket = fromSocket(accepted);
		_connections.push_back(connection);

		HNetEvent connected;
		connected.Type = ENetEventType::Connected;
		connected.Peer = connection.Peer;
		outEvents.push_back(connected);
	}
}

void PNetTcpTransport::updateConnecting(HList<HNetEvent>& outEvents)
{
	fd_set writeSet;
	fd_set exceptSet;
	FD_ZERO(&writeSet);
	FD_ZERO(&exceptSet);

	int32 count = 0;
	for (const HConnection& connection : _connections)
	{
		if (connection.bConnecting == false || connection.bDead == true)
		{
			continue;
		}
		if (count >= FD_SETSIZE)
		{
			break;
		}
		FD_SET(toSocket(connection.Socket), &writeSet);
		FD_SET(toSocket(connection.Socket), &exceptSet);
		++count;
	}
	if (count == 0)
	{
		return;
	}

	timeval zero = {};
	if (select(0, nullptr, &writeSet, &exceptSet, &zero) == SOCKET_ERROR)
	{
		JG_LOG(Network, ELogLevel::Warning, "PNetTcpTransport: select failed (WSA %d)", WSAGetLastError());
		return;
	}

	for (HConnection& connection : _connections)
	{
		if (connection.bConnecting == false || connection.bDead == true)
		{
			continue;
		}

		SOCKET connectSocket = toSocket(connection.Socket);
		if (FD_ISSET(connectSocket, &exceptSet))
		{
			fail(connection, ENetDisconnectReason::Refused, outEvents);
			continue;
		}
		if (FD_ISSET(connectSocket, &writeSet))
		{
			connection.bConnecting = false;

			HNetEvent connected;
			connected.Type = ENetEventType::Connected;
			connected.Peer = connection.Peer;
			outEvents.push_back(connected);

			if (flush(connection) == false)
			{
				fail(connection, ENetDisconnectReason::Error, outEvents);
			}
		}
	}
}

void PNetTcpTransport::receive(HConnection& connection, HList<HNetEvent>& outEvents)
{
	char buffer[ReceiveChunkBytes];
	bool bRemoteClosed = false;
	bool bError        = false;

	while (true)
	{
		int32 received = recv(toSocket(connection.Socket), buffer, ReceiveChunkBytes, 0);
		if (received > 0)
		{
			connection.Received.insert(connection.Received.end(), (const uint8*)buffer, (const uint8*)buffer + received);
			continue;
		}
		if (received == 0)
		{
			bRemoteClosed = true;
			break;
		}
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			bError = true;
		}
		break;
	}

	// 끊김을 알리기 전에 이미 받은 메시지를 먼저 낸다.
	while (true)
	{
		uint64 available = connection.Received.size() - connection.ReceivedOffset;
		if (available < 4)
		{
			break;
		}

		const uint8* cursor = connection.Received.data() + connection.ReceivedOffset;
		uint32 size = (uint32)cursor[0] | ((uint32)cursor[1] << 8) | ((uint32)cursor[2] << 16) | ((uint32)cursor[3] << 24);
		if (size > MaxMessageBytes)
		{
			JG_LOG(Network, ELogLevel::Error, "PNetTcpTransport: frame of %u bytes exceeds the limit", size);
			fail(connection, ENetDisconnectReason::Error, outEvents);
			return;
		}
		if (available < 4 + (uint64)size)
		{
			break;
		}

		HNetEvent message;
		message.Type = ENetEventType::Message;
		message.Peer = connection.Peer;
		message.Data.assign(cursor + 4, cursor + 4 + size);
		outEvents.push_back(message);

		connection.ReceivedOffset += 4 + (uint64)size;
	}

	if (connection.ReceivedOffset > 0)
	{
		if (connection.ReceivedOffset == connection.Received.size())
		{
			connection.Received.clear();
			connection.ReceivedOffset = 0;
		}
		else if (connection.ReceivedOffset > CompactThreshold)
		{
			connection.Received.erase(connection.Received.begin(), connection.Received.begin() + (int64)connection.ReceivedOffset);
			connection.ReceivedOffset = 0;
		}
	}

	if (bError == true)
	{
		fail(connection, ENetDisconnectReason::Error, outEvents);
	}
	else if (bRemoteClosed == true)
	{
		fail(connection, ENetDisconnectReason::Closed, outEvents);
	}
}

bool PNetTcpTransport::flush(HConnection& connection)
{
	while (connection.OutgoingOffset < connection.Outgoing.size())
	{
		uint64 remaining = connection.Outgoing.size() - connection.OutgoingOffset;
		int32 chunk = remaining > (uint64)SendChunkBytes ? SendChunkBytes : (int32)remaining;
		int32 sent = send(toSocket(connection.Socket), (const char*)connection.Outgoing.data() + connection.OutgoingOffset, chunk, 0);
		if (sent > 0)
		{
			connection.OutgoingOffset += (uint64)sent;
			continue;
		}
		if (sent == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
		{
			break;
		}
		return false;
	}

	if (connection.OutgoingOffset >= connection.Outgoing.size())
	{
		connection.Outgoing.clear();
		connection.OutgoingOffset = 0;
	}
	return true;
}

void PNetTcpTransport::fail(HConnection& connection, ENetDisconnectReason reason, HList<HNetEvent>& outEvents)
{
	if (connection.bDead == true)
	{
		return;
	}
	closeSocket(connection);
	connection.bDead = true;

	HNetEvent disconnected;
	disconnected.Type   = ENetEventType::Disconnected;
	disconnected.Peer   = connection.Peer;
	disconnected.Reason = reason;
	outEvents.push_back(disconnected);
}

void PNetTcpTransport::closeSocket(HConnection& connection)
{
	if (connection.Socket == InvalidSocketValue)
	{
		return;
	}
	closesocket(toSocket(connection.Socket));
	connection.Socket = InvalidSocketValue;
}

void PNetTcpTransport::removeDead()
{
	for (auto iter = _connections.begin(); iter != _connections.end();)
	{
		if ((*iter).bDead == true)
		{
			iter = _connections.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}
