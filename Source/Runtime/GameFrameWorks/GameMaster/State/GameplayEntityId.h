#pragma once
#include "GameMaster/GameMasterDefines.h"

// 엔티티 참조. 포인터가 아니라 인덱스 + 세대. 죽은 참조는 세대 불일치로 O(1) 감지한다.
// Generation 0 은 "없음" 이다. 살아 있는 엔티티의 세대는 항상 1 이상.
struct GAMEFRAMEWORKS_API HGameplayEntityId : public IJsonable
{
	uint32 Index      = 0;
	uint32 Generation = 0;

	HGameplayEntityId() = default;
	HGameplayEntityId(uint32 inIndex, uint32 inGeneration);

	static HGameplayEntityId None();

	bool   IsValid() const;
	uint64 ToKey() const;
	PString ToString() const;

	bool operator==(const HGameplayEntityId& rhs) const;
	bool operator!=(const HGameplayEntityId& rhs) const;
	bool operator<(const HGameplayEntityId& rhs) const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

template<>
struct std::hash<HGameplayEntityId>
{
	size_t operator()(const HGameplayEntityId& id) const noexcept
	{
		return std::hash<uint64>()(id.ToKey());
	}
};

// 보드 좌표. 정수만 쓴다 (결정론).
struct GAMEFRAMEWORKS_API HGameplayCoord : public IJsonable
{
	int32 X = 0;
	int32 Y = 0;

	HGameplayCoord() = default;
	HGameplayCoord(int32 inX, int32 inY);

	bool operator==(const HGameplayCoord& rhs) const;
	bool operator!=(const HGameplayCoord& rhs) const;
	bool operator<(const HGameplayCoord& rhs) const;

	PString ToString() const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
