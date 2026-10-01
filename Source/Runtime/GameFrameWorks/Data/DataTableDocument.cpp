#include "PCH/PCH.h"
#include "Data/DataTableDocument.h"
#include "AssetDatabase.h"
#include "AssetPath.h"

namespace
{
	bool fail(PString* outError, const PString& inMessage)
	{
		if (outError != nullptr)
		{
			*outError = inMessage;
		}
		return false;
	}

	bool isKindOf(const HDataTableValue& inValue, const HDataTableColumn& inColumn)
	{
		return inValue.GetKind() == DataTableValueKindOf(inColumn.Type);
	}

	// 행 값이 열 수보다 짧으면(있어서는 안 되지만) 기본값으로 채운다
	void padRow(HDataTableRow& row, const HDataTableSchema& inSchema)
	{
		while (row.Values.size() < inSchema.Columns.size())
		{
			row.Values.push_back(inSchema.Columns[row.Values.size()].Default);
		}
	}

	template<class T>
	void moveElement(HList<T>& list, int32 inFrom, int32 inTo)
	{
		if (inFrom == inTo)
		{
			return;
		}
		T element = list[inFrom];
		list.erase(list.begin() + inFrom);
		list.insert(list.begin() + inTo, element);
	}
}

HDataTableEdit HDataTableEdit::MakeSetCells(const HList<HDataTableCellAssignment>& inCells)
{
	HDataTableEdit edit;
	edit.Type  = EDataTableEditType::SetCells;
	edit.Cells = inCells;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeSetKeys(const HList<HPair<int32, PString>>& inKeys)
{
	HDataTableEdit edit;
	edit.Type = EDataTableEditType::SetKeys;
	edit.Keys = inKeys;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeInsertRows(const HList<HPair<int32, HDataTableRow>>& inRows)
{
	HDataTableEdit edit;
	edit.Type = EDataTableEditType::InsertRows;
	edit.Rows = inRows;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeRemoveRows(const HList<int32>& inSortedRows)
{
	HDataTableEdit edit;
	edit.Type    = EDataTableEditType::RemoveRows;
	edit.Indices = inSortedRows;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeReorderRows(const HList<int32>& inNewOrder)
{
	HDataTableEdit edit;
	edit.Type    = EDataTableEditType::ReorderRows;
	edit.Indices = inNewOrder;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeInsertColumn(int32 inAt, const HDataTableColumn& inColumn, const HList<HDataTableValue>& inValues)
{
	HDataTableEdit edit;
	edit.Type         = EDataTableEditType::InsertColumn;
	edit.Column       = inAt;
	edit.ColumnDef    = inColumn;
	edit.ColumnValues = inValues;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeRemoveColumn(int32 inColumn)
{
	HDataTableEdit edit;
	edit.Type   = EDataTableEditType::RemoveColumn;
	edit.Column = inColumn;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeReplaceColumn(int32 inColumn, const HDataTableColumn& inColumnDef, const HList<HDataTableValue>& inValues)
{
	HDataTableEdit edit;
	edit.Type         = EDataTableEditType::ReplaceColumn;
	edit.Column       = inColumn;
	edit.ColumnDef    = inColumnDef;
	edit.ColumnValues = inValues;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeMoveColumn(int32 inFrom, int32 inTo)
{
	HDataTableEdit edit;
	edit.Type     = EDataTableEditType::MoveColumn;
	edit.Column   = inFrom;
	edit.ToColumn = inTo;
	return edit;
}

HDataTableEdit HDataTableEdit::MakeSetKeyColumn(const PString& inName)
{
	HDataTableEdit edit;
	edit.Type = EDataTableEditType::SetKeyColumn;
	edit.Name = inName;
	return edit;
}

bool ApplyDataTableEdit(HDataTableContent& content, const HDataTableEdit& inEdit, HDataTableEdit* outInverse, PString* outError)
{
	const int32 rowCount    = content.GetRowCount();
	const int32 columnCount = content.GetColumnCount();

	// 모든 경우에 먼저 검사하고 나서 바꾼다 — 실패하면 내용은 그대로다
	HDataTableEdit inverse;
	switch (inEdit.Type)
	{
	case EDataTableEditType::SetCells:
	{
		for (const HDataTableCellAssignment& cell : inEdit.Cells)
		{
			if (cell.Row < 0 || cell.Row >= rowCount || cell.Column < 0 || cell.Column >= columnCount)
			{
				return fail(outError, PString::Format("cell (%d, %d) is out of range", cell.Row, cell.Column));
			}
			if (isKindOf(cell.Value, content.Schema.Columns[cell.Column]) == false)
			{
				return fail(outError, PString::Format("value does not match the %s column '%s'", PString(DataTableColumnTypeToString(content.Schema.Columns[cell.Column].Type)), content.Schema.Columns[cell.Column].Name));
			}
		}

		inverse.Type = EDataTableEditType::SetCells;
		inverse.Cells.reserve(inEdit.Cells.size());
		for (const HDataTableCellAssignment& cell : inEdit.Cells)
		{
			HDataTableRow& row = content.Rows[cell.Row];
			padRow(row, content.Schema);

			HDataTableCellAssignment before;
			before.Row    = cell.Row;
			before.Column = cell.Column;
			before.Value  = row.Values[cell.Column];
			inverse.Cells.push_back(before);

			row.Values[cell.Column] = cell.Value;
		}
		// 같은 칸을 두 번 바꾼 편집도 되돌리면 처음 값이 되게 역순으로
		std::reverse(inverse.Cells.begin(), inverse.Cells.end());
		break;
	}
	case EDataTableEditType::SetKeys:
	{
		for (const HPair<int32, PString>& key : inEdit.Keys)
		{
			if (key.first < 0 || key.first >= rowCount)
			{
				return fail(outError, PString::Format("row %d is out of range", key.first));
			}
		}

		inverse.Type = EDataTableEditType::SetKeys;
		for (const HPair<int32, PString>& key : inEdit.Keys)
		{
			inverse.Keys.push_back(HPair<int32, PString>(key.first, content.Rows[key.first].Key));
			content.Rows[key.first].Key = key.second;
		}
		std::reverse(inverse.Keys.begin(), inverse.Keys.end());
		break;
	}
	case EDataTableEditType::InsertRows:
	{
		int32 count    = rowCount;
		int32 previous = -1;
		for (const HPair<int32, HDataTableRow>& entry : inEdit.Rows)
		{
			if (entry.first < 0 || entry.first > count || entry.first <= previous)
			{
				return fail(outError, PString::Format("insert position %d is invalid", entry.first));
			}
			if ((int32)entry.second.Values.size() != columnCount)
			{
				return fail(outError, PString::Format("inserted row has %d values for %d columns", (int32)entry.second.Values.size(), columnCount));
			}
			for (int32 c = 0; c < columnCount; ++c)
			{
				if (isKindOf(entry.second.Values[c], content.Schema.Columns[c]) == false)
				{
					return fail(outError, PString::Format("inserted row value does not match column '%s'", content.Schema.Columns[c].Name));
				}
			}
			previous = entry.first;
			++count;
		}

		inverse.Type = EDataTableEditType::RemoveRows;
		for (const HPair<int32, HDataTableRow>& entry : inEdit.Rows)
		{
			content.Rows.insert(content.Rows.begin() + entry.first, entry.second);
			inverse.Indices.push_back(entry.first);
		}
		break;
	}
	case EDataTableEditType::RemoveRows:
	{
		int32 previous = -1;
		for (int32 index : inEdit.Indices)
		{
			if (index < 0 || index >= rowCount || index <= previous)
			{
				return fail(outError, PString::Format("remove row %d is invalid", index));
			}
			previous = index;
		}

		inverse.Type = EDataTableEditType::InsertRows;
		for (int32 index : inEdit.Indices)
		{
			inverse.Rows.push_back(HPair<int32, HDataTableRow>(index, content.Rows[index]));
		}
		for (int32 i = (int32)inEdit.Indices.size() - 1; i >= 0; --i)
		{
			content.Rows.erase(content.Rows.begin() + inEdit.Indices[i]);
		}
		break;
	}
	case EDataTableEditType::ReorderRows:
	{
		if ((int32)inEdit.Indices.size() != rowCount)
		{
			return fail(outError, "row order does not cover every row");
		}
		HList<uint8> seen(rowCount, 0);
		for (int32 index : inEdit.Indices)
		{
			if (index < 0 || index >= rowCount || seen[index] != 0)
			{
				return fail(outError, "row order is not a permutation");
			}
			seen[index] = 1;
		}

		HList<HDataTableRow> reordered;
		reordered.reserve(rowCount);
		inverse.Type = EDataTableEditType::ReorderRows;
		inverse.Indices.resize(rowCount);
		for (int32 i = 0; i < rowCount; ++i)
		{
			reordered.push_back(content.Rows[inEdit.Indices[i]]);
			inverse.Indices[inEdit.Indices[i]] = i;
		}
		content.Rows = reordered;
		break;
	}
	case EDataTableEditType::InsertColumn:
	{
		if (inEdit.Column < 0 || inEdit.Column > columnCount)
		{
			return fail(outError, PString::Format("insert column position %d is invalid", inEdit.Column));
		}
		if (isKindOf(inEdit.ColumnDef.Default, inEdit.ColumnDef) == false)
		{
			return fail(outError, "column default does not match its type");
		}
		if (inEdit.ColumnValues.empty() == false && (int32)inEdit.ColumnValues.size() != rowCount)
		{
			return fail(outError, PString::Format("inserted column has %d values for %d rows", (int32)inEdit.ColumnValues.size(), rowCount));
		}
		for (const HDataTableValue& value : inEdit.ColumnValues)
		{
			if (isKindOf(value, inEdit.ColumnDef) == false)
			{
				return fail(outError, "inserted column value does not match its type");
			}
		}

		// 열을 넣기 전에 행 폭을 맞춘다(스키마 번호가 아직 그대로일 때)
		for (HDataTableRow& row : content.Rows)
		{
			padRow(row, content.Schema);
		}
		content.Schema.Columns.insert(content.Schema.Columns.begin() + inEdit.Column, inEdit.ColumnDef);
		for (int32 r = 0; r < rowCount; ++r)
		{
			HDataTableRow& row = content.Rows[r];
			const HDataTableValue& value = inEdit.ColumnValues.empty() ? inEdit.ColumnDef.Default : inEdit.ColumnValues[r];
			row.Values.insert(row.Values.begin() + inEdit.Column, value);
		}

		inverse.Type   = EDataTableEditType::RemoveColumn;
		inverse.Column = inEdit.Column;
		break;
	}
	case EDataTableEditType::RemoveColumn:
	{
		if (inEdit.Column < 0 || inEdit.Column >= columnCount)
		{
			return fail(outError, PString::Format("remove column %d is invalid", inEdit.Column));
		}

		inverse.Type      = EDataTableEditType::InsertColumn;
		inverse.Column    = inEdit.Column;
		inverse.ColumnDef = content.Schema.Columns[inEdit.Column];
		inverse.ColumnValues.reserve(rowCount);
		for (HDataTableRow& row : content.Rows)
		{
			padRow(row, content.Schema);
			inverse.ColumnValues.push_back(row.Values[inEdit.Column]);
			row.Values.erase(row.Values.begin() + inEdit.Column);
		}
		content.Schema.Columns.erase(content.Schema.Columns.begin() + inEdit.Column);
		break;
	}
	case EDataTableEditType::ReplaceColumn:
	{
		if (inEdit.Column < 0 || inEdit.Column >= columnCount)
		{
			return fail(outError, PString::Format("replace column %d is invalid", inEdit.Column));
		}
		if ((int32)inEdit.ColumnValues.size() != rowCount)
		{
			return fail(outError, PString::Format("replaced column has %d values for %d rows", (int32)inEdit.ColumnValues.size(), rowCount));
		}
		if (isKindOf(inEdit.ColumnDef.Default, inEdit.ColumnDef) == false)
		{
			return fail(outError, "column default does not match its type");
		}
		for (const HDataTableValue& value : inEdit.ColumnValues)
		{
			if (isKindOf(value, inEdit.ColumnDef) == false)
			{
				return fail(outError, "replaced column value does not match its type");
			}
		}

		inverse.Type      = EDataTableEditType::ReplaceColumn;
		inverse.Column    = inEdit.Column;
		inverse.ColumnDef = content.Schema.Columns[inEdit.Column];
		inverse.ColumnValues.reserve(rowCount);
		for (int32 r = 0; r < rowCount; ++r)
		{
			HDataTableRow& row = content.Rows[r];
			padRow(row, content.Schema);
			inverse.ColumnValues.push_back(row.Values[inEdit.Column]);
			row.Values[inEdit.Column] = inEdit.ColumnValues[r];
		}
		content.Schema.Columns[inEdit.Column] = inEdit.ColumnDef;
		break;
	}
	case EDataTableEditType::MoveColumn:
	{
		if (inEdit.Column < 0 || inEdit.Column >= columnCount || inEdit.ToColumn < 0 || inEdit.ToColumn >= columnCount)
		{
			return fail(outError, PString::Format("move column %d -> %d is invalid", inEdit.Column, inEdit.ToColumn));
		}

		moveElement(content.Schema.Columns, inEdit.Column, inEdit.ToColumn);
		for (HDataTableRow& row : content.Rows)
		{
			padRow(row, content.Schema);
			moveElement(row.Values, inEdit.Column, inEdit.ToColumn);
		}

		inverse.Type     = EDataTableEditType::MoveColumn;
		inverse.Column   = inEdit.ToColumn;
		inverse.ToColumn = inEdit.Column;
		break;
	}
	case EDataTableEditType::SetKeyColumn:
	{
		if (inEdit.Name.Empty())
		{
			return fail(outError, "key column name is empty");
		}

		inverse.Type = EDataTableEditType::SetKeyColumn;
		inverse.Name = content.Schema.KeyColumn;
		content.Schema.KeyColumn = inEdit.Name;
		break;
	}
	default:
		return fail(outError, "unknown edit");
	}

	if (outInverse != nullptr)
	{
		*outInverse = inverse;
	}
	return true;
}

HList<HDataTableValue> ConvertDataTableColumnValues(const HDataTableContent& inContent, int32 inColumn, const HDataTableColumn& inNewColumn, HList<int32>* outFailedRows)
{
	HList<HDataTableValue> values;
	if (inColumn < 0 || inColumn >= inContent.GetColumnCount())
	{
		return values;
	}

	const HDataTableColumn& oldColumn = inContent.Schema.Columns[inColumn];
	const int32 rowCount = inContent.GetRowCount();
	values.reserve(rowCount);
	for (int32 r = 0; r < rowCount; ++r)
	{
		const HDataTableRow& row = inContent.Rows[r];
		const HDataTableValue& oldValue = (inColumn < (int32)row.Values.size()) ? row.Values[inColumn] : oldColumn.Default;

		HDataTableValue newValue;
		if (oldColumn.Type == inNewColumn.Type)
		{
			newValue = oldValue;
		}
		else if (inNewColumn.ConvertFrom(oldValue, oldColumn.Type, &newValue) == false)
		{
			newValue = inNewColumn.Default;
			if (outFailedRows != nullptr)
			{
				outFailedRows->push_back(r);
			}
		}
		values.push_back(newValue);
	}
	return values;
}

PSharedPtr<PDataTableDocument> PDataTableDocument::Open(const PString& inTokenPath, PString* outError)
{
	const HAssetPath assetPath(inTokenPath);
	if (assetPath.IsValid() == false)
	{
		fail(outError, PString::Format("'%s' is not an asset path (/JGGame/... or /JGEngine/...)", inTokenPath));
		return nullptr;
	}

	const PString rawPath = assetPath.GetRawAssetPath().ToString();
	PString error;
	PSharedPtr<JGDataTable> working = JGDataTable::LoadFromFile(rawPath, &error);
	if (working == nullptr)
	{
		fail(outError, error);
		return nullptr;
	}

	PSharedPtr<PDataTableDocument> document = Allocate<PDataTableDocument>();
	document->_working    = working;
	document->_tokenPath  = assetPath.GetAssetPath().ToString();
	document->_rawPath    = rawPath;
	document->_fileStamp  = readFileStamp(rawPath);
	document->_loadIssues = working->GetLoadIssues();
	return document;
}

PSharedPtr<PDataTableDocument> PDataTableDocument::CreateNew(const PString& inTokenPath, const PString& inKeyColumn, PString* outError)
{
	const HAssetPath assetPath(inTokenPath);
	if (assetPath.IsValid() == false)
	{
		fail(outError, PString::Format("'%s' is not an asset path (/JGGame/... or /JGEngine/...)", inTokenPath));
		return nullptr;
	}

	const PString rawPath = assetPath.GetRawAssetPath().ToString();
	if (HFileHelper::Exists(rawPath))
	{
		fail(outError, PString::Format("%s already exists", assetPath.GetAssetPath().ToString()));
		return nullptr;
	}

	HDataTableContent content;
	content.Schema.KeyColumn = inKeyColumn.Empty() ? PString("Id") : inKeyColumn;

	PSharedPtr<JGDataTable> table = Allocate<JGDataTable>();
	table->InitializeNew(assetPath.GetAssetPath().ToString(), content);
	if (table->SaveToFile(rawPath, outError) == false)
	{
		return nullptr;
	}

	// 디스크와 똑같은 작업 사본으로 연다
	return Open(assetPath.GetAssetPath().ToString(), outError);
}

PSharedPtr<PDataTableDocument> PDataTableDocument::OpenRawFile(const PString& inRawPath, PString* outError)
{
	PString error;
	PSharedPtr<JGDataTable> working = JGDataTable::LoadFromFile(inRawPath, &error);
	if (working == nullptr)
	{
		fail(outError, error);
		return nullptr;
	}

	PSharedPtr<PDataTableDocument> document = Allocate<PDataTableDocument>();
	document->_working    = working;
	document->_rawPath    = inRawPath;
	document->_fileStamp  = readFileStamp(inRawPath);
	document->_loadIssues = working->GetLoadIssues();
	return document;
}

PSharedPtr<PDataTableDocument> PDataTableDocument::FromContent(const HDataTableContent& inContent)
{
	PSharedPtr<JGDataTable> working = Allocate<JGDataTable>();
	working->_content = inContent;
	working->rebuildIndex();

	PSharedPtr<PDataTableDocument> document = Allocate<PDataTableDocument>();
	document->_working = working;
	return document;
}

const HDataTableContent& PDataTableDocument::GetContent() const
{
	return _working->_content;
}

PString PDataTableDocument::GetDisplayName() const
{
	const PString& path = _tokenPath.Empty() ? _rawPath : _tokenPath;
	if (path.Empty())
	{
		return PString("(untitled)");
	}

	PString name;
	HFileHelper::FileNameOnly(path, &name);
	return name;
}

PString PDataTableDocument::GetUndoLabel() const
{
	return _undoSteps.empty() ? PString() : _undoSteps.back().Label;
}

PString PDataTableDocument::GetRedoLabel() const
{
	return _redoSteps.empty() ? PString() : _redoSteps.back().Label;
}

bool PDataTableDocument::Apply(const HList<HDataTableEdit>& inEdits, const PString& inLabel, PString* outError)
{
	if (inEdits.empty())
	{
		return true;
	}

	HList<HDataTableEdit> inverses;
	if (applyEdits(inEdits, &inverses, outError) == false)
	{
		return false;
	}

	HUndoStep step;
	step.Label  = inLabel;
	step.Serial = _nextSerial++;
	step.Redo   = inEdits;
	step.Undo.assign(inverses.rbegin(), inverses.rend());

	_undoSteps.push_back(step);
	while ((int32)_undoSteps.size() > MaxUndoSteps)
	{
		_undoSteps.pop_front();
	}
	_redoSteps.clear();
	_currentSerial = step.Serial;

	contentChanged();
	return true;
}

bool PDataTableDocument::Undo()
{
	if (CanUndo() == false)
	{
		return false;
	}

	HUndoStep step = _undoSteps.back();
	PString error;
	HList<HDataTableEdit> ignored;
	if (applyEdits(step.Undo, &ignored, &error) == false)
	{
		JG_LOG(DataTable, ELogLevel::Error, "%s : undo '%s' failed: %s", GetDisplayName(), step.Label, error);
		return false;
	}

	_undoSteps.pop_back();
	_redoSteps.push_back(step);
	_currentSerial = _undoSteps.empty() ? 0 : _undoSteps.back().Serial;

	contentChanged();
	return true;
}

bool PDataTableDocument::Redo()
{
	if (CanRedo() == false)
	{
		return false;
	}

	HUndoStep step = _redoSteps.back();
	PString error;
	HList<HDataTableEdit> ignored;
	if (applyEdits(step.Redo, &ignored, &error) == false)
	{
		JG_LOG(DataTable, ELogLevel::Error, "%s : redo '%s' failed: %s", GetDisplayName(), step.Label, error);
		return false;
	}

	_redoSteps.pop_back();
	_undoSteps.push_back(step);
	while ((int32)_undoSteps.size() > MaxUndoSteps)
	{
		_undoSteps.pop_front();
	}
	_currentSerial = step.Serial;

	contentChanged();
	return true;
}

void PDataTableDocument::Validate(const IDataTableReferenceResolver* inResolver, HList<HDataTableIssue>& outIssues) const
{
	ValidateDataTableContent(GetContent(), inResolver, outIssues);
}

bool PDataTableDocument::Save(const IDataTableReferenceResolver* inResolver, PString* outError)
{
	if (_rawPath.Empty())
	{
		return fail(outError, "the document has no file");
	}

	HList<HDataTableIssue> issues;
	Validate(inResolver, issues);
	for (const HDataTableIssue& issue : issues)
	{
		if (issue.Severity == EDataTableIssueSeverity::Error)
		{
			const PString where = (issue.Row >= 0) ? PString::Format(" (row %d)", issue.Row + 1) : PString();
			return fail(outError, PString::Format("not saved: %s%s", issue.Message, where));
		}
	}

	if (_working->SaveToFile(_rawPath, outError) == false)
	{
		return false;
	}

	_fileStamp   = readFileStamp(_rawPath);
	_savedSerial = _currentSerial;
	_loadIssues.clear();   // 모르는 키는 이제 파일에 없다

	PSharedPtr<JGDataTable> live = GetLiveTable();
	if (live != nullptr && live.GetRawPointer() != _working.GetRawPointer())
	{
		live->ApplyContent(_working->GetContent());
	}

	JG_LOG(DataTable, ELogLevel::Info, "%s : saved (%d rows, %d columns)", _tokenPath.Empty() ? _rawPath : _tokenPath, GetContent().GetRowCount(), GetContent().GetColumnCount());
	return true;
}

bool PDataTableDocument::Reload(PString* outError)
{
	if (_rawPath.Empty())
	{
		return fail(outError, "the document has no file");
	}

	PSharedPtr<JGDataTable> table = JGDataTable::LoadFromFile(_rawPath, outError);
	if (table == nullptr)
	{
		return false;
	}

	_working    = table;
	_loadIssues = table->GetLoadIssues();
	_undoSteps.clear();
	_redoSteps.clear();
	_currentSerial = 0;
	_savedSerial   = 0;
	_fileStamp     = readFileStamp(_rawPath);
	++_revision;

	PSharedPtr<JGDataTable> live = GetLiveTable();
	if (live != nullptr)
	{
		live->ApplyContent(_working->GetContent());
	}
	return true;
}

bool PDataTableDocument::HasExternalChange() const
{
	if (_rawPath.Empty())
	{
		return false;
	}
	return readFileStamp(_rawPath) != _fileStamp;
}

void PDataTableDocument::AcknowledgeExternalChange()
{
	if (_rawPath.Empty() == false)
	{
		_fileStamp = readFileStamp(_rawPath);
	}
}

PSharedPtr<JGDataTable> PDataTableDocument::GetLiveTable() const
{
	if (_tokenPath.Empty() || GAssetDatabase::IsValid() == false)
	{
		return nullptr;
	}

	PSharedPtr<JGAsset> asset = GAssetDatabase::GetInstance().GetLoadedAsset(HAssetPath(_tokenPath)).Pin();
	if (asset == nullptr)
	{
		return nullptr;
	}
	return Cast<JGDataTable>(asset);
}

int64 PDataTableDocument::readFileStamp(const PString& inRawPath)
{
	std::error_code errCode;
	const fs::file_time_type writeTime = fs::last_write_time(fs::path(inRawPath.GetRawString()), errCode);
	if (errCode)
	{
		return 0;
	}
	return (int64)writeTime.time_since_epoch().count();
}

void PDataTableDocument::contentChanged()
{
	_working->rebuildIndex();
	++_working->_revision;
	++_revision;
}

bool PDataTableDocument::applyEdits(const HList<HDataTableEdit>& inEdits, HList<HDataTableEdit>* outInverses, PString* outError)
{
	HList<HDataTableEdit> inverses;
	inverses.reserve(inEdits.size());
	for (const HDataTableEdit& edit : inEdits)
	{
		HDataTableEdit inverse;
		if (ApplyDataTableEdit(_working->_content, edit, &inverse, outError) == false)
		{
			// 앞에서 건 것을 되돌린다
			for (int32 i = (int32)inverses.size() - 1; i >= 0; --i)
			{
				ApplyDataTableEdit(_working->_content, inverses[i], nullptr, nullptr);
			}
			return false;
		}
		inverses.push_back(inverse);
	}

	if (outInverses != nullptr)
	{
		*outInverses = inverses;
	}
	return true;
}
