#include "PCH/PCH.h"
#include "Network/Messages/GameplayNetMessage.h"
#include "zlib/zlib.h"

namespace
{
	constexpr uint8  FlagCompressed     = 0x01;
	constexpr uint32 MaxDecodedTextBytes = 256u * 1024u * 1024u;

	void writeUint32(HList<uint8>& outBytes, uint32 value)
	{
		outBytes.push_back((uint8)(value & 0xff));
		outBytes.push_back((uint8)((value >> 8) & 0xff));
		outBytes.push_back((uint8)((value >> 16) & 0xff));
		outBytes.push_back((uint8)((value >> 24) & 0xff));
	}

	uint32 readUint32(const uint8* bytes)
	{
		return (uint32)bytes[0] | ((uint32)bytes[1] << 8) | ((uint32)bytes[2] << 16) | ((uint32)bytes[3] << 24);
	}
}

// ---- 메시지 ------------------------------------------------------------------

void HGameplayNetHello::WriteJson(PJsonData& json) const
{
	json.AddMember("Protocol", Protocol);
	json.AddMember("Schema", Schema);
	json.AddMember("Name", Name);
	json.AddMember("Token", Token);
}

void HGameplayNetHello::ReadJson(const PJsonData& json)
{
	json.GetData("Protocol", &Protocol);
	json.GetData("Schema", &Schema);
	json.GetData("Name", &Name);
	json.GetData("Token", &Token);
}

void HGameplayNetWelcome::WriteJson(PJsonData& json) const
{
	json.AddMember("Slot", Slot);
	json.AddMember("Fingerprint", Fingerprint);
	json.AddMember("World", World);
	json.AddMember("Started", bStarted);
	json.AddMember("Slots", Slots);
}

void HGameplayNetWelcome::ReadJson(const PJsonData& json)
{
	Slots.clear();

	json.GetData("Slot", &Slot);
	json.GetData("Fingerprint", &Fingerprint);
	json.GetData("World", &World);
	json.GetData("Started", &bStarted);
	json.GetData("Slots", &Slots);
}

void HGameplayNetRefuse::WriteJson(PJsonData& json) const
{
	json.AddMember("Reason", Reason);
}

void HGameplayNetRefuse::ReadJson(const PJsonData& json)
{
	json.GetData("Reason", &Reason);
}

void HGameplayNetSlots::WriteJson(PJsonData& json) const
{
	json.AddMember("Slots", Slots);
}

void HGameplayNetSlots::ReadJson(const PJsonData& json)
{
	Slots.clear();
	json.GetData("Slots", &Slots);
}

void HGameplayNetStartGame::WriteJson(PJsonData& json) const
{
	json.AddMember("Seed", Seed);
	json.AddMember("Fingerprint", Fingerprint);
	json.AddMember("InitialState", InitialState);
}

void HGameplayNetStartGame::ReadJson(const PJsonData& json)
{
	json.GetData("Seed", &Seed);
	json.GetData("Fingerprint", &Fingerprint);
	json.GetData("InitialState", &InitialState);
}

void HGameplayNetCommandRequest::WriteJson(PJsonData& json) const
{
	json.AddMember("ClientSeq", ClientSeq);
	json.AddMember("Command", Command);
}

void HGameplayNetCommandRequest::ReadJson(const PJsonData& json)
{
	json.GetData("ClientSeq", &ClientSeq);
	json.GetData("Command", &Command);
}

void HGameplayNetCommandAccepted::WriteJson(PJsonData& json) const
{
	json.AddMember("Seq", Seq);
	json.AddMember("Slot", Slot);
	json.AddMember("ClientSeq", ClientSeq);
	json.AddMember("Checksum", Checksum);
	json.AddMember("Command", Command);
}

void HGameplayNetCommandAccepted::ReadJson(const PJsonData& json)
{
	json.GetData("Seq", &Seq);
	json.GetData("Slot", &Slot);
	json.GetData("ClientSeq", &ClientSeq);
	json.GetData("Checksum", &Checksum);
	json.GetData("Command", &Command);
}

void HGameplayNetCommandRejected::WriteJson(PJsonData& json) const
{
	json.AddMember("ClientSeq", ClientSeq);
	json.AddMember("Reason", Reason);
}

