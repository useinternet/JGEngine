#pragma once
#include "Components/ActorComponent.h"
#include "Classes/Scene.h"
#include "CameraComponent.generation.h"

// 원근 카메라 하나를 월드 장면(PWorld::GetScene)에 둔다. 위치 · 방향은 소유 액터의 월드 행렬이다 (로컬 +Z 가 바라보는 쪽, +Y 가 위).
//   BeginPlay → CreateCamera (월드에 활성 카메라가 없으면 스스로 활성),  틱마다 → SetCamera,  EndPlay → DestroyCamera
// 어느 카메라로 그릴지는 월드의 활성 카메라(PWorld::SetActiveCamera)가 정하고, 씬 뷰포트가 그 카메라로 그리고 피킹한다.
JGCLASS()
class GAMEFRAMEWORKS_API JGCameraComponent : public JGActorComponent
{
	JG_GENERATED_CLASS_BODY

private:
	float32        _fovY  = HMath::ConvertToRadians(45.0f);
	float32        _nearZ = 0.1f;
	float32        _farZ  = 5000.0f;
	bool           _bAutoActivate = true;
	HSceneCameraID _sceneCameraID;

public:
	JGCameraComponent() = default;
	virtual ~JGCameraComponent() = default;

	void    SetFovY(float32 radians);
	float32 GetFovY() const;
	void    SetClipPlanes(float32 nearZ, float32 farZ);
	float32 GetNearZ() const;
	float32 GetFarZ() const;

	// BeginPlay 에서 활성 카메라가 없을 때 스스로 활성이 될지. 기본 true.
	void SetAutoActivate(bool bAutoActivate);
	void Activate();
	bool IsActive() const;

	HSceneCameraID GetSceneCameraID() const;

	// 소유 액터의 지금 월드 행렬로 만든 장면 카메라 값. 스케일은 쓰지 않는다.
	HSceneCamera MakeSceneCamera() const;

	// 뷰포트 안의 점을 지나는 월드 광선 (원점 = 카메라 위치, dir 정규화).
	// viewportPoint 는 0~1 (왼쪽 위가 (0, 0)), aspectRatio 는 그린 이미지의 가로/세로다 (PSceneRenderer 출력 크기).
	HRay ViewportPointToRay(const HVector2& viewportPoint, float32 aspectRatio) const;

protected:
	virtual void OnBeginPlay() override;
	virtual void OnTick(float32 deltaSeconds) override;
	virtual void OnEndPlay() override;

private:
	void syncSceneCamera();
};
