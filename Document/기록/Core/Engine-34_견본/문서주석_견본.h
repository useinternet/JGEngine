// Engine-34 견본 — 공개 헤더 문서 주석 꼴.
// 본 헤더(Source/Runtime/Core/ConsoleCommand/ConsoleCommandGlobalSystem.h · ConsoleCommandArgs.h)는 고치지 않았다.
// 아래는 두 헤더의 공개 부분에 문서 주석을 단 모습이다(private 멤버 · 구현은 줄임). 계획 확인 뒤 본 헤더에 이 꼴로 단다.
//
// 꼴: 선언 바로 위 /// 줄. 첫 문단 = 역할 한두 줄, 둘째 문단 = 쓰는 상황,
//     "스레드: / 수명: / 순서: / 오류:" 네 줄(해당하는 것만), 끝에 @param · @returns.
//     Visual Studio Quick Info 가 기본으로 보여 주는 Doxygen 태그(param · tparam · returns · deprecated · exception)만 쓴다.
//     게임 코드가 부르지 않는 public 멤버는 "엔진 내부용" 한 줄만 달고 API Reference 에서 뺀다.

#pragma once
#include "CoreDefines.h"
#include "String/String.h"

// ---- ConsoleCommand/ConsoleCommandArgs.h ----------------------------------------------------------------------------------

/// 콘솔 명령 한 줄을 나눈 인자. 핸들러가 받는다.
///
/// 문법: <이름> <위치 인자>... -플래그 -키=값 "따옴표 값". 이름과 -키는 소문자로 비교하고, 값과 위치 인자는 그대로 둔다.
/// "-키 값"(빈칸 구분)은 받지 않는다 — -로 시작하는 토큰은 모두 이름 인자다(음수는 -offset=-1 처럼 쓴다).
///
/// 스레드: 핸들러 안에서 메인 스레드로만 쓴다.
/// 수명: 핸들러가 돌 동안만 유효하다. 값을 남기려면 PString 으로 복사한다.
class HConsoleCommandArgs
{
public:
	/// 명령 이름(소문자). 예: "sample.greet".
	const PString& GetName() const;

	/// 위치 인자 수(이름과 -키 인자는 세지 않는다).
	uint32 GetPositionalCount() const;

	/// 위치 인자 하나.
	/// @param InIndex  0부터.
	/// @returns 그 인자. 범위 밖이면 빈 문자열.
	PString GetPositional(uint32 InIndex) const;

	/// -키 또는 -키=값이 있는지. 값은 보지 않는다(플래그 확인용).
	/// @param InName  "-" 없이. 대소문자를 가리지 않는다.
	bool Has(const PString& InName) const;

	/// -키=값의 값을 문자열로 읽는다.
	///
	/// 오류: 인자가 없으면 false 를 돌려주고 OutValue 를 바꾸지 않는다 — 기본값을 먼저 넣고 부른다.
	/// @param InName    "-" 없이.
	/// @param OutValue  읽은 값. 실패하면 그대로.
	/// @returns 읽었으면 true.
	bool TryGetString(const PString& InName, PString& OutValue) const;

	/// -키=값의 값을 정수로 읽는다.
	///
	/// 오류: 인자가 없거나 정수가 아니면 false, OutValue 는 그대로. "주어졌는데 틀린 값"을 가리려면 Has 와 함께 쓴다:
	///       if (InArgs.Has("count") && InArgs.TryGetInt("count", count) == false) { return false; }
	/// @param InName    "-" 없이.
	/// @param OutValue  읽은 값. 실패하면 그대로.
	/// @returns 읽었으면 true.
	bool TryGetInt(const PString& InName, int32& OutValue) const;

	/// 엔진 내부용 — 레지스트리가 줄을 나눌 때 쓴다.
	static bool Tokenize(const PString& InLine, HList<PString>& OutTokens, PString& OutError);
};

// ---- ConsoleCommand/ConsoleCommandGlobalSystem.h --------------------------------------------------------------------------

class GConsoleCommandGlobalSystem;

/// 파일 범위 전역 변수로 선언하는 콘솔 명령. 선언한 모듈이 연결되면 등록되고, 모듈이 내려가면 해제된다.
///
/// 에디터 DevConsole 창과 명령줄 도구 JGConsole 이 같은 명령을 실행한다. 게임 모듈의 시험 · 점검 명령을 이것으로 만든다.
/// 객체에 묶인 핸들러나 조건부 등록이 필요하면 GConsoleCommandGlobalSystem::Register 를 직접 쓴다.
///
/// 스레드: 생성은 정적 초기화(DLL 로드) 중, 등록 · 실행은 메인 스레드.
/// 수명: 정적 저장 기간이어야 한다. 소멸자가 목록에서 빠지지 않으므로 지역 변수 · 동적 할당으로 만들면 안 된다.
/// 순서: 생성자 안에서는 엔진 시스템(로그 · 메모리 · 레지스트리)이 아직 연결되지 않았다 — 생성자는 목록에 잇기만 한다.
///       등록은 그 모듈의 StartupModule 이 끝난 직후, 해제는 ShutdownModule 직전에 엔진이 한다.
/// 오류: 이름이 비었거나 빈칸 · 따옴표가 있거나 이미 등록된 이름이면 오류 로그만 남기고 등록하지 않는다(먼저 등록된 명령이 남는다).
class HAutoConsoleCommand
{
public:
	/// 핸들러 형. 캡처 없는 람다나 정적 함수만 넘길 수 있다.
	using HandlerFunction = bool (*)(const HConsoleCommandArgs&);

	/// @param InName         명령 이름. 소문자 "영역.동작"(예: "audio.selftest"). 대소문자를 가리지 않는다.
	/// @param InUsage        사용법 한 줄. help 와 실패 때 출력한다.
	/// @param InDescription  help 목록에 나오는 한 줄 설명(영어).
	/// @param InHandler      명령 본문. true = 성공, false = 실패 — 레지스트리가 사용법을 출력하고 JGConsole 은 종료 코드 1.
	HAutoConsoleCommand(const char* InName, const char* InUsage, const char* InDescription, HandlerFunction InHandler);

	/// 엔진 내부용 — IModuleInterface 와 레지스트리가 부른다. 게임 코드는 부르지 않는다.
	static void RegisterAll(GConsoleCommandGlobalSystem& InRegistry);

	/// 엔진 내부용 — IModuleInterface 와 레지스트리가 부른다. 게임 코드는 부르지 않는다.
	static void UnregisterAll(GConsoleCommandGlobalSystem& InRegistry);
};
