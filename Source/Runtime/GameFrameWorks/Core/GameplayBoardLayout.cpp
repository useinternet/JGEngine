#include "PCH/PCH.h"
#include "Core/GameplayBoardLayout.h"

namespace
{
	constexpr float32 BoardSqrt3            = 1.73205080757f;
	constexpr float32 BoardParallelEpsilon  = 1e-6f;

	// 실수 축 좌표를 가장 가까운 육각 칸으로. 큐브 좌표(q + r + s = 0)로 반올림하고 오차가 가장 큰 성분을 나머지로 맞춘다.
	HGameplayCoord roundHex(float32 q, float32 r)
	{
		const float32 s = -q - r;

		float32 rq = HMath::RoundToFloat32(q);
		float32 rr = HMath::RoundToFloat32(r);
		float32 rs = HMath::RoundToFloat32(s);

		const float32 dq = HMath::Abs(rq - q);
		const float32 dr = HMath::Abs(rr - r);
		const float32 ds = HMath::Abs(rs - s);

		if (dq > dr && dq > ds)
		{
			rq = -rr - rs;
		}
		else if (dr > ds)
		{
			rr = -rq - rs;
		}

		return HGameplayCoord((int32)rq, (int32)rr);
	}
}

HGameplayBoardLayout::HGameplayBoardLayout(EGameplayBoardKind inKind, const HVector3& inOrigin, float32 inCellSize)
	: Kind(inKind)
	, Origin(inOrigin)
	, CellSize(inCellSize)
{
}

bool HGameplayBoardLayout::IsValid() const
{
	return Kind != EGameplayBoardKind::None && CellSize > 0.0f;
}

HVector3 HGameplayBoardLayout::CoordToWorld(const HGameplayCoord& coord) const
{
	if (Kind == EGameplayBoardKind::Hex)
	{
		const float32 q = (float32)coord.X;
		const float32 r = (float32)coord.Y;
		return Origin + HVector3(CellSize * (q + r * 0.5f), 0.0f, CellSize * r * BoardSqrt3 * 0.5f);
	}
	return Origin + HVector3((float32)coord.X * CellSize, 0.0f, (float32)coord.Y * CellSize);
}

bool HGameplayBoardLayout::WorldToCoord(const HVector3& position, HGameplayCoord* outCoord) const
{
	if (IsValid() == false)
	{
		return false;
	}

	const float32 localX = position.x - Origin.x;
	const float32 localZ = position.z - Origin.z;

	HGameplayCoord coord;
	if (Kind == EGameplayBoardKind::Hex)
	{
		const float32 r = localZ / (CellSize * BoardSqrt3 * 0.5f);
		const float32 q = localX / CellSize - r * 0.5f;
		coord = roundHex(q, r);
	}
	else
	{
		// 칸 중심이 정수 좌표이므로 칸 경계는 ±0.5 칸.
		coord = HGameplayCoord(HMath::FloorToInt(localX / CellSize + 0.5f), HMath::FloorToInt(localZ / CellSize + 0.5f));
	}

	if (outCoord != nullptr)
	{
		*outCoord = coord;
	}
	return true;
}

bool HGameplayBoardLayout::PickCoord(const HRay& ray, HGameplayCoord* outCoord, HVector3* outPosition) const
{
	if (IsValid() == false)
	{
		return false;
	}

	// 평면 y = Origin.y 와의 교차.
	if (HMath::Abs(ray.dir.y) < BoardParallelEpsilon)
	{
		return false;
	}

	const float32 t = (Origin.y - ray.origin.y) / ray.dir.y;
	if (t < 0.0f)
	{
		return false;
	}

	const HVector3 hit = ray.origin + ray.dir * t;
	if (outPosition != nullptr)
	{
		*outPosition = hit;
	}
	return WorldToCoord(hit, outCoord);
}
