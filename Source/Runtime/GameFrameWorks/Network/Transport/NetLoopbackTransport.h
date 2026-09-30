#pragma once
#include "Network/Transport/NetTransport.h"

// 한 프로세스 안의 가상 네트워크. 호스트 1 + 클라 여럿을 한 프로세스에서 돌리는 테스트용.
// 포트 번호로 Listen · Connect 하고, 메시지는 받는 쪽 엔드포인트의 받은 편지함에 순서대로 쌓인다.
class GAMEFRAMEWORKS_API PNetLoopbackHub : public IMemoryObject
{
	struct HPacket
	{
		HNetEvent Event;
		int32     ReadyAtPoll = 0;   // 받는 쪽 Poll 횟수가 이 값에 이르면 전달
	};

	struct HEndpoint
	{
		bool            bAlive     = false;
		uint16          ListenPort = 0;
		int32           PollCount  = 0;
		HNetPeerId      NextPeer   = 1;
		HDeque<HPacket> Inbox;
	};

	struct HLink
	{
		bool       bOpen     = false;
		int32      EndpointA = INDEX_NONE;
		HNetPeerId PeerA     = NetPeerNone;
		int32      EndpointB = INDEX_NONE;
		HNetPeerId PeerB     = NetPeerNone;
	};

	HList<HEndpoint> _endpoints;
	HList<HLink>     _links;
	int32            _delayPolls = 0;

public:
	PNetLoopbackHub() = default;
	virtual ~PNetLoopbackHub() = default;

	int32 AddEndpoint();
	void  RemoveEndpoint(int32 endpoint);

	bool       Listen(int32 endpoint, uint16 port);
	HNetPeerId Connect(int32 endpoint, uint16 port);
	bool       Send(int32 endpoint, HNetPeerId peer, const HList<uint8>& data);
	void       Poll(int32 endpoint, HList<HNetEvent>& outEvents);
	void       Close(int32 endpoint, HNetPeerId peer);
	bool       HasPendingFrom(int32 endpoint) const;

	// 테스트: 받는 쪽 Poll 몇 번 뒤에 도착할지. 0 이면 다음 Poll.
	void SetDelayPolls(int32 polls);
	// 테스트: 연결 유실. 양쪽 모두 Disconnected(Error).
	void Break(int32 endpoint, HNetPeerId peer);

private:
	int32 findLink(int32 endpoint, HNetPeerId peer) const;
	void  push(int32 endpoint, const HNetEvent& event);
	void  closeLink(int32 linkIndex, ENetDisconnectReason reasonForA, ENetDisconnectReason reasonForB, bool bNotifyA, bool bNotifyB);
};

class GAMEFRAMEWORKS_API PNetLoopbackTransport : public INetTransport
{
	PSharedPtr<PNetLoopbackHub> _hub;
	int32                       _endpoint = INDEX_NONE;

public:
	explicit PNetLoopbackTransport(PSharedPtr<PNetLoopbackHub> hub);
	virtual ~PNetLoopbackTransport();

	virtual bool       Listen(uint16 port) override;
	virtual HNetPeerId Connect(const PString& address, uint16 port) override;
	virtual bool       Send(HNetPeerId peer, const HList<uint8>& data) override;
	virtual void       Poll(HList<HNetEvent>& outEvents) override;
	virtual void       Close(HNetPeerId peer) override;
	virtual void       Shutdown() override;
	virtual bool       HasPendingSends() const override;

	PSharedPtr<PNetLoopbackHub> GetHub() const;
	int32                       GetEndpoint() const;
};
