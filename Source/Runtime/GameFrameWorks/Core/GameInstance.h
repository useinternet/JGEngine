#pragma once
#include "Core/GameFrameWorksDefines.h"
#include "Network/Session/GameplaySession.h"
#include "GameInstance.generation.h"

class PWorld;
class JGGameEntryActor;
class PGameplayHostSession;
class PGameplayClientSession;
class PGameUIManager;

// 프로세스 수명의 게임 객체. 월드의 소유 · 로드 · 언로드 · 틱과 세션(싱글 · 리슨 서버 · 클라)을 맡는다 (언리얼의 GameInstance 자리).
// 모듈이 하나를 만들어 소유한다. 게임은 PWorld 를 직접 만지지 않고 이 객체를 통해서만 월드를 다룬다.
// 게임은 파생해 프로필 · 캠페인 · 설정처럼 월드보다 오래 사는 상태를 둔다 (HGameFrameWorksModule::ReplaceGameInstance).
// 세션은 월드보다 오래 산다 (월드가 바뀌어도 연결 유지). 틱마다 세션을 먼저 돌리고 월드를 돌린다.
JGCLASS()
class GAMEFRAMEWORKS_API JGGameInstance : public JGObject
{
	JG_GENERATED_CLASS_BODY

	friend class HGameFrameWorksModule;

private:
	PSharedPtr<PWorld>  _world;        // 활성 월드. 지금은 하나
	PSharedPtr<JGClass> _entryClass;   // 월드 생성 직후 스폰할 엔트리 액터 클래스 (JGGameEntryActor 파생)
	bool                _bInitialized = false;

	PSharedPtr<PGameplaySession> _session;          // 기본은 Standalone. 없을 때는 init 전 · shutdown 뒤뿐이다
	HDelegateHandle              _travelHandle;
	PName                        _pendingTravel;    // 클라: 호스트를 따라 로드할 월드 (세션 틱 뒤에 로드한다)
	bool                         _bTravelPending = false;

	PSharedPtr<PGameUIManager>   _ui;               // 게임 UI(JGGameWidget 레이어 스택). 월드보다 오래 산다

	static JGGameInstance* s_instance;

public:
	HMulticastDelegate<PSharedPtr<PWorld>> OnWorldLoaded;      // 엔트리 액터 스폰 후 · BeginPlay 전
	HMulticastDelegate<PSharedPtr<PWorld>> OnWorldUnloading;   // EndPlay 전

public:
	JGGameInstance() = default;
	virtual ~JGGameInstance() = default;

	static bool             HasInstance();
	static JGGameInstance&  Get();

	// 엔트리 액터 클래스. 월드를 로드할 때마다 엔진이 이 클래스를 스폰하고 OnEnterWorld 를 부른다.
	template<class T>
	void SetEntryClass()
	{
		SetEntryClass(StaticClass<T>());
	}

	void                SetEntryClass(PSharedPtr<JGClass> entryClass);
	PSharedPtr<JGClass> GetEntryClass() const;

	// 활성 월드가 있으면 먼저 언로드한다. 호스트 세션이면 참가자에게도 같은 월드를 로드하게 알린다.
	PSharedPtr<PWorld> LoadWorld(const PName& name = PName());
	void               UnloadWorld();
	PSharedPtr<PWorld> GetWorld() const;
	bool               HasWorld() const;
	PSharedPtr<JGGameEntryActor> GetEntryActor() const;

	// 세션. 기본은 Standalone(싱글플레이도 호스트 경로를 탄다). 실행 인자가 있으면 시작할 때 그 세션을 연다:
	//   -host[=포트] 리슨 서버 (모든 인터페이스, -bind=주소 로 좁힌다) · -join=주소[:포트] 클라 · -name=이름 플레이어 이름. 포트 기본 47771.
	// 세션을 바꾸면 활성 월드의 GameMaster 를 새 세션에 다시 붙인다. transport 가 nullptr 이면 TCP.
	PSharedPtr<PGameplaySession>       GetSession() const;
	// 이 기계가 권한 쪽인가 (Standalone · ListenServer). 게임은 초기 상태 구성과 시작을 이것이 true 일 때만 한다.
	bool                               IsAuthority() const;
	PSharedPtr<PGameplayHostSession>   HostSession(uint16 port, const HGameplaySessionConfig& config, PSharedPtr<INetTransport> transport = nullptr);
	PSharedPtr<PGameplayClientSession> JoinSession(const PString& address, uint16 port, const HGameplaySessionConfig& config, PSharedPtr<INetTransport> transport = nullptr);
	// Standalone 으로 돌아간다 (연결을 닫는다).
	void                               LeaveSession();

	// 게임 UI 관리자(JGGameWidget 레이어 스택 · 입력 라우팅 · 뒤로가기). init 뒤 항상 있고, 월드를 바꿔도 유지된다.
	// 호스트(에디터 Scene Viewport)가 이것으로 그리고 입력을 넘긴다. 게임은 PushWidget<T>(layer) 로 화면을 올린다.
	PSharedPtr<PGameUIManager>         GetUI() const;

	// 모듈이 프레임마다 부른다. 세션 → (클라면 월드 이동) → 월드 → UI 순서.
	void Tick(float32 deltaSeconds);

protected:
	virtual void OnInit() {}
	virtual void OnShutdown() {}
	virtual void OnWorldReady(PSharedPtr<PWorld> world) {}     // 엔트리 액터 스폰 후 · BeginPlay 전
	virtual void OnWorldUnload(PSharedPtr<PWorld> world) {}    // EndPlay 전

private:
	void init();
	void shutdown();

	void createLaunchSession();
	PSharedPtr<PGameplaySession> detachSession();   // 게임 인스턴스 교체: 닫지 않고 넘긴다
	PSharedPtr<PGameUIManager>   detachUI();        // 게임 인스턴스 교체: 떠 있는 화면을 그대로 넘긴다
	void replaceSession(PSharedPtr<PGameplaySession> session);
	void closeSession(const PString& reason);
	void onTravelRequested(const PName& world);
};
