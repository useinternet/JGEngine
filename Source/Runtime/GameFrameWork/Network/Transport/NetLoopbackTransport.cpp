#include "PCH/PCH.h"
#include "Network/Transport/NetLoopbackTransport.h"

// ---- 허브 ----------------------------------------------------------------------

int32 PNetLoopbackHub::AddEndpoint()
{
	HEndpoint endpoint;
	endpoint.bAlive = true;
	_endpoints.push_back(endpoint);
	return (int32)_endpoints.size() - 1;
}

void PNetLoopbackHub::RemoveEndpoint(int32 endpoint)
{
	if (endpoint < 0 || endpoint >= (int32)_endpoints.size())
	{
		return;
	}

	int32 linkCount = (int32)_links.size();
	for (int32 i = 0; i < linkCount; ++i)
	{
		HLink& link = _links[i];
		if (link.bOpen == false)
		{
			continue;
		}
		if (link.EndpointA == endpoint)
		{
			closeLink(i, ENetDisconnectReason::Closed, ENetDisconnectReason::Closed, false, true);
		}
		else if (link.EndpointB == endpoint)
		{
			closeLink(i, ENetDisconnectReason::Closed, ENetDisconnectReason::Closed, true, false);
		}
	}

	HEndpoint& removed = _endpoints[endpoint];
	removed.bAlive     = false;
	removed.ListenPort = 0;
	removed.Inbox.clear();
}

bool PNetLoopbackHub::Listen(int32 endpoint, uint16 port)
{
	if (endpoint < 0 || endpoint >= (int32)_endpoints.size() || port == 0)
	{
		return false;
	}
	for (const HEndpoint& other : _endpoints)
	{
		if (other.bAlive == true && other.ListenPort == port)
		{
			return false;
		}
	}
	_endpoints[endpoint].ListenPort = port;
	return true;
}

HNetPeerId PNetLoopbackHub::Connect(int32 endpoint, uint16 port)
{
	if (endpoint < 0 || endpoint >= (int32)_endpoints.size())
	{
		return NetPeerNone;
	}

	HEndpoint& self = _endpoints[endpoint];
	HNetPeerId selfPeer = self.NextPeer++;

	int32 listener = INDEX_NONE;
	int32 count = (int32)_endpoints.size();
	for (int32 i = 0; i < count; ++i)
	{
		if (_endpoints[i].bAlive == true && _endpoints[i].ListenPort == port && i != endpoint)
		{
			listener = i;
			break;
		}
	}

	if (listener == INDEX_NONE)
	{
		HNetEvent refused;
		refused.Type   = ENetEventType::Disconnected;
		refused.Peer   = selfPeer;
		refused.Reason = ENetDisconnectReason::Refused;
		push(endpoint, refused);
		return selfPeer;
	}

	HLink link;
	link.bOpen     = true;
	link.EndpointA = endpoint;
	link.PeerA     = selfPeer;
	link.EndpointB = listener;
	link.PeerB     = _endpoints[listener].NextPeer++;
	_links.push_back(link);

	HNetEvent connectedA;
	connectedA.Type = ENetEventType::Connected;
	connectedA.Peer = link.PeerA;
	push(link.EndpointA, connectedA);

	HNetEvent connectedB;
	connectedB.Type = ENetEventType::Connected;
	connectedB.Peer = link.PeerB;
	push(link.EndpointB, connectedB);

	return selfPeer;
}

bool PNetLoopbackHub::Send(int32 endpoint, HNetPeerId peer, const HList<uint8>& data)
{
	int32 linkIndex = findLink(endpoint, peer);
	if (linkIndex == INDEX_NONE)
	{
		return false;
	}

	const HLink& link = _links[linkIndex];
	HNetEvent message;
	message.Type = ENetEventType::Message;
	message.Data = data;
	if (link.EndpointA == endpoint)
	{
		message.Peer = link.PeerB;
		push(link.EndpointB, message);
	}
	else
	{
		message.Peer = link.PeerA;
		push(link.EndpointA, message);
	}
	return true;
}

void PNetLoopbackHub::Poll(int32 endpoint, HList<HNetEvent>& outEvents)
{
	if (endpoint < 0 || endpoint >= (int32)_endpoints.size())
	{
		return;
	}

	HEndpoint& self = _endpoints[endpoint];
	++self.PollCount;

	// 순서를 지키기 위해 아직 도착 시각이 안 된 첫 패킷에서 멈춘다.
	while (self.Inbox.empty() == false && self.Inbox.front().ReadyAtPoll <= self.PollCount)
	{
		outEvents.push_back(self.Inbox.front().Event);
		self.Inbox.pop_front();
	}
}

