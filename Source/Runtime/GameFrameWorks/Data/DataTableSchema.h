#pragma once
#include "Data/DataTableTypes.h"

// 열 하나의 정의(파일 "Columns" 의 원소). 값의 해석 — 텍스트 ↔ 값, JSON ↔ 값, 타입 바꾸기 — 은 열이 한다.
struct GAMEFRAMEWORKS_API HDataTableColumn : public IJsonable
{
	PString              Name;
	EDataTableColumnType Type = EDataTableColumnType::String;
	HDataTableValue      Default;              // 종류는 DataTableValueKindOf(Type)
	PString              Description;

	// Int · Float. 범위 밖은 검증 경고(값은 그대로 둔다)
	bool    bHasMin = false;
	bool    bHasMax = false;
	float64 Min     = 0.0;
	float64 Max     = 0.0;

	// Enum — EnumValues 가 있으면 그 목록, 비어 있으면 EnumType(리플렉션 열거형 이름)의 목록
	HList<PString> EnumValues;
	PString        EnumType;

	PString AssetClass;   // AssetRef: 고르기 목록을 이 클래스(와 파생)로 거른다. 비면 모든 에셋
	PString Table;        // RowRef: 대상 테이블 토큰 경로(/JGGame/…). 비면 같은 테이블

	HDataTableColumn();
	HDataTableColumn(const PString& inName, EDataTableColumnType inType);   // Default = 타입 기본값

	// Enum 열의 이름 목록. EnumType 을 리플렉션에서 못 찾으면(그 모듈이 안 올라옴) false.
	bool GetEnumNames(HList<PString>& outNames) const;

	// 텍스트(셀 입력 · 붙여넣기) → 값. 타입에 안 맞으면 false 와 이유. 범위 · 참조 존재는 보지 않는다(검증 몫).
	bool ParseText(const PString& inText, HDataTableValue* outValue, PString* outError) const;
	// 값 → 셀에 보이는 글 · 복사하는 글. ParseText 로 되돌리면 같은 값이다.
	PString FormatText(const HDataTableValue& inValue) const;
	// 다른 타입 열의 값을 이 열 타입으로 바꾼다(열 타입 바꾸기). 바꿀 수 없으면 false.
	bool ConvertFrom(const HDataTableValue& inValue, EDataTableColumnType inFromType, HDataTableValue* outValue) const;

	// 셀 JSON 값 → 값. 종류가 다르면 false. 정수로 쓴 수는 Float 열에서도 받는다.
	bool ReadValueJson(const PJsonData& inJson, HDataTableValue* outValue) const;
	// 행 객체 json 에 inKey = 값을 쓴다.
	void WriteValueJson(PJsonData& json, const PString& inKey, const HDataTableValue& inValue) const;

	// 열 정의 → 파일. (IJsonable — HList<HDataTableColumn> 을 그대로 AddMember 할 수 있다)
	virtual void WriteJson(PJsonData& json) const override;
	// 파일 → 열 정의. 이름 · 타입이 없거나 틀리면 false 와 이유. 기본값 · 옵션이 틀리면 기본으로 두고 outWarnings 에 적는다.
	bool Read(const PJsonData& json, PString* outError, HList<PString>* outWarnings);

	bool operator==(const HDataTableColumn& inOther) const;
	bool operator!=(const HDataTableColumn& inOther) const;
};

// 테이블의 키 열 이름과 열 목록.
struct GAMEFRAMEWORKS_API HDataTableSchema
{
	PString                 KeyColumn = "Id";
	HList<HDataTableColumn> Columns;

	// 이름이 같은 열의 번호. 없으면 -1.
	int32 FindColumn(const PString& inName) const;
	int32 GetColumnCount() const { return (int32)Columns.size(); }
};

// 열 · 키 이름 규칙: 비어 있지 않고, 앞뒤 공백 · 제어 문자 · 따옴표가 없다. 키는 공백도 없다.
GAMEFRAMEWORKS_API bool IsValidDataTableColumnName(const PString& inName, PString* outReason);
GAMEFRAMEWORKS_API bool IsValidDataTableKey(const PString& inKey, PString* outReason);
