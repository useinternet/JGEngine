#pragma once
#include "Actors/Actor.h"
#include "Components/CameraComponent.h"
#include "CameraActor.generation.h"

enum class ECameraActorMode : int32
{
	Fixed = 0,   // 액터 트랜스폼 그대로 (게임이 위치 · 회전을 정한다)
	Orbit,       // 대상점을 중심으로 도는 보드 카메라. 틱마다 궤도 값으로 트랜스폼을 다시 만든다
};

// 카메라 액터. 스폰될 때 JGCameraComponent 를 붙인다.
// Orbit: 위치 = 대상점 - 앞 방향 · 거리. yaw 는 +Y 축 회전(라디안, 0 이면 +Z 를 바라봄), pitch 는 양수일수록 내려다본다.
//        궤도 모드는 루트 액터를 가정한다 (부모가 있으면 부모 기준 로컬 트랜스폼으로 들어간다).
JGCLASS()
class GAMEFRAMEWORKS_API JGCameraActor : public JGActor
{
	JG_GENERATED_CLASS_BODY

private:
	PSharedPtr<JGCameraComponent> _camera;
	ECameraActorMode _mode          = ECameraActorMode::Fixed;
	HVector3         _orbitTarget;
	float32          _orbitYaw      = 0.0f;
	float32          _orbitPitch    = HMath::ConvertToRadians(45.0f);
	float32          _orbitDistance = 10.0f;
	float32          _minPitch      = HMath::ConvertToRadians(5.0f);
	float32          _maxPitch      = HMath::ConvertToRadians(89.0f);
	float32          _minDistance   = 0.1f;
	float32          _maxDistance   = 100000.0f;

public:
	JGCameraActor() = default;
	virtual ~JGCameraActor() = default;

	PSharedPtr<JGCameraComponent> GetCameraComponent() const;

	void             SetMode(ECameraActorMode mode);
	ECameraActorMode GetMode() const;

	// 궤도 값을 한 번에 정하고 모드를 Orbit 으로 바꾼다. 범위를 벗어난 pitch · distance 는 자른다.
	void SetOrbit(const HVector3& target, float32 yaw, float32 pitch, float32 distance);
	void SetOrbitTarget(const HVector3& target);
	// 뷰포트 끌기 · 휠 입력을 더한다 (라디안, 거리 배율). distanceScale 1 은 그대로, 0.9 는 10% 가까이.
	void AddOrbitInput(float32 deltaYaw, float32 deltaPitch, float32 distanceScale);
	void SetOrbitLimits(float32 minPitch, float32 maxPitch, float32 minDistance, float32 maxDistance);

	const HVector3& GetOrbitTarget() const;
	float32         GetOrbitYaw() const;
	float32         GetOrbitPitch() const;
	float32         GetOrbitDistance() const;

protected:
	virtual void OnSpawned() override;
	virtual void OnTick(float32 deltaSeconds) override;

private:
	void clampOrbit();
	void applyOrbit();
};
