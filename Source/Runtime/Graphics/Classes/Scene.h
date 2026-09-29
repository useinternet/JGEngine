#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class IMesh;
class IRawMaterial;

// 장면(PScene)에 놓인 것을 가리키는 번호. 0은 "없음"이고, 한 장면 안에서 번호를 다시 쓰지 않는다.
// 밖(DevScene, GameFrameWorks 컴포넌트)은 장면 객체 대신 이 번호만 들고 있는다. 장면이나 그 객체가 먼저 없어져도 조회가 실패할 뿐이다.
// (객체를 강참조하면 GC 종료 순서 문제, 약참조하면 Graphics_TODO 5-18 문제가 생긴다)
struct GRAPHICS_API HSceneMeshID
{
	uint64 Value = 0;

	bool IsValid() const { return Value != 0; }
	bool operator==(const HSceneMeshID& rhs) const { return Value == rhs.Value; }
	bool operator!=(const HSceneMeshID& rhs) const { return Value != rhs.Value; }
};

struct GRAPHICS_API HSceneCameraID
{
	uint64 Value = 0;

	bool IsValid() const { return Value != 0; }
	bool operator==(const HSceneCameraID& rhs) const { return Value == rhs.Value; }
	bool operator!=(const HSceneCameraID& rhs) const { return Value != rhs.Value; }
};

// 장면에 놓인 메시 하나(배치). 메시와 머터리얼은 에셋의 리소스라 참조만 한다. 여러 배치와 여러 장면이 나눠 쓰고 수명은 에셋이 정한다.
struct GRAPHICS_API HSceneMesh
{
	PSharedPtr<IMesh> Mesh;                      // 에셋의 렌더 메시 (JGStaticMesh::GetMesh)
	HList<PSharedPtr<IRawMaterial>> Materials;   // 서브메시 순서. 비었거나 null인 칸은 메시의 머터리얼을 쓴다
	HMatrix WorldMatrix = HMatrix::Identity();
};

// 원근 카메라. 화면 비율은 그리는 쪽(PSceneRenderer의 출력 크기)이 정한다.
// ViewMatrix와 Position은 SetLookAt / SetWorldMatrix로 함께 바꾼다.
struct GRAPHICS_API HSceneCamera
{
	HMatrix  ViewMatrix = HMatrix::Identity();   // 월드 -> 카메라
	HVector3 Position;                           // 월드 공간 카메라 위치
	float32  FovY  = HMath::ConvertToRadians(45.0f);
	float32  NearZ = 0.1f;
	float32  FarZ  = 5000.0f;

	// 바라볼 점으로 정한다 (DevScene).
	void SetLookAt(const HVector3& inEye, const HVector3& inTarget, const HVector3& inUp);
	// 카메라의 월드 행렬(카메라 -> 월드)로 정한다 (액터 트랜스폼).
	void SetWorldMatrix(const HMatrix& inCameraToWorld);
	HMatrix GetProjMatrix(float32 inAspectRatio) const;
};

// 장면. 카메라와 메시 배치를 번호로 관리한다. 그리는 일은 하지 않는다(PSceneRenderer가 이 데이터를 읽는다).
// 밖에서 Allocate<PScene>()으로 만들어 소유한다(DevScene, 나중에 GameFrameWorks의 PWorld). 메인 스레드 전용.
class GRAPHICS_API PScene : public IMemoryObject
{
	HHashMap<uint64, HSceneMesh>   _meshes;
	HHashMap<uint64, HSceneCamera> _cameras;
	uint64 _nextID = 1;   // 메시와 카메라가 번호 공간을 나눠 쓴다

public:
	virtual ~PScene() = default;

	HSceneMeshID CreateMesh(const HSceneMesh& inMesh);
	void         DestroyMesh(HSceneMeshID inID);
	// 읽기 전용. 포인터는 그 자리에서만 쓰고 들고 있지 않는다(다음 Create/Destroy에서 무효가 될 수 있다). 없으면 nullptr.
	const HSceneMesh* FindMesh(HSceneMeshID inID) const;
	bool SetMeshWorldMatrix(HSceneMeshID inID, const HMatrix& inWorldMatrix);
	bool SetMeshMaterial(HSceneMeshID inID, uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial);

	HSceneCameraID CreateCamera(const HSceneCamera& inCamera);
	void           DestroyCamera(HSceneCameraID inID);
	const HSceneCamera* FindCamera(HSceneCameraID inID) const;
	bool SetCamera(HSceneCameraID inID, const HSceneCamera& inCamera);

	// PSceneRenderer가 순회한다.
	const HHashMap<uint64, HSceneMesh>& GetMeshes() const;

private:
	bool checkMainThread(const char* inFunction) const;
};
