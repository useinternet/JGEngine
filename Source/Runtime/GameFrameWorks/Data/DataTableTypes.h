#pragma once
#include "Core/GameFrameWorksDefines.h"

// 데이터 테이블 열 타입. 파일에는 이름 문자열("Int" 등)로 쓴다.
// 새 타입(리플렉션 구조체 등)은 Count 앞에 붙이고 JGDataTable::FormatVersion 을 올린다.
enum class EDataTableColumnType : int32
{
	Bool,
	Int,
	Float,
	String,
	Enum,
	AssetRef,
	RowRef,
	Count,
};

GAMEFRAMEWORKS_API const char* DataTableColumnTypeToString(EDataTableColumnType inType);
// 대소문자를 무시한다(손으로 고친 파일). 모르는 이름이면 false.
GAMEFRAMEWORKS_API bool DataTableColumnTypeFromString(const PString& inText, EDataTableColumnType* outType);

// 셀 값을 담는 그릇의 종류. 열 타입마다 하나로 정해진다 — Bool · Int · Float 는 그대로, 나머지(String · Enum · AssetRef · RowRef)는 Text.
enum class EDataTableValueKind : uint8
{
	Bool,
	Int,
	Float,
	Text,
};

GAMEFRAMEWORKS_API EDataTableValueKind DataTableValueKindOf(EDataTableColumnType inType);

// A-Z 만 소문자로 바꾼다. 나머지 바이트(한글 UTF-8 등)는 그대로.
// PString::ToLower 는 바이트마다 ::tolower 를 불러 0x80 이상 바이트에서 디버그 CRT 어설션이 난다 — 사용자 글에는 이것을 쓴다.
GAMEFRAMEWORKS_API PString DataTableToLowerAscii(const PString& inText);
// 대소문자(ASCII)를 무시하고 inText 에 inPattern 이 들어 있나. 빈 패턴은 true.
GAMEFRAMEWORKS_API bool DataTableContainsIgnoreCase(const PString& inText, const PString& inPattern);

// 글이 같나(바이트 비교). PString::operator== 는 문자열 테이블 ID 를 비교해 기본 생성한 빈 글과 "" 로 만든 빈 글이 다르다.
inline bool DataTableSameText(const PString& a, const PString& b)
{
	return a.GetRawString() == b.GetRawString();
}

inline bool DataTableSameTexts(const HList<PString>& a, const HList<PString>& b)
{
	if (a.size() != b.size())
	{
		return false;
	}
	for (size_t i = 0; i < a.size(); ++i)
	{
		if (DataTableSameText(a[i], b[i]) == false)
		{
			return false;
		}
	}
	return true;
}

// 셀 값 하나. 열 타입을 모른다 — 이름 목록 · 참조 같은 해석은 열(HDataTableColumn)이 한다.
class GAMEFRAMEWORKS_API HDataTableValue
{
	EDataTableValueKind _kind  = EDataTableValueKind::Text;
	bool                _bool  = false;
	int64               _int   = 0;
	float64             _float = 0.0;
	PString             _text;

public:
	HDataTableValue() = default;

	static HDataTableValue MakeBool(bool inValue);
	static HDataTableValue MakeInt(int64 inValue);
	static HDataTableValue MakeFloat(float64 inValue);
	static HDataTableValue MakeText(const PString& inValue);
	// 종류의 기본값(false · 0 · 0.0 · 빈 글)
	static HDataTableValue MakeDefault(EDataTableValueKind inKind);

	EDataTableValueKind GetKind() const { return _kind; }
	bool                GetBool() const { return _bool; }
	int64               GetInt() const { return _int; }
	float64             GetFloat() const { return _float; }
	const PString&      GetText() const { return _text; }

	// 종류와 값이 같다. 실수는 비트가 같아야 같다(저장 바이트 기준).
	bool operator==(const HDataTableValue& inOther) const;
	bool operator!=(const HDataTableValue& inOther) const;
};
