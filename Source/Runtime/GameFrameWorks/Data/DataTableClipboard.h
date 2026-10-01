#pragma once
#include "Data/DataTableDocument.h"

// 글 표(행 × 칸). 클립보드 · 복사 · 붙여넣기에 쓴다.
using HDataTableTextGrid = HList<HList<PString>>;

struct GAMEFRAMEWORKS_API HDataTableClipboard
{
	// 엑셀 · 구글 시트와 같은 TSV: 칸 = 탭, 행 = \r\n(마지막 행 뒤에도). 탭 · 줄바꿈 · " 가 든 칸은 "…" 로 감싸고 안의 " 는 "".
	static PString EncodeTsv(const HDataTableTextGrid& inCells);
	// TSV → 표. \r\n · \n 을 받고 끝의 줄바꿈 하나는 무시한다. 짧은 행은 빈 칸으로 채워 사각형으로 만든다. 빈 글이면 false.
	static bool DecodeTsv(const PString& inText, HDataTableTextGrid* outCells);
};

// 그리드 범위. 보기 행(정렬 · 필터를 거친 순서, inViewRows[보기 행] = 데이터 행) × 그리드 열(0 = 키 열, c ≥ 1 = Schema.Columns[c - 1]).
struct GAMEFRAMEWORKS_API HDataTableGridRange
{
	int32 FirstViewRow    = 0;
	int32 FirstGridColumn = 0;
	int32 RowCount        = 1;
	int32 ColumnCount     = 1;
};

// 범위를 글 표로(복사). 키 열은 키, 나머지는 열의 FormatText.
GAMEFRAMEWORKS_API HDataTableTextGrid CopyDataTableRange(const HDataTableContent& inContent, const HList<int32>& inViewRows, const HDataTableGridRange& inRange);

// 범위를 열 기본값으로(Delete · 잘라내기). 키 열은 건드리지 않는다. 바뀌는 칸이 없으면 빈 목록.
GAMEFRAMEWORKS_API HList<HDataTableEdit> PlanDataTableClear(const HDataTableContent& inContent, const HList<int32>& inViewRows, const HDataTableGridRange& inRange);

// 붙여넣기 계획. inRange = 지금 선택(왼쪽 위가 붙일 자리).
//   클립보드가 한 칸이고 선택이 더 크면 선택 전체를 그 값으로 채운다.
//   블록이 아래로 넘치면 새 행을 끝에 붙인다 — 키 열부터 붙일 때, 보기가 파일 순서 그대로일 때(bViewIsFileOrder)만.
//   모든 칸이 열 타입에 맞아야 한다. 하나라도 안 맞으면 false 와 이유 목록(편집 없음).
GAMEFRAMEWORKS_API bool PlanDataTablePaste(const HDataTableContent& inContent, const HList<int32>& inViewRows, bool bViewIsFileOrder,
	const HDataTableGridRange& inRange, const HDataTableTextGrid& inCells, HList<HDataTableEdit>* outEdits, HList<PString>* outErrors);
