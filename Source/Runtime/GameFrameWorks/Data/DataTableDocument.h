#pragma once
#include "Data/DataTable.h"
#include "Data/DataTableValidation.h"

struct GAMEFRAMEWORKS_API HDataTableCellAssignment
{
	int32           Row    = -1;
	int32           Column = -1;   // Schema.Columns 번호
	HDataTableValue Value;
};

enum class EDataTableEditType : uint8
{
	SetCells,        // Cells
	SetKeys,         // Keys
	InsertRows,      // Rows
	RemoveRows,      // Indices
	ReorderRows,     // Indices
	InsertColumn,    // Column · ColumnDef · ColumnValues
	RemoveColumn,    // Column
	ReplaceColumn,   // Column · ColumnDef · ColumnValues (이름 · 타입 · 옵션 바꾸기)
	MoveColumn,      // Column → ToColumn
	SetKeyColumn,    // Name
};

// 편집 하나. ApplyDataTableEdit 가 내용에 걸고 그 역(되돌리기)을 만든다.
struct GAMEFRAMEWORKS_API HDataTableEdit
{
	EDataTableEditType                 Type = EDataTableEditType::SetCells;
	HList<HDataTableCellAssignment>    Cells;
	HList<HPair<int32, PString>>       Keys;           // (행, 새 키)
	HList<HPair<int32, HDataTableRow>> Rows;           // (끝난 뒤의 위치, 행). 위치 오름차순
	HList<int32>                       Indices;        // RemoveRows: 지울 행(오름차순) · ReorderRows: 새 위치 i 의 행 = 옛 Indices[i]
	int32                              Column   = -1;
	int32                              ToColumn = -1;
	HDataTableColumn                   ColumnDef;
	HList<HDataTableValue>             ColumnValues;   // 행마다. InsertColumn 에서 비면 ColumnDef.Default
	PString                            Name;

	static HDataTableEdit MakeSetCells(const HList<HDataTableCellAssignment>& inCells);
	static HDataTableEdit MakeSetKeys(const HList<HPair<int32, PString>>& inKeys);
	static HDataTableEdit MakeInsertRows(const HList<HPair<int32, HDataTableRow>>& inRows);
	static HDataTableEdit MakeRemoveRows(const HList<int32>& inSortedRows);
	static HDataTableEdit MakeReorderRows(const HList<int32>& inNewOrder);
	static HDataTableEdit MakeInsertColumn(int32 inAt, const HDataTableColumn& inColumn, const HList<HDataTableValue>& inValues = HList<HDataTableValue>());
	static HDataTableEdit MakeRemoveColumn(int32 inColumn);
	static HDataTableEdit MakeReplaceColumn(int32 inColumn, const HDataTableColumn& inColumnDef, const HList<HDataTableValue>& inValues);
	static HDataTableEdit MakeMoveColumn(int32 inFrom, int32 inTo);
	static HDataTableEdit MakeSetKeyColumn(const PString& inName);
};

// 편집을 내용에 건다. 성공하면 outInverse 에 되돌리는 편집. 잘못된 편집(범위 밖 · 종류가 다른 값)이면 내용을 바꾸지 않고 false 와 이유.
GAMEFRAMEWORKS_API bool ApplyDataTableEdit(HDataTableContent& content, const HDataTableEdit& inEdit, HDataTableEdit* outInverse, PString* outError);

// 열 타입 · 옵션을 바꾼 새 열 정의로 그 열의 값을 바꾼다(ReplaceColumn 의 값). 바꿀 수 없는 값은 새 기본값 + outFailedRows.
GAMEFRAMEWORKS_API HList<HDataTableValue> ConvertDataTableColumnValues(const HDataTableContent& inContent, int32 inColumn, const HDataTableColumn& inNewColumn, HList<int32>* outFailedRows);

