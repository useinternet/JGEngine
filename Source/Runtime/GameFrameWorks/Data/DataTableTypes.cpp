#include "PCH/PCH.h"
#include "Data/DataTableTypes.h"

namespace
{
	const char* const ColumnTypeNames[] = { "Bool", "Int", "Float", "String", "Enum", "AssetRef", "RowRef" };
	static_assert(sizeof(ColumnTypeNames) / sizeof(ColumnTypeNames[0]) == (size_t)EDataTableColumnType::Count, "ColumnTypeNames must match EDataTableColumnType");
}

const char* DataTableColumnTypeToString(EDataTableColumnType inType)
{
	const int32 index = (int32)inType;
	if (index < 0 || index >= (int32)EDataTableColumnType::Count)
	{
		return "Unknown";
	}
	return ColumnTypeNames[index];
}

bool DataTableColumnTypeFromString(const PString& inText, EDataTableColumnType* outType)
{
	if (outType == nullptr)
	{
		return false;
	}

	const PString lowerText = DataTableToLowerAscii(inText);
	for (int32 i = 0; i < (int32)EDataTableColumnType::Count; ++i)
	{
		if (DataTableToLowerAscii(PString(ColumnTypeNames[i])) == lowerText)
		{
			*outType = (EDataTableColumnType)i;
			return true;
		}
	}
	return false;
}

PString DataTableToLowerAscii(const PString& inText)
{
	HRawString raw = inText.GetRawString();
	for (char& c : raw)
	{
		if (c >= 'A' && c <= 'Z')
		{
			c = (char)(c - 'A' + 'a');
		}
	}
	return PString(raw.c_str());
}

bool DataTableContainsIgnoreCase(const PString& inText, const PString& inPattern)
{
	if (inPattern.Empty())
	{
		return true;
	}
	return DataTableToLowerAscii(inText).GetRawString().find(DataTableToLowerAscii(inPattern).GetRawString()) != HRawString::npos;
}

EDataTableValueKind DataTableValueKindOf(EDataTableColumnType inType)
{
	switch (inType)
	{
	case EDataTableColumnType::Bool:
		return EDataTableValueKind::Bool;
	case EDataTableColumnType::Int:
		return EDataTableValueKind::Int;
	case EDataTableColumnType::Float:
		return EDataTableValueKind::Float;
	default:
		return EDataTableValueKind::Text;
	}
}

HDataTableValue HDataTableValue::MakeBool(bool inValue)
{
	HDataTableValue value;
	value._kind = EDataTableValueKind::Bool;
	value._bool = inValue;
	return value;
}

HDataTableValue HDataTableValue::MakeInt(int64 inValue)
{
	HDataTableValue value;
	value._kind = EDataTableValueKind::Int;
	value._int  = inValue;
	return value;
}

HDataTableValue HDataTableValue::MakeFloat(float64 inValue)
{
	HDataTableValue value;
	value._kind  = EDataTableValueKind::Float;
	value._float = inValue;
	return value;
}

HDataTableValue HDataTableValue::MakeText(const PString& inValue)
{
	HDataTableValue value;
	value._kind = EDataTableValueKind::Text;
	value._text = inValue;
	return value;
}

HDataTableValue HDataTableValue::MakeDefault(EDataTableValueKind inKind)
{
	switch (inKind)
	{
	case EDataTableValueKind::Bool:
		return MakeBool(false);
	case EDataTableValueKind::Int:
		return MakeInt(0);
	case EDataTableValueKind::Float:
		return MakeFloat(0.0);
	default:
		return MakeText(PString());
	}
}

bool HDataTableValue::operator==(const HDataTableValue& inOther) const
{
	if (_kind != inOther._kind)
	{
		return false;
	}

	switch (_kind)
	{
	case EDataTableValueKind::Bool:
		return _bool == inOther._bool;
	case EDataTableValueKind::Int:
		return _int == inOther._int;
	case EDataTableValueKind::Float:
		return memcmp(&_float, &inOther._float, sizeof(_float)) == 0;
	default:
		return DataTableSameText(_text, inOther._text);
	}
}

bool HDataTableValue::operator!=(const HDataTableValue& inOther) const
{
	return (*this == inOther) == false;
}
