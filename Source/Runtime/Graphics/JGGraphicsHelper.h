#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class HJGGraphicsHelper
{
public:
	static uint64 GetShaderDataTypeSize(EShaderDataType dataType);
	static PName  GetShaderDataTypeHLSLName(EShaderDataType dataType);
	static uint8  GetTextureFormatChannels(ETextureFormat format);
	// 픽셀 하나의 바이트 수. 포맷별 고정값이라 리플렉션 메타(Channels)에 의존하지 않는다. 알 수 없는 포맷은 0.
	static uint32 GetTextureFormatPixelSize(ETextureFormat format);
	static bool   IsDepthStencilFormat(ETextureFormat format);
};