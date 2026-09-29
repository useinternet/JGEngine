#pragma once
#include "GameMaster/State/GameplayEntityId.h"

// 엔티티 생성 · 파괴 · 유효성. 인덱스는 재사용하고 세대는 파괴마다 증가한다.
// 순회는 항상 인덱스 오름차순이라 결정론적이다.
struct GAMEFRAMEWORKS_API HGameplayEntityRegistry : public IJsonable
{
	HList<uint32> Generations;   // 인덱스별 현재 세대. 0 = 아직 만들어진 적 없음
	HList<uint8>  Alive;         // 인덱스별 생존 여부
	HList<uint32> FreeIndices;   // 재사용 대기 인덱스 (스택. 마지막이 다음 재사용)
	uint32        AliveCount = 0;

	HGameplayEntityId Create();
	bool   Destroy(const HGameplayEntityId& id);
	bool   IsAlive(const HGameplayEntityId& id) const;
	uint32 Capacity() const;
	uint32 Count() const;

	// 살아 있는 엔티티를 인덱스 오름차순으로 순회한다.
	template<class Fn>
	void Each(Fn fn) const
	{
		uint32 capacity = (uint32)Alive.size();
		for (uint32 i = 0; i < capacity; ++i)
		{
			if (Alive[i] != 0)
			{
				fn(HGameplayEntityId(i, Generations[i]));
			}
		}
	}

	void CollectAlive(HList<HGameplayEntityId>& outIds) const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
