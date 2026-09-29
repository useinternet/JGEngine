#pragma once
#include "GameMaster/GameMasterDefines.h"

// 결정론 난수 스트림 (PCG32). 시드 · 증분 · 호출 횟수가 곧 상태라 스냅샷과 함께 복사되고 직렬화된다.
// std::mt19937 + std::uniform_int_distribution 은 STL 구현마다 분포 알고리즘이 달라 플랫폼 간 결정론이 깨지므로 쓰지 않는다.
struct GAMEFRAMEWORKS_API HGameplayRandomStream : public IJsonable
{
	uint64 State     = 0;
	uint64 Increment = 0;
	uint64 CallCount = 0;

	// sequence 는 스트림 구분값. 같은 seed 라도 sequence 가 다르면 다른 수열이 나온다.
	void Seed(uint64 seed, uint64 sequence);

	uint32 Next();
	// [0, bound) 의 정수. bound 가 0 이면 0.
	uint32 Below(uint32 bound);
	// [minInclusive, maxInclusive] 의 정수.
	int32  Range(int32 minInclusive, int32 maxInclusive);

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
