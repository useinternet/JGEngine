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
	// 인덱스별 다음에 낼 세대의 바닥. 되감기(ER-027)만 채운다 — 되감기 전에 낸 번호를 다시 내지 않게. 비면 쓰지 않는다 (JSON 에도 안 씀)
	HList<uint32> GenerationFloors;

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

	// 되감은 뒤(이 등록부 = 지점 때 것) 부른다. before(되감기 직전 등록부)가 낸 세대를 다시 내지 않게 바닥을 올리고,
	// 지점 뒤에 생긴 인덱스는 빈 칸으로 둔다. 지점 때 살아 있던 엔티티는 원래 번호 그대로다.
	void KeepIssuedGenerations(const HGameplayEntityRegistry& before);

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
