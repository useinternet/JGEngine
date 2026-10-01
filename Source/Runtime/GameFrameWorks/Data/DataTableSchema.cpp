#include "PCH/PCH.h"
#include "Data/DataTableSchema.h"
#include "Object/ObjectGlobalSystem.h"
#include "AssetDefines.h"
#include <charconv>
#include <cmath>

namespace
{
	PString trimmedCopy(const PString& inText)
	{
		PString copy = inText;
		copy.Trim();
		return copy;
	}

	bool parseInt64(const PString& inText, int64* outValue)
	{
		const HRawString& raw = inText.GetRawString();
		const char* begin = raw.data();
		const char* end   = begin + raw.size();
		if (begin != end && *begin == '+')
		{
			++begin;
		}
		if (begin == end)
		{
			return false;
		}

		int64 value = 0;
		const std::from_chars_result result = std::from_chars(begin, end, value);
		if (result.ec != std::errc() || result.ptr != end)
		{
			return false;
		}
		*outValue = value;
		return true;
	}

	bool parseFloat64(const PString& inText, float64* outValue)
	{
		const HRawString& raw = inText.GetRawString();
		const char* begin = raw.data();
		const char* end   = begin + raw.size();
		if (begin != end && *begin == '+')
		{
			++begin;
		}
		if (begin == end)
		{
			return false;
		}

		float64 value = 0.0;
		const std::from_chars_result result = std::from_chars(begin, end, value, std::chars_format::general);
		if (result.ec != std::errc() || result.ptr != end || std::isfinite(value) == false)
		{
			return false;
		}
		*outValue = value;
		return true;
	}

	PString formatInt64(int64 inValue)
	{
		char buffer[32];
		const std::to_chars_result result = std::to_chars(buffer, buffer + sizeof(buffer), inValue);
		return PString(HRawString(buffer, result.ptr).c_str());
	}

	// 최단 왕복 표기. 정수처럼 보이면 ".0" 을 붙여 실수 열임이 보이게 한다(1 → "1.0").
	PString formatFloat64(float64 inValue)
	{
		char buffer[64];
		const std::to_chars_result result = std::to_chars(buffer, buffer + sizeof(buffer), inValue);
		HRawString text(buffer, result.ptr);
		if (text.find_first_of(".eE") == HRawString::npos)
		{
			text += ".0";
		}
		return PString(text.c_str());
	}

	// 다른 타입 값을 글로 (타입 바꾸기의 중간 단계)
	PString valueToText(const HDataTableValue& inValue)
	{
		switch (inValue.GetKind())
		{
		case EDataTableValueKind::Bool:
			return inValue.GetBool() ? PString("true") : PString("false");
		case EDataTableValueKind::Int:
			return formatInt64(inValue.GetInt());
		case EDataTableValueKind::Float:
			return formatFloat64(inValue.GetFloat());
		default:
			return inValue.GetText();
		}
	}

	bool readNumber(const PJsonData& inJson, float64* outValue)
	{
		const EJsonValueType jsonType = inJson.GetValueType();
		if (jsonType == EJsonValueType::Int)
		{
			int64 intValue = 0;
			if (inJson.GetData(&intValue) == false)
			{
				return false;
			}
			*outValue = (float64)intValue;
			return true;
		}
		if (jsonType == EJsonValueType::Float)
		{
			return inJson.GetData(outValue);
		}
		return false;
	}

	bool readOptionalString(const PJsonData& json, const char* inKey, PString* outValue, HList<PString>* outWarnings)
	{
		PJsonData member;
		if (json.FindMember(inKey, &member) == false)
		{
			return false;
		}
		if (member.GetValueType() != EJsonValueType::String || member.GetData(outValue) == false)
		{
			if (outWarnings != nullptr)
			{
				outWarnings->push_back(PString::Format("%s is not a string", PString(inKey)));
			}
			return false;
		}
		return true;
	}

