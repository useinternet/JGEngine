#include "PCH/PCH.h"
#include "GameMaster/State/GameplayEntityId.h"

HGameplayEntityId::HGameplayEntityId(uint32 inIndex, uint32 inGeneration)
	: Index(inIndex)
	, Generation(inGeneration)
{
}

HGameplayEntityId HGameplayEntityId::None()
{
	return HGameplayEntityId();
}

bool HGameplayEntityId::IsValid() const
{
	return Generation != 0;
}

uint64 HGameplayEntityId::ToKey() const
{
	return ((uint64)Generation << 32) | (uint64)Index;
}

PString HGameplayEntityId::ToString() const
{
	return PString::Format("E%u.%u", Index, Generation);
}

bool HGameplayEntityId::operator==(const HGameplayEntityId& rhs) const
{
	return Index == rhs.Index && Generation == rhs.Generation;
}

bool HGameplayEntityId::operator!=(const HGameplayEntityId& rhs) const
{
	return !(*this == rhs);
}

bool HGameplayEntityId::operator<(const HGameplayEntityId& rhs) const
{
	if (Index != rhs.Index)
	{
		return Index < rhs.Index;
	}
	return Generation < rhs.Generation;
}

void HGameplayEntityId::WriteJson(PJsonData& json) const
{
	json.AddMember("Index", Index);
	json.AddMember("Generation", Generation);
}

void HGameplayEntityId::ReadJson(const PJsonData& json)
{
	json.GetData("Index", &Index);
	json.GetData("Generation", &Generation);
}

HGameplayCoord::HGameplayCoord(int32 inX, int32 inY)
	: X(inX)
	, Y(inY)
{
}

bool HGameplayCoord::operator==(const HGameplayCoord& rhs) const
{
	return X == rhs.X && Y == rhs.Y;
}

bool HGameplayCoord::operator!=(const HGameplayCoord& rhs) const
{
	return !(*this == rhs);
}

bool HGameplayCoord::operator<(const HGameplayCoord& rhs) const
{
	if (X != rhs.X)
	{
		return X < rhs.X;
	}
	return Y < rhs.Y;
}

PString HGameplayCoord::ToString() const
{
	return PString::Format("(%d, %d)", X, Y);
}

void HGameplayCoord::WriteJson(PJsonData& json) const
{
	json.AddMember("X", X);
	json.AddMember("Y", Y);
}

void HGameplayCoord::ReadJson(const PJsonData& json)
{
	json.GetData("X", &X);
	json.GetData("Y", &Y);
}
