#include "PCH/PCH.h"
#include "Scene.h"
#include "Classes/Mesh.h"
#include "Classes/Material.h"

void HSceneCamera::SetLookAt(const HVector3& inEye, const HVector3& inTarget, const HVector3& inUp)
{
	ViewMatrix = HMatrix::LookAtLH(inEye, inTarget, inUp);
	Position   = inEye;
}

void HSceneCamera::SetWorldMatrix(const HMatrix& inCameraToWorld)
{
	ViewMatrix = HMatrix::Inverse(inCameraToWorld);
	Position   = inCameraToWorld.TransformPoint(HVector3(0.0f, 0.0f, 0.0f));
}

HMatrix HSceneCamera::GetProjMatrix(float32 inAspectRatio) const
{
	return HMatrix::PerspectiveFovLH(FovY, inAspectRatio, NearZ, FarZ);
}

HSceneMeshID PScene::CreateMesh(const HSceneMesh& inMesh)
{
	if (checkMainThread("CreateMesh") == false)
	{
		return HSceneMeshID();
	}
	if (inMesh.Mesh == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "PScene::CreateMesh : Mesh is null");
		return HSceneMeshID();
	}

	HSceneMeshID id;
	id.Value = _nextID++;
	_meshes.emplace(id.Value, inMesh);
	return id;
}

void PScene::DestroyMesh(HSceneMeshID inID)
{
	if (checkMainThread("DestroyMesh") == false)
	{
		return;
	}
	// 메시·머터리얼 참조가 여기서 풀린다. GPU가 아직 쓰는 중이어도 리소스 해제는 프레임이 끝날 때까지 미뤄진다(5-5).
	_meshes.erase(inID.Value);
}

const HSceneMesh* PScene::FindMesh(HSceneMeshID inID) const
{
	auto iter = _meshes.find(inID.Value);
	if (iter == _meshes.end())
	{
		return nullptr;
	}
	return &(iter->second);
}

bool PScene::SetMeshWorldMatrix(HSceneMeshID inID, const HMatrix& inWorldMatrix)
{
	if (checkMainThread("SetMeshWorldMatrix") == false)
	{
		return false;
	}

	auto iter = _meshes.find(inID.Value);
	if (iter == _meshes.end())
	{
		return false;
	}
	iter->second.WorldMatrix = inWorldMatrix;
	return true;
}

bool PScene::SetMeshMaterial(HSceneMeshID inID, uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial)
{
	if (checkMainThread("SetMeshMaterial") == false)
	{
		return false;
	}

	auto iter = _meshes.find(inID.Value);
	if (iter == _meshes.end())
	{
		return false;
	}

	HSceneMesh& sceneMesh = iter->second;
	if (inSlot >= sceneMesh.Mesh->GetSubMeshCount())
	{
		JG_LOG(Graphics, ELogLevel::Error, "PScene::SetMeshMaterial : slot %d is out of range (sub meshes %d)", (int32)inSlot, (int32)sceneMesh.Mesh->GetSubMeshCount());
		return false;
	}

	if (sceneMesh.Materials.size() <= inSlot)
	{
		sceneMesh.Materials.resize(inSlot + 1);
	}
	sceneMesh.Materials[inSlot] = inMaterial;
	return true;
}

HSceneCameraID PScene::CreateCamera(const HSceneCamera& inCamera)
{
	if (checkMainThread("CreateCamera") == false)
	{
		return HSceneCameraID();
	}

	HSceneCameraID id;
	id.Value = _nextID++;
	_cameras.emplace(id.Value, inCamera);
	return id;
}

void PScene::DestroyCamera(HSceneCameraID inID)
{
	if (checkMainThread("DestroyCamera") == false)
	{
		return;
	}
	_cameras.erase(inID.Value);
}

const HSceneCamera* PScene::FindCamera(HSceneCameraID inID) const
{
	auto iter = _cameras.find(inID.Value);
	if (iter == _cameras.end())
	{
		return nullptr;
	}
	return &(iter->second);
}

bool PScene::SetCamera(HSceneCameraID inID, const HSceneCamera& inCamera)
{
	if (checkMainThread("SetCamera") == false)
	{
		return false;
	}

	auto iter = _cameras.find(inID.Value);
	if (iter == _cameras.end())
	{
		return false;
	}
	iter->second = inCamera;
	return true;
}

const HHashMap<uint64, HSceneMesh>& PScene::GetMeshes() const
{
	return _meshes;
}

bool PScene::checkMainThread(const char* inFunction) const
{
	const ThreadID currentThreadID = std::hash<std::thread::id>()(std::this_thread::get_id());
	if (currentThreadID == GCoreSystem::GetMainThreadID())
	{
		return true;
	}

	JG_LOG(Graphics, ELogLevel::Error, "PScene::%s must be called on the main thread", PString(inFunction));
	return false;
}