	bool hasBadNameCharacter(const PString& inName, bool bRejectInnerSpace, PString* outReason)
	{
		const HRawString& raw = inName.GetRawString();
		if (raw.empty())
		{
			if (outReason != nullptr)
			{
				*outReason = "empty";
			}
			return true;
		}
		if (raw.front() == ' ' || raw.front() == '\t' || raw.back() == ' ' || raw.back() == '\t')
		{
			if (outReason != nullptr)
			{
				*outReason = "leading or trailing space";
			}
			return true;
		}
		for (char c : raw)
		{
			const uint8 byte = (uint8)c;
			if (byte < 0x20 || byte == 0x7F || c == '"')
			{
				if (outReason != nullptr)
				{
					*outReason = "control character or quote";
				}
				return true;
			}
			if (bRejectInnerSpace && (c == ' ' || c == '\t'))
			{
				if (outReason != nullptr)
				{
					*outReason = "space";
				}
				return true;
			}
		}
		return false;
	}
}

bool IsValidDataTableColumnName(const PString& inName, PString* outReason)
{
	return hasBadNameCharacter(inName, false, outReason) == false;
}

bool IsValidDataTableKey(const PString& inKey, PString* outReason)
{
	return hasBadNameCharacter(inKey, true, outReason) == false;
}

HDataTableColumn::HDataTableColumn()
	: Default(HDataTableValue::MakeDefault(EDataTableValueKind::Text))
{
}

HDataTableColumn::HDataTableColumn(const PString& inName, EDataTableColumnType inType)
	: Name(inName)
	, Type(inType)
	, Default(HDataTableValue::MakeDefault(DataTableValueKindOf(inType)))
{
}

bool HDataTableColumn::GetEnumNames(HList<PString>& outNames) const
{
	outNames.clear();
	if (EnumValues.empty() == false)
	{
		outNames = EnumValues;
		return true;
	}
	if (EnumType.Empty())
	{
		return true;
	}

	const JGType& enumType = GObjectGlobalSystem::GetInstance().GetType(PName(EnumType));
	PSharedPtr<JGEnum> reflectedEnum = GObjectGlobalSystem::GetInstance().GetStaticEnum(enumType);
	if (reflectedEnum == nullptr)
	{
		return false;
	}

	for (int32 i = 0; ; ++i)
	{
		const PName elementName = reflectedEnum->GetEnumNameByIndex(i);
		if (elementName == NAME_NONE)
		{
			break;
		}
		outNames.push_back(elementName.ToString());
	}
	return true;
}

