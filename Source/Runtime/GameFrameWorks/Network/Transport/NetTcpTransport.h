#pragma once
#include "Network/Transport/NetTransport.h"

// Winsock TCP 전송. 비블로킹 소켓을 Poll 에서 처리한다 (스레드 없음).
// 메시지 경계는 [uint32 길이(리틀 엔디언)][본문] 프레임으로 지킨다. IPv4 만.
// 헤더에는 Winsock 을 넣지 않는다 (소켓 값은 uint64 로 보관).
class GAMEFRAMEWORKS_API PNetTcpTransport : public INetTransport
{
	struct HConnection
	{
		HNetPeerId   Peer           = NetPeerNone;
		uint64       Socket         = ~0ull;
		bool         bConnecting    = false;
		bool         bClosing       = false;   // Close 요청. 다 보낸 뒤 송신 종료 → 상대가 닫을 때까지 읽어 버린다
		bool         bShutdownSent  = false;
		bool         bDead          = false;
		HList<uint8> Received;
		uint64       ReceivedOffset = 0;
		HList<uint8> Outgoing;
		uint64       OutgoingOffset = 0;
	};

	uint64             _listenSocket = ~0ull;
	PString            _listenAddress;
	HList<HConnection> _connections;
	HList<HNetEvent>   _deferredEvents;   // Send 도중 발견한 끊김. 다음 Poll 에 낸다
	HNetPeerId         _nextPeer     = 1;
	bool               _bWinsockReady = false;

public:
	// 한 메시지의 최대 크기. 넘으면 연결 오류로 본다.
	static constexpr uint32 MaxMessageBytes = 64u * 1024u * 1024u;

	PNetTcpTransport();
	virtual ~PNetTcpTransport();

	// Listen 이 묶을 주소. 비우면 모든 인터페이스(LAN 에서 접속받는 기본값).
	// 한 기계 안 테스트는 127.0.0.1 로 묶는다 (모든 인터페이스로 열면 Windows 방화벽 확인 창이 뜰 수 있다).
	void SetListenAddress(const PString& address);

	virtual bool       Listen(uint16 port) override;
	virtual HNetPeerId Connect(const PString& address, uint16 port) override;
	virtual bool       Send(HNetPeerId peer, const HList<uint8>& data) override;
	virtual void       Poll(HList<HNetEvent>& outEvents) override;
	virtual void       Close(HNetPeerId peer) override;
	virtual void       Shutdown() override;
	virtual bool       HasPendingSends() const override;

private:
	bool         startWinsock();
	HConnection* find(HNetPeerId peer);
	void acceptPending(HList<HNetEvent>& outEvents);
	void updateConnecting(HList<HNetEvent>& outEvents);
	void receive(HConnection& connection, HList<HNetEvent>& outEvents);
	bool flush(HConnection& connection);
	void fail(HConnection& connection, ENetDisconnectReason reason, HList<HNetEvent>& outEvents);
	void closeSocket(HConnection& connection);
	void removeDead();
};
