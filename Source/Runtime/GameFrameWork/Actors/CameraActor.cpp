#include "PCH/PCH.h"
#include "Actors/CameraActor.h"

PSharedPtr<JGCameraComponent> JGCameraActor::GetCameraComponent() const
{
	return _camera;
}

void JGCameraActor::SetMode(ECameraActorMode mode)
{
	_mode = mode;
	if (_mode == ECameraActorMode::Orbit)
	{
		applyOrbit();
	}
}

ECameraActorMode JGCameraActor::GetMode() const
{
	return _mode;
}

void JGCameraActor::SetOrbit(const HVector3& target, float32 yaw, float32 pitch, float32 distance)
{
	_orbitTarget   = target;
	_orbitYaw      = yaw;
	_orbitPitch    = pitch;
	_orbitDistance = distance;
	_mode          = ECameraActorMode::Orbit;
	clampOrbit();
	applyOrbit();
}

void JGCameraActor::SetOrbitTarget(const HVector3& target)
{
	_orbitTarget = target;
	applyOrbit();
}

void JGCameraActor::AddOrbitInput(float32 deltaYaw, float32 deltaPitch, float32 distanceScale)
{
	_orbitYaw      += deltaYaw;
	_orbitPitch    += deltaPitch;
	_orbitDistance *= distanceScale;
	clampOrbit();
	applyOrbit();
}

void JGCameraActor::SetOrbitLimits(float32 minPitch, float32 maxPitch, float32 minDistance, float32 maxDistance)
{
	_minPitch    = minPitch;
	_maxPitch    = maxPitch;
	_minDistance = minDistance;
	_maxDistance = maxDistance;
	clampOrbit();
	applyOrbit();
}

const HVector3& JGCameraActor::GetOrbitTarget() const
{
	return _orbitTarget;
}

float32 JGCameraActor::GetOrbitYaw() const
{
	return _orbitYaw;
}

float32 JGCameraActor::GetOrbitPitch() const
{
	return _orbitPitch;
}

float32 JGCameraActor::GetOrbitDistance() const
{
	return _orbitDistance;
}

void JGCameraActor::OnSpawned()
{
	_camera = FindOrAddComponent<JGCameraComponent>();
}

void JGCameraActor::OnTick(float32 deltaSeconds)
{
	if (_mode == ECameraActorMode::Orbit)
	{
		applyOrbit();
	}
}

void JGCameraActor::clampOrbit()
{
	_orbitPitch    = HMath::Clamp(_orbitPitch, _minPitch, _maxPitch);
	_orbitDistance = HMath::Clamp(_orbitDistance, _minDistance, _maxDistance);
}

void JGCameraActor::applyOrbit()
{
	if (_mode != ECameraActorMode::Orbit)
	{
		return;
	}

	// RollPitchYaw 회전: 로컬 +Z(앞)가 pitch 만큼 아래로, yaw 만큼 +Y 축으로 돈다.
	const HQuaternion rotation = HQuaternion::ToQuaternion(_orbitPitch, _orbitYaw, 0.0f);
	const HVector3    forward  = HMatrix::Rotation(rotation).TransformVector(HVector3(0.0f, 0.0f, 1.0f));

	HTransform transform = GetLocalTransform();
	transform.Position = _orbitTarget - forward * _orbitDistance;
	transform.Rotation = rotation;
	SetLocalTransform(transform);
}
