#include "PCH/PCH.h"
#include "Texture.h"
#include "zlib/zlib.h"
#include "JGGraphics.h"
#include "JGGraphicsHelper.h"

namespace
{
	// JSON에 담는 압축 픽셀은 HList(엔진 풀)라 블록 한도인 2MB를 넘을 수 없다. 큰 텍스처 저장은 5-23에서 다룬다.
	constexpr uint64 TextureJsonPixelLimit = 2 * 1024 * 1024;
}

bool JGTexture::IsValid() const
{
	return _texture != nullptr && _texture->IsValid();
}

void JGTexture::WriteJson(PJsonData& json) const
{
	JG_SUPER::WriteJson(json);
	if (IsValid() == false)
	{
		return;
	}
	const HTextureInfo& texInfo = _texture->GetTextureInfo();

	json.AddMember("Name", GetName());
	json.AddMember("Width", texInfo.Width);
	json.AddMember("Height", texInfo.Height);
	json.AddMember("PixelPerUnit", texInfo.PixelPerUnit);
	json.AddMember("Format", StaticEnum<ETextureFormat>()->GetEnumNameByValue((int32)texInfo.Format));
	json.AddMember("FilterMode", StaticEnum<ETextureFilterMode>()->GetEnumNameByValue((int32)texInfo.FilterMode));
	json.AddMember("WrapMode", StaticEnum<ETextureWrapMode>()->GetEnumNameByValue((int32)texInfo.WrapMode));
	json.AddMember("Flags", (int32)texInfo.Flags);
	json.AddMember("MipLevel", texInfo.MipLevel);
	json.AddMember("ArraySize", texInfo.ArraySize);
	json.AddMember("ClearColor", texInfo.ClearColor);
	json.AddMember("ClearDepth", texInfo.ClearDepth);
	json.AddMember("ClearStencil", texInfo.ClearStencil);

	// 픽셀은 GPU에서 읽어 온다. 저장은 도구 경로라 동기 리드백(ReadbackTextureImmediate)을 쓴다.
	// (DEFAULT 힙 텍스처는 Map할 수 없다. 이전 코드는 Map 실패로 널 포인터를 압축해 깨진 데이터를 저장했다. 5-1)
	HList<uint8> compressedPixels;
	HTexturePixels pixels;
	if (GetGraphicsAPI().ReadbackTextureImmediate(_texture, pixels) && pixels.IsValid())
	{
		// 압축 작업 버퍼는 std 할당자. 엔진 풀은 블록 하나가 최대 2MB라 큰 텍스처를 HList에 담을 수 없다. (5-23)
		uLongf compressedSize = compressBound((uLong)pixels.Data.size());
		std::vector<uint8> compressBuffer(compressedSize);

		const int32 result = compress((Bytef*)compressBuffer.data(), &compressedSize, (const Bytef*)pixels.Data.data(), (uLong)pixels.Data.size());
		if (result != Z_OK)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : Fail compress pixels (zlib %d). Pixels are not written", texInfo.Name, result);
		}
		else if (compressedSize > TextureJsonPixelLimit)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : Compressed pixels(%d bytes) exceed the JSON pixel limit(%d bytes). Pixels are not written", texInfo.Name, (int32)compressedSize, (int32)TextureJsonPixelLimit);
		}
		else
		{
			compressedPixels.assign(compressBuffer.begin(), compressBuffer.begin() + compressedSize);
		}
	}
	else
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail read pixels from GPU. Pixels are not written", texInfo.Name);
	}

	json.AddMember("Pixels", compressedPixels);
}

void JGTexture::ReadJson(const PJsonData& json)
{
	JG_SUPER::ReadJson(json);

	HTextureInfo texInfo;
	if (json.GetData("Name", &texInfo.Name) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	if (json.GetData("Width", &texInfo.Width) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	if (json.GetData("Height", &texInfo.Height) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	if (json.GetData("PixelPerUnit", &texInfo.PixelPerUnit) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	PName enumName;
	if (json.GetData("Format", &enumName) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	texInfo.Format = static_cast<ETextureFormat>(StaticEnum<ETextureFormat>()->GetValueByEnumName(enumName));

	if (json.GetData("FilterMode", &enumName) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	texInfo.FilterMode = static_cast<ETextureFilterMode>(StaticEnum<ETextureFilterMode>()->GetValueByEnumName(enumName));

	if (json.GetData("WrapMode", &enumName) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	texInfo.WrapMode = static_cast<ETextureWrapMode>(StaticEnum<ETextureWrapMode>()->GetValueByEnumName(enumName));

	int32 Flags = 0;
	if (json.GetData("Flags", &Flags) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}
	texInfo.Flags = (ETextureFlags)Flags;

	if (json.GetData("MipLevel", &texInfo.MipLevel) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	if (json.GetData("ArraySize", &texInfo.ArraySize) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	if (json.GetData("ClearColor", &texInfo.ClearColor) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}
	if (json.GetData("ClearDepth", &texInfo.ClearDepth) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}
	if (json.GetData("ClearStencil", &texInfo.ClearStencil) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	HList<uint8> compressedPixels;
	if (json.GetData("Pixels", &compressedPixels) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Fail Read Json in Texture");
	}

	// 픽셀 크기는 포맷에서 바로 구한다. (이전에는 채널 수를 바이트 수로 써서 16비트 포맷의 크기가 틀렸다)
	const uint64 pixelDataSize = (uint64)texInfo.Width * texInfo.Height * HJGGraphicsHelper::GetTextureFormatBytesPerPixel(texInfo.Format);
	if (compressedPixels.empty() || pixelDataSize == 0)
	{
		JG_LOG(Graphics, ELogLevel::Warning, "%s : Texture asset has no pixel data. an empty texture is created", texInfo.Name);
		_texture = GetGraphicsAPI().CreateRawTexture(texInfo);
		return;
	}

	std::vector<uint8> pixels(pixelDataSize);   // 풀 블록 한도(2MB) 때문에 std 할당자

	uLongf destLength   = (uLongf)pixelDataSize;
	uLong  sourceLength = (uLong)compressedPixels.size();
	const int32 result = uncompress2((Bytef*)pixels.data(), &destLength, (const Bytef*)compressedPixels.data(), &sourceLength);
	if (result != Z_OK || destLength != pixelDataSize)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail uncompress pixels (zlib %d, %d / %d bytes)", texInfo.Name, result, (int32)destLength, (int32)pixelDataSize);
		_texture = GetGraphicsAPI().CreateRawTexture(texInfo);
		return;
	}

	_texture = GetGraphicsAPI().CreateRawTexture(pixels.data(), texInfo);
}
