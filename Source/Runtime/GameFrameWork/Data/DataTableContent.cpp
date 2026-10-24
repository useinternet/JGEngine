#include "PCH/PCH.h"
#include "Data/DataTableContent.h"

namespace
{
	void addIssue(HList<HDataTableIssue>* outIssues, EDataTableIssueSeverity inSeverity, int32 inRow, int32 inColumn, const PString& inMessage)
	{
		if (outIssues == nullptr)
		{
			return;
		}

		HDataTableIssue issue;
		issue.Severity = inSeverity;
		issue.Row      = inRow;
		issue.Column   = inColumn;
		issue.Message  = inMessage;
		outIssues->push_back(issue);
	}

	// 행 하나를 객체로 쓴다. 키 열 → 열 순서. (HList 로 모아 AddMember 하면 배열이 된다)
	class HDataTableRowJsonWriter : public IJsonable
	{
	public:
		const HDataTableSchema* Schema = nullptr;
		const HDataTableRow*    Row    = nullptr;

	protected:
		virtual void WriteJson(PJsonData& json) const override
		{
			json.AddMember(Schema->KeyColumn, Row->Key);

			const int32 columnCount = (int32)Schema->Columns.size();
			for (int32 i = 0; i < columnCount; ++i)
			{
				const HDataTableColumn& column = Schema->Columns[i];
				const HDataTableValue&  value  = (i < (int32)Row->Values.size()) ? Row->Values[i] : column.Default;
				column.WriteValueJson(json, column.Name, value);
			}
		}
	};

	struct HFnv1aHash
	{
		uint64 Hash = 14695981039346656037ull;

		void Bytes(const void* inData, uint64 inSize)
		{
			const uint8* bytes = (const uint8*)inData;
			for (uint64 i = 0; i < inSize; ++i)
			{
				Hash ^= bytes[i];
				Hash *= 1099511628211ull;
			}
		}

		void Int(int64 inValue)
		{
			Bytes(&inValue, sizeof(inValue));
		}

		void Float(float64 inValue)
		{
			Bytes(&inValue, sizeof(inValue));
		}

		void Text(const PString& inText)
		{
			const HRawString& raw = inText.GetRawString();
			Int((int64)raw.size());
			Bytes(raw.data(), raw.size());
		}

		void Value(const HDataTableValue& inValue)
		{
			const uint8 kind = (uint8)inValue.GetKind();
			Bytes(&kind, 1);
			switch (inValue.GetKind())
			{
			case EDataTableValueKind::Bool:
				Int(inValue.GetBool() ? 1 : 0);
				break;
			case EDataTableValueKind::Int:
				Int(inValue.GetInt());
				break;
			case EDataTableValueKind::Float:
				Float(inValue.GetFloat());
				break;
			default:
				Text(inValue.GetText());
				break;
			}
		}
	};
}

bool HDataTableRow::operator==(const HDataTableRow& inOther) const
{
	return DataTableSameText(Key, inOther.Key) && Values == inOther.Values;
}

bool HDataTableRow::operator!=(const HDataTableRow& inOther) const
{
	return (*this == inOther) == false;
}

int32 HDataTableContent::FindRowByKey(const PString& inKey) const
{
	const int32 rowCount = (int32)Rows.size();
	for (int32 i = 0; i < rowCount; ++i)
	{
		if (Rows[i].Key == inKey)
		{
			return i;
		}
	}
	return -1;
}

HDataTableRow HDataTableContent::MakeDefaultRow(const PString& inKey) const
{
	HDataTableRow row;
	row.Key = inKey;
	row.Values.reserve(Schema.Columns.size());
	for (const HDataTableColumn& column : Schema.Columns)
	{
		row.Values.push_back(column.Default);
	}
	return row;
}