bool HDataTableColumn::ParseText(const PString& inText, HDataTableValue* outValue, PString* outError) const
{
	if (outValue == nullptr)
	{
		return false;
	}

	const PString text = trimmedCopy(inText);
	switch (Type)
	{
	case EDataTableColumnType::Bool:
	{
		const PString lowerText = DataTableToLowerAscii(text);
		if (lowerText == "true" || lowerText == "1")
		{
			*outValue = HDataTableValue::MakeBool(true);
			return true;
		}
		if (lowerText == "false" || lowerText == "0")
		{
			*outValue = HDataTableValue::MakeBool(false);
			return true;
		}
		if (outError != nullptr)
		{
			*outError = PString::Format("'%s' is not true / false", text);
		}
		return false;
	}
	case EDataTableColumnType::Int:
	{
		int64 value = 0;
		if (parseInt64(text, &value) == false)
		{
			if (outError != nullptr)
			{
				*outError = PString::Format("'%s' is not an integer", text);
			}
			return false;
		}
		*outValue = HDataTableValue::MakeInt(value);
		return true;
	}
	case EDataTableColumnType::Float:
	{
		float64 value = 0.0;
		if (parseFloat64(text, &value) == false)
		{
			if (outError != nullptr)
			{
				*outError = PString::Format("'%s' is not a number", text);
			}
			return false;
		}
		*outValue = HDataTableValue::MakeFloat(value);
		return true;
	}
	case EDataTableColumnType::String:
		// 글은 그대로 둔다(앞뒤 공백도 데이터일 수 있다)
		*outValue = HDataTableValue::MakeText(inText);
		return true;
	case EDataTableColumnType::Enum:
	{
		HList<PString> names;
		if (GetEnumNames(names) == false || names.empty())
		{
			// 목록을 모른다(리플렉션 열거형이 안 올라옴). 글은 받아 두고 검증이 경고한다.
			if (text.Empty())
			{
				if (outError != nullptr)
				{
					*outError = "enum value is empty";
				}
				return false;
			}
			*outValue = HDataTableValue::MakeText(text);
			return true;
		}

		const PString lowerText = DataTableToLowerAscii(text);
		for (const PString& name : names)
		{
			if (DataTableToLowerAscii(name) == lowerText)
			{
				*outValue = HDataTableValue::MakeText(name);
				return true;
			}
		}
		if (outError != nullptr)
		{
			*outError = PString::Format("'%s' is not in the enum list", text);
		}
		return false;
	}
	case EDataTableColumnType::AssetRef:
		if (text.Empty() == false && text.StartWidth(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN) == false && text.StartWidth(JG_ASSET_GAME_PATH_RECOGNITION_TOEKN) == false)
		{
			if (outError != nullptr)
			{
				*outError = PString::Format("'%s' is not an asset path (/JGGame/... or /JGEngine/...)", text);
			}
			return false;
		}
		*outValue = HDataTableValue::MakeText(text);
		return true;
	case EDataTableColumnType::RowRef:
	{
		PString reason;
		if (text.Empty() == false && IsValidDataTableKey(text, &reason) == false)
		{
			if (outError != nullptr)
			{
				*outError = PString::Format("'%s' is not a row key (%s)", text, reason);
			}
			return false;
		}
		*outValue = HDataTableValue::MakeText(text);
		return true;
	}
	default:
		break;
	}

	if (outError != nullptr)
	{
		*outError = "unknown column type";
	}
	return false;
}

PString HDataTableColumn::FormatText(const HDataTableValue& inValue) const
{
	return valueToText(inValue);
}

bool HDataTableColumn::ConvertFrom(const HDataTableValue& inValue, EDataTableColumnType inFromType, HDataTableValue* outValue) const
{
	if (outValue == nullptr)
	{
		return false;
	}

	const EDataTableValueKind targetKind = DataTableValueKindOf(Type);
	const EDataTableValueKind sourceKind = inValue.GetKind();

	// 숫자 · 참거짓끼리는 글을 거치지 않는다(1.0 → 1, true → 1)
	if (targetKind == EDataTableValueKind::Int && sourceKind == EDataTableValueKind::Float)
	{
		const float64 value = inValue.GetFloat();
		if (std::floor(value) != value || value < -9.2e18 || value > 9.2e18)
		{
			return false;
		}
		*outValue = HDataTableValue::MakeInt((int64)value);
		return true;
	}
	if ((targetKind == EDataTableValueKind::Int || targetKind == EDataTableValueKind::Float) && sourceKind == EDataTableValueKind::Bool)
	{
		*outValue = (targetKind == EDataTableValueKind::Int) ? HDataTableValue::MakeInt(inValue.GetBool() ? 1 : 0) : HDataTableValue::MakeFloat(inValue.GetBool() ? 1.0 : 0.0);
		return true;
	}
	if (targetKind == EDataTableValueKind::Float && sourceKind == EDataTableValueKind::Int)
	{
		*outValue = HDataTableValue::MakeFloat((float64)inValue.GetInt());
		return true;
	}
	if (targetKind == EDataTableValueKind::Bool && (sourceKind == EDataTableValueKind::Int || sourceKind == EDataTableValueKind::Float))
	{
		const float64 value = (sourceKind == EDataTableValueKind::Int) ? (float64)inValue.GetInt() : inValue.GetFloat();
		if (value != 0.0 && value != 1.0)
		{
			return false;
		}
		*outValue = HDataTableValue::MakeBool(value == 1.0);
		return true;
	}

	return ParseText(valueToText(inValue), outValue, nullptr);
}