// 데이터 테이블 편집 문서(에디터의 탭 하나). UI 가 없어 헤드리스로 검증한다(datatable.selftest).
//   작업 사본: 파일에서 읽은 JGDataTable(같은 GUID, 에셋 DB 에 없음)을 고친다.
//   저장: 검증(오류면 막음) → 파일(임시 → 교체) → 에셋 DB 의 테이블에 ApplyContent(OnChanged).
//   실행 취소: 사용자 동작 하나(Apply 한 번) = 한 단계, 최대 MaxUndoSteps. 저장해도 남는다.
class GAMEFRAMEWORKS_API PDataTableDocument : public IMemoryObject
{
public:
	static constexpr int32 MaxUndoSteps = 500;

private:
	struct HUndoStep
	{
		PString               Label;
		uint64                Serial = 0;
		HList<HDataTableEdit> Redo;   // 다시 실행할 때 거는 편집(순서대로)
		HList<HDataTableEdit> Undo;   // 되돌릴 때 거는 편집(이미 역순)
	};

	PSharedPtr<JGDataTable> _working;
	PString                 _tokenPath;
	PString                 _rawPath;
	HDeque<HUndoStep>       _undoSteps;
	HList<HUndoStep>        _redoSteps;
	uint64                  _nextSerial    = 1;
	uint64                  _currentSerial = 0;   // 맨 위 실행 취소 단계(없으면 0)
	uint64                  _savedSerial   = 0;
	uint64                  _revision      = 1;
	int64                   _fileStamp     = 0;
	HList<HDataTableIssue>  _loadIssues;

public:
	PDataTableDocument() = default;
	virtual ~PDataTableDocument() = default;

	// 파일에서 연다. inTokenPath = /JGGame/… · /JGEngine/… (확장자는 있어도 없어도 된다)
	static PSharedPtr<PDataTableDocument> Open(const PString& inTokenPath, PString* outError);
	// 새 테이블 파일을 만들고 연다(이미 있으면 실패). 에셋 DB 에 올리는 것은 부르는 쪽(LoadAssetAsync).
	static PSharedPtr<PDataTableDocument> CreateNew(const PString& inTokenPath, const PString& inKeyColumn, PString* outError);
	// 아무 위치의 파일을 연다(도구 · 셀프 테스트). 토큰 경로가 없어 에셋 DB 의 테이블과는 이어지지 않는다.
	static PSharedPtr<PDataTableDocument> OpenRawFile(const PString& inRawPath, PString* outError);
	// 파일 없는 문서(셀프 테스트). Save · Reload 는 실패한다.
	static PSharedPtr<PDataTableDocument> FromContent(const HDataTableContent& inContent);

	const HDataTableContent& GetContent() const;
	const PString& GetTokenPath() const { return _tokenPath; }
	const PString& GetRawPath() const { return _rawPath; }
	PString GetDisplayName() const;
	// 내용이 바뀔 때마다(편집 · 실행 취소 · 다시 읽기) +1. UI 캐시용
	uint64 GetRevision() const { return _revision; }
	bool IsDirty() const { return _currentSerial != _savedSerial; }
	const HList<HDataTableIssue>& GetLoadIssues() const { return _loadIssues; }

	// 편집 묶음을 실행 취소 한 단계로 건다. 하나라도 실패하면 전부 되돌리고 false.
	bool Apply(const HList<HDataTableEdit>& inEdits, const PString& inLabel, PString* outError);
	bool CanUndo() const { return _undoSteps.empty() == false; }
	bool CanRedo() const { return _redoSteps.empty() == false; }
	PString GetUndoLabel() const;
	PString GetRedoLabel() const;
	bool Undo();
	bool Redo();

	void Validate(const IDataTableReferenceResolver* inResolver, HList<HDataTableIssue>& outIssues) const;
	// 오류(빈 키 · 중복 키 · 열 이름)가 있으면 저장하지 않는다.
	bool Save(const IDataTableReferenceResolver* inResolver, PString* outError);
	// 디스크에서 다시 읽는다(실행 취소 기록은 비운다). 에셋 DB 의 테이블에도 넣는다.
	bool Reload(PString* outError);
	// 연 뒤 · 저장한 뒤 다른 프로그램이 파일을 바꿨나(파일 시각)
	bool HasExternalChange() const;
	// 바깥 수정을 무시한다 — 다음 저장이 덮어쓴다
	void AcknowledgeExternalChange();

	// 에셋 DB 에 올라온 같은 경로의 테이블(없으면 null)
	PSharedPtr<JGDataTable> GetLiveTable() const;

private:
	static int64 readFileStamp(const PString& inRawPath);
	void contentChanged();
	bool applyEdits(const HList<HDataTableEdit>& inEdits, HList<HDataTableEdit>* outInverses, PString* outError);
};
