#include "PCH/PCH.h"
#include "Data/DataTableValidation.h"

namespace
{
	// 한 종류의 문제가 너무 많으면(행 수천) 처음 몇 개만 칸별로 남기고 나머지는 개수로
	constexpr int32 MaxCellIssuesPerColumn = 50;

	void addIssue(HList<HDataTableIssue>& outIssues, EDataTableIssueSeverity inSeverity, int32 inRow, int32 inColumn, const PString& inMessage)
	{
		HDataTableIssue issue;
		issue.Severity = inSeverity;
		issue.Row      = inRow;
		issue.Column   = inColumn;
		issue.Message  = inMessage;
		outIssues.push_back(issue);
	}

	void validateSchema(const HDataTableSchema& inSchema, HList<HDataTableIssue>& outIssues)
	{
		PString reason;
		if (IsValidDataTableColumnName(inSchema.KeyColumn, &reason) == false)
		{
			addIssue(outIssues, EDataTableIssueSeverity::Error, -1, -1, PString::Format("key column name '%s' is invalid (%s)", inSchema.KeyColumn, reason));
		}

		HHashMap<PString, int32> firstIndexByName;
		const int32 columnCount = (int32)inSchema.Columns.size();
		for (int32 c = 0; c < columnCount; ++c)
		{
			const HDataTableColumn& column = inSchema.Columns[c];
			if (IsValidDataTableColumnName(column.Name, &reason) == false)
			{
				addIssue(outIssues, EDataTableIssueSeverity::Error, -1, c, PString::Format("column name '%s' is invalid (%s)", column.Name, reason));
			}
			if (column.Name == inSchema.KeyColumn)
			{
				addIssue(outIssues, EDataTableIssueSeverity::Error, -1, c, PString::Format("column '%s' has the key column name", column.Name));
			}

			if (firstIndexByName.contains(column.Name))
			{
				addIssue(outIssues, EDataTableIssueSeverity::Error, -1, c, PString::Format("column '%s' is a duplicate of column %d", column.Name, firstIndexByName[column.Name] + 1));
			}
			else
			{
				firstIndexByName[column.Name] = c;
			}

			if (column.Type == EDataTableColumnType::Enum)
			{
				HList<PString> names;
				if (column.GetEnumNames(names) == false)
				{
					addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, c, PString::Format("enum type '%s' of column '%s' is not loaded. Values are not checked", column.EnumType, column.Name));
				}
				else if (names.empty())
				{
					addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, c, PString::Format("enum column '%s' has no EnumValues or EnumType", column.Name));
				}
			}

			if (column.bHasMin && column.bHasMax && column.Min > column.Max)
			{
				addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, c, PString::Format("column '%s': Min is greater than Max", column.Name));
			}
		}
	}

	void validateKeys(const HDataTableContent& inContent, HList<HDataTableIssue>& outIssues)
	{
		HHashMap<PString, int32> firstRowByKey;
		const int32 rowCount = inContent.GetRowCount();
		for (int32 r = 0; r < rowCount; ++r)
		{
			const PString& key = inContent.Rows[r].Key;

			PString reason;
			if (IsValidDataTableKey(key, &reason) == false)
			{
				addIssue(outIssues, EDataTableIssueSeverity::Error, r, -1, PString::Format("key '%s' is invalid (%s)", key, reason));
				continue;
			}

			if (firstRowByKey.contains(key))
			{
				addIssue(outIssues, EDataTableIssueSeverity::Error, r, -1, PString::Format("key '%s' is a duplicate of row %d", key, firstRowByKey[key] + 1));
			}
			else
			{
				firstRowByKey[key] = r;
			}
		}
	}

	struct HColumnIssueCounter
	{
		int32 Count = 0;

		// 처음 MaxCellIssuesPerColumn 개만 칸 문제로 남긴다
		bool ShouldReport()
		{
			++Count;
			return Count <= MaxCellIssuesPerColumn;
		}
	};

	void validateCells(const HDataTableContent& inContent, const IDataTableReferenceResolver* inResolver, HList<HDataTableIssue>& outIssues)
	{
		const HDataTableSchema& schema      = inContent.Schema;
		const int32             columnCount = (int32)schema.Columns.size();
		const int32             rowCount    = inContent.GetRowCount();

		HHashSet<PString> ownKeys;
		for (const HDataTableRow& row : inContent.Rows)
		{
			ownKeys.insert(row.Key);
		}

		for (int32 c = 0; c < columnCount; ++c)
		{
			const HDataTableColumn& column = schema.Columns[c];
			HColumnIssueCounter counter;

			// 열마다 필요한 것을 한 번만 준비한다
			HHashSet<PString> enumNames;
			bool bCheckEnum = false;
			if (column.Type == EDataTableColumnType::Enum)
			{
				HList<PString> names;
				if (column.GetEnumNames(names) == true && names.empty() == false)
				{
					bCheckEnum = true;
					for (const PString& name : names)
					{
						enumNames.insert(name);
					}
				}
			}

			const HHashSet<PString>* targetKeys = nullptr;
			HHashSet<PString> otherTableKeys;
			bool bCheckRowRef = false;
			if (column.Type == EDataTableColumnType::RowRef)
			{
				if (column.Table.Empty())
				{
					targetKeys   = &ownKeys;
					bCheckRowRef = true;
				}
				else if (inResolver != nullptr)
				{
					HList<PString> keys;
					if (inResolver->GetTableKeys(column.Table, &keys) == true)
					{
						for (const PString& key : keys)
						{
							otherTableKeys.insert(key);
						}
						targetKeys   = &otherTableKeys;
						bCheckRowRef = true;
					}
					else
					{
						addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, c, PString::Format("table '%s' of column '%s' is not found", column.Table, column.Name));
					}
				}
			}

			for (int32 r = 0; r < rowCount; ++r)
			{
				const HDataTableRow& row = inContent.Rows[r];
				if (c >= (int32)row.Values.size())
				{
					continue;
				}
				const HDataTableValue& value = row.Values[c];

				switch (column.Type)
				{
				case EDataTableColumnType::Int:
				case EDataTableColumnType::Float:
				{
					const float64 number = (column.Type == EDataTableColumnType::Int) ? (float64)value.GetInt() : value.GetFloat();
					const bool bBelow = column.bHasMin && number < column.Min;
					const bool bAbove = column.bHasMax && number > column.Max;
					if ((bBelow || bAbove) && counter.ShouldReport())
					{
						addIssue(outIssues, EDataTableIssueSeverity::Warning, r, c, PString::Format("%s is out of range", column.FormatText(value)));
					}
					break;
				}
				case EDataTableColumnType::Enum:
					if (bCheckEnum && enumNames.contains(value.GetText()) == false && counter.ShouldReport())
					{
						addIssue(outIssues, EDataTableIssueSeverity::Warning, r, c, PString::Format("'%s' is not in the enum list", value.GetText()));
					}
					break;
				case EDataTableColumnType::RowRef:
					if (bCheckRowRef && value.GetText().Empty() == false && targetKeys->contains(value.GetText()) == false && counter.ShouldReport())
					{
						addIssue(outIssues, EDataTableIssueSeverity::Warning, r, c, PString::Format("row '%s' is not found", value.GetText()));
					}
					break;
				case EDataTableColumnType::AssetRef:
					if (inResolver != nullptr && value.GetText().Empty() == false && inResolver->HasAsset(value.GetText()) == false && counter.ShouldReport())
					{
						addIssue(outIssues, EDataTableIssueSeverity::Warning, r, c, PString::Format("asset '%s' is not found", value.GetText()));
					}
					break;
				default:
					break;
				}
			}

			if (counter.Count > MaxCellIssuesPerColumn)
			{
				addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, c, PString::Format("column '%s': %d more value issue(s) not listed", column.Name, counter.Count - MaxCellIssuesPerColumn));
			}
		}
	}
}

void ValidateDataTableContent(const HDataTableContent& inContent, const IDataTableReferenceResolver* inResolver, HList<HDataTableIssue>& outIssues)
{
	validateSchema(inContent.Schema, outIssues);
	validateKeys(inContent, outIssues);
	validateCells(inContent, inResolver, outIssues);
}
