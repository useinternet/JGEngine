#pragma once
#include "Components/ActorComponent.h"
#include "PickShapeComponent.generation.h"

class JGActor;
class JGPickShapeComponent;

// PWorld::PickActor 의 결과. 광선이 맞은 가장 가까운 판정 모양.
struct HWorldPickHit
{
	PSharedPtr<JGActor>              Actor;
	PSharedPtr<JGPickShapeComponent> Shape;
	HVector3                         Position;          // 월드 공간 교차점
	float32                          Distance = 0.0f;   // 광선 원점에서 교차점까지 (월드 단위)
};

// 피킹 판정 모양. 소유 액터의 로컬 공간 상자 하나이고, 액터의 월드 행렬(이동 · 회전 · 스케일)을 따라간다.
// 두께가 0 인 상자는 납작한 사각형이다 (카드 · 타일처럼 평평한 것).
// 상자를 정하지 않았으면 같은 액터의 JGStaticMeshComponent 경계 상자를 쓴다.
JGCLASS()
class GAMEFRAMEWORKS_API JGPickShapeComponent : public JGActorComponent
{
	JG_GENERATED_CLASS_BODY

private:
	HBBox _localBox;
	bool  _bHasLocalBox = false;
	bool  _bPickEnabled = true;

public:
	JGPickShapeComponent() = default;
	virtual ~JGPickShapeComponent() = default;

	void SetLocalBox(const HBBox& box);
	void SetLocalBox(const HVector3& center, const HVector3& halfExtents);
	// 판정에 쓰는 로컬 상자. 직접 정한 상자 → 메시 경계 상자 순. 둘 다 없으면 false.
	bool GetLocalBox(HBBox& outBox) const;

	void SetPickEnabled(bool bEnabled);
	bool IsPickEnabled() const;

	// 월드 광선과의 교차. 광선 원점이 상자 안이면 거리 0. ray.dir 은 정규화돼 있어야 거리가 월드 단위다.
	bool IntersectRay(const HRay& worldRay, float32* outDistance, HVector3* outPosition) const;
};
