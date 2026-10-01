#include "PCH/PCH.h"
#include "Components/PickShapeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Actors/Actor.h"

namespace
{
	constexpr float32 PickParallelEpsilon = 1e-8f;

	float32 axisValue(const HVector3& v, int32 axis)
	{
		if (axis == 0)
		{
			return v.x;
		}
		if (axis == 1)
		{
			return v.y;
		}
		return v.z;
	}

	// 슬랩 방법. 광선 o + t·d (t ≥ 0) 가 상자에 들어가는 첫 t. d 는 정규화하지 않아도 된다.
	bool intersectBox(const HVector3& origin, const HVector3& dir, const HBBox& box, float32* outT)
	{
		float32 tEnter = 0.0f;
		float32 tExit  = FLT_MAX;

		for (int32 axis = 0; axis < 3; ++axis)
		{
			const float32 o     = axisValue(origin, axis);
			const float32 d     = axisValue(dir, axis);
			const float32 minV  = axisValue(box.min, axis);
			const float32 maxV  = axisValue(box.max, axis);

			if (HMath::Abs(d) < PickParallelEpsilon)
			{
				// 이 축과 평행하면 원점이 두 면 사이에 있어야 한다.
				if (o < minV || o > maxV)
				{
					return false;
				}
				continue;
			}

			float32 t1 = (minV - o) / d;
			float32 t2 = (maxV - o) / d;
			if (t1 > t2)
			{
				std::swap(t1, t2);
			}

			tEnter = HMath::Max(tEnter, t1);
			tExit  = HMath::Min(tExit, t2);
			if (tEnter > tExit)
			{
				return false;
			}
		}

		*outT = tEnter;
		return true;
	}
}

void JGPickShapeComponent::SetLocalBox(const HBBox& box)
{
	_localBox     = box;
	_bHasLocalBox = true;
}

void JGPickShapeComponent::SetLocalBox(const HVector3& center, const HVector3& halfExtents)
{
	HBBox box;
	box.min = center - halfExtents;
	box.max = center + halfExtents;
	SetLocalBox(box);
}

bool JGPickShapeComponent::GetLocalBox(HBBox& outBox) const
{
	if (_bHasLocalBox == true)
	{
		outBox = _localBox;
		return true;
	}

	PSharedPtr<JGActor> owner = GetOwner();
	if (owner == nullptr)
	{
		return false;
	}

	PSharedPtr<JGStaticMeshComponent> mesh = owner->FindComponent<JGStaticMeshComponent>();
	if (mesh == nullptr)
	{
		return false;
	}
	return mesh->GetLocalBounds(outBox);
}

void JGPickShapeComponent::SetPickEnabled(bool bEnabled)
{
	_bPickEnabled = bEnabled;
}

bool JGPickShapeComponent::IsPickEnabled() const
{
	return _bPickEnabled;
}

bool JGPickShapeComponent::IntersectRay(const HRay& worldRay, float32* outDistance, HVector3* outPosition) const
{
	if (_bPickEnabled == false)
	{
		return false;
	}

	PSharedPtr<JGActor> owner = GetOwner();
	if (owner == nullptr)
	{
		return false;
	}

	HBBox box;
	if (GetLocalBox(box) == false)
	{
		return false;
	}

	// 광선을 액터 로컬 공간으로 옮긴다. 아핀 변환이라 로컬에서 구한 t 가 월드 광선의 t 와 같다.
	const HMatrix worldToLocal = HMatrix::Inverse(owner->GetWorldMatrix());
	const HVector3 localOrigin = worldToLocal.TransformPoint(worldRay.origin);
	const HVector3 localDir    = worldToLocal.TransformVector(worldRay.dir);

	float32 t = 0.0f;
	if (intersectBox(localOrigin, localDir, box, &t) == false)
	{
		return false;
	}

	if (outDistance != nullptr)
	{
		*outDistance = t * HVector3::Length(worldRay.dir);
	}
	if (outPosition != nullptr)
	{
		*outPosition = worldRay.origin + worldRay.dir * t;
	}
	return true;
}
