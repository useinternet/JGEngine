#include "PCH/PCH.h"
#include "GameMaster/State/GameplayRandomStream.h"

namespace
{
	constexpr uint64 PCG32Multiplier = 6364136223846793005ULL;
}

void HGameplayRandomStream::Seed(uint64 seed, uint64 sequence)
{
	State     = 0;
	Increment = (sequence << 1u) | 1u;
	CallCount = 0;

	Next();
	State += seed;
	Next();

	// 시드 절차의 두 호출은 세지 않는다. CallCount 는 시드 이후의 소비 횟수다.
	CallCount = 0;
}

uint32 HGameplayRandomStream::Next()
{
	uint64 oldState = State;
	State = oldState * PCG32Multiplier + Increment;

	uint32 xorShifted = (uint32)(((oldState >> 18u) ^ oldState) >> 27u);
	uint32 rot        = (uint32)(oldState >> 59u);

	++CallCount;
	return (xorShifted >> rot) | (xorShifted << ((-(int32)rot) & 31));
}

uint32 HGameplayRandomStream::Below(uint32 bound)
{
	if (bound == 0)
	{
		return 0;
	}

	// 편향 제거 (Lemire 방식과 같은 거부 샘플링). 결정론에는 영향이 없다.
	uint32 threshold = (uint32)(-(int32)bound) % bound;
	while (true)
	{
		uint32 r = Next();
		if (r >= threshold)
		{
			return r % bound;
		}
	}
}

int32 HGameplayRandomStream::Range(int32 minInclusive, int32 maxInclusive)
{
	if (maxInclusive <= minInclusive)
	{
		return minInclusive;
	}

	uint32 span = (uint32)((int64)maxInclusive - (int64)minInclusive + 1);
	return minInclusive + (int32)Below(span);
}

void HGameplayRandomStream::WriteJson(PJsonData& json) const
{
	json.AddMember("State", State);
	json.AddMember("Increment", Increment);
	json.AddMember("CallCount", CallCount);
}

void HGameplayRandomStream::ReadJson(const PJsonData& json)
{
	json.GetData("State", &State);
	json.GetData("Increment", &Increment);
	json.GetData("CallCount", &CallCount);
}
