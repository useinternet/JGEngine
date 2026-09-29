#pragma once
#include "Core/GameFrameWorksDefines.h"

// 위치 · 회전 · 스케일. 액터가 직접 갖는다 (컴포넌트에는 트랜스폼이 없다).
struct GAMEFRAMEWORKS_API HTransform : public IJsonable
{
	HVector3    Position;
	HQuaternion Rotation = HQuaternion::Identity();
	HVector3    Scale    = HVector3(1.0f, 1.0f, 1.0f);

	HTransform() = default;
	HTransform(const HVector3& inPosition, const HQuaternion& inRotation, const HVector3& inScale);

	static HTransform Identity();

	HMatrix ToMatrix() const;
	// 부모 월드 행렬에 로컬을 합성한다.
	HMatrix ToWorldMatrix(const HMatrix& parentWorld) const;

	bool operator==(const HTransform& rhs) const;
	bool operator!=(const HTransform& rhs) const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
