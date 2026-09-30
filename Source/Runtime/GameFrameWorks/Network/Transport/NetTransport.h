#pragma once
#include "Core/GameFrameWorksDefines.h"

// 전송 계층. 연결마다 신뢰 · 순서가 보장되는 메시지(바이트 묶음)를 주고받는다.
// 게임을 모른다. 세션은 이 인터페이스만 보므로 TCP 인지 루프백인지 모른다. 전부 메인 스레드에서 Poll 로 진행한다.

// 연결 번호. 전송 객체마다 1 부터 매긴다. 0 은 없음.
using HNetPeerId = uint32;
constexpr HNetPeerId NetPeerNone = 0;

enum class ENetEventType : int32
{
	Connected = 0,     // 받은 연결(호스트) 또는 연결 완료(클라)
	Disconnected,      // 끊김. Reason 참고
	Message,           // 메시지 하나. Data 에 바이트
};

enum class ENetDisconnectReason : int32
{
	None = 0,
	Closed,            // 상대가 정상 종료
	Error,             // 소켓 오류 · 연결 유실
	Refused,           // 연결 실패
};

struct GAMEFRAMEWORKS_API HNetEvent
{
	ENetEventType        Type   = ENetEventType::Message;
	HNetPeerId           Peer   = NetPeerNone;
	ENetDisconnectReason Reason = ENetDisconnectReason::None;
	HList<uint8>         Data;
};

class GAMEFRAMEWORKS_API INetTransport : public IMemoryObject
{
public:
	virtual ~INetTransport() = default;

	// 호스트: 포트에서 연결을 받는다.
	virtual bool Listen(uint16 port) = 0;
	// 클라: 연결을 시작한다. 성공하면 Poll 이 Connected 를, 실패하면 Disconnected(Refused) 를 낸다.
	// 주소를 해석하지 못하면 NetPeerNone.
	virtual HNetPeerId Connect(const PString& address, uint16 port) = 0;

	virtual bool Send(HNetPeerId peer, const HList<uint8>& data) = 0;
	virtual void Poll(HList<HNetEvent>& outEvents) = 0;

	// 보낼 것을 다 보낸 뒤 닫는다. 상대는 남은 메시지를 모두 받은 다음 Disconnected(Closed) 를 받는다. 이쪽에는 이벤트가 없다.
	virtual void Close(HNetPeerId peer) = 0;
	// 모든 연결과 대기 소켓을 즉시 닫는다.
	virtual void Shutdown() = 0;

	// 아직 내보내지 못한 데이터나 닫는 중인 연결이 있는가. 정상 종료 전에 Poll 을 더 돌릴지 판단한다.
	virtual bool HasPendingSends() const = 0;
};