bool HDataTableColumn::ReadValueJson(const PJsonData& inJson, HDataTableValue* outValue) const
{
	if (outValue == nullptr)
	{
		return false;
	}

	const EJsonValueType jsonType = inJson.GetValueType();
	switch (DataTableValueKindOf(Type))
	{
	case EDataTableValueKind::Bool:
	{
		bool value = false;
		if (jsonType != EJsonValueType::Bool || inJson.GetData(&value) == false)
		{
			return false;
		}
		*outValue = HDataTableValue::MakeBool(value);
		return true;
	}
	case EDataTableValueKind::Int:
	{
		int64 value = 0;
		if (jsonType != EJsonValueType::Int || inJson.GetData(&value) == false)
		{
			return false;
		}
		*outValue = HDataTableValue::MakeInt(value);
		return true;
	}
	case EDataTableValueKind::Float:
	{
		float64 value = 0.0;
		if (readNumber(inJson, &value) == false)
		{
			return false;
		}
		*outValue = HDataTableValue::MakeFloat(value);
		return true;
	}
	default:
	{
		PString value;
		if (jsonType != EJsonValueType::String || inJson.GetData(&value) == false)
		{
			return false;
		}
		*outValue = HDataTableValue::MakeText(value);
		return true;
	}
	}
}

void HDataTableColumn::WriteValueJson(PJsonData& json, const PString& inKey, const HDataTableValue& inValue) const
{
	switch (inValue.GetKind())
	{
	case EDataTableValueKind::Bool:
		json.AddMember(inKey, inValue.GetBool());
		break;
	case EDataTableValueKind::Int:
		json.AddMember(inKey, inValue.GetInt());
		break;
	case EDataTableValueKind::Float:
		json.AddMember(inKey, inValue.GetFloat());
		break;
	default:
		json.AddMember(inKey, inValue.GetText());
		break;
	}
}

void HDataTableColumn::WriteJson(PJsonData& json) const
{
	json.AddMember("Name", Name);
	json.AddMember("Type", PString(DataTableColumnTypeToString(Type)));
	WriteValueJson(json, "Default", Default);
	if (Description.Empty() == false)
	{
		json.AddMember("Description", Description);
	}

	// 범위는 열 타입에 맞춰 쓴다(Int 열이면 정수)
	if (bHasMin)
	{
		if (Type == EDataTableColumnType::Int)
		{
			json.AddMember("Min", (int64)Min);
		}
		else
		{
			json.AddMember("Min", Min);
		}
	}
	if (bHasMax)
	{
		if (Type == EDataTableColumnType::Int)
		{
			json.AddMember("Max", (int64)Max);
		}
		else
		{
			json.AddMember("Max", Max);
		}
	}

	// 타입에 쓰이는 옵션만 쓴다
	if (Type == EDataTableColumnType::Enum)
	{
		if (EnumValues.empty() == false)
		{
			json.AddMember("EnumValues", EnumValues);
		}
		if (EnumType.Empty() == false)
		{
			json.AddMember("EnumType", EnumType);
		}
	}
	if (Type == EDataTableColumnType::AssetRef && AssetClass.Empty() == false)
	{
		json.AddMember("AssetClass", AssetClass);
	}
	if (Type == EDataTableColumnType::RowRef && Table.Empty() == false)
	{
		json.AddMember("Table", Table);
	}
}

