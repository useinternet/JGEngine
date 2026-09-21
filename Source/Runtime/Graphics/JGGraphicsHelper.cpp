#include "PCH/PCH.h"
#include "JGGraphicsHelper.h"

namespace
{
	// 열거자 메타(JGENUMMETA)에서 값 하나를 읽는다. 열거형·열거자·키가 없거나 값이 비어 있으면 false.
	// (이전 헬퍼들은 비어 있는 집합의 begin()을 역참조해 키가 없을 때 정의되지 않은 동작이었다)
	template<class TEnum>
	bool readEnumMeta(TEnum inValue, const PString& inKey, PName& outValue)
	{
		PSharedPtr<JGEnum> Enum = StaticEnum<TEnum>();
		if (Enum == nullptr)
		{
			return false;
		}

		PSharedPtr<JGMeta> Meta = Enum->GetMetaDataByValue((int32)inValue);
		if (Meta == nullptr)
		{
			return false;
		}

		HHashSet<PName> Values;
		if (Meta->GetMetaValues(PName(inKey), Values) == false || Values.empty())
		{
			return false;
		}

		outValue = *Values.begin();
		return true;
	}

	template<class TEnum>
	int32 readEnumMetaInt(TEnum inValue, const PString& inKey)
	{
		PSharedPtr<JGEnum> Enum = StaticEnum<TEnum>();
		if (Enum == nullptr)
		{
			return false;
		}

		PName value;
		if (readEnumMeta(inValue, inKey, value) == false)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : enumerator %d has no '%s' meta. check JGENUMMETA and re-run JGHeaderTool", Enum->GetName(), static_cast<int32>(inValue), PString(inKey));
			return 0;
		}

		return value.ToString().ToInt();
	}
}

uint64 HJGGraphicsHelper::GetShaderDataTypeSize(EShaderDataType dataType)
{
	return static_cast<uint64>(readEnumMetaInt(dataType, "DataSize"));
}

PName HJGGraphicsHelper::GetShaderDataTypeHLSLName(EShaderDataType dataType)
{
	PName value;
	if (readEnumMeta(dataType, "HLSLName", value) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "EShaderDataType : enumerator %d has no 'HLSLName' meta. check JGENUMMETA and re-run JGHeaderTool", (int32)dataType);
		return NAME_NONE;
	}

	return value;
}

uint8 HJGGraphicsHelper::GetTextureFormatChannels(ETextureFormat format)
{
	return static_cast<uint8>(readEnumMetaInt(format, "Channels"));
}

uint32 HJGGraphicsHelper::GetTextureFormatBytesPerPixel(ETextureFormat format)
{
	return static_cast<uint32>(readEnumMetaInt(format, "BytesPerPixel"));
}

bool HJGGraphicsHelper::IsDepthStencilFormat(ETextureFormat format)
{
	return format == ETextureFormat::D24_Unorm_S8_Uint || format == ETextureFormat::R24G8_TYPELESS;
}
