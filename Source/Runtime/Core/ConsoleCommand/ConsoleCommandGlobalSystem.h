#pragma once
#include "CoreDefines.h"
#include "CoreSystem.h"
#include "String/String.h"
#include "Misc/Delegate.h"
#include "ConsoleCommand/ConsoleCommandArgs.h"

/*
콘솔 명령 (Document/DevConsole_TODO.md "명령 등록 방법")

기본: 아무 .cpp 의 파일 범위에 전역 변수로 선언한다. 모듈의 Startup/Shutdown 은 고치지 않는다.

	static HAutoConsoleCommand ReadbackCommand(
		"devscene.readback",                                  // 이름: 소문자 area.action
		"devscene.readback [-path=<dir>] [-count=N]",         // 사용법: help 와 실패 시 출력
		"Save G-buffer albedo and scene readbacks as PNG",    // 설명: help 목록 한 줄 (영어)
		[](const HConsoleCommandArgs& InArgs)                 // 캡처 없는 람다나 정적 함수만 된다
		{
			PString Directory = "Temp/Readback";
			InArgs.TryGetString("path", Directory);           // 없으면 기본값 유지
			return true;                                      // false 면 레지스트리가 사용법을 출력한다
		});

- 모듈 DLL 에 선언하면 그 모듈의 StartupModule 직후 등록되고 ShutdownModule 직전 해제된다(모듈 시스템이 자동으로).
  EXE(JGConsole 등)에 선언하면 GCoreSystem::Create 에서 등록되고 Destroy 에서 해제된다.
- 모듈 상태는 핸들러 안에서 GModuleGlobalSystem::GetInstance().FindModule<T>() 로 찾는다(명령은 모듈이 연결된 동안만 있다).
- Core(정적 라이브러리) 안에는 선언하지 않는다. 링커가 빼거나 바이너리마다 사본이 생겨 중복 등록된다.
- 출력은 JG_LOG 로 한다. 사용자 문자열은 반드시 "%s" 인자로 넘긴다(AddLog 는 서식을 두 번 거친다).

동적인 경우(객체에 묶인 핸들러, 조건부 등록, 테스트)만 Register/Unregister 를 직접 쓴다. 그때는 등록한 쪽이 반드시 해제한다.
실행: Execute 는 즉시(메인 스레드), Submit 은 다음 GCoreSystem::Update 끝(프레임 밖)에 실행한다. DevConsole 입력은 Submit 이다.
*/

JG_DECLARE_DELEGATE_RET(HConsoleCommandDelegate, bool, const HConsoleCommandArgs&)

struct HConsoleCommandDesc
{
	PString Name;        // area.action. 등록할 때 소문자로 바꾼다.
	PString Usage;       // help 와 실패 시 출력
	PString Description; // help 목록의 한 줄 설명
	HConsoleCommandDelegate Handler;
};

// 명령 레지스트리. 메인 스레드 전용이다.
// GCoreSystem::Create 에서 GStringTable 다음, GModuleGlobalSystem 앞에 등록한다.
// 그래서 Update 는 스케줄러의 모든 버킷(프레임) 뒤에, Destroy 는 모듈이 모두 해제된 뒤에 온다.
class GConsoleCommandGlobalSystem : public GGlobalSystemInstance<GConsoleCommandGlobalSystem>
{
private:
	HHashMap<PString, HConsoleCommandDesc> _commands;
	HList<PString> _pendingLines;

public:
	GConsoleCommandGlobalSystem();
	virtual ~GConsoleCommandGlobalSystem() = default;

protected:
	virtual void Update() override;
	virtual void Destroy() override;

public:
	// 빈 이름·공백이나 따옴표가 든 이름·핸들러 없음·중복 이름이면 오류 로그를 남기고 false.
	bool Register(const HConsoleCommandDesc& InDesc);
	bool Unregister(const PString& InName);
	bool IsRegistered(const PString& InName) const;

	// 즉시 실행한다. 빈 줄은 true. 없는 명령·따옴표 오류·핸들러 실패면 false.
	bool Execute(const PString& InLine);
	// 이미 나뉜 토큰으로 즉시 실행한다(JGConsole 의 argv). 따옴표를 다시 해석하지 않는다.
	bool Execute(const HList<PString>& InTokens);

	// 다음 Update(프레임 밖)에 실행한다. 헤드리스(JGConsole)는 Update 가 돌지 않으므로 ExecutePending 을 직접 부른다.
	void Submit(const PString& InLine);
	void ExecutePending();

private:
	bool executeTokens(const HList<PString>& InTokens, const PString& InEchoLine);
	bool executeHelp(const HConsoleCommandArgs& InArgs);
};

// 전역 변수로 선언하는 명령. 생성자는 정적 초기화 도중(LoadDll 안, 모듈 DLL 의 Core 연결 전)에 불리므로
// 레지스트리·엔진 풀·로그를 쓰지 않고, 이 바이너리의 목록에 자기를 잇기만 한다. 등록은 RegisterAll 이 한다.
class HAutoConsoleCommand
{
public:
	using HandlerFunction = bool (*)(const HConsoleCommandArgs&);

private:
	const char*          _name;
	const char*          _usage;
	const char*          _description;
	HandlerFunction      _handler;
	HAutoConsoleCommand* _next;
	bool                 _bRegistered = false; // 이름이 겹쳐 등록에 실패한 명령은 해제할 때 건너뛴다(남의 명령을 지우지 않게).

	// 이 바이너리(DLL/EXE)의 목록 머리. Core 가 정적 라이브러리라 바이너리마다 따로 있다. 상수 초기화(nullptr)라 정적 초기화 순서와 무관하다.
	static HAutoConsoleCommand* Head;

public:
	HAutoConsoleCommand(const char* InName, const char* InUsage, const char* InDescription, HandlerFunction InHandler);
	~HAutoConsoleCommand() = default;

	HAutoConsoleCommand(const HAutoConsoleCommand&) = delete;
	HAutoConsoleCommand& operator=(const HAutoConsoleCommand&) = delete;

	// 이 바이너리에 선언된 명령을 모두 등록/해제한다. IModuleInterface 와 레지스트리가 부른다.
	static void RegisterAll(GConsoleCommandGlobalSystem& InRegistry);
	static void UnregisterAll(GConsoleCommandGlobalSystem& InRegistry);
};
