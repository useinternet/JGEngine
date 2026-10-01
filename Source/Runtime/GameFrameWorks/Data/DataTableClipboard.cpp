#include "PCH/PCH.h"
#include "Data/DataTableClipboard.h"

namespace
{
	constexpr int32 MaxPasteErrors = 20;

	void addError(HList<PString>* outErrors, const PString& inMessage)
	{
		if (outErrors != nullptr && (int32)outErrors->size() < MaxPasteErrors)
		{
			outErrors->push_back(inMessage);
		}
	}

	PString gridColumnName(const HDataTableContent& inContent, int32 inGridColumn)
	{
		if (inGridColumn == 0)
		{
			return inContent.Schema.KeyColumn;
		}
		return inContent.Schema.Columns[inGridColumn - 1].Name;
	}
}

PString HDataTableClipboard::EncodeTsv(const HDataTableTextGrid& inCells)
{
	HRawString text;
	for (const HList<PString>& row : inCells)
	{
		const int32 cellCount = (int32)row.size();
		for (int32 i = 0; i < cellCount; ++i)
		{
			if (i > 0)
			{
				text.push_back('\t');
			}

			const HRawString& cell = row[i].GetRawString();
			const bool bQuote = cell.find_first_of("\t\r\n\"") != HRawString::npos;
			if (bQuote == false)
			{
				text += cell;
				continue;
			}

			text.push_back('"');
			for (char c : cell)
			{
				if (c == '"')
				{
					text.push_back('"');
				}
				text.push_back(c);
			}
			text.push_back('"');
		}
		text += "\r\n";
	}
	return PString(text.c_str());
}

bool HDataTableClipboard::DecodeTsv(const PString& inText, HDataTableTextGrid* outCells)
{
	if (outCells == nullptr)
	{
		return false;
	}
	outCells->clear();

	const HRawString& text = inText.GetRawString();
	const uint64      length = text.size();
	if (length == 0)
	{
		return false;
	}

	HList<PString> row;
	HRawString     field;
	bool bInQuotes    = false;
	bool bFieldStart  = true;
	bool bPendingRow  = false;   // 마지막 줄바꿈 뒤에 읽은 것이 있나

	auto endField = [&]()
	{
		row.push_back(PString(field.c_str()));
		field.clear();
		bFieldStart = true;
	};
	auto endRow = [&]()
	{
		endField();
		outCells->push_back(row);
		row.clear();
		bPendingRow = false;
	};

	uint64 i = 0;
	while (i < length)
	{
		const char c = text[i];
		if (bInQuotes)
		{
			if (c == '"')
			{
				if (i + 1 < length && text[i + 1] == '"')
				{
					field.push_back('"');
					i += 2;
					continue;
				}
				bInQuotes = false;
				++i;
				continue;
			}
			field.push_back(c);
			++i;
			continue;
		}

		bPendingRow = true;
		if (bFieldStart && c == '"')
		{
			bInQuotes   = true;
			bFieldStart = false;
			++i;
			continue;
		}
		if (c == '\t')
		{
			endField();
			++i;
			continue;
		}
		if (c == '\r' || c == '\n')
		{
			endRow();
			i += (c == '\r' && i + 1 < length && text[i + 1] == '\n') ? 2 : 1;
			continue;
		}

		field.push_back(c);
		bFieldStart = false;
		++i;
	}

	if (bPendingRow || bInQuotes)
	{
		endRow();
	}

	if (outCells->empty())
	{
		return false;
	}

	// 사각형으로
	uint64 width = 0;
	for (const HList<PString>& cells : *outCells)
	{
		width = (cells.size() > width) ? cells.size() : width;
	}
	for (HList<PString>& cells : *outCells)
	{
		while (cells.size() < width)
		{
			cells.push_back(PString());
		}
	}
	return true;
}

HDataTableTextGrid CopyDataTableRange(const HDataTableContent& inContent, const HList<int32>& inViewRows, const HDataTableGridRange& inRange)
{
	HDataTableTextGrid cells;
	const int32 gridColumnCount = inContent.GetColumnCount() + 1;
	for (int32 v = inRange.FirstViewRow; v < inRange.FirstViewRow + inRange.RowCount; ++v)
	{
		if (v < 0 || v >= (int32)inViewRows.size())
		{
			continue;
		}
		const HDataTableRow& row = inContent.Rows[inViewRows[v]];

		HList<PString> texts;
		for (int32 g = inRange.FirstGridColumn; g < inRange.FirstGridColumn + inRange.ColumnCount; ++g)
		{
			if (g < 0 || g >= gridColumnCount)
			{
				continue;
			}
			if (g == 0)
			{
				texts.push_back(row.Key);
				continue;
			}
			const HDataTableColumn& column = inContent.Schema.Columns[g - 1];
			texts.push_back(column.FormatText(row.Values[g - 1]));
		}
		cells.push_back(texts);
	}
	return cells;
}