void PNetLoopbackHub::Close(int32 endpoint, HNetPeerId peer)
{
	int32 linkIndex = findLink(endpoint, peer);
	if (linkIndex == INDEX_NONE)
	{
		return;
	}

	bool bIsA = _links[linkIndex].EndpointA == endpoint;
	closeLink(linkIndex, ENetDisconnectReason::Closed, ENetDisconnectReason::Closed, bIsA == false, bIsA == true);
}

bool PNetLoopbackHub::HasPendingFrom(int32 endpoint) const
{
	// 보낸 것은 즉시 상대 편지함에 들어가므로 보낼 대기열이 없다.
	return false;
}

void PNetLoopbackHub::SetDelayPolls(int32 polls)
{
	_delayPolls = polls < 0 ? 0 : polls;
}

void PNetLoopbackHub::Break(int32 endpoint, HNetPeerId peer)
{
	int32 linkIndex = findLink(endpoint, peer);
	if (linkIndex == INDEX_NONE)
	{
		return;
	}
	closeLink(linkIndex, ENetDisconnectReason::Error, ENetDisconnectReason::Error, true, true);
}

int32 PNetLoopbackHub::findLink(int32 endpoint, HNetPeerId peer) const
{
	int32 count = (int32)_links.size();
	for (int32 i = 0; i < count; ++i)
	{
		const HLink& link = _links[i];
		if (link.bOpen == false)
		{
			continue;
		}
		if ((link.EndpointA == endpoint && link.PeerA == peer) || (link.EndpointB == endpoint && link.PeerB == peer))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void PNetLoopbackHub::push(int32 endpoint, const HNetEvent& event)
{
	if (endpoint < 0 || endpoint >= (int32)_endpoints.size())
	{
		return;
	}

	HEndpoint& target = _endpoints[endpoint];
	if (target.bAlive == false)
	{
		return;
	}

	HPacket packet;
	packet.Event       = event;
	packet.ReadyAtPoll = target.PollCount + 1 + _delayPolls;
	target.Inbox.push_back(packet);
}

void PNetLoopbackHub::closeLink(int32 linkIndex, ENetDisconnectReason reasonForA, ENetDisconnectReason reasonForB, bool bNotifyA, bool bNotifyB)
{
	HLink& link = _links[linkIndex];
	if (link.bOpen == false)
	{
		return;
	}
	link.bOpen = false;

	if (bNotifyA == true)
	{
		HNetEvent disconnected;
		disconnected.Type   = ENetEventType::Disconnected;
		disconnected.Peer   = link.PeerA;
		disconnected.Reason = reasonForA;
		push(link.EndpointA, disconnected);
	}
	if (bNotifyB == true)
	{
		HNetEvent disconnected;
		disconnected.Type   = ENetEventType::Disconnected;
		disconnected.Peer   = link.PeerB;
		disconnected.Reason = reasonForB;
		push(link.EndpointB, disconnected);
	}
}

// ---- 전송 ----------------------------------------------------------------------

PNetLoopbackTransport::PNetLoopbackTransport(PSharedPtr<PNetLoopbackHub> hub)
	: _hub(hub)
{
	if (_hub != nullptr)
	{
		_endpoint = _hub->AddEndpoint();
	}
}

PNetLoopbackTransport::~PNetLoopbackTransport()
{
	Shutdown();
}

bool PNetLoopbackTransport::Listen(uint16 port)
{
	if (_hub == nullptr)
	{
		return false;
	}
	return _hub->Listen(_endpoint, port);
}

HNetPeerId PNetLoopbackTransport::Connect(const PString& address, uint16 port)
{
	if (_hub == nullptr)
	{
		return NetPeerNone;
	}
	return _hub->Connect(_endpoint, port);
}

bool PNetLoopbackTransport::Send(HNetPeerId peer, const HList<uint8>& data)
{
	if (_hub == nullptr)
	{
		return false;
	}
	return _hub->Send(_endpoint, peer, data);
}

void PNetLoopbackTransport::Poll(HList<HNetEvent>& outEvents)
{
	if (_hub == nullptr)
	{
		return;
	}
	_hub->Poll(_endpoint, outEvents);
}

void PNetLoopbackTransport::Close(HNetPeerId peer)
{
	if (_hub == nullptr)
	{
		return;
	}
	_hub->Close(_endpoint, peer);
}

void PNetLoopbackTransport::Shutdown()
{
	if (_hub == nullptr || _endpoint == INDEX_NONE)
	{
		return;
	}
	_hub->RemoveEndpoint(_endpoint);
	_endpoint = INDEX_NONE;
}

bool PNetLoopbackTransport::HasPendingSends() const
{
	if (_hub == nullptr)
	{
		return false;
	}
	return _hub->HasPendingFrom(_endpoint);
}

PSharedPtr<PNetLoopbackHub> PNetLoopbackTransport::GetHub() const
{
	return _hub;
}

int32 PNetLoopbackTransport::GetEndpoint() const
{
	return _endpoint;
}
