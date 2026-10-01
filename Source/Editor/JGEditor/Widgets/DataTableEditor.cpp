#include "PCH/PCH.h"
#include "Widgets/DataTableEditor.h"
#include "GUI.h"
#include "Grid/GUIGrid.h"
#include "AssetDatabase.h"
#include "AssetPath.h"
#include "Data/DataTableDocument.h"
#include "Data/DataTableClipboard.h"
#include "Data/DataTableAssets.h"
#include <chrono>

namespace
{
	constexpr float32 ColumnPanelWidth   = 290.0f;
	constexpr float32 IssueListHeight    = 120.0f;
	constexpr int64   FileCheckInterval  = 1000;   // ms

	int64 nowMs()
	{
		return (int64)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	PString normalizeTokenPath(const PString& inPath)
	{
		const HAssetPath assetPath(inPath);
		return assetPath.IsValid() ? assetPath.GetAssetPath().ToString() : inPath;
	}

	float32 defaultColumnWidth(EDataTableColumnType inType)
	{
		switch (inType)
		{
		case EDataTableColumnType::Bool:
			return 70.0f;
		case EDataTableColumnType::Int:
		case EDataTableColumnType::Float:
			return 90.0f;
		case EDataTableColumnType::Enum:
			return 110.0f;
		case EDataTableColumnType::RowRef:
			return 130.0f;
		case EDataTableColumnType::AssetRef:
			return 230.0f;
		default:
			return 180.0f;
		}
	}

	PString trimmedCopy(const PString& inText)
	{
		PString copy = inText;
		copy.Trim();
		return copy;
	}

	// "A, B, C" → { A, B, C } (빈 항목은 뺀다)
	HList<PString> splitList(const PString& inText)
	{
		HList<PString> items;
		for (const PString& part : inText.Split(','))
		{
			const PString item = trimmedCopy(part);
			if (item.Empty() == false)
			{
				items.push_back(item);
			}
		}
		return items;
	}

	PString joinList(const HList<PString>& inItems)
	{
		PString text;
		for (const PString& item : inItems)
		{
			text = text.Empty() ? item : PString::Format("%s, %s", text, item);
		}
		return text;
	}

	PString formatNumber(float64 inValue)
	{
		HDataTableColumn column("N", EDataTableColumnType::Float);
		return column.FormatText(HDataTableValue::MakeFloat(inValue));
	}

	const HLinearColor& errorColor()
	{
		static const HLinearColor color = HGUI::DisplayColor(0xEB5757);
		return color;
	}

	const HLinearColor& warningColor()
	{
		static const HLinearColor color = HGUI::DisplayColor(0xE2A93B);
		return color;
	}

	const HLinearColor& mutedColor()
	{
		static const HLinearColor color = HGUI::DisplayColor(0x9A9A9A);
		return color;
	}
}

// 열린 테이블 하나(탭). 그리드 원본이고, 보기(필터 · 정렬)와 검증 결과를 문서 Revision 기준으로 캐시한다.
class HDataTableEditorTab : public IGUIGridSource
{
public:
	JGDataTableEditor*             Editor = nullptr;
	PSharedPtr<PDataTableDocument> Document;
	PGUIGrid                       Grid;

	// 보기: ViewRows[보기 행] = 데이터 행
	PString      Filter;
	int32        SortGridColumn = -1;   // -1 = 파일 순서
	bool         bSortAscending = true;
	HList<int32> ViewRows;
	uint64       ViewRevision   = 0;
	PString      ViewFilter;
	int32        ViewSortColumn = -1;
	bool         bViewSortAscending = true;
	int32        PendingFocusDataRow = -1;   // 다음 보기 갱신 뒤 이 데이터 행의 키 칸을 고른다

	// 검증
	HList<HDataTableIssue> Issues;
	HHashMap<int64, int32> IssueByCell;   // (데이터 행 << 20) | 그리드 열 → Issues 번호
	int32 ErrorCount   = 0;
	int32 WarningCount = 0;

	// 메뉴 · 대화상자
	int32   MenuGridColumn = -1;
	int32   MenuViewRow    = -1;
	bool    bNewColumnRequest = false;
	int32   NewColumnAt       = 0;
	PString NewColumnName;
	int32   NewColumnType     = (int32)EDataTableColumnType::String;
	PString NewColumnError;
	bool    bFocusFilterRequest = false;
	bool    bExternalChange     = false;
	bool    bDropConfirmed      = false;

	// 열 속성 패널(그리드 열 기준. 0 = 키 열)
	int32            SelectedGridColumn = -1;
	int32            PendingForColumn   = -2;
	HDataTableColumn PendingSource;
	bool             bPendingEdited     = false;
	PString PendingName;
	int32   PendingType = (int32)EDataTableColumnType::String;
	PString PendingDefault;
	PString PendingDescription;
	bool    bPendingHasMin = false;
	bool    bPendingHasMax = false;
	PString PendingMin;
	PString PendingMax;
	PString PendingEnumValues;
	PString PendingEnumType;
	PString PendingAssetClass;
	PString PendingTable;
	PString PendingKeyColumn;

public:
	const HDataTableContent& GetContent() const
	{
		return Document->GetContent();
	}

	bool IsFileOrder() const
	{
		return ViewFilter.Empty() && ViewSortColumn < 0;
	}

	int32 DataRow(int32 inViewRow) const
	{
		return (inViewRow >= 0 && inViewRow < (int32)ViewRows.size()) ? ViewRows[inViewRow] : -1;
	}

	int32 ViewRowOf(int32 inDataRow) const
	{
		for (int32 v = 0; v < (int32)ViewRows.size(); ++v)
		{
			if (ViewRows[v] == inDataRow)
			{
				return v;
			}
		}
		return -1;
	}

	HDataTableGridRange SelectionRange() const
	{
		const HGUIGridSelection& selection = Grid.GetSelection();
		HDataTableGridRange range;
		range.FirstViewRow    = selection.GetFirstRow();
		range.FirstGridColumn = selection.GetFirstColumn();
		range.RowCount        = selection.GetRowCount();
		range.ColumnCount     = selection.GetColumnCount();
		return range;
	}

	const HDataTableIssue* FindIssue(int32 inDataRow, int32 inGridColumn) const
	{
		const int64 key = ((int64)inDataRow << 20) | (int64)inGridColumn;
		HHashMap<int64, int32>::const_iterator iter = IssueByCell.find(key);
		return (iter == IssueByCell.end()) ? nullptr : &Issues[iter->second];
	}

	// 문서가 바뀌었거나 필터 · 정렬이 바뀌었으면 보기와 검증을 다시 만든다
	void RefreshView(const IDataTableReferenceResolver* inResolver)
	{
		const bool bChanged = ViewRevision != Document->GetRevision() || DataTableSameText(ViewFilter, Filter) == false
			|| ViewSortColumn != SortGridColumn || bViewSortAscending != bSortAscending;
		if (bChanged == false)
		{
			return;
		}

		const HDataTableContent& content = GetContent();
		ViewFilter         = Filter;
		ViewSortColumn     = (SortGridColumn <= content.GetColumnCount()) ? SortGridColumn : -1;
		bViewSortAscending = bSortAscending;
		ViewRevision       = Document->GetRevision();

		ViewRows.clear();
		const PString filter = trimmedCopy(ViewFilter);
		for (int32 r = 0; r < content.GetRowCount(); ++r)
		{
			if (filter.Empty() || rowMatches(content, r, filter))
			{
				ViewRows.push_back(r);
			}
		}
		if (ViewSortColumn >= 0)
		{
			SortRows(content, ViewSortColumn, bViewSortAscending, ViewRows);
		}

		Issues = Document->GetLoadIssues();
		Document->Validate(inResolver, Issues);
		IssueByCell.clear();
		ErrorCount   = 0;
		WarningCount = 0;
		for (int32 i = 0; i < (int32)Issues.size(); ++i)
		{
			const HDataTableIssue& issue = Issues[i];
			(issue.Severity == EDataTableIssueSeverity::Error) ? ++ErrorCount : ++WarningCount;
			if (issue.Row >= 0)
			{
				const int64 key = ((int64)issue.Row << 20) | (int64)(issue.Column + 1);
				if (IssueByCell.contains(key) == false)
				{
					IssueByCell[key] = i;
				}
			}
		}

		if (PendingFocusDataRow >= 0)
		{
			const int32 viewRow = ViewRowOf(PendingFocusDataRow);
			if (viewRow >= 0)
			{
				Grid.SetFocusCell(viewRow, 0);
			}
			PendingFocusDataRow = -1;
		}
	}

	static bool rowMatches(const HDataTableContent& inContent, int32 inRow, const PString& inFilter)
	{
		const HDataTableRow& row = inContent.Rows[inRow];
		if (DataTableContainsIgnoreCase(row.Key, inFilter))
		{
			return true;
		}
		for (int32 c = 0; c < inContent.GetColumnCount(); ++c)
		{
			if (DataTableContainsIgnoreCase(inContent.Schema.Columns[c].FormatText(row.Values[c]), inFilter))
			{
				return true;
			}
		}
		return false;
	}