HList<HDataTableEdit> PlanDataTableClear(const HDataTableContent& inContent, const HList<int32>& inViewRows, const HDataTableGridRange& inRange)
{
	HList<HDataTableCellAssignment> cells;
	const int32 gridColumnCount = inContent.GetColumnCount() + 1;
	for (int32 v = inRange.FirstViewRow; v < inRange.FirstViewRow + inRange.RowCount; ++v)
	{
		if (v < 0 || v >= (int32)inViewRows.size())
		{
			continue;
		}
		const int32 dataRow = inViewRows[v];

		for (int32 g = inRange.FirstGridColumn; g < inRange.FirstGridColumn + inRange.ColumnCount; ++g)
		{
			if (g <= 0 || g >= gridColumnCount)
			{
				continue;
			}
			const int32 column = g - 1;
			const HDataTableValue& defaultValue = inContent.Schema.Columns[column].Default;
			if (inContent.Rows[dataRow].Values[column] == defaultValue)
			{
				continue;
			}

			HDataTableCellAssignment cell;
			cell.Row    = dataRow;
			cell.Column = column;
			cell.Value  = defaultValue;
			cells.push_back(cell);
		}
	}

	HList<HDataTableEdit> edits;
	if (cells.empty() == false)
	{
		edits.push_back(HDataTableEdit::MakeSetCells(cells));
	}
	return edits;
}

bool PlanDataTablePaste(const HDataTableContent& inContent, const HList<int32>& inViewRows, bool bViewIsFileOrder,
	const HDataTableGridRange& inRange, const HDataTableTextGrid& inCells, HList<HDataTableEdit>* outEdits, HList<PString>* outErrors)
{
	if (outEdits == nullptr)
	{
		return false;
	}
	outEdits->clear();

	if (inCells.empty() || inCells[0].empty())
	{
		addError(outErrors, "the clipboard is empty");
		return false;
	}

	const int32 clipRows        = (int32)inCells.size();
	const int32 clipColumns     = (int32)inCells[0].size();
	const bool  bFill           = (clipRows == 1 && clipColumns == 1) && (inRange.RowCount > 1 || inRange.ColumnCount > 1);
	const int32 targetRows      = bFill ? inRange.RowCount : clipRows;
	const int32 targetColumns   = bFill ? inRange.ColumnCount : clipColumns;
	const int32 gridColumnCount = inContent.GetColumnCount() + 1;

	if (inRange.FirstGridColumn < 0 || inRange.FirstGridColumn + targetColumns > gridColumnCount)
	{
		addError(outErrors, PString::Format("the paste needs %d column(s) but only %d remain from the selected column", targetColumns, gridColumnCount - inRange.FirstGridColumn));
		return false;
	}

	const int32 existingRows = (int32)inViewRows.size() - inRange.FirstViewRow;
	const int32 appendRows   = (targetRows > existingRows) ? targetRows - existingRows : 0;
	if (appendRows > 0)
	{
		if (inRange.FirstGridColumn != 0)
		{
			addError(outErrors, PString::Format("the paste needs %d more row(s). Paste from the key column to add rows", appendRows));
			return false;
		}
		if (bViewIsFileOrder == false)
		{
			addError(outErrors, "rows cannot be added while the view is sorted or filtered");
			return false;
		}
	}

	HList<HDataTableCellAssignment>    cellEdits;
	HList<HPair<int32, PString>>       keyEdits;
	HList<HPair<int32, HDataTableRow>> newRows;
	bool bOk = true;

	for (int32 i = 0; i < targetRows; ++i)
	{
		const int32 viewRow = inRange.FirstViewRow + i;
		const bool  bNewRow = viewRow >= (int32)inViewRows.size();
		const int32 dataRow = bNewRow ? -1 : inViewRows[viewRow];

		HDataTableRow newRow;
		if (bNewRow)
		{
			newRow = inContent.MakeDefaultRow(PString());
		}

		for (int32 j = 0; j < targetColumns; ++j)
		{
			const PString& text       = bFill ? inCells[0][0] : inCells[i][j];
			const int32    gridColumn = inRange.FirstGridColumn + j;
			const PString  where      = PString::Format("row %d, %s", viewRow + 1, gridColumnName(inContent, gridColumn));

			if (gridColumn == 0)
			{
				PString key = text;
				key.Trim();
				PString reason;
				if (IsValidDataTableKey(key, &reason) == false)
				{
					addError(outErrors, PString::Format("%s: key '%s' is invalid (%s)", where, key, reason));
					bOk = false;
					continue;
				}

				if (bNewRow)
				{
					newRow.Key = key;
				}
				else
				{
					keyEdits.push_back(HPair<int32, PString>(dataRow, key));
				}
				continue;
			}

			const int32 column = gridColumn - 1;
			HDataTableValue value;
			PString error;
			if (inContent.Schema.Columns[column].ParseText(text, &value, &error) == false)
			{
				addError(outErrors, PString::Format("%s: %s", where, error));
				bOk = false;
				continue;
			}

			if (bNewRow)
			{
				newRow.Values[column] = value;
			}
			else
			{
				HDataTableCellAssignment cell;
				cell.Row    = dataRow;
				cell.Column = column;
				cell.Value  = value;
				cellEdits.push_back(cell);
			}
		}

		if (bNewRow)
		{
			newRows.push_back(HPair<int32, HDataTableRow>(inContent.GetRowCount() + (int32)newRows.size(), newRow));
		}
	}

	if (bOk == false)
	{
		return false;
	}

	if (keyEdits.empty() == false)
	{
		outEdits->push_back(HDataTableEdit::MakeSetKeys(keyEdits));
	}
	if (cellEdits.empty() == false)
	{
		outEdits->push_back(HDataTableEdit::MakeSetCells(cellEdits));
	}
	if (newRows.empty() == false)
	{
		outEdits->push_back(HDataTableEdit::MakeInsertRows(newRows));
	}
	return true;
}
