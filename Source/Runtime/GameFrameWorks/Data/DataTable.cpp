#include "PCH/PCH.h"
#include "Data/DataTable.h"

namespace
{
	const PString& emptyText()
	{
		static PString text;
		return text;
	}

	const HDataTableValue& emptyValue()
	{
		static HDataTableValue value;
		return value;
	}
}

JGDataTable::JGDataTable()
{
}

const PString& JGDataTable::GetRowKey(int32 inRowIndex) const
{
	if (inRowIndex < 0 || inRowIndex >= _content.GetRowCount())
	{
		return emptyText();
	}
	return _content.Rows[inRowIndex].Key;
}

const HDataTableValue& JGDataTable::GetValue(int32 inRowIndex, int32 inColumnIndex) const
{
	if (inColumnIndex < 0 || inColumnIndex >= _content.GetColumnCount())
	{
		return emptyValue();
	}
	if (inRowIndex < 0 || inRowIndex >= _content.GetRowCount())
	{
		return _content.Schema.Columns[inColumnIndex].Default;
	}

	const HDataTableRow& row = _content.Rows[inRowIndex];
	if (inColumnIndex >= (int32)row.Values.size())
	{
		return _content.Schema.Columns[inColumnIndex].Default;
	}
	return row.Values[inColumnIndex];
}

int32 JGDataTable::FindRowIndex(const PName& inKey) const
{
	HHashMap<PName, int32>::const_iterator iter = _rowIndexByKey.find(inKey);
	if (iter == _rowIndexByKey.end())
	{
		return -1;
	}
	return iter->second;
}

int32 JGDataTable::FindRowIndex(const PString& inKey) const
{
	if (inKey.Empty())
	{
		return -1;
	}
	return FindRowIndex(PName(inKey));
}

uint64 JGDataTable::ComputeContentHash() const
{
	return ComputeDataTableContentHash(_content);
}

void JGDataTable::InitializeNew(const PString& inTokenAssetPath, const HDataTableContent& inContent)
{
	// JGAsset::SetName 이 에셋 경로(AssetPath)를 정한다. 이름은 다시 타입 이름으로 돌린다:
	// 읽을 때 JGObject 가 이름을 되살리지 않아(타입 이름이 된다) 다른 이름을 남기면 첫 저장 때마다 파일이 바뀐다.
	SetName(PName(inTokenAssetPath));
	JGObject::SetName(JGTYPE(JGDataTable).GetName());

	_content = inContent;
	_loadIssues.clear();
	rebuildIndex();
	++_revision;
}

void JGDataTable::ApplyContent(const HDataTableContent& inContent)
{
	_content = inContent;
	_loadIssues.clear();
	rebuildIndex();
	++_revision;

	OnChanged.BroadCast(*this);
}

PSharedPtr<JGDataTable> JGDataTable::FromJsonText(const PString& inText, PString* outError)
{
	PJson json;
	PString parseError;
	if (PJson::ToObjectWithError(inText, &json, &parseError) == false)
	{
		if (outError != nullptr)
		{
			*outError = PString::Format("JSON %s", parseError);
		}
		return nullptr;
	}

	const PString expectedType = JGTYPE(JGDataTable).GetName().ToString();
	PString objectType;
	PJsonData typeJson;
	if (json.FindMember("JGObjectType", &typeJson) == false || typeJson.GetValueType() != EJsonValueType::String || typeJson.GetData(&objectType) == false || objectType != expectedType)
	{
		if (outError != nullptr)
		{
			*outError = PString::Format("not a %s asset (JGObjectType '%s')", expectedType, objectType);
		}
		return nullptr;
	}

	PJsonData objectJson;
	if (json.FindMember("JGObject", &objectJson) == false || objectJson.GetValueType() != EJsonValueType::Object)
	{
		if (outError != nullptr)
		{
			*outError = "JGObject section is missing";
		}
		return nullptr;
	}

	PSharedPtr<JGDataTable> table = Allocate<JGDataTable>();
	objectJson.GetData(table.GetRawPointer());
	return table;
}

PSharedPtr<JGDataTable> JGDataTable::LoadFromFile(const PString& inRawPath, PString* outError)
{
	PString text;
	if (HFileHelper::ReadAllText(inRawPath, &text) == false)
	{
		if (outError != nullptr)
		{
			*outError = PString::Format("cannot read %s", inRawPath);
		}
		return nullptr;
	}

	PString error;
	PSharedPtr<JGDataTable> table = FromJsonText(text, &error);
	if (table == nullptr && outError != nullptr)
	{
		*outError = PString::Format("%s : %s", inRawPath, error);
	}
	return table;
}

bool JGDataTable::ToJsonText(PString* outText) const
{
	if (outText == nullptr)
	{
		return false;
	}

	// SaveObject 와 같은 껍데기({"JGObjectType", "JGObject"}). LoadObject · FromJsonText 가 그대로 읽는다.
	PJson json;
	json.AddMember("JGObjectType", JGTYPE(JGDataTable));
	json.AddMember("JGObject", *this);
	return PJson::ToString(json, outText);
}

bool JGDataTable::SaveToFile(const PString& inRawPath, PString* outError) const
{
	PString text;
	if (ToJsonText(&text) == false)
	{
		if (outError != nullptr)
		{
			*outError = "cannot serialize the table";
		}
		return false;
	}

	std::error_code errCode;
	const fs::path folder = fs::path(inRawPath.GetRawString()).parent_path();
	if (folder.empty() == false)
	{
		fs::create_directories(folder, errCode);
	}

	if (HFileHelper::WriteAllTextAtomic(inRawPath, text) == false)
	{
		if (outError != nullptr)
		{
			*outError = PString::Format("cannot write %s", inRawPath);
		}
		return false;
	}
	return true;
}

void JGDataTable::WriteJson(PJsonData& json) const
{
	JG_SUPER::WriteJson(json);

	PJsonData tableJson = json.CreateJsonData();
	WriteDataTableContentJson(tableJson, _content);
	json.AddMember("JGDataTable", tableJson);
}

void JGDataTable::ReadJson(const PJsonData& json)
{
	JG_SUPER::ReadJson(json);

	_loadIssues.clear();
	PJsonData tableJson;
	if (json.FindMember("JGDataTable", &tableJson) == false || tableJson.GetValueType() != EJsonValueType::Object)
	{
		_content = HDataTableContent();

		HDataTableIssue issue;
		issue.Severity = EDataTableIssueSeverity::Error;
		issue.Message  = "JGDataTable section is missing";
		_loadIssues.push_back(issue);
	}
	else
	{
		ReadDataTableContentJson(tableJson, &_content, &_loadIssues);
	}

	if (HasDataTableErrors(_loadIssues))
	{
		LogDataTableIssues("JGDataTable load", _loadIssues, 5);
	}

	rebuildIndex();
	++_revision;
}

void JGDataTable::rebuildIndex()
{
	_rowIndexByKey.clear();
	_rowIndexByKey.reserve(_content.Rows.size());

	const int32 rowCount = _content.GetRowCount();
	for (int32 i = 0; i < rowCount; ++i)
	{
		const PString& key = _content.Rows[i].Key;
		if (key.Empty())
		{
			continue;
		}

		const PName name(key);
		if (_rowIndexByKey.contains(name) == false)
		{
			_rowIndexByKey[name] = i;
		}
	}
}