PString HDataTableContent::MakeUniqueKey(const PString& inBase) const
{
	HHashSet<PString> usedKeys;
	for (const HDataTableRow& row : Rows)
	{
		usedKeys.insert(row.Key);
	}

	const PString base = inBase.Empty() ? PString("Row") : inBase;
	if (usedKeys.contains(base) == false)
	{
		return base;
	}

	for (int32 i = 1; ; ++i)
	{
		const PString candidate = PString::Format("%s_%d", base, i);
		if (usedKeys.contains(candidate) == false)
		{
			return candidate;
		}
	}
}

bool HDataTableContent::operator==(const HDataTableContent& inOther) const
{
	return DataTableSameText(Schema.KeyColumn, inOther.Schema.KeyColumn) && Schema.Columns == inOther.Schema.Columns && Rows == inOther.Rows;
}

bool HDataTableContent::operator!=(const HDataTableContent& inOther) const
{
	return (*this == inOther) == false;
}

bool HasDataTableErrors(const HList<HDataTableIssue>& inIssues)
{
	for (const HDataTableIssue& issue : inIssues)
	{
		if (issue.Severity == EDataTableIssueSeverity::Error)
		{
			return true;
		}
	}
	return false;
}

void LogDataTableIssues(const PString& inContext, const HList<HDataTableIssue>& inIssues, int32 inMaxLines)
{
	int32 logged = 0;
	for (const HDataTableIssue& issue : inIssues)
	{
		if (logged >= inMaxLines)
		{
			break;
		}

		const ELogLevel level = (issue.Severity == EDataTableIssueSeverity::Error) ? ELogLevel::Error : ELogLevel::Warning;
		JG_LOG(DataTable, level, "%s : %s (row %d, column %d)", inContext, issue.Message, issue.Row, issue.Column);
		++logged;
	}

	if ((int32)inIssues.size() > logged)
	{
		JG_LOG(DataTable, ELogLevel::Warning, "%s : %d more issue(s) not shown", inContext, (int32)inIssues.size() - logged);
	}
}

uint64 ComputeDataTableContentHash(const HDataTableContent& inContent)
{
	HFnv1aHash hash;
	hash.Int(DataTableFormatVersion);
	hash.Text(inContent.Schema.KeyColumn);

	hash.Int((int64)inContent.Schema.Columns.size());
	for (const HDataTableColumn& column : inContent.Schema.Columns)
	{
		hash.Text(column.Name);
		hash.Int((int64)column.Type);
		hash.Value(column.Default);
		hash.Text(column.Description);
		hash.Int(column.bHasMin ? 1 : 0);
		hash.Float(column.Min);
		hash.Int(column.bHasMax ? 1 : 0);
		hash.Float(column.Max);
		hash.Int((int64)column.EnumValues.size());
		for (const PString& enumValue : column.EnumValues)
		{
			hash.Text(enumValue);
		}
		hash.Text(column.EnumType);
		hash.Text(column.AssetClass);
		hash.Text(column.Table);
	}

	hash.Int((int64)inContent.Rows.size());
	for (const HDataTableRow& row : inContent.Rows)
	{
		hash.Text(row.Key);
		hash.Int((int64)row.Values.size());
		for (const HDataTableValue& value : row.Values)
		{
			hash.Value(value);
		}
	}

	return hash.Hash;
}

