#include "PCH/PCH.h"
#include "Components/CameraComponent.h"
#include "Actors/Actor.h"
#include "Core/World.h"
#include "Classes/Scene.h"

void JGCameraComponent::SetFovY(float32 radians)
{
	_fovY = radians;
	syncSceneCamera();
}

float32 JGCameraComponent::GetFovY() const
{
	return _fovY;
}

void JGCameraComponent::SetClipPlanes(float32 nearZ, float32 farZ)
{
	_nearZ = nearZ;
	_farZ  = farZ;
	syncSceneCamera();
}

float32 JGCameraComponent::GetNearZ() const
{
	return _nearZ;
}

float32 JGCameraComponent::GetFarZ() const
{
	return _farZ;
}

void JGCameraComponent::SetAutoActivate(bool bAutoActivate)
{
	_bAutoActivate = bAutoActivate;
}

void JGCameraComponent::Activate()
{
	PSharedPtr<PWorld> world = GetWorld();
	if (world == nullptr)
	{
		return;
	}
	world->SetActiveCamera(SharedWrap(this));
}

bool JGCameraComponent::IsActive() const
{
	PSharedPtr<PWorld> world = GetWorld();
	if (world == nullptr)
	{
		return false;
	}
	return world->GetActiveCamera().GetRawPointer() == this;
}

HSceneCameraID JGCameraComponent::GetSceneCameraID() const
{
	return _sceneCameraID;
}

HSceneCamera JGCameraComponent::MakeSceneCamera() const
{
	HSceneCamera camera;
	camera.FovY  = _fovY;
	camera.NearZ = _nearZ;
	camera.FarZ  = _farZ;

	PSharedPtr<JGActor> owner = GetOwner();
	if (owner == nullptr)
	{
		return camera;
	}

	// 액터 스케일이 뷰 행렬에 섞이지 않도록 방향 벡터만 뽑아 LookAt 으로 만든다.
	const HMatrix  world    = owner->GetWorldMatrix();
	const HVector3 position = world.TransformPoint(HVector3(0.0f, 0.0f, 0.0f));
	const HVector3 forward  = HVector3::Normalize(world.TransformVector(HVector3(0.0f, 0.0f, 1.0f)));
	const HVector3 up       = HVector3::Normalize(world.TransformVector(HVector3(0.0f, 1.0f, 0.0f)));
	camera.SetLookAt(position, position + forward, up);
	return camera;
}

HRay JGCameraComponent::ViewportPointToRay(const HVector2& viewportPoint, float32 aspectRatio) const
{
	const HSceneCamera camera      = MakeSceneCamera();
	const HMatrix      cameraToWorld = HMatrix::Inverse(camera.ViewMatrix);

	// PerspectiveFovLH 의 역: 뷰 공간 z = 1 평면에서 x = ndcX · tan(fov/2) · aspect, y = ndcY · tan(fov/2).
	const float32 ndcX       = viewportPoint.x * 2.0f - 1.0f;
	const float32 ndcY       = 1.0f - viewportPoint.y * 2.0f;
	const float32 tanHalfFov = tanf(camera.FovY * 0.5f);
	const HVector3 viewDir(ndcX * tanHalfFov * aspectRatio, ndcY * tanHalfFov, 1.0f);

	HRay ray;
	ray.origin = camera.Position;
	ray.dir    = HVector3::Normalize(cameraToWorld.TransformVector(viewDir));
	return ray;
}

void JGCameraComponent::OnBeginPlay()
{
	PSharedPtr<PWorld> world = GetWorld();
	if (world == nullptr)
	{
		return;
	}

	_sceneCameraID = world->GetScene()->CreateCamera(MakeSceneCamera());

	if (_bAutoActivate == true && world->GetActiveCamera() == nullptr)
	{
		world->SetActiveCamera(SharedWrap(this));
	}
}

void JGCameraComponent::OnTick(float32 deltaSeconds)
{
	syncSceneCamera();
}

void JGCameraComponent::OnEndPlay()
{
	PSharedPtr<PWorld> world = GetWorld();
	if (world == nullptr)
	{
		_sceneCameraID = HSceneCameraID();
		return;
	}

	if (world->GetActiveCamera().GetRawPointer() == this)
	{
		world->SetActiveCamera(nullptr);
	}

	if (_sceneCameraID.IsValid() == true)
	{
		world->GetScene()->DestroyCamera(_sceneCameraID);
		_sceneCameraID = HSceneCameraID();
	}
}

void JGCameraComponent::syncSceneCamera()
{
	if (_sceneCameraID.IsValid() == false)
	{
		return;
	}

	PSharedPtr<PWorld> world = GetWorld();
	if (world == nullptr)
	{
		return;
	}
	world->GetScene()->SetCamera(_sceneCameraID, MakeSceneCamera());
}
