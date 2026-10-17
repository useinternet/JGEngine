#pragma once
#include "GameMaster/GameMaster.h"
#include "Network/Transport/NetTransport.h"
#include "Network/Messages/GameplayNetMessage.h"
#include "Network/Messages/GameplayPlayerSlot.h"

// 넷 모드. Standalone 과 ListenServer 는 호스트 세션, Client 는 클라 세션이 맡는다.
enum class EGameplayNetMode : int32
{
	Standalone = 0,    // 네트워크 없는 호스트 (싱글플레이)
	ListenServer,      // 권한 + 로컬 플레이어
	Client,            // 복제
};

enum class EGameplaySessionState : int32
{
	Connecting = 0,    // 클라: 연결 · 핸드셰이크 중
	Ready,             // 게임 시작 전
	Playing,           // 붙은 GameMaster 가 시작됐다
	Resyncing,         // 클라: 불일치. 문서를 기다린다
	Closed,
};

// SubmitLocal 결과.
enum class EGameplaySessionSubmit : int32
{
	Rejected = 0,
	Executed,          // 호스트: 실행 완료
	PendingChoice,     // 호스트: 선택 대기로 멈춤
	Sent,              // 클라: 호스트로 보냈다. 결과는 승인 · 거절 메시지로 온다
	Queued,            // 클라: 앞선 로컬 명령의 결과를 기다리는 중. 순서대로 보낸다
};

struct GAMEFRAMEWORKS_API HGameplaySessionConfig
{
	PString PlayerName          = "Player";
	int32   MaxPlayers          = 4;        // 호스트 슬롯 수 (로컬 플레이어 포함)
	bool    bLocalPlayer        = true;     // 호스트: 슬롯 0 을 로컬 플레이어로 쓴다. false 면 전용 서버처럼 연다
	float32 PingInterval        = 1.0f;
	float32 TimeoutSeconds      = 10.0f;
	PString DesyncDumpDirectory = "NetDesync";   // 비우면 불일치 덤프를 쓰지 않는다
	int32   MaxSessionDataBytes = 64 * 1024;     // 호스트: 세션 공유 데이터 전체(키 + 값 바이트 합). 넘는 SetSessionData 는 거부
	// 클라: 재접속 토큰(ER-026). 비우면 세션마다 새로 만든다. 창(프로세스)을 다시 띄워도 판 중 같은 자리로 돌아오려면
	// 게임이 PGameplayClientSession::GetReconnectToken() 을 저장했다가 여기에 넣는다. 1~128 바이트의 보이는 ASCII 만 쓴다(아니면 새 토큰).
	// 창마다 다른 토큰을 쓴다 — 같은 토큰으로 두 창이 들어오면 나중 창이 그 자리를 이어받고 앞 창은 끊긴다.
	PString ReconnectToken;
};

// 권한 경로의 유일한 입구. PGameMaster::Submit 은 세션만 부른다 (자체 테스트 제외).
//   입력(컨트롤러 · AI) → 세션 → GameMaster → 관찰자 → 연출. 연출은 권한 진행을 막지 않는다.
// 소유: 게임이 준 "행동자 → 조작 주체" 함수(PGameMaster::SetTeamOfActorFunction)로 행동자의 조작 주체를 구하고,
//   에이전트가 등록된 조작 주체는 AI(호스트가 구동), 아니면 그 조작 주체를 가진 슬롯의 플레이어다.
// 전부 메인 스레드. 게임 인스턴스 틱 맨 앞에서 Tick 한다.
class GAMEFRAMEWORKS_API PGameplaySession : public IMemoryObject
{
protected:
	EGameplayNetMode           _mode  = EGameplayNetMode::Standalone;
	EGameplaySessionState      _state = EGameplaySessionState::Ready;
	HGameplaySessionConfig     _config;
	PSharedPtr<PGameMaster>    _gameMaster;
	HList<HGameplayPlayerSlot> _slots;
	int32                      _localSlot = INDEX_NONE;
	float32                    _time      = 0.0f;
	PString                    _closeReason;
	HList<HGameplaySessionDataEntry> _sessionData;   // 넣은 순서. 키가 몇 개뿐이라 선형으로 찾는다

public:
	HMulticastDelegate<const HGameplayCommand&, const PString&> OnLocalCommandRejected;
	HMulticastDelegate<>                                        OnSlotsChanged;
	HMulticastDelegate<uint32>                                  OnDesync;             // 클라: 어긋난 순번
	HMulticastDelegate<int32>                                   OnPeerDisconnected;   // 호스트: 끊긴 슬롯
	HMulticastDelegate<const PString&>                          OnClosed;
	HMulticastDelegate<const PName&>                            OnTravelRequested;    // 클라: 호스트가 월드를 바꿨다
	HMulticastDelegate<const PName&>                            OnSessionDataChanged; // 세션 공유 데이터의 바뀐 키(지움 포함). 호스트도 자기가 바꾼 키를 받는다

public:
	PGameplaySession() = default;
	virtual ~PGameplaySession() = default;