	// 그리드 열 값으로 안정 정렬(같은 값은 파일 순서)
	static void SortRows(const HDataTableContent& inContent, int32 inGridColumn, bool bAscending, HList<int32>& rows)
	{
		std::stable_sort(rows.begin(), rows.end(), [&](int32 a, int32 b)
			{
				int32 order = 0;
				if (inGridColumn == 0)
				{
					order = inContent.Rows[a].Key.GetRawString().compare(inContent.Rows[b].Key.GetRawString());
				}
				else
				{
					const HDataTableValue& x = inContent.Rows[a].Values[inGridColumn - 1];
					const HDataTableValue& y = inContent.Rows[b].Values[inGridColumn - 1];
					switch (x.GetKind())
					{
					case EDataTableValueKind::Bool:
						order = (int32)x.GetBool() - (int32)y.GetBool();
						break;
					case EDataTableValueKind::Int:
						order = (x.GetInt() < y.GetInt()) ? -1 : ((x.GetInt() > y.GetInt()) ? 1 : 0);
						break;
					case EDataTableValueKind::Float:
						order = (x.GetFloat() < y.GetFloat()) ? -1 : ((x.GetFloat() > y.GetFloat()) ? 1 : 0);
						break;
					default:
						order = x.GetText().GetRawString().compare(y.GetText().GetRawString());
						break;
					}
				}
				return bAscending ? (order < 0) : (order > 0);
			});
	}

	// IGUIGridSource
	virtual int32 GetRowCount() const override
	{
		return (int32)ViewRows.size();
	}

	virtual int32 GetColumnCount() const override
	{
		return GetContent().GetColumnCount() + 1;
	}

	virtual PString GetColumnHeader(int32 inColumn) const override
	{
		const HDataTableContent& content = GetContent();
		const PString sortMark = (inColumn == ViewSortColumn) ? PString(bViewSortAscending ? "  ^" : "  v") : PString();
		if (inColumn == 0)
		{
			return PString::Format("%s  [key]%s", content.Schema.KeyColumn, sortMark);
		}
		const HDataTableColumn& column = content.Schema.Columns[inColumn - 1];
		return PString::Format("%s  [%s]%s", column.Name, PString(DataTableColumnTypeToString(column.Type)), sortMark);
	}

	virtual PString GetColumnTooltip(int32 inColumn) const override
	{
		if (inColumn == 0)
		{
			return "Row key (unique ID). Other tables refer to rows by this key";
		}
		const HDataTableColumn& column = GetContent().Schema.Columns[inColumn - 1];
		PString tooltip = PString::Format("%s : %s\ndefault: %s", column.Name, PString(DataTableColumnTypeToString(column.Type)), column.FormatText(column.Default));
		if (column.Description.Empty() == false)
		{
			tooltip = PString::Format("%s\n%s", tooltip, column.Description);
		}
		return tooltip;
	}

	virtual float32 GetColumnWidth(int32 inColumn) const override
	{
		if (inColumn == 0)
		{
			return 140.0f;
		}
		return defaultColumnWidth(GetContent().Schema.Columns[inColumn - 1].Type);
	}

	virtual PString GetRowLabel(int32 inRow) const override
	{
		// 정렬 · 필터 중에도 파일의 행 번호를 보인다(문제 목록 · 로그의 행 번호와 같다)
		return PString::FromInt32(DataRow(inRow) + 1);
	}

	virtual void GetCell(int32 inRow, int32 inColumn, HGUIGridCell& outCell) const override
	{
		const HDataTableContent& content = GetContent();
		const int32 dataRow = DataRow(inRow);
		if (dataRow < 0)
		{
			return;
		}
		const HDataTableRow& row = content.Rows[dataRow];

		if (inColumn == 0)
		{
			outCell.Text = row.Key;
		}
		else
		{
			const HDataTableColumn& column = content.Schema.Columns[inColumn - 1];
			const HDataTableValue&  value  = row.Values[inColumn - 1];
			outCell.Text        = column.FormatText(value);
			outCell.bAlignRight = column.Type == EDataTableColumnType::Int || column.Type == EDataTableColumnType::Float;
			outCell.bMuted      = value == column.Default;
			outCell.bChecked    = column.Type == EDataTableColumnType::Bool && value.GetBool();
			if (column.Type == EDataTableColumnType::String && outCell.Text.Contains("\n"))
			{
				outCell.Tooltip = outCell.Text;
			}
		}

		const HDataTableIssue* issue = FindIssue(dataRow, inColumn);
		if (issue != nullptr)
		{
			outCell.bError  = true;
			outCell.Tooltip = issue->Message;
		}
	}

	virtual EGUIGridEditor GetEditor(int32 inRow, int32 inColumn) const override
	{
		if (inColumn == 0)
		{
			return EGUIGridEditor::Text;
		}
		switch (GetContent().Schema.Columns[inColumn - 1].Type)
		{
		case EDataTableColumnType::Bool:
			return EGUIGridEditor::Checkbox;
		case EDataTableColumnType::Enum:
		case EDataTableColumnType::RowRef:
		case EDataTableColumnType::AssetRef:
			return EGUIGridEditor::Combo;
		default:
			return EGUIGridEditor::Text;
		}
	}

	virtual void GetComboItems(int32 inRow, int32 inColumn, HList<PString>& outItems) const override;

	virtual PString GetEditText(int32 inRow, int32 inColumn) const override
	{
		HGUIGridCell cell;
		const int32 dataRow = DataRow(inRow);
		if (dataRow < 0)
		{
			return PString();
		}
		const HDataTableContent& content = GetContent();
		if (inColumn == 0)
		{
			return content.Rows[dataRow].Key;
		}
		return content.Schema.Columns[inColumn - 1].FormatText(content.Rows[dataRow].Values[inColumn - 1]);
	}
	// ~IGUIGridSource

	// 동작 (실행 취소 한 단계씩)
	bool Apply(const HList<HDataTableEdit>& inEdits, const PString& inLabel);
	void Undo();
	void Redo();
	void CommitText(int32 inViewRow, int32 inGridColumn, const PString& inText);
	void Toggle(int32 inViewRow, int32 inGridColumn);
	void Copy(bool bCut);
	void Paste();
	void Clear();
	void InsertRow(int32 inViewRow);
	void DuplicateRow(int32 inViewRow);
	void DeleteSelectedRows();
	void MoveSelectedRows(int32 inDirection);
	void InsertColumn(int32 inAt, const PString& inName, EDataTableColumnType inType);
	void DeleteColumn(int32 inGridColumn);
	void MoveColumn(int32 inGridColumn, int32 inDirection);
	void ApplySortToRowOrder();
	void FocusIssue(const HDataTableIssue& inIssue);

	// 그리기
	void HandleEvents(const HList<HGUIGridEvent>& inEvents);
	void DrawMenus();
	void DrawNewColumnModal();
	void DrawColumnPanel();
	void LoadPending(bool bForce);
	void ApplyPending();
	void DrawIssues(float32 inHeight);
	void DrawStatus();
};

// 참조 찾기: 열린 탭(저장 안 된 내용 포함)을 먼저 보고, 없으면 에셋 DB. 에셋 존재는 한 번의 검증 안에서 캐시한다.
class HDataTableEditorResolver : public IDataTableReferenceResolver
{
	const JGDataTableEditor& _editor;
	HDataTableAssetResolver  _assets;
	mutable HHashMap<PString, bool> _assetCache;

public:
	explicit HDataTableEditorResolver(const JGDataTableEditor& inEditor)
		: _editor(inEditor)
	{
	}

	virtual bool GetTableKeys(const PString& inTablePath, HList<PString>* outKeys) const override
	{
		const PString tokenPath = normalizeTokenPath(inTablePath);
		for (const HSTLUniquePtr<HDataTableEditorTab>& tab : _editor._tabs)
		{
			if (DataTableSameText(tab->Document->GetTokenPath(), tokenPath))
			{
				if (outKeys != nullptr)
				{
					outKeys->clear();
					for (const HDataTableRow& row : tab->GetContent().Rows)
					{
						outKeys->push_back(row.Key);
					}
				}
				return true;
			}
		}
		return _assets.GetTableKeys(inTablePath, outKeys);
	}