bool ReadDataTableContentJson(const PJsonData& json, HDataTableContent* outContent, HList<HDataTableIssue>* outIssues)
{
	if (outContent == nullptr)
	{
		return false;
	}
	*outContent = HDataTableContent();

	// 값은 찾으면 문서에서 옮겨진다 — 키마다 한 번만 찾고, 종류는 GetValueType 으로 먼저 묻는다
	PJsonData versionJson;
	if (json.FindMember("FormatVersion", &versionJson) == true)
	{
		int64 version = 0;
		if (versionJson.GetValueType() != EJsonValueType::Int || versionJson.GetData(&version) == false)
		{
			addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, -1, "FormatVersion is not an integer (1 assumed)");
		}
		else if (version > DataTableFormatVersion)
		{
			addIssue(outIssues, EDataTableIssueSeverity::Error, -1, -1, PString::Format("FormatVersion %d is newer than this build reads (%d)", (int32)version, DataTableFormatVersion));
		}
	}
	else
	{
		addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, -1, "FormatVersion is missing (1 assumed)");
	}

	PJsonData keyColumnJson;
	if (json.FindMember("KeyColumn", &keyColumnJson) == true)
	{
		PString keyColumn;
		if (keyColumnJson.GetValueType() == EJsonValueType::String && keyColumnJson.GetData(&keyColumn) == true)
		{
			outContent->Schema.KeyColumn = keyColumn;
		}
		else
		{
			addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, -1, "KeyColumn is not a string ('Id' assumed)");
		}
	}

	PJsonData columnsJson;
	if (json.FindMember("Columns", &columnsJson) == false || columnsJson.GetValueType() != EJsonValueType::Array)
	{
		addIssue(outIssues, EDataTableIssueSeverity::Error, -1, -1, "Columns is missing or not a list");
		return false;
	}

	const int32 columnJsonCount = columnsJson.GetSize();
	for (int32 i = 0; i < columnJsonCount; ++i)
	{
		PJsonData columnJson;
		if (columnsJson.FindMemberFromIndex(i, &columnJson) == false || columnJson.GetValueType() != EJsonValueType::Object)
		{
			addIssue(outIssues, EDataTableIssueSeverity::Error, -1, -1, PString::Format("Columns[%d] is not an object (skipped)", i));
			continue;
		}

		HDataTableColumn column;
		PString          error;
		HList<PString>   warnings;
		if (column.Read(columnJson, &error, &warnings) == false)
		{
			addIssue(outIssues, EDataTableIssueSeverity::Error, -1, -1, PString::Format("Columns[%d]: %s (skipped)", i, error));
			continue;
		}

		const int32 columnIndex = (int32)outContent->Schema.Columns.size();
		for (const PString& warning : warnings)
		{
			addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, columnIndex, warning);
		}
		outContent->Schema.Columns.push_back(column);
	}

	PJsonData rowsJson;
	if (json.FindMember("Rows", &rowsJson) == false || rowsJson.GetValueType() != EJsonValueType::Array)
	{
		addIssue(outIssues, EDataTableIssueSeverity::Error, -1, -1, "Rows is missing or not a list");
		return false;
	}

	const HDataTableSchema& schema      = outContent->Schema;
	const int32             columnCount = (int32)schema.Columns.size();

	HHashSet<PString> knownKeys;
	knownKeys.insert(schema.KeyColumn);
	for (const HDataTableColumn& column : schema.Columns)
	{
		knownKeys.insert(column.Name);
	}

	// 같은 문제는 열(키)마다 한 줄로 모은다 — 행이 수천이면 문제도 수천이 된다
	HList<PString>           unknownKeyOrder;
	HHashMap<PString, int32> unknownKeyCounts;
	HList<int32> missingCounts(columnCount, 0);
	HList<int32> missingFirstRow(columnCount, -1);
	HList<int32> wrongTypeCounts(columnCount, 0);
	HList<int32> wrongTypeFirstRow(columnCount, -1);

	const int32 rowJsonCount = rowsJson.GetSize();
	outContent->Rows.reserve(rowJsonCount);
	for (int32 r = 0; r < rowJsonCount; ++r)
	{
		HDataTableRow row = outContent->MakeDefaultRow(PString());

		PJsonData rowJson;
		if (rowsJson.FindMemberFromIndex(r, &rowJson) == false || rowJson.GetValueType() != EJsonValueType::Object)
		{
			// 빈 키 행으로 둔다(행 번호가 파일과 맞고, 빈 키 오류로 보인다)
			addIssue(outIssues, EDataTableIssueSeverity::Error, r, -1, "row is not an object");
			outContent->Rows.push_back(row);
			continue;
		}

		HList<PString> memberKeys;
		rowJson.GetMemberKeys(&memberKeys);
		for (const PString& memberKey : memberKeys)
		{
			if (knownKeys.contains(memberKey) == false)
			{
				if (unknownKeyCounts.contains(memberKey) == false)
				{
					unknownKeyOrder.push_back(memberKey);
					unknownKeyCounts[memberKey] = 0;
				}
				++unknownKeyCounts[memberKey];
			}
		}

		PJsonData keyJson;
		if (rowJson.FindMember(schema.KeyColumn, &keyJson) == true)
		{
			const EJsonValueType keyType = keyJson.GetValueType();
			if (keyType == EJsonValueType::String)
			{
				keyJson.GetData(&row.Key);
			}
			else if (keyType == EJsonValueType::Int)
			{
				// 손으로 쓴 숫자 키(101). 글로 바꿔 둔다 — 저장하면 "101"
				int64 numberKey = 0;
				keyJson.GetData(&numberKey);
				row.Key = PString::FromInt64(numberKey);
				addIssue(outIssues, EDataTableIssueSeverity::Warning, r, -1, PString::Format("key %s is a number (saved as text)", row.Key));
			}
			else
			{
				addIssue(outIssues, EDataTableIssueSeverity::Error, r, -1, PString::Format("key '%s' is not a string", schema.KeyColumn));
			}
		}
		else
		{
			addIssue(outIssues, EDataTableIssueSeverity::Error, r, -1, PString::Format("row has no key '%s'", schema.KeyColumn));
		}

		for (int32 c = 0; c < columnCount; ++c)
		{
			const HDataTableColumn& column = schema.Columns[c];

			PJsonData cellJson;
			if (rowJson.FindMember(column.Name, &cellJson) == false)
			{
				if (missingCounts[c]++ == 0)
				{
					missingFirstRow[c] = r;
				}
				continue;
			}

			HDataTableValue value;
			if (column.ReadValueJson(cellJson, &value) == false)
			{
				if (wrongTypeCounts[c]++ == 0)
				{
					wrongTypeFirstRow[c] = r;
				}
				continue;
			}
			row.Values[c] = value;
		}

		outContent->Rows.push_back(row);
	}

	for (const PString& unknownKey : unknownKeyOrder)
	{
		addIssue(outIssues, EDataTableIssueSeverity::Warning, -1, -1,
			PString::Format("key '%s' in %d row(s) is not a column. It is dropped on save", unknownKey, unknownKeyCounts[unknownKey]));
	}
	for (int32 c = 0; c < columnCount; ++c)
	{
		if (missingCounts[c] > 0)
		{
			addIssue(outIssues, EDataTableIssueSeverity::Warning, missingFirstRow[c], c,
				PString::Format("%d row(s) have no '%s' value (default used)", missingCounts[c], schema.Columns[c].Name));
		}
		if (wrongTypeCounts[c] > 0)
		{
			addIssue(outIssues, EDataTableIssueSeverity::Warning, wrongTypeFirstRow[c], c,
				PString::Format("%d row(s) have a '%s' value that is not %s (default used)", wrongTypeCounts[c], schema.Columns[c].Name, PString(DataTableColumnTypeToString(schema.Columns[c].Type))));
		}
	}

	return true;
}

void WriteDataTableContentJson(PJsonData& json, const HDataTableContent& inContent)
{
	json.AddMember("FormatVersion", DataTableFormatVersion);
	json.AddMember("KeyColumn", inContent.Schema.KeyColumn);
	json.AddMember("Columns", inContent.Schema.Columns);

	HList<HDataTableRowJsonWriter> rowWriters;
	rowWriters.reserve(inContent.Rows.size());
	for (const HDataTableRow& row : inContent.Rows)
	{
		HDataTableRowJsonWriter writer;
		writer.Schema = &inContent.Schema;
		writer.Row    = &row;
		rowWriters.push_back(writer);
	}
	json.AddMember("Rows", rowWriters);
}
