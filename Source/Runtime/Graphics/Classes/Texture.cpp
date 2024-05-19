#include "PCH/PCH.h"
#include "Texture.h"
#include "zlib/zlib.h"
#include "JGGraphics.h"

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

	HList<uint8> pixels;
	_texture->AccessPixels(HOnAccessTexturePixels::CreateLambda([&](const void* inPixels)
		{
			int32 channel = 4;
			PSharedPtr<JGEnum> formatEnum = StaticEnum<ETextureFormat>();
			PName channelStr = formatEnum->GetMetaDataByValue((int32)texInfo.Format)->GetMetaValue(PName("Channels"));
			if (channelStr != NAME_NONE)
			{
				channel = channelStr.ToString().ToInt(channel);
			}

			uint64 pixelSize = texInfo.Width * texInfo.Height * channel;
			uint64 compressedPixelSize = pixelSize;
			pixels.resize(pixelSize);
			int32 result = compress((Bytef*)(pixels.data()), (uLongf*)(&compressedPixelSize), (const Bytef*)(inPixels), (uLong)pixelSize);
			JG_CHECK(result == S_OK);
			pixels.resize(compressedPixelSize);
		}));
	
	json.AddMember("Pixels", pixels);
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
	
	int32 channel = 4;
	PSharedPtr<JGEnum> formatEnum = StaticEnum<ETextureFormat>();
	PName channelStr = formatEnum->GetMetaDataByValue((int32)texInfo.Format)->GetMetaValue(PName("Channels"));
	if (channelStr != NAME_NONE)
	{
		channel = channelStr.ToString().ToInt(channel);
	}

	uint64 compressedPixelSize = compressedPixels.size();
	uint64 pixelSize = texInfo.Width * texInfo.Height * channel;


	HList<uint8> pixels;
	pixels.resize(pixelSize);

	int32 result = uncompress2((Bytef*)pixels.data(), (uLongf*)(&pixelSize), (const Bytef*)compressedPixels.data(), (uLong*)(&compressedPixelSize));
	JG_CHECK(result == S_OK);

	_texture = GetGraphicsAPI().CreateRawTexture(pixels.data(), texInfo);
}