	EGameplayNetMode      GetMode() const;
	EGameplaySessionState GetState() const;
	bool                  IsAuthority() const;
	const PString&        GetCloseReason() const;

	// GameMaster 연결. 붙어 있는 동안 참조를 쥔다. 네트워크 모드에서는 붙어 있는 동안 GameMaster 의 로컬 Undo 를 막는다.
	void BindGameMaster(PSharedPtr<PGameMaster> gameMaster);
	void UnbindGameMaster();
	PSharedPtr<PGameMaster> GetGameMaster() const;

	// 로컬 플레이어의 명령. 연출 쪽 입력 정책을 통과한 것만 넣는다.
	virtual EGameplaySessionSubmit SubmitLocal(const HGameplayCommand& command, PString* outReason = nullptr) = 0;
	// 권한 쪽만: 붙은 GameMaster 의 초기 상태로 시작하고 전원에게 알린다.
	virtual bool StartGame(uint64 seed) = 0;
	virtual void Tick(float32 deltaSeconds) = 0;
	virtual void Close(const PString& reason) = 0;
	// 정상 종료가 끝나지 않은 연결이 있는가 (Close 뒤 Tick 을 더 돌릴지).
	virtual bool HasPendingSends() const = 0;

	// 로컬 Undo. Standalone 에서만 된다 (한 기계만 되돌리면 즉시 어긋나므로).
	bool Undo();

	const HList<HGameplayPlayerSlot>& GetSlots() const;
	const HGameplayPlayerSlot*        FindSlot(int32 slot) const;
	int32                             GetLocalSlot() const;

	// 대기방 준비 표시. 판 시작 전(Ready)에만 된다 — 아니면 false. 클라는 호스트로 보내고, 바뀐 슬롯 표(OnSlotsChanged)로 돌아온다.
	// 판이 시작되거나 연결이 끊기면 지워진다. "전원 준비면 시작" 판단은 게임이 한다.
	virtual bool SetLocalReady(bool bReady) = 0;
	bool         IsLocalReady() const;

	// 세션 공유 데이터(방 정보): 호스트가 정한 키 → 값(PGameplayHostSession::SetSessionData). 클라는 Welcome(전체) · SessionData(바뀐 키)로 받는다.
	// 호스트 · 클라 · 싱글이 같은 함수로 읽는다. 월드 이동에도 남고, 새 세션(Host · Join · Leave)은 빈 표로 시작한다.
	// 게임 상태 밖이다(체크섬 · 문서 · 순번에 들어가지 않는다). 기계마다 도착 시점이 달라 규칙 코드는 읽지 않는다 — 판에 쓰는 값은 초기 상태에 넣는다.
	const PString*                          FindSessionData(const PName& key) const;   // 없으면 nullptr
	const HList<HGameplaySessionDataEntry>& GetSessionData() const;

	// 이 행동자를 로컬 플레이어가 조작하는가 (AI 가 맡은 조작 주체는 아니다).
	bool IsLocallyControlled(const HGameplayEntityId& actor) const;
	// 지금 입력을 받는 첫 행동자 (HGameplayState::FirstInputActor): 선택 대기 중이면 선택자, 아니면 흐름이 정한 Turn.Actors 의 첫 행동자.
	HGameplayEntityId GetInputActor() const;

protected:
	virtual void onBound() {}
	virtual void onUnbound() {}

	bool slotControls(const HGameplayPlayerSlot& slot, const HGameplayEntityId& actor) const;
	HGameplayPlayerSlot* findSlotMutable(int32 slot);
	void setClosed(const PString& reason);

	// 세션 공유 데이터 표 고치기. 바뀌었으면 true. 알리지 않는다 — 부른 쪽이 (호스트는 참가자에게 보낸 뒤) OnSessionDataChanged 로 알린다.
	bool putSessionData(const PName& key, const PString& value);
	bool dropSessionData(const PName& key);
	// 표를 통째로 바꾸고(Welcome) 값이 달라진 · 생긴 · 없어진 키를 돌려준다.
	HList<PName> replaceSessionData(const HList<HGameplaySessionDataEntry>& entries);
	void notifySessionData(const HList<PName>& keys);
};
