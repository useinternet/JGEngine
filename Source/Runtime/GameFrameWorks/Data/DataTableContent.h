#pragma once
#include "Data/DataTableSchema.h"

// 파일 형식 버전("FormatVersion"). 형식을 바꾸면 올리고, 읽는 쪽이 옛 버전을 올려 읽는다.
constexpr int32 DataTableFormatVersion = 1;

// 행 하나. 키는 테이블 안에서 고유한 ID 이고, 값은 Schema.Columns 순서다.
struct GAMEFRAMEWORKS_API HDataTableRow
{
	PString                Key;
	HList<HDataTableValue> Values;

	bool operator==(const HDataTableRow& inOther) const;
	bool operator!=(const HDataTableRow& inOther) const;
};

// 스키마 + 행(파일 순서 = 순회 순서). 에셋(JGDataTable)과 편집 문서가 같은 형식을 쓴다.
struct GAMEFRAMEWORKS_API HDataTableContent
{
	HDataTableSchema    Schema;
	HList<HDataTableRow> Rows;

	int32 GetRowCount() const { return (int32)Rows.size(); }
	int32 GetColumnCount() const { return (int32)Schema.Columns.size(); }
	// 키가 같은 첫 행. 없으면 -1. (선형 — 에셋은 색인을 따로 둔다)
	int32 FindRowByKey(const PString& inKey) const;
	// 열 기본값으로 채운 행
	HDataTableRow MakeDefaultRow(const PString& inKey) const;
	// 없는 키를 만든다. inBase 가 비어 있지 않고 안 쓰였으면 그대로, 아니면 inBase_1, inBase_2 …
	PString MakeUniqueKey(const PString& inBase) const;

	bool operator==(const HDataTableContent& inOther) const;
	bool operator!=(const HDataTableContent& inOther) const;
};

enum class EDataTableIssueSeverity : uint8
{
	Warning,   // 값 문제(범위 · 목록 · 참조). 저장은 된다
	Error,     // 구조 문제(빈 키 · 중복 키 · 열 이름). 저장을 막는다
};

// 검증 · 읽기 문제 하나. Row = 행 번호(-1 = 행과 무관), Column = Schema.Columns 번호(-1 = 키 열, 행도 -1 이면 테이블 전체).
struct GAMEFRAMEWORKS_API HDataTableIssue
{
	EDataTableIssueSeverity Severity = EDataTableIssueSeverity::Warning;
	int32                   Row      = -1;
	int32                   Column   = -1;
	PString                 Message;
};

GAMEFRAMEWORKS_API bool HasDataTableErrors(const HList<HDataTableIssue>& inIssues);
// 처음 inMaxLines 개를 로그로(나머지는 개수만). inContext = 테이블 경로 등
GAMEFRAMEWORKS_API void LogDataTableIssues(const PString& inContext, const HList<HDataTableIssue>& inIssues, int32 inMaxLines = 10);

// 스키마 + 행의 지문(FNV-1a 64). 파일 서식 · 공백과 무관하고 값 · 순서가 같으면 같다.
GAMEFRAMEWORKS_API uint64 ComputeDataTableContentHash(const HDataTableContent& inContent);

// "JGDataTable" 절 ↔ 내용. 읽기는 모르는 키 · 타입이 다른 값 · 빠진 값을 문제로 보고하고(값은 열 기본값) 계속한다.
// 절 자체가 깨졌으면(Columns · Rows 가 배열이 아님) false.
GAMEFRAMEWORKS_API bool ReadDataTableContentJson(const PJsonData& json, HDataTableContent* outContent, HList<HDataTableIssue>* outIssues);
GAMEFRAMEWORKS_API void WriteDataTableContentJson(PJsonData& json, const HDataTableContent& inContent);
