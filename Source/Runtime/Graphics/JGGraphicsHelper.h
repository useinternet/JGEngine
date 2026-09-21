#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class HJGGraphicsHelper
{
public:
	static uint64 GetShaderDataTypeSize(EShaderDataType dataType);
	static PName  GetShaderDataTypeHLSLName(EShaderDataType dataType);
	static uint8  GetTextureFormatChannels(ETextureFormat format);
	// 픽셀 하나의 바이트 수. 채널 수(GetTextureFormatChannels)와 다르다. R16G16B16A16_Float은 채널 4개에 8바이트.
	// ETextureFormat 열거자의 JGENUMMETA(BytesPerPixel = N)를 읽는다. 메타가 없으면 오류 로그 후 0.
	static uint32 GetTextureFormatBytesPerPixel(ETextureFormat format);
	static bool   IsDepthStencilFormat(ETextureFormat format);
};