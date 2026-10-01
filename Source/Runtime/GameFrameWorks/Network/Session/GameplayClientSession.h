#pragma once
#include "Network/Session/GameplaySession.h"

// 클라 세션. 로컬 명령을 호스트로 보내고, 호스트가 승인한 명령을 받은 순서대로 복제 GameMaster 에 적용한다.
// 적용 뒤 순번 · 체크섬이 호스트와 다르면 양쪽 상태를 덤프하고 문서를 다시 받는다.
// GameMaster 가 붙기 전(월드 로드 중)에 온 게임 메시지는 보관했다가 붙을 때 적용한다.
class GAMEFRAMEWORKS_API PGameplayClientSession : public PGameplaySession
{
	struct HHeldMessage
	{
		EGameplayNetMessage Type = EGameplayNetMessage::None;
		HRawString          Text;
	};

	PSharedPtr<INetTransport> _transport;
	PString                   _address;
	uint16                    _port            = 0;
	HNetPeerId                _hostPeer        = NetPeerNone;
	PString                   _token;
	uint64                    _hostFingerprint = 0;

	HDeque<HHeldMessage>      _held;
	HDeque<HGameplayCommand>  _localQueue;
	bool                      _bInFlight     = false;
	uint32                    _inFlightSeq   = 0;
	HGameplayCommand          _inFlightCommand;
	uint32                    _nextClientSeq = 1;

	uint32                    _lastAppliedSeq      = 0;
	int32                     _desyncCount         = 0;
	uint32                    _desyncSeq           = 0;
	bool                      _bDumpHostOnDocument = false;
	float32                   _lastHeard           = 0.0f;
	float32                   _pingTimer           = 0.0f;

public:
	PGameplayClientSession() = default;
	virtual ~PGameplayClientSession();

	// 접속을 시작한다. 주소를 해석하지 못하면 Closed 상태로 돌려준다.
	static PSharedPtr<PGameplayClientSession> Create(PSharedPtr<INetTransport> transport, const PString& address, uint16 port, const HGameplaySessionConfig& config = HGameplaySessionConfig());

	virtual EGameplaySessionSubmit SubmitLocal(const HGameplayCommand& command, PString* outReason = nullptr) override;
	virtual bool StartGame(uint64 seed) override;
	virtual void Tick(float32 deltaSeconds) override;
	virtual void Close(const PString& reason) override;
	virtual bool HasPendingSends() const override;

	// 끊긴 뒤 같은 토큰으로 다시 접속한다. 호스트가 같은 슬롯을 돌려주고 문서를 보낸다.
	bool Reconnect();

	int32  GetDesyncCount() const;
	uint32 GetLastAppliedSeq() const;
	bool   HasPendingLocalCommand() const;
	uint64 GetHostFingerprint() const;

protected:
	virtual void onBound() override;
	virtual void onUnbound() override;

private:
	void handleEvent(const HNetEvent& event);
	void handleMessage(EGameplayNetMessage type, const HRawString& text);
	bool needsGameMaster(EGameplayNetMessage type) const;
	void applyStartGame(const HRawString& text);
	void applyAccepted(const HRawString& text);
	void applyDocument(const HRawString& text);

	void sendHello();
	bool sendLocal(const HGameplayCommand& command, PString* outReason);
	void sendNextQueued();
	void beginResync(uint32 seq);
	void dumpState(const char* tag, const HGameplayState& state) const;
	void send(const HList<uint8>& bytes);
	void fail(const PString& reason);
};