	virtual bool HasAsset(const PString& inAssetPath) const override
	{
		HHashMap<PString, bool>::const_iterator iter = _assetCache.find(inAssetPath);
		if (iter != _assetCache.end())
		{
			return iter->second;
		}
		const bool bExists = _assets.HasAsset(inAssetPath);
		_assetCache[inAssetPath] = bExists;
		return bExists;
	}
};

void HDataTableEditorTab::GetComboItems(int32 inRow, int32 inColumn, HList<PString>& outItems) const
{
	outItems.clear();
	if (inColumn <= 0)
	{
		return;
	}

	const HDataTableColumn& column = GetContent().Schema.Columns[inColumn - 1];
	switch (column.Type)
	{
	case EDataTableColumnType::Enum:
		column.GetEnumNames(outItems);
		break;
	case EDataTableColumnType::RowRef:
	{
		outItems.push_back(PString());
		HList<PString> keys;
		if (column.Table.Empty())
		{
			for (const HDataTableRow& row : GetContent().Rows)
			{
				keys.push_back(row.Key);
			}
		}
		else
		{
			HDataTableEditorResolver resolver(*Editor);
			resolver.GetTableKeys(column.Table, &keys);
		}
		outItems.insert(outItems.end(), keys.begin(), keys.end());
		break;
	}
	case EDataTableColumnType::AssetRef:
	{
		outItems.push_back(PString());
		HList<PString> paths;
		GetLoadedAssetPaths(column.AssetClass, paths);
		outItems.insert(outItems.end(), paths.begin(), paths.end());
		break;
	}
	default:
		break;
	}
}

// ---- 탭 동작 ----

bool HDataTableEditorTab::Apply(const HList<HDataTableEdit>& inEdits, const PString& inLabel)
{
	if (inEdits.empty())
	{
		return true;
	}

	PString error;
	if (Document->Apply(inEdits, inLabel, &error) == false)
	{
		Editor->setStatus(PString::Format("%s failed: %s", inLabel, error), true);
		return false;
	}
	// 앞선 실패 문구가 고친 뒤에도 남지 않게 한다(성공 문구는 각 동작이 덮어쓴다)
	Editor->clearErrorStatus();
	return true;
}

void HDataTableEditorTab::Undo()
{
	if (Document->CanUndo() == false)
	{
		Editor->setStatus("Nothing to undo", false);
		return;
	}
	const PString label = Document->GetUndoLabel();
	Document->Undo();
	Editor->setStatus(PString::Format("Undo: %s", label), false);
}

void HDataTableEditorTab::Redo()
{
	if (Document->CanRedo() == false)
	{
		Editor->setStatus("Nothing to redo", false);
		return;
	}
	const PString label = Document->GetRedoLabel();
	Document->Redo();
	Editor->setStatus(PString::Format("Redo: %s", label), false);
}

void HDataTableEditorTab::CommitText(int32 inViewRow, int32 inGridColumn, const PString& inText)
{
	const int32 dataRow = DataRow(inViewRow);
	if (dataRow < 0)
	{
		return;
	}
	const HDataTableContent& content = GetContent();

	if (inGridColumn == 0)
	{
		const PString key = trimmedCopy(inText);
		PString reason;
		if (IsValidDataTableKey(key, &reason) == false)
		{
			Editor->setStatus(PString::Format("Key '%s' is invalid (%s)", key, reason), true);
			return;
		}
		if (DataTableSameText(key, content.Rows[dataRow].Key))
		{
			return;
		}
		Apply({ HDataTableEdit::MakeSetKeys({ HPair<int32, PString>(dataRow, key) }) }, "Rename key");
		return;
	}

	const HDataTableColumn& column = content.Schema.Columns[inGridColumn - 1];
	HDataTableValue value;
	PString error;
	if (column.ParseText(inText, &value, &error) == false)
	{
		Editor->setStatus(PString::Format("%s: %s", column.Name, error), true);
		return;
	}
	if (value == content.Rows[dataRow].Values[inGridColumn - 1])
	{
		return;
	}

	HDataTableCellAssignment cell;
	cell.Row    = dataRow;
	cell.Column = inGridColumn - 1;
	cell.Value  = value;
	Apply({ HDataTableEdit::MakeSetCells({ cell }) }, PString::Format("Edit %s", column.Name));
}

void HDataTableEditorTab::Toggle(int32 inViewRow, int32 inGridColumn)
{
	const int32 dataRow = DataRow(inViewRow);
	if (dataRow < 0 || inGridColumn <= 0)
	{
		return;
	}

	const HDataTableValue& current = GetContent().Rows[dataRow].Values[inGridColumn - 1];
	HDataTableCellAssignment cell;
	cell.Row    = dataRow;
	cell.Column = inGridColumn - 1;
	cell.Value  = HDataTableValue::MakeBool(current.GetBool() == false);
	Apply({ HDataTableEdit::MakeSetCells({ cell }) }, PString::Format("Toggle %s", GetContent().Schema.Columns[inGridColumn - 1].Name));
}

void HDataTableEditorTab::Copy(bool bCut)
{
	if (ViewRows.empty())
	{
		return;
	}

	const HDataTableGridRange range = SelectionRange();
	const HDataTableTextGrid cells = CopyDataTableRange(GetContent(), ViewRows, range);
	HGUI::SetClipboardText(HDataTableClipboard::EncodeTsv(cells));

	if (bCut)
	{
		Apply(PlanDataTableClear(GetContent(), ViewRows, range), "Cut");
	}
	Editor->setStatus(PString::Format("%s %d x %d cell(s)", PString(bCut ? "Cut" : "Copied"), range.RowCount, range.ColumnCount), false);
}

void HDataTableEditorTab::Paste()
{
	HDataTableTextGrid cells;
	if (HDataTableClipboard::DecodeTsv(HGUI::GetClipboardText(), &cells) == false)
	{
		Editor->setStatus("The clipboard is empty", true);
		return;
	}

	const HDataTableGridRange range = SelectionRange();
	HList<HDataTableEdit> edits;
	HList<PString> errors;
	if (PlanDataTablePaste(GetContent(), ViewRows, IsFileOrder(), range, cells, &edits, &errors) == false)
	{
		const PString more = (errors.size() > 1) ? PString::Format(" (+%d more, see log)", (int32)errors.size() - 1) : PString();
		Editor->setStatus(PString::Format("Paste rejected: %s%s", errors.empty() ? PString("unknown") : errors[0], more), true);
		for (const PString& error : errors)
		{
			JG_LOG(DataTable, ELogLevel::Warning, "%s : paste rejected: %s", Document->GetDisplayName(), error);
		}
		return;
	}

	if (Apply(edits, "Paste"))
	{
		const bool bFill = cells.size() == 1 && cells[0].size() == 1;
		const int32 rows    = bFill ? range.RowCount : (int32)cells.size();
		const int32 columns = bFill ? range.ColumnCount : (int32)cells[0].size();

		HGUIGridSelection selection;
		selection.AnchorRow    = range.FirstViewRow;
		selection.AnchorColumn = range.FirstGridColumn;
		selection.FocusRow     = range.FirstViewRow + rows - 1;
		selection.FocusColumn  = range.FirstGridColumn + columns - 1;
		Grid.SetSelection(selection, false);
		Editor->setStatus(PString::Format("Pasted %d x %d cell(s)", rows, columns), false);
	}
}

void HDataTableEditorTab::Clear()
{
	if (ViewRows.empty())
	{
		return;
	}
	Apply(PlanDataTableClear(GetContent(), ViewRows, SelectionRange()), "Clear");
}

void HDataTableEditorTab::InsertRow(int32 inViewRow)
{
	const HDataTableContent& content = GetContent();
	const int32 dataRow = (inViewRow >= 0 && inViewRow < (int32)ViewRows.size()) ? ViewRows[inViewRow] : content.GetRowCount();
	const HDataTableRow row = content.MakeDefaultRow(content.MakeUniqueKey("Row"));
	if (Apply({ HDataTableEdit::MakeInsertRows({ HPair<int32, HDataTableRow>(dataRow, row) }) }, "Insert row"))
	{
		PendingFocusDataRow = dataRow;
		Editor->setStatus(PString::Format("Inserted row '%s'%s", row.Key, PString(ViewFilter.Empty() ? "" : " (the filter may hide it)")), false);
	}
}

void HDataTableEditorTab::DuplicateRow(int32 inViewRow)
{
	const int32 dataRow = DataRow(inViewRow);
	if (dataRow < 0)
	{
		return;
	}

	const HDataTableContent& content = GetContent();
	const PString sourceKey = content.Rows[dataRow].Key;
	HDataTableRow row = content.Rows[dataRow];
	row.Key = content.MakeUniqueKey(PString::Format("%s_copy", sourceKey));
	if (Apply({ HDataTableEdit::MakeInsertRows({ HPair<int32, HDataTableRow>(dataRow + 1, row) }) }, "Duplicate row"))
	{
		PendingFocusDataRow = dataRow + 1;
		Editor->setStatus(PString::Format("Duplicated row '%s' as '%s'", sourceKey, row.Key), false);
	}
}

void HDataTableEditorTab::DeleteSelectedRows()
{
	const HDataTableGridRange range = SelectionRange();
	HList<int32> rows;
	for (int32 v = range.FirstViewRow; v < range.FirstViewRow + range.RowCount; ++v)
	{
		const int32 dataRow = DataRow(v);
		if (dataRow >= 0)
		{
			rows.push_back(dataRow);
		}
	}
	std::sort(rows.begin(), rows.end());
	rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
	if (rows.empty())
	{
		return;
	}

	if (Apply({ HDataTableEdit::MakeRemoveRows(rows) }, "Delete rows"))
	{
		Editor->setStatus(PString::Format("Deleted %d row(s)", (int32)rows.size()), false);
	}
}

void HDataTableEditorTab::MoveSelectedRows(int32 inDirection)
{
	if (IsFileOrder() == false)
	{
		Editor->setStatus("Rows move only in file order (clear the sort and filter)", true);
		return;
	}

	const HDataTableGridRange range = SelectionRange();
	const int32 rowCount = GetContent().GetRowCount();
	const int32 first = range.FirstViewRow;
	const int32 last  = range.FirstViewRow + range.RowCount - 1;
	if ((inDirection < 0 && first <= 0) || (inDirection > 0 && last >= rowCount - 1))
	{
		return;
	}

	// [first, last] 묶음을 한 칸 옮기는 순서표
	HList<int32> order;
	for (int32 r = 0; r < rowCount; ++r)
	{
		order.push_back(r);
	}
	if (inDirection < 0)
	{
		std::rotate(order.begin() + first - 1, order.begin() + first, order.begin() + last + 1);
	}
	else
	{
		std::rotate(order.begin() + first, order.begin() + last + 1, order.begin() + last + 2);
	}

	if (Apply({ HDataTableEdit::MakeReorderRows(order) }, inDirection < 0 ? "Move rows up" : "Move rows down"))
	{
		HGUIGridSelection selection = Grid.GetSelection();
		selection.AnchorRow += inDirection;
		selection.FocusRow  += inDirection;
		Grid.SetSelection(selection);
	}
}

void HDataTableEditorTab::InsertColumn(int32 inAt, const PString& inName, EDataTableColumnType inType)
{
	if (Apply({ HDataTableEdit::MakeInsertColumn(inAt, HDataTableColumn(inName, inType)) }, PString::Format("Add column %s", inName)))
	{
		SelectedGridColumn = inAt + 1;
		PendingForColumn   = -2;
	}
}

void HDataTableEditorTab::DeleteColumn(int32 inGridColumn)
{
	if (inGridColumn <= 0)
	{
		return;
	}
	const PString name = GetContent().Schema.Columns[inGridColumn - 1].Name;
	if (Apply({ HDataTableEdit::MakeRemoveColumn(inGridColumn - 1) }, PString::Format("Delete column %s", name)))
	{
		SelectedGridColumn = -1;
		Editor->setStatus(PString::Format("Deleted column '%s' (Ctrl+Z to undo)", name), false);
	}
}

void HDataTableEditorTab::MoveColumn(int32 inGridColumn, int32 inDirection)
{
	const int32 from = inGridColumn - 1;
	const int32 to   = from + inDirection;
	if (from < 0 || to < 0 || to >= GetContent().GetColumnCount())
	{
		return;
	}
	if (Apply({ HDataTableEdit::MakeMoveColumn(from, to) }, "Move column"))
	{
		SelectedGridColumn = to + 1;
		PendingForColumn   = -2;
	}
}

void HDataTableEditorTab::ApplySortToRowOrder()
{
	if (ViewSortColumn < 0)
	{
		return;
	}

	// 필터와 상관없이 모든 행을 같은 기준으로 정렬한다
	HList<int32> order;
	for (int32 r = 0; r < GetContent().GetRowCount(); ++r)
	{
		order.push_back(r);
	}
	SortRows(GetContent(), ViewSortColumn, bViewSortAscending, order);
	if (Apply({ HDataTableEdit::MakeReorderRows(order) }, "Sort rows"))
	{
		SortGridColumn = -1;
		Editor->setStatus("Applied the sort to the row order", false);
	}
}

void HDataTableEditorTab::FocusIssue(const HDataTableIssue& inIssue)
{
	if (inIssue.Row < 0)
	{
		if (inIssue.Column >= 0)
		{
			SelectedGridColumn = inIssue.Column + 1;
		}
		return;
	}

	int32 viewRow = ViewRowOf(inIssue.Row);
	if (viewRow < 0)
	{
		// 필터에 가려졌다 — 필터를 지우고 다음 갱신 뒤 고른다
		Filter.Reset();
		PendingFocusDataRow = inIssue.Row;
		return;
	}
	Grid.SetFocusCell(viewRow, inIssue.Column + 1);
	Grid.SetKeyboardFocus(true);
}

// ---- 탭 그리기 ----

void HDataTableEditorTab::HandleEvents(const HList<HGUIGridEvent>& inEvents)
{
	for (const HGUIGridEvent& event : inEvents)
	{
		switch (event.Type)
		{
		case EGUIGridEventType::CommitText:
		case EGUIGridEventType::PickComboItem:
			CommitText(event.Row, event.Column, event.Text);
			break;
		case EGUIGridEventType::ToggleCheckbox:
			Toggle(event.Row, event.Column);
			break;
		case EGUIGridEventType::Copy:
			Copy(false);
			break;
		case EGUIGridEventType::Cut:
			Copy(true);
			break;
		case EGUIGridEventType::Paste:
			Paste();
			break;
		case EGUIGridEventType::Delete:
			Clear();
			break;
		case EGUIGridEventType::Undo:
			Undo();
			break;
		case EGUIGridEventType::Redo:
			Redo();
			break;
		case EGUIGridEventType::Save:
			Editor->saveTab(*this, false);
			break;
		case EGUIGridEventType::Find:
			bFocusFilterRequest = true;
			break;
		case EGUIGridEventType::InsertRow:
			InsertRow(event.Row);
			break;
		case EGUIGridEventType::DeleteRows:
			DeleteSelectedRows();
			break;
		case EGUIGridEventType::HeaderClicked:
			SelectedGridColumn = event.Column;
			break;
		case EGUIGridEventType::HeaderContextMenu:
			MenuGridColumn = event.Column;
			HGUI::OpenPopup("##DataTableHeaderMenu");
			break;
		case EGUIGridEventType::CellContextMenu:
			MenuViewRow    = event.Row;
			MenuGridColumn = event.Column;
			HGUI::OpenPopup("##DataTableCellMenu");
			break;
		default:
			break;
		}
	}
}

void HDataTableEditorTab::DrawMenus()
{
	if (HGUI::BeginPopup("##DataTableHeaderMenu"))
	{
		const int32 columnCount = GetContent().GetColumnCount();
		if (HGUI::MenuItem("Sort ascending"))
		{
			SortGridColumn = MenuGridColumn;
			bSortAscending = true;
		}
		if (HGUI::MenuItem("Sort descending"))
		{
			SortGridColumn = MenuGridColumn;
			bSortAscending = false;
		}
		if (HGUI::MenuItem("Clear sort", PString(), SortGridColumn >= 0))
		{
			SortGridColumn = -1;
		}
		if (HGUI::MenuItem("Apply sort to row order", PString(), ViewSortColumn >= 0))
		{
			ApplySortToRowOrder();
		}
		HGUI::Separator();
		if (HGUI::MenuItem("Insert column left", PString(), MenuGridColumn > 0))
		{
			bNewColumnRequest = true;
			NewColumnAt       = MenuGridColumn - 1;
		}
		if (HGUI::MenuItem("Insert column right"))
		{
			bNewColumnRequest = true;
			NewColumnAt       = MenuGridColumn;
		}
		if (HGUI::MenuItem("Move column left", PString(), MenuGridColumn > 1))
		{
			MoveColumn(MenuGridColumn, -1);
		}
		if (HGUI::MenuItem("Move column right", PString(), MenuGridColumn > 0 && MenuGridColumn < columnCount))
		{
			MoveColumn(MenuGridColumn, 1);
		}
		if (HGUI::MenuItem("Delete column", PString(), MenuGridColumn > 0))
		{
			DeleteColumn(MenuGridColumn);
		}
		HGUI::Separator();
		if (HGUI::MenuItem("Properties"))
		{
			SelectedGridColumn = MenuGridColumn;
		}
		HGUI::EndPopup();
	}

	if (HGUI::BeginPopup("##DataTableCellMenu"))
	{
		const bool bHasRow = DataRow(MenuViewRow) >= 0;
		if (HGUI::MenuItem("Insert row above", "Ctrl+Shift+="))
		{
			InsertRow(MenuViewRow);
		}
		if (HGUI::MenuItem("Insert row below", PString(), bHasRow))
		{
			InsertRow(MenuViewRow + 1 < (int32)ViewRows.size() ? MenuViewRow + 1 : -1);
		}
		if (HGUI::MenuItem("Duplicate row", PString(), bHasRow))
		{
			DuplicateRow(MenuViewRow);
		}
		if (HGUI::MenuItem("Delete rows", "Ctrl+-", bHasRow))
		{
			DeleteSelectedRows();
		}
		if (HGUI::MenuItem("Move rows up", PString(), bHasRow && IsFileOrder()))
		{
			MoveSelectedRows(-1);
		}
		if (HGUI::MenuItem("Move rows down", PString(), bHasRow && IsFileOrder()))
		{
			MoveSelectedRows(1);
		}
		HGUI::Separator();
		if (HGUI::MenuItem("Copy", "Ctrl+C", bHasRow))
		{
			Copy(false);
		}
		if (HGUI::MenuItem("Cut", "Ctrl+X", bHasRow))
		{
			Copy(true);
		}
		if (HGUI::MenuItem("Paste", "Ctrl+V"))
		{
			Paste();
		}
		if (HGUI::MenuItem("Reset to default", "Delete", bHasRow))
		{
			Clear();
		}
		HGUI::EndPopup();
	}

	DrawNewColumnModal();
}

void HDataTableEditorTab::DrawNewColumnModal()
{
	if (bNewColumnRequest)
	{
		bNewColumnRequest = false;
		NewColumnName     = GetContent().Schema.FindColumn("NewColumn") < 0 ? PString("NewColumn") : PString::Format("NewColumn%d", GetContent().GetColumnCount() + 1);
		NewColumnType     = (int32)EDataTableColumnType::String;
		NewColumnError.Reset();
		HGUI::OpenPopup("Add Column##DataTable");
	}

	if (HGUI::BeginPopupModal("Add Column##DataTable") == false)
	{
		return;
	}

	HGUI::AlignTextToFramePadding();
	HGUI::Text("Name");
	HGUI::SameLineAt(80.0f);
	HGUI::InputTextWithHint("NewColumnName", "column name", NewColumnName, 220.0f);

	HGUI::AlignTextToFramePadding();
	HGUI::Text("Type");
	HGUI::SameLineAt(80.0f);
	if (HGUI::BeginCombo("NewColumnType", DataTableColumnTypeToString((EDataTableColumnType)NewColumnType), 220.0f))
	{
		for (int32 t = 0; t < (int32)EDataTableColumnType::Count; ++t)
		{
			if (HGUI::Selectable(DataTableColumnTypeToString((EDataTableColumnType)t), t == NewColumnType))
			{
				NewColumnType = t;
			}
		}
		HGUI::EndCombo();
	}

	if (NewColumnError.Empty() == false)
	{
		HGUI::Text(NewColumnError, errorColor());
	}

	if (HGUI::Button("Add", HVector2(100.0f, 0.0f)))
	{
		const PString name = trimmedCopy(NewColumnName);
		PString reason;
		if (IsValidDataTableColumnName(name, &reason) == false)
		{
			NewColumnError = PString::Format("Name is invalid (%s)", reason);
		}
		else if (GetContent().Schema.FindColumn(name) >= 0 || DataTableSameText(name, GetContent().Schema.KeyColumn))
		{
			NewColumnError = PString::Format("'%s' is already used", name);
		}
		else
		{
			InsertColumn(NewColumnAt, name, (EDataTableColumnType)NewColumnType);
			HGUI::CloseCurrentPopup();
		}
	}
	HGUI::SameLine();
	if (HGUI::Button("Cancel", HVector2(100.0f, 0.0f)))
	{
		HGUI::CloseCurrentPopup();
	}
	HGUI::EndPopup();
}

void HDataTableEditorTab::LoadPending(bool bForce)
{
	const HDataTableContent& content = GetContent();
	if (SelectedGridColumn < 0 || SelectedGridColumn > content.GetColumnCount())
	{
		SelectedGridColumn = -1;
		PendingForColumn   = -2;
		return;
	}

	// 고른 열이 바뀌었거나, 고치는 중이 아닌데 열 정의가 바뀌었으면(실행 취소 등) 다시 채운다
	const bool bColumnChanged = PendingForColumn != SelectedGridColumn;
	const bool bSourceChanged = (SelectedGridColumn == 0) ? (DataTableSameText(PendingKeyColumn, content.Schema.KeyColumn) == false && bPendingEdited == false)
		: (PendingSource != content.Schema.Columns[SelectedGridColumn - 1] && bPendingEdited == false);
	if (bForce == false && bColumnChanged == false && bSourceChanged == false)
	{
		return;
	}

	PendingForColumn = SelectedGridColumn;
	bPendingEdited   = false;
	PendingKeyColumn = content.Schema.KeyColumn;
	if (SelectedGridColumn == 0)
	{
		return;
	}

	const HDataTableColumn& column = content.Schema.Columns[SelectedGridColumn - 1];
	PendingSource      = column;
	PendingName        = column.Name;
	PendingType        = (int32)column.Type;
	PendingDefault     = column.FormatText(column.Default);
	PendingDescription = column.Description;
	bPendingHasMin     = column.bHasMin;
	bPendingHasMax     = column.bHasMax;
	PendingMin         = column.bHasMin ? formatNumber(column.Min) : PString();
	PendingMax         = column.bHasMax ? formatNumber(column.Max) : PString();
	PendingEnumValues  = joinList(column.EnumValues);
	PendingEnumType    = column.EnumType;
	PendingAssetClass  = column.AssetClass;
	PendingTable       = column.Table;
}

void HDataTableEditorTab::ApplyPending()
{
	const HDataTableContent& content = GetContent();
	if (SelectedGridColumn == 0)
	{
		const PString name = trimmedCopy(PendingKeyColumn);
		PString reason;
		if (IsValidDataTableColumnName(name, &reason) == false)
		{
			Editor->setStatus(PString::Format("Key column name is invalid (%s)", reason), true);
			return;
		}
		if (content.Schema.FindColumn(name) >= 0)
		{
			Editor->setStatus(PString::Format("'%s' is already a column", name), true);
			return;
		}
		if (DataTableSameText(name, content.Schema.KeyColumn) == false)
		{
			Apply({ HDataTableEdit::MakeSetKeyColumn(name) }, "Rename key column");
		}
		LoadPending(true);
		return;
	}

	const int32 columnIndex = SelectedGridColumn - 1;
	HDataTableColumn column = content.Schema.Columns[columnIndex];
	column.Name        = trimmedCopy(PendingName);
	column.Type        = (EDataTableColumnType)PendingType;
	column.Description = PendingDescription;
	column.EnumValues  = splitList(PendingEnumValues);
	column.EnumType    = trimmedCopy(PendingEnumType);
	column.AssetClass  = trimmedCopy(PendingAssetClass);
	column.Table       = trimmedCopy(PendingTable).Empty() ? PString() : normalizeTokenPath(trimmedCopy(PendingTable));

	PString reason;
	if (IsValidDataTableColumnName(column.Name, &reason) == false)
	{
		Editor->setStatus(PString::Format("Column name is invalid (%s)", reason), true);
		return;
	}
	const int32 sameName = content.Schema.FindColumn(column.Name);
	if ((sameName >= 0 && sameName != columnIndex) || DataTableSameText(column.Name, content.Schema.KeyColumn))
	{
		Editor->setStatus(PString::Format("'%s' is already used", column.Name), true);
		return;
	}

	const bool bNumber = column.Type == EDataTableColumnType::Int || column.Type == EDataTableColumnType::Float;
	column.bHasMin = bNumber && bPendingHasMin;
	column.bHasMax = bNumber && bPendingHasMax;
	HDataTableColumn floatParser("N", EDataTableColumnType::Float);
	HDataTableValue limit;
	if (column.bHasMin)
	{
		if (floatParser.ParseText(PendingMin, &limit, nullptr) == false)
		{
			Editor->setStatus("Min is not a number", true);
			return;
		}
		column.Min = limit.GetFloat();
	}
	if (column.bHasMax)
	{
		if (floatParser.ParseText(PendingMax, &limit, nullptr) == false)
		{
			Editor->setStatus("Max is not a number", true);
			return;
		}
		column.Max = limit.GetFloat();
	}

	// 기본값: 빈 칸이면 타입 기본값(Enum 이면 첫 이름)
	HDataTableValue defaultValue = HDataTableValue::MakeDefault(DataTableValueKindOf(column.Type));
	if (column.Type == EDataTableColumnType::Enum)
	{
		HList<PString> names;
		if (column.GetEnumNames(names) && names.empty() == false)
		{
			defaultValue = HDataTableValue::MakeText(names[0]);
		}
	}
	if (trimmedCopy(PendingDefault).Empty() == false || column.Type == EDataTableColumnType::String)
	{
		PString error;
		if (column.ParseText(PendingDefault, &defaultValue, &error) == false)
		{
			Editor->setStatus(PString::Format("Default: %s", error), true);
			return;
		}
	}
	column.Default = defaultValue;

	if (column == content.Schema.Columns[columnIndex])
	{
		LoadPending(true);
		return;
	}

	HList<int32> failedRows;
	const HList<HDataTableValue> values = ConvertDataTableColumnValues(content, columnIndex, column, &failedRows);
	if (Apply({ HDataTableEdit::MakeReplaceColumn(columnIndex, column, values) }, PString::Format("Edit column %s", column.Name)))
	{
		if (failedRows.empty() == false)
		{
			Editor->setStatus(PString::Format("%d value(s) could not convert to %s and were reset to the default (Ctrl+Z to undo)",
				(int32)failedRows.size(), PString(DataTableColumnTypeToString(column.Type))), true);
		}
		LoadPending(true);
	}
}

void HDataTableEditorTab::DrawColumnPanel()
{
	LoadPending(false);
	if (SelectedGridColumn < 0)
	{
		return;
	}

	const float32 labelWidth = 90.0f;
	const float32 fieldWidth = -1.0f;
	auto field = [&](const char* inLabel, const char* inId, PString& value, const char* inHint)
	{
		HGUI::AlignTextToFramePadding();
		HGUI::Text(inLabel);
		HGUI::SameLineAt(labelWidth);
		HGUI::SetNextItemWidth(fieldWidth);
		if (HGUI::InputTextWithHint(inId, inHint, value))
		{
			bPendingEdited = true;
		}
	};

	if (SelectedGridColumn == 0)
	{
		HGUI::Text("Key column");
		HGUI::Separator();
		field("Name", "PendingKeyColumn", PendingKeyColumn, "Id");
		HGUI::TextWrapped("Rows are found by this key. Keys must be unique and have no spaces.");
		if (HGUI::Button("Apply", HVector2(90.0f, 0.0f)))
		{
			ApplyPending();
		}
		return;
	}

	HGUI::Text(PString::Format("Column %d", SelectedGridColumn));
	HGUI::Separator();
	field("Name", "PendingName", PendingName, "column name");

	HGUI::AlignTextToFramePadding();
	HGUI::Text("Type");
	HGUI::SameLineAt(labelWidth);
	if (HGUI::BeginCombo("PendingType", DataTableColumnTypeToString((EDataTableColumnType)PendingType), -1.0f))
	{
		for (int32 t = 0; t < (int32)EDataTableColumnType::Count; ++t)
		{
			if (HGUI::Selectable(DataTableColumnTypeToString((EDataTableColumnType)t), t == PendingType))
			{
				PendingType    = t;
				bPendingEdited = true;
			}
		}
		HGUI::EndCombo();
	}

	field("Default", "PendingDefault", PendingDefault, "type default");
	field("Description", "PendingDescription", PendingDescription, "shown as the header tooltip");

	const EDataTableColumnType type = (EDataTableColumnType)PendingType;
	if (type == EDataTableColumnType::Int || type == EDataTableColumnType::Float)
	{
		if (HGUI::Checkbox("Min##PendingHasMin", bPendingHasMin))
		{
			bPendingEdited = true;
		}
		HGUI::SameLineAt(labelWidth);
		HGUI::SetNextItemWidth(fieldWidth);
		if (HGUI::InputTextWithHint("PendingMin", "none", PendingMin))
		{
			bPendingEdited = true;
		}
		if (HGUI::Checkbox("Max##PendingHasMax", bPendingHasMax))
		{
			bPendingEdited = true;
		}
		HGUI::SameLineAt(labelWidth);
		HGUI::SetNextItemWidth(fieldWidth);
		if (HGUI::InputTextWithHint("PendingMax", "none", PendingMax))
		{
			bPendingEdited = true;
		}
	}
	else if (type == EDataTableColumnType::Enum)
	{
		field("Values", "PendingEnumValues", PendingEnumValues, "A, B, C");
		field("Enum type", "PendingEnumType", PendingEnumType, "or a reflected enum name");
	}
	else if (type == EDataTableColumnType::AssetRef)
	{
		field("Asset class", "PendingAssetClass", PendingAssetClass, "e.g. JGStaticMesh (empty = all)");
	}
	else if (type == EDataTableColumnType::RowRef)
	{
		field("Table", "PendingTable", PendingTable, "/JGGame/... (empty = this table)");
	}

	HGUI::Spacing();
	if (HGUI::Button("Apply", HVector2(90.0f, 0.0f)))
	{
		ApplyPending();
	}
	HGUI::SameLine();
	HGUI::BeginDisabled(bPendingEdited == false);
	if (HGUI::Button("Revert", HVector2(90.0f, 0.0f)))
	{
		LoadPending(true);
	}
	HGUI::EndDisabled();

	HGUI::Spacing();
	HGUI::Separator();
	if (HGUI::Button("< Move", HVector2(70.0f, 0.0f)))
	{
		MoveColumn(SelectedGridColumn, -1);
	}
	HGUI::SameLine();
	if (HGUI::Button("Move >", HVector2(70.0f, 0.0f)))
	{
		MoveColumn(SelectedGridColumn, 1);
	}
	HGUI::SameLine();
	if (HGUI::Button("Delete", HVector2(70.0f, 0.0f)))
	{
		DeleteColumn(SelectedGridColumn);
	}
}

void HDataTableEditorTab::DrawIssues(float32 inHeight)
{
	HGUI::BeginChild("##DataTableIssues", HVector2(0.0f, inHeight));
	if (Issues.empty())
	{
		HGUI::Text("No issues", mutedColor());
	}
	for (int32 i = 0; i < (int32)Issues.size(); ++i)
	{
		const HDataTableIssue& issue = Issues[i];
		const bool bError = issue.Severity == EDataTableIssueSeverity::Error;
		PString where;
		if (issue.Row >= 0)
		{
			const PString columnName = (issue.Column >= 0 && issue.Column < GetContent().GetColumnCount()) ? GetContent().Schema.Columns[issue.Column].Name : GetContent().Schema.KeyColumn;
			where = PString::Format("row %d, %s: ", issue.Row + 1, columnName);
		}
		else if (issue.Column >= 0 && issue.Column < GetContent().GetColumnCount())
		{
			where = PString::Format("column %s: ", GetContent().Schema.Columns[issue.Column].Name);
		}

		HGUI::PushStyleColor(EGUIColor::Text, bError ? errorColor() : warningColor());
		const PString label = PString::Format("%s %s%s##issue%d", PString(bError ? "[Error]" : "[Warning]"), where, issue.Message, i);
		if (HGUI::Selectable(label, false))
		{
			FocusIssue(issue);
		}
		HGUI::PopStyleColor();
	}
	HGUI::EndChild();
}

void HDataTableEditorTab::DrawStatus()
{
	const HGUIGridSelection& selection = Grid.GetSelection();
	const PString rows = (ViewRows.size() == (size_t)GetContent().GetRowCount())
		? PString::Format("%d rows", GetContent().GetRowCount())
		: PString::Format("%d of %d rows", (int32)ViewRows.size(), GetContent().GetRowCount());
	const PString order = IsFileOrder() ? PString() : PString("  |  view sorted or filtered (file order unchanged)");
	HGUI::Text(PString::Format("%s  |  selection %d x %d  |  %s%s  |  %s", rows, selection.GetRowCount(), selection.GetColumnCount(),
		PString(Document->IsDirty() ? "unsaved" : "saved"), order, Document->GetTokenPath()), mutedColor());
}

// ---- 창 ----

JGDataTableEditor::JGDataTableEditor()
{
}

JGDataTableEditor::~JGDataTableEditor()
{
}

PString JGDataTableEditor::GetTitleName() const
{
	return "Data Table Editor";
}

void JGDataTableEditor::setStatus(const PString& inText, bool bError)
{
	_statusText   = inText;
	_bStatusError = bError;
}

void JGDataTableEditor::clearErrorStatus()
{
	if (_bStatusError)
	{
		setStatus(PString(), false);
	}
}

HDataTableEditorTab* JGDataTableEditor::findTab(const PString& inTokenPath) const
{
	const PString tokenPath = normalizeTokenPath(inTokenPath);
	for (const HSTLUniquePtr<HDataTableEditorTab>& tab : _tabs)
	{
		if (DataTableSameText(tab->Document->GetTokenPath(), tokenPath))
		{
			return tab.get();
		}
	}
	return nullptr;
}

bool JGDataTableEditor::OpenTable(const PString& inTokenPath)
{
	HDataTableEditorTab* existing = findTab(inTokenPath);
	if (existing != nullptr)
	{
		_selectRequest = existing;
		return true;
	}

	PString error;
	PSharedPtr<PDataTableDocument> document = PDataTableDocument::Open(inTokenPath, &error);
	if (document == nullptr)
	{
		setStatus(PString::Format("Cannot open %s: %s", inTokenPath, error), true);
		return false;
	}

	HSTLUniquePtr<HDataTableEditorTab> tab = HSTLUniquePtr<HDataTableEditorTab>(new HDataTableEditorTab());
	tab->Editor   = this;
	tab->Document = document;
	_selectRequest = tab.get();
	_tabs.push_back(std::move(tab));

	const int32 loadIssues = (int32)document->GetLoadIssues().size();
	setStatus(PString::Format("Opened %s%s", document->GetTokenPath(), loadIssues > 0 ? PString(" - the file has load issues (see below)") : PString()), loadIssues > 0);
	return true;
}

void JGDataTableEditor::removeTab(HDataTableEditorTab* inTab)
{
	for (HList<HSTLUniquePtr<HDataTableEditorTab>>::iterator iter = _tabs.begin(); iter != _tabs.end(); ++iter)
	{
		if (iter->get() == inTab)
		{
			// 닫힌 탭의 실행 취소 문구 등이 남지 않게 한다
			setStatus(PString::Format("Closed %s", inTab->Document->GetTokenPath()), false);
			_tabs.erase(iter);
			break;
		}
	}
	if (_selectRequest == inTab)
	{
		_selectRequest = nullptr;
	}
	if (_closeRequest == inTab)
	{
		_closeRequest = nullptr;
	}
	if (_saveConfirm == inTab)
	{
		_saveConfirm = nullptr;
	}
	if (_visibleTab == inTab)
	{
		_visibleTab = nullptr;
	}
}

bool JGDataTableEditor::saveTab(HDataTableEditorTab& tab, bool bConfirmed)
{
	// 파일의 모르는 열은 저장하면 사라진다 — 처음 한 번 묻는다
	if (bConfirmed == false && tab.bDropConfirmed == false)
	{
		for (const HDataTableIssue& issue : tab.Document->GetLoadIssues())
		{
			if (issue.Message.Contains("dropped on save"))
			{
				_saveConfirm        = &tab;
				_bSaveConfirmRequest = true;
				return false;
			}
		}
	}

	HDataTableEditorResolver resolver(*this);
	PString error;
	if (tab.Document->Save(&resolver, &error) == false)
	{
		setStatus(error, true);
		return false;
	}

	tab.bExternalChange = false;
	tab.bDropConfirmed  = false;
	setStatus(PString::Format("Saved %s", tab.Document->GetTokenPath()), false);
	return true;
}

void JGDataTableEditor::OnUpdate()
{
	const int64 now = nowMs();
	if (now - _lastFileCheckMs < FileCheckInterval)
	{
		return;
	}
	_lastFileCheckMs = now;

	// 바깥 수정(텍스트 편집기 · git · 다른 에이전트). 저장 안 된 변경이 없으면 바로 다시 읽는다
	for (const HSTLUniquePtr<HDataTableEditorTab>& tab : _tabs)
	{
		if (tab->bExternalChange || tab->Document->HasExternalChange() == false)
		{
			continue;
		}

		if (tab->Document->IsDirty())
		{
			tab->bExternalChange = true;
			continue;
		}

		PString error;
		if (tab->Document->Reload(&error))
		{
			setStatus(PString::Format("Reloaded %s (changed on disk)", tab->Document->GetTokenPath()), false);
		}
		else
		{
			tab->bExternalChange = true;
			setStatus(error, true);
		}
	}
}

void JGDataTableEditor::OnGenerateGUI()
{
	// 도구 막대는 지난 프레임에 보인 탭(고른 탭)을 대상으로 한다
	HDataTableEditorTab* activeTab = nullptr;
	for (const HSTLUniquePtr<HDataTableEditorTab>& tab : _tabs)
	{
		if (tab.get() == _visibleTab || activeTab == nullptr)
		{
			activeTab = tab.get();
		}
	}
	_visibleTab = nullptr;

	drawToolbar(activeTab);

	if (_tabs.empty())
	{
		HGUI::Spacing();
		HGUI::TextWrapped("No table is open. Open one from the list above, or create a new table (Content/Data/....jgasset).");
		drawModals();
		return;
	}

	HDataTableEditorTab* closeRequested = nullptr;
	if (HGUI::BeginTabBar("##DataTableTabs"))
	{
		for (const HSTLUniquePtr<HDataTableEditorTab>& tab : _tabs)
		{
			bool bOpen = true;
			const bool bSelect = (_selectRequest == tab.get());
			const PString label = PString::Format("%s###%s", tab->Document->GetDisplayName(), tab->Document->GetTokenPath());
			if (HGUI::BeginDocumentTabItem(label, bOpen, tab->Document->IsDirty(), bSelect))
			{
				drawTab(*tab);
				HGUI::EndTabItem();
			}
			if (bOpen == false)
			{
				closeRequested = tab.get();
			}
		}
		HGUI::EndTabBar();
	}
	_selectRequest = nullptr;

	if (closeRequested != nullptr)
	{
		if (closeRequested->Document->IsDirty())
		{
			_closeRequest = closeRequested;
			HGUI::OpenPopup("Close Table##DataTableClose");
		}
		else
		{
			removeTab(closeRequested);
		}
	}

	drawModals();
}

void JGDataTableEditor::drawToolbar(HDataTableEditorTab* activeTab)
{
	if (HGUI::BeginCombo("DataTableOpen", "Open table...", 240.0f))
	{
		HList<HDataTableAssetEntry> tables;
		GetLoadedDataTables(tables);
		if (tables.empty())
		{
			HGUI::Text("No data table is loaded", mutedColor());
		}
		for (const HDataTableAssetEntry& entry : tables)
		{
			if (HGUI::Selectable(entry.TokenPath, findTab(entry.TokenPath) != nullptr))
			{
				OpenTable(entry.TokenPath);
			}
		}
		HGUI::EndCombo();
	}
	HGUI::SameLine();
	if (HGUI::Button("New..."))
	{
		_bNewTableRequest = true;
	}

	HGUI::SameLine();
	HGUI::BeginDisabled(activeTab == nullptr);
	if (HGUI::Button("Save") && activeTab != nullptr)
	{
		saveTab(*activeTab, false);
	}
	HGUI::SameLine();
	if (HGUI::Button("Reload") && activeTab != nullptr)
	{
		PString error;
		if (activeTab->Document->Reload(&error))
		{
			activeTab->bExternalChange = false;
			setStatus(PString::Format("Reloaded %s", activeTab->Document->GetTokenPath()), false);
		}
		else
		{
			setStatus(error, true);
		}
	}
	HGUI::SameLine();
	HGUI::BeginDisabled(activeTab == nullptr || activeTab->Document->CanUndo() == false);
	if (HGUI::Button("Undo") && activeTab != nullptr)
	{
		activeTab->Undo();
	}
	HGUI::EndDisabled();
	if (activeTab != nullptr && activeTab->Document->CanUndo())
	{
		HGUI::ItemTooltip(PString::Format("Undo: %s (Ctrl+Z)", activeTab->Document->GetUndoLabel()));
	}
	HGUI::SameLine();
	HGUI::BeginDisabled(activeTab == nullptr || activeTab->Document->CanRedo() == false);
	if (HGUI::Button("Redo") && activeTab != nullptr)
	{
		activeTab->Redo();
	}
	HGUI::EndDisabled();
	HGUI::SameLine();
	if (HGUI::Button("+ Row") && activeTab != nullptr)
	{
		activeTab->InsertRow(-1);
	}
	HGUI::SameLine();
	if (HGUI::Button("+ Column") && activeTab != nullptr)
	{
		activeTab->bNewColumnRequest = true;
		activeTab->NewColumnAt       = activeTab->GetContent().GetColumnCount();
	}
	if (activeTab != nullptr)
	{
		// 같은 줄은 필터가 있을 때만 잇는다(탭이 없으면 상태 문구가 다음 줄에 와야 한다)
		HGUI::SameLine();
		if (activeTab->bFocusFilterRequest)
		{
			HGUI::SetKeyboardFocusHere();
			activeTab->bFocusFilterRequest = false;
		}
		HGUI::InputTextWithHint("DataTableFilter", "Filter (Ctrl+F)", activeTab->Filter, 220.0f);
		if (activeTab->ViewSortColumn >= 0)
		{
			HGUI::SameLine();
			if (HGUI::Button("Apply sort"))
			{
				activeTab->ApplySortToRowOrder();
			}
			HGUI::ItemTooltip("Reorder the rows in the file the way the view is sorted");
			HGUI::SameLine();
			if (HGUI::Button("Clear sort"))
			{
				activeTab->SortGridColumn = -1;
			}
		}
	}
	HGUI::EndDisabled();

	if (_statusText.Empty() == false)
	{
		HGUI::Text(_statusText, _bStatusError ? errorColor() : mutedColor());
	}
}

void JGDataTableEditor::drawTab(HDataTableEditorTab& tab)
{
	_visibleTab = &tab;
	HDataTableEditorResolver resolver(*this);
	tab.RefreshView(&resolver);

	if (tab.bExternalChange)
	{
		HGUI::Text("The file changed on disk while you have unsaved changes.", warningColor());
		HGUI::SameLine();
		if (HGUI::Button("Reload (discard mine)"))
		{
			PString error;
			if (tab.Document->Reload(&error))
			{
				tab.bExternalChange = false;
				setStatus("Reloaded from disk", false);
			}
			else
			{
				setStatus(error, true);
			}
		}
		HGUI::SameLine();
		if (HGUI::Button("Keep mine"))
		{
			tab.Document->AcknowledgeExternalChange();
			tab.bExternalChange = false;
			setStatus("Kept your changes. Saving overwrites the file", false);
		}
	}

	const HVector2 available = HGUI::GetContentRegionAvail();
	const float32 statusHeight = HGUI::GetFrameHeightWithSpacing();
	const float32 issuesHeight = IssueListHeight;
	const bool    bShowPanel   = tab.SelectedGridColumn >= 0;
	const float32 gridWidth    = bShowPanel ? available.x - ColumnPanelWidth - 8.0f : available.x;
	float32       gridHeight   = available.y - issuesHeight - statusHeight - HGUI::GetFrameHeightWithSpacing();
	if (gridHeight < 120.0f)
	{
		gridHeight = 120.0f;
	}

	HList<HGUIGridEvent> events;
	tab.Grid.Draw(tab.Document->GetTokenPath(), tab, HVector2(gridWidth, gridHeight), events);
	tab.HandleEvents(events);
	tab.DrawMenus();

	if (bShowPanel)
	{
		HGUI::SameLine();
		HGUI::BeginChild("##DataTableColumnPanel", HVector2(ColumnPanelWidth, gridHeight));
		tab.DrawColumnPanel();
		if (HGUI::Button("Close panel", HVector2(-1.0f, 0.0f)))
		{
			tab.SelectedGridColumn = -1;
		}
		HGUI::EndChild();
	}

	HGUI::Text(PString::Format("Issues: %d error(s), %d warning(s)%s", tab.ErrorCount, tab.WarningCount,
		PString(tab.ErrorCount > 0 ? " - errors block saving" : "")), tab.ErrorCount > 0 ? errorColor() : mutedColor());
	tab.DrawIssues(issuesHeight);
	tab.DrawStatus();
}

void JGDataTableEditor::drawModals()
{
	drawNewTableModal();
	drawCloseModal();
	drawSaveConfirmModal();
}

void JGDataTableEditor::drawNewTableModal()
{
	if (_bNewTableRequest)
	{
		_bNewTableRequest = false;
		_newTableContent  = HFileHelper::GameContentDirectory().Empty() ? PString(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN) : PString(JG_ASSET_GAME_PATH_RECOGNITION_TOEKN);
		_newTableError.Reset();
		HGUI::OpenPopup("New Data Table##DataTableNew");
	}

	if (HGUI::BeginPopupModal("New Data Table##DataTableNew") == false)
	{
		return;
	}

	HGUI::AlignTextToFramePadding();
	HGUI::Text("Content");
	HGUI::SameLineAt(110.0f);
	if (HGUI::BeginCombo("NewTableContent", _newTableContent, 260.0f))
	{
		if (HFileHelper::GameContentDirectory().Empty() == false && HGUI::Selectable(JG_ASSET_GAME_PATH_RECOGNITION_TOEKN, DataTableSameText(_newTableContent, JG_ASSET_GAME_PATH_RECOGNITION_TOEKN)))
		{
			_newTableContent = JG_ASSET_GAME_PATH_RECOGNITION_TOEKN;
		}
		if (HGUI::Selectable(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN, DataTableSameText(_newTableContent, JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN)))
		{
			_newTableContent = JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN;
		}
		HGUI::EndCombo();
	}

	HGUI::AlignTextToFramePadding();
	HGUI::Text("Path");
	HGUI::SameLineAt(110.0f);
	HGUI::InputTextWithHint("NewTablePath", "Data/MyTable", _newTablePath, 260.0f);

	HGUI::AlignTextToFramePadding();
	HGUI::Text("Key column");
	HGUI::SameLineAt(110.0f);
	HGUI::InputTextWithHint("NewTableKeyColumn", "Id", _newTableKeyColumn, 260.0f);

	HGUI::Text("ASCII letters, digits, _ - / only (file paths are narrow-character).", mutedColor());
	if (_newTableError.Empty() == false)
	{
		HGUI::Text(_newTableError, errorColor());
	}

	if (HGUI::Button("Create", HVector2(110.0f, 0.0f)))
	{
		PString tokenPath;
		PString error;
		PString reason;
		const PString keyColumn = trimmedCopy(_newTableKeyColumn);
		if (MakeDataTableTokenPath(_newTableContent, _newTablePath, &tokenPath, &error) == false)
		{
			_newTableError = error;
		}
		else if (IsValidDataTableColumnName(keyColumn, &reason) == false)
		{
			_newTableError = PString::Format("Key column name is invalid (%s)", reason);
		}
		else
		{
			PSharedPtr<PDataTableDocument> document = PDataTableDocument::CreateNew(tokenPath, keyColumn, &error);
			if (document == nullptr)
			{
				_newTableError = error;
			}
			else
			{
				// 에셋 DB 에도 올린다(다른 창 · 게임이 찾을 수 있게)
				if (GAssetDatabase::IsValid())
				{
					GAssetDatabase::GetInstance().LoadAssetAsync(HAssetPath(tokenPath));
				}

				HSTLUniquePtr<HDataTableEditorTab> tab = HSTLUniquePtr<HDataTableEditorTab>(new HDataTableEditorTab());
				tab->Editor   = this;
				tab->Document = document;
				_selectRequest = tab.get();
				_tabs.push_back(std::move(tab));
				setStatus(PString::Format("Created %s. Add columns with '+ Column' or the header menu", tokenPath), false);
				HGUI::CloseCurrentPopup();
			}
		}
	}
	HGUI::SameLine();
	if (HGUI::Button("Cancel", HVector2(110.0f, 0.0f)))
	{
		HGUI::CloseCurrentPopup();
	}
	HGUI::EndPopup();
}

void JGDataTableEditor::drawCloseModal()
{
	if (HGUI::BeginPopupModal("Close Table##DataTableClose") == false)
	{
		return;
	}

	HDataTableEditorTab* tab = _closeRequest;
	bool bTabAlive = false;
	for (const HSTLUniquePtr<HDataTableEditorTab>& entry : _tabs)
	{
		bTabAlive |= (entry.get() == tab);
	}
	if (bTabAlive == false)
	{
		HGUI::CloseCurrentPopup();
		HGUI::EndPopup();
		return;
	}

	HGUI::Text(PString::Format("%s has unsaved changes.", tab->Document->GetTokenPath()));
	if (HGUI::Button("Save and close", HVector2(130.0f, 0.0f)))
	{
		if (saveTab(*tab, true))
		{
			removeTab(tab);
		}
		HGUI::CloseCurrentPopup();
	}
	HGUI::SameLine();
	if (HGUI::Button("Discard", HVector2(100.0f, 0.0f)))
	{
		removeTab(tab);
		HGUI::CloseCurrentPopup();
	}
	HGUI::SameLine();
	if (HGUI::Button("Cancel", HVector2(100.0f, 0.0f)))
	{
		_closeRequest = nullptr;
		HGUI::CloseCurrentPopup();
	}
	HGUI::EndPopup();
}

void JGDataTableEditor::drawSaveConfirmModal()
{
	if (_bSaveConfirmRequest)
	{
		_bSaveConfirmRequest = false;
		HGUI::OpenPopup("Save Table##DataTableDrop");
	}

	if (HGUI::BeginPopupModal("Save Table##DataTableDrop") == false)
	{
		return;
	}

	HDataTableEditorTab* tab = _saveConfirm;
	bool bTabAlive = false;
	for (const HSTLUniquePtr<HDataTableEditorTab>& entry : _tabs)
	{
		bTabAlive |= (entry.get() == tab);
	}
	if (bTabAlive == false)
	{
		HGUI::CloseCurrentPopup();
		HGUI::EndPopup();
		return;
	}

	HGUI::Text("The file has keys that are not columns. Saving drops them:");
	for (const HDataTableIssue& issue : tab->Document->GetLoadIssues())
	{
		if (issue.Message.Contains("dropped on save"))
		{
			HGUI::Text(PString::Format("  %s", issue.Message), warningColor());
		}
	}
	HGUI::Text("Add them as columns first (+ Column) to keep the values.", mutedColor());
	if (HGUI::Button("Save anyway", HVector2(120.0f, 0.0f)))
	{
		tab->bDropConfirmed = true;
		saveTab(*tab, true);
		_saveConfirm = nullptr;
		HGUI::CloseCurrentPopup();
	}
	HGUI::SameLine();
	if (HGUI::Button("Cancel", HVector2(100.0f, 0.0f)))
	{
		_saveConfirm = nullptr;
		HGUI::CloseCurrentPopup();
	}
	HGUI::EndPopup();
}
