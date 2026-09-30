#pragma once
#include "GameMaster/State/GameplayEntityId.h"

// 보드 상태 (값 타입). 규칙 코드는 이 구조체를 직접 만지지 않고 IGameplayBoard 질의를 쓴다.
// 격자 종류(Square · Hex)는 셀 배열을 갖고, Free 는 좌표만 갖는다. None 은 아무것도 없다.
struct GAMEFRAMEWORKS_API HGameplayBoardState : public IJsonable
{
	EGameplayBoardKind Kind   = EGameplayBoardKind::None;
	int32                Width  = 0;
	int32                Height = 0;

	// 격자 셀 (Width * Height). 행 우선.
	HList<HGameplayEntityId> Occupants;
	HList<uint8>               Blocked;

	// 엔티티 인덱스별 좌표. PositionGenerations 가 0 이면 좌표 없음.
	HList<HGameplayCoord> Positions;
	HList<uint32>           PositionGenerations;

	void Reset(EGameplayBoardKind kind, int32 width, int32 height);

	bool  IsGrid() const;
	bool  InBounds(const HGameplayCoord& coord) const;
	int32 CellIndex(const HGameplayCoord& coord) const;

	bool                    HasPosition(const HGameplayEntityId& id) const;
	const HGameplayCoord* FindPosition(const HGameplayEntityId& id) const;
	// 엔티티 생존을 모른다. 규칙 코드는 HGameplayState::SetBoardPosition(죽은 ID 거부)을 쓴다.
	bool                    SetPosition(const HGameplayEntityId& id, const HGameplayCoord& coord);
	bool                    ClearPosition(const HGameplayEntityId& id);

	HGameplayEntityId OccupantAt(const HGameplayCoord& coord) const;
	bool                IsBlocked(const HGameplayCoord& coord) const;
	void                SetBlocked(const HGameplayCoord& coord, bool bBlocked);

	// 좌표를 가진 엔티티를 인덱스 오름차순으로 순회. fn(const HGameplayEntityId&, const HGameplayCoord&)
	template<class Fn>
	void EachPositioned(Fn fn) const
	{
		uint32 capacity = (uint32)PositionGenerations.size();
		for (uint32 i = 0; i < capacity; ++i)
		{
			if (PositionGenerations[i] != 0)
			{
				fn(HGameplayEntityId(i, PositionGenerations[i]), Positions[i]);
			}
		}
	}

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;

private:
	void ensurePositionCapacity(uint32 capacity);
};
