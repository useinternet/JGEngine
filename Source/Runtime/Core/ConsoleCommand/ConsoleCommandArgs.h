#pragma once
#include "CoreDefines.h"
#include "String/String.h"

// 콘솔 명령 한 줄의 인자. 문법은 Document/DevConsole_TODO.md 0-3.
//   <name> <positional>...  -flag  -key=value  "quoted value"
// - 명령 이름과 이름 인자(-key)는 소문자로 저장·비교한다. 값과 위치 인자는 그대로 둔다.
// - "-key value"(공백 구분)는 받지 않는다. -로 시작하는 토큰은 모두 이름 인자다(음수 위치 인자도 불가, -offset=-1 로 쓴다).
// - 값은 첫 번째 '=' 뒤 전부다. 같은 이름이 두 번 오면 뒤의 값을 쓴다.
class HConsoleCommandArgs
{
private:
	PString                    _name;
	HList<PString>             _positionals;
	HHashMap<PString, PString> _options;

public:
	HConsoleCommandArgs() = default;
	explicit HConsoleCommandArgs(const HList<PString>& InTokens);

	// 공백으로 나누고 "..." 를 한 토큰으로 묶는다. 백슬래시는 이스케이프 문자가 아니다(Windows 경로를 그대로 쓴다).
	// 따옴표가 닫히지 않으면 false 와 오류 문구를 돌려주고 OutTokens 는 비운다.
	static bool Tokenize(const PString& InLine, HList<PString>& OutTokens, PString& OutError);

	// 명령 이름·인자 이름 비교에 쓰는 소문자화. ASCII 만 바꾼다.
	// PString::ToLower 는 ::tolower 에 음수 char(한글 등)를 넘겨 디버그 CRT 에서 assert 가 나므로 쓰지 않는다.
	static PString NormalizeName(const PString& InName);

public:
	const PString& GetName() const;

	uint32  GetPositionalCount() const;
	// 범위 밖이면 빈 문자열
	PString GetPositional(uint32 InIndex) const;

	// -name 또는 -name=value 가 있으면 true. 값은 보지 않는다(플래그 확인용).
	bool Has(const PString& InName) const;

	// 인자가 없거나 변환에 실패하면 false 를 돌려주고 OutValue 를 바꾸지 않는다. 기본값을 먼저 넣고 부른다.
	// "주어졌는데 틀린 값"을 가리려면 Has 와 함께 쓴다: if (Has("n") && TryGetInt("n", Value) == false) { return false; }
	bool TryGetString(const PString& InName, PString& OutValue) const;
	bool TryGetInt(const PString& InName, int32& OutValue) const;
};
