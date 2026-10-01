#pragma once
#include "Network/Session/GameplaySession.h"

// 호스트 세션. Standalone(전송 없음, 싱글플레이) · ListenServer(권한 + 로컬 플레이어).
// 로컬 입력 · 원격 요청 · AI 명령이 모두 executeAuthoritative 한 곳에서 실행된다. 그 실행 순서가 곧 모든 기계의 명령 순서다.
class GAMEFRAMEWORKS_API PGameplayHostSession : public PGameplaySession
{
	struct HPendingPeer
	{
		HNetPeerId Peer        = NetPeerNone;
		float32    ConnectedAt = 0.0f;
	};

	PSharedPtr<INetTransport> _transport;
	HList<HPendingPeer>       _handshaking;
	float32                   _pingTimer            = 0.0f;
	int32                     _maxAgentStepsPerTick = 64;
	PName                     _world;
	bool                      _bGameAnnounced       = false;   // 붙은 GameMaster 의 시작을 참가자에게 알렸다 (StartGame 또는 이어받기)

public:
	PGameplayHostSession() = default;
	virtual ~PGameplayHostSession();

	static PSharedPtr<PGameplayHostSession> CreateStandalone(const HGameplaySessionConfig& config = HGameplaySessionConfig());
	// 포트를 열지 못하면 nullptr.
	static PSharedPtr<PGameplayHostSession> CreateListenServer(PSharedPtr<INetTransport> transport, uint16 port, const HGameplaySessionConfig& config = HGameplaySessionConfig());

	virtual EGameplaySessionSubmit SubmitLocal(const HGameplayCommand& command, PString* outReason = nullptr) override;
	virtual bool StartGame(uint64 seed) override;
	virtual void Tick(float32 deltaSeconds) override;
	virtual void Close(const PString& reason) override;
	virtual bool HasPendingSends() const override;

	// 슬롯이 조작할 조작 주체. 입장 전에 정해 두면 그 슬롯에 들어온 플레이어가 받는다. 기본은 { 슬롯 번호 }.
	void SetSlotControllers(int32 slot, const HList<int32>& controllers);
	// 호스트가 월드를 바꿨다. 클라도 같은 월드를 로드하게 알린다.
	void NotifyTravel(const PName& world);
	int32 GetConnectedPeerCount() const;
	// 한 Tick 에 AI 명령을 최대 몇 개 실행할지.
	void SetMaxAgentStepsPerTick(int32 steps);

protected:
	virtual void onBound() override;
	virtual void onUnbound() override;

private:
	EGameplaySessionSubmit executeAuthoritative(const HGameplayCommand& command, int32 originSlot, uint32 clientSeq, PString* outReason);
	void driveAgents();
	void adoptStartedGameMaster();

	void handleEvent(const HNetEvent& event);
	void handleHello(HNetPeerId peer, const HRawString& text);
	void handleSlotMessage(HGameplayPlayerSlot& slot, EGameplayNetMessage type, const HRawString& text);
	void refuse(HNetPeerId peer, const PString& reason);
	void markDisconnected(HGameplayPlayerSlot& slot);

	void sendTo(HNetPeerId peer, const HList<uint8>& bytes);
	void broadcast(const HList<uint8>& bytes);
	void broadcastSlots();
	void sendDocument(HNetPeerId peer);
	void updateTimers(float32 deltaSeconds);

	HGameplayPlayerSlot* findSlotByPeer(HNetPeerId peer);
	int32                findHandshaking(HNetPeerId peer) const;
	HGameplayPlayerSlot* reserveSlot(const PString& token);
};