bool HDataTableColumn::Read(const PJsonData& json, PString* outError, HList<PString>* outWarnings)
{
	*this = HDataTableColumn();

	PJsonData nameJson;
	if (json.FindMember("Name", &nameJson) == false || nameJson.GetValueType() != EJsonValueType::String || nameJson.GetData(&Name) == false)
	{
		if (outError != nullptr)
		{
			*outError = "column without a Name string";
		}
		return false;
	}

	PString typeText;
	PJsonData typeJson;
	if (json.FindMember("Type", &typeJson) == false || typeJson.GetValueType() != EJsonValueType::String || typeJson.GetData(&typeText) == false
		|| DataTableColumnTypeFromString(typeText, &Type) == false)
	{
		if (outError != nullptr)
		{
			*outError = PString::Format("column '%s' has an unknown Type '%s'", Name, typeText);
		}
		return false;
	}

	readOptionalString(json, "Description", &Description, outWarnings);

	PJsonData minJson;
	if (json.FindMember("Min", &minJson) == true)
	{
		bHasMin = readNumber(minJson, &Min);
		if (bHasMin == false && outWarnings != nullptr)
		{
			outWarnings->push_back(PString::Format("column '%s': Min is not a number", Name));
		}
	}
	PJsonData maxJson;
	if (json.FindMember("Max", &maxJson) == true)
	{
		bHasMax = readNumber(maxJson, &Max);
		if (bHasMax == false && outWarnings != nullptr)
		{
			outWarnings->push_back(PString::Format("column '%s': Max is not a number", Name));
		}
	}

	PJsonData enumValuesJson;
	if (json.FindMember("EnumValues", &enumValuesJson) == true)
	{
		if (enumValuesJson.GetValueType() != EJsonValueType::Array || enumValuesJson.GetData(&EnumValues) == false)
		{
			EnumValues.clear();
			if (outWarnings != nullptr)
			{
				outWarnings->push_back(PString::Format("column '%s': EnumValues is not a list of strings", Name));
			}
		}
	}
	readOptionalString(json, "EnumType", &EnumType, outWarnings);
	readOptionalString(json, "AssetClass", &AssetClass, outWarnings);
	readOptionalString(json, "Table", &Table, outWarnings);

	// 기본값은 옵션(Enum 목록)을 읽은 뒤에 본다
	Default = HDataTableValue::MakeDefault(DataTableValueKindOf(Type));
	if (Type == EDataTableColumnType::Enum)
	{
		HList<PString> names;
		if (GetEnumNames(names) == true && names.empty() == false)
		{
			Default = HDataTableValue::MakeText(names[0]);
		}
	}

	PJsonData defaultJson;
	if (json.FindMember("Default", &defaultJson) == true)
	{
		HDataTableValue value;
		if (ReadValueJson(defaultJson, &value) == true)
		{
			Default = value;
		}
		else if (outWarnings != nullptr)
		{
			outWarnings->push_back(PString::Format("column '%s': Default does not match the %s type. The type default is used", Name, PString(DataTableColumnTypeToString(Type))));
		}
	}

	return true;
}

bool HDataTableColumn::operator==(const HDataTableColumn& inOther) const
{
	return DataTableSameText(Name, inOther.Name)
		&& Type == inOther.Type
		&& Default == inOther.Default
		&& DataTableSameText(Description, inOther.Description)
		&& bHasMin == inOther.bHasMin
		&& bHasMax == inOther.bHasMax
		&& Min == inOther.Min
		&& Max == inOther.Max
		&& DataTableSameTexts(EnumValues, inOther.EnumValues)
		&& DataTableSameText(EnumType, inOther.EnumType)
		&& DataTableSameText(AssetClass, inOther.AssetClass)
		&& DataTableSameText(Table, inOther.Table);
}

bool HDataTableColumn::operator!=(const HDataTableColumn& inOther) const
{
	return (*this == inOther) == false;
}

int32 HDataTableSchema::FindColumn(const PString& inName) const
{
	const int32 count = (int32)Columns.size();
	for (int32 i = 0; i < count; ++i)
	{
		if (Columns[i].Name == inName)
		{
			return i;
		}
	}
	return -1;
}
