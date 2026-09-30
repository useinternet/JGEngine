#pragma once
#include "GameMaster/Messages/GameplayCommand.h"
#include "Network/Messages/GameplayPlayerSlot.h"

namespace HGameplayNetProtocol
{
	// 메시지 형식이 바뀌면 올린다. 핸드셰이크에서 다르면 입장을 거부한다.
	constexpr uint32 Version = 1;
}

enum class EGameplayNetMessage : uint8
{
	None = 0,
	Hello,             // 클라 → 호스트: 프로토콜 · 스키마 · 이름 · 재접속 토큰
	Welcome,           // 호스트 → 클라: 슬롯 · 규칙 지문 · 월드 · 진행 여부 · 슬롯 표
	Refuse,            // 호스트 → 클라: 입장 거부 사유
	SlotChanged,       // 호스트 → 전원: 슬롯 표
	StartGame,         // 호스트 → 전원: 시드 · 규칙 지문 · 초기 상태
	CommandRequest,    // 클라 → 호스트: 로컬 명령
	CommandAccepted,   // 호스트 → 전원: 순번 · 보낸 슬롯 · 명령 · 체크섬
	CommandRejected,   // 호스트 → 보낸 클라: 거절 사유
	ResyncRequest,     // 클라 → 호스트: 불일치. 문서 요청
	Document,          // 호스트 → 클라: 저장 문서(초기 + 현재 + 로그). 압축
	Travel,            // 호스트 → 전원: 월드 변경
	Ping,              // 양방향: 생존 확인
	Count,
};

struct GAMEFRAMEWORKS_API HGameplayNetHello : public IJsonable
{
	uint32  Protocol = 0;
	uint32  Schema   = 0;
	PString Name;
	PString Token;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetWelcome : public IJsonable
{
	int32                      Slot        = INDEX_NONE;
	uint64                     Fingerprint = 0;      // 호스트에 GameMaster 가 붙어 있지 않으면 0
	PName                      World;
	bool                       bStarted    = false;  // 진행 중이면 곧 Document 가 온다
	HList<HGameplayPlayerSlot> Slots;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetRefuse : public IJsonable
{
	PString Reason;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetSlots : public IJsonable
{
	HList<HGameplayPlayerSlot> Slots;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetStartGame : public IJsonable
{
	uint64  Seed        = 0;
	uint64  Fingerprint = 0;
	PString InitialState;   // HGameplayState::ToJsonString

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetCommandRequest : public IJsonable
{
	uint32           ClientSeq = 0;
	HGameplayCommand Command;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetCommandAccepted : public IJsonable
{
	uint32           Seq       = 0;            // 실행 뒤 상태의 Sequence
	int32            Slot      = INDEX_NONE;   // 보낸 슬롯. 호스트 AI 는 INDEX_NONE
	uint32           ClientSeq = 0;            // 보낸 클라의 번호. 그 클라가 자기 명령의 결과를 찾는다
	uint64           Checksum  = 0;            // 실행 뒤 상태의 체크섬
	HGameplayCommand Command;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetCommandRejected : public IJsonable
{
	uint32  ClientSeq = 0;
	PString Reason;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetResyncRequest : public IJsonable
{
	uint32 Seq = 0;   // 클라가 마지막으로 맞게 적용한 순번

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

struct GAMEFRAMEWORKS_API HGameplayNetTravel : public IJsonable
{
	PName World;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

// 메시지 바이트 = [종류 u8][플래그 u8][본문]. 플래그 1 이면 zlib: 본문 = [원래 크기 u32][압축 바이트].
// 본문은 JSON 텍스트다 (Document 만 저장 문서 텍스트 그대로). Ping 은 본문이 없다.
class GAMEFRAMEWORKS_API HGameplayNetCodec
{
public:
	static void Encode(EGameplayNetMessage type, const HRawString& text, bool bCompress, HList<uint8>& outBytes);
	static bool Decode(const HList<uint8>& bytes, EGameplayNetMessage* outType, HRawString* outText);

	template<class T>
	static void EncodeMessage(EGameplayNetMessage type, const T& message, HList<uint8>& outBytes)
	{
		PJson json;
		json.AddMember("M", message);
		PString text;
		PJson::ToString(json, &text);
		Encode(type, text.GetRawString(), false, outBytes);
	}

	template<class T>
	static bool DecodeMessage(const HRawString& text, T* outMessage)
	{
		PJson json;
		if (PJson::ToObject(PString(text.c_str()), &json) == false)
		{
			return false;
		}
		return json.GetData("M", outMessage);
	}

	static const char* ToString(EGameplayNetMessage type);
};
