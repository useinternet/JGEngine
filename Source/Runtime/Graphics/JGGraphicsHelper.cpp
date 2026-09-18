#include "PCH/PCH.h"
#include "JGGraphicsHelper.h"

uint64 HJGGraphicsHelper::GetShaderDataTypeSize(EShaderDataType dataType)
{
	PSharedPtr<JGEnum> Enum = StaticEnum<EShaderDataType>();
	if (Enum == nullptr)
	{
		return 0;
	}

	PSharedPtr<JGMeta> Meta = Enum->GetMetaDataByIndex((int32)dataType);
	if (Meta == nullptr)
	{
		return 0;
	}

	HHashSet<PName> Values;
	Meta->GetMetaValues(PName("DataSize"), Values);

	PString sizeStr = (*Values.begin()).ToString();
	int32 sizeInt   = sizeStr.ToInt();

	return (uint64)sizeInt;
}

PName HJGGraphicsHelper::GetShaderDataTypeHLSLName(EShaderDataType dataType)
{
	PSharedPtr<JGEnum> Enum = StaticEnum<EShaderDataType>();
	if (Enum == nullptr)
	{
		return NAME_NONE;
	}

	PSharedPtr<JGMeta> Meta = Enum->GetMetaDataByIndex((int32)dataType);
	if (Meta == nullptr)
	{
		return NAME_NONE;
	}

	HHashSet<PName> Values;
	Meta->GetMetaValues(PName("HLSLName"), Values);

	return (*Values.begin());
}

uint8  HJGGraphicsHelper::GetTextureFormatChannels(ETextureFormat format)
{
	PSharedPtr<JGEnum> Enum = StaticEnum<ETextureFormat>();
	if (Enum == nullptr)
	{
		return 0;
	}

	PSharedPtr<JGMeta> Meta = Enum->GetMetaDataByIndex((int32)format);
	if (Meta == nullptr)
	{
		return 0;
	}

	HHashSet<PName> Values;
	Meta->GetMetaValues(PName("Channels"), Values);

	PString sizeStr = (*Values.begin()).ToString();
	int32   sizeInt = sizeStr.ToInt();

	return (uint8)sizeInt;
}

uint32 HJGGraphicsHelper::GetTextureFormatPixelSize(ETextureFormat format)
{
	switch (format)
	{
	case ETextureFormat::R8_Unorm:
	case ETextureFormat::R8_Uint:             return 1;
	case ETextureFormat::R16_Float:
	case ETextureFormat::R16_Uint:            return 2;
	case ETextureFormat::R32_Float:
	case ETextureFormat::R32_Uint:
	case ETextureFormat::R16G16_Float:
	case ETextureFormat::R8G8B8A8_Unorm:
	case ETextureFormat::R11G11B10_Float:
	case ETextureFormat::R24G8_TYPELESS:
	case ETextureFormat::D24_Unorm_S8_Uint:   return 4;
	case ETextureFormat::R16G16B16A16_Unorm:
	case ETextureFormat::R16G16B16A16_Float:
	case ETextureFormat::R16G16B16A16_Uint:   return 8;
	case ETextureFormat::R32G32B32A32_Float:  return 16;
	case ETextureFormat::None:
	default:                                  return 0;
	}
}

bool HJGGraphicsHelper::IsDepthStencilFormat(ETextureFormat format)
{
	return format == ETextureFormat::D24_Unorm_S8_Uint || format == ETextureFormat::R24G8_TYPELESS;
}