void HGameplayNetCommandRejected::ReadJson(const PJsonData& json)
{
	json.GetData("ClientSeq", &ClientSeq);
	json.GetData("Reason", &Reason);
}

void HGameplayNetResyncRequest::WriteJson(PJsonData& json) const
{
	json.AddMember("Seq", Seq);
}

void HGameplayNetResyncRequest::ReadJson(const PJsonData& json)
{
	json.GetData("Seq", &Seq);
}

void HGameplayNetTravel::WriteJson(PJsonData& json) const
{
	json.AddMember("World", World);
}

void HGameplayNetTravel::ReadJson(const PJsonData& json)
{
	json.GetData("World", &World);
}

// ---- 코덱 --------------------------------------------------------------------

void HGameplayNetCodec::Encode(EGameplayNetMessage type, const HRawString& text, bool bCompress, HList<uint8>& outBytes)
{
	outBytes.clear();
	outBytes.push_back((uint8)type);

	if (bCompress == true && text.empty() == false)
	{
		uLong sourceLength = (uLong)text.size();
		uLong bound        = compressBound(sourceLength);
		HList<uint8> compressed(bound);
		uLongf compressedLength = bound;

		int32 result = compress2((Bytef*)compressed.data(), &compressedLength, (const Bytef*)text.data(), sourceLength, Z_DEFAULT_COMPRESSION);
		if (result == Z_OK)
		{
			outBytes.push_back(FlagCompressed);
			writeUint32(outBytes, (uint32)text.size());
			outBytes.insert(outBytes.end(), compressed.begin(), compressed.begin() + (int64)compressedLength);
			return;
		}
		JG_LOG(Network, ELogLevel::Warning, "HGameplayNetCodec: compress failed (%d), sending uncompressed", result);
	}

	outBytes.push_back(0);
	outBytes.insert(outBytes.end(), (const uint8*)text.data(), (const uint8*)text.data() + text.size());
}

bool HGameplayNetCodec::Decode(const HList<uint8>& bytes, EGameplayNetMessage* outType, HRawString* outText)
{
	if (outType == nullptr || outText == nullptr || bytes.size() < 2)
	{
		return false;
	}

	uint8 type = bytes[0];
	if (type == (uint8)EGameplayNetMessage::None || type >= (uint8)EGameplayNetMessage::Count)
	{
		return false;
	}
	*outType = (EGameplayNetMessage)type;

	uint8 flags = bytes[1];
	if ((flags & FlagCompressed) == 0)
	{
		outText->assign((const char*)bytes.data() + 2, bytes.size() - 2);
		return true;
	}

	if (bytes.size() < 6)
	{
		return false;
	}
	uint32 originalLength = readUint32(bytes.data() + 2);
	if (originalLength > MaxDecodedTextBytes)
	{
		return false;
	}

	outText->resize(originalLength);
	uLongf decodedLength = (uLongf)originalLength;
	int32 result = uncompress((Bytef*)&(*outText)[0], &decodedLength, (const Bytef*)bytes.data() + 6, (uLong)(bytes.size() - 6));
	if (result != Z_OK || decodedLength != (uLongf)originalLength)
	{
		JG_LOG(Network, ELogLevel::Error, "HGameplayNetCodec: uncompress failed (%d)", result);
		outText->clear();
		return false;
	}
	return true;
}

const char* HGameplayNetCodec::ToString(EGameplayNetMessage type)
{
	switch (type)
	{
	case EGameplayNetMessage::Hello:           return "Hello";
	case EGameplayNetMessage::Welcome:         return "Welcome";
	case EGameplayNetMessage::Refuse:          return "Refuse";
	case EGameplayNetMessage::SlotChanged:     return "SlotChanged";
	case EGameplayNetMessage::StartGame:       return "StartGame";
	case EGameplayNetMessage::CommandRequest:  return "CommandRequest";
	case EGameplayNetMessage::CommandAccepted: return "CommandAccepted";
	case EGameplayNetMessage::CommandRejected: return "CommandRejected";
	case EGameplayNetMessage::ResyncRequest:   return "ResyncRequest";
	case EGameplayNetMessage::Document:        return "Document";
	case EGameplayNetMessage::Travel:          return "Travel";
	case EGameplayNetMessage::Ping:            return "Ping";
	default:                                   return "None";
	}
}
