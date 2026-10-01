#include "PCH/PCH.h"
#include "Components/StaticMeshComponent.h"
#include "Actors/Actor.h"
#include "Core/World.h"
#include "Classes/Scene.h"
#include "Classes/Mesh.h"
#include "Classes/StaticMesh.h"
#include "AssetDatabase.h"

void JGStaticMeshComponent::SetStaticMesh(PSharedPtr<JGStaticMesh> meshAsset)
{
	_meshAsset = meshAsset;
	_mesh      = nullptr;
	if (_meshAsset != nullptr)
	{
		_mesh = _meshAsset->GetMesh();
	}
	recreateSceneMesh();
}

PSharedPtr<JGStaticMesh> JGStaticMeshComponent::GetStaticMesh() const
{
	return _meshAsset;
}

bool JGStaticMeshComponent::LoadStaticMesh(const HAssetPath& assetPath)
{
	if (GAssetDatabase::IsValid() == false)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Warning, "JGStaticMeshComponent: asset database is not available, %s is not loaded", assetPath.GetRawAssetPath().ToString());
		return false;
	}

	_loadingPath = assetPath;
	_bLoading    = true;

	// 콜백은 로드가 끝난 뒤 메인 스레드에서 온다. 그 사이 컴포넌트가 없어질 수 있으므로 약참조로 잡는다.
	PWeakPtr<JGStaticMeshComponent> weakThis = SharedWrap(this);
	const bool bRequested = GAssetDatabase::GetInstance().LoadAssetAsync(assetPath,
		POnLoadCompelete::CreateLambda([weakThis, assetPath](PWeakPtr<JGAsset> asset)
		{
			PSharedPtr<JGStaticMeshComponent> self = weakThis.Pin();
			if (self != nullptr)
			{
				self->onMeshLoaded(assetPath, asset);
			}
		}));

	if (bRequested == false)
	{
		_bLoading = false;
		JG_LOG(GameFrameWorks, ELogLevel::Error, "JGStaticMeshComponent: fail to request %s", assetPath.GetRawAssetPath().ToString());
	}
	return bRequested;
}

bool JGStaticMeshComponent::IsLoading() const
{
	return _bLoading;
}

void JGStaticMeshComponent::SetMesh(PSharedPtr<IMesh> mesh)
{
	_meshAsset = nullptr;
	_mesh      = mesh;
	recreateSceneMesh();
}

PSharedPtr<IMesh> JGStaticMeshComponent::GetMesh() const
{
	return _mesh;
}

void JGStaticMeshComponent::SetMaterial(uint32 slot, PSharedPtr<IRawMaterial> material)
{
	if (_materials.size() <= slot)
	{
		_materials.resize(slot + 1);
	}
	_materials[slot] = material;

	if (_sceneMeshID.IsValid() == false)
	{
		return;
	}

	PSharedPtr<PWorld> world = GetWorld();
	if (world != nullptr)
	{
		world->GetScene()->SetMeshMaterial(_sceneMeshID, slot, material);
	}
}

bool JGStaticMeshComponent::GetLocalBounds(HBBox& outBounds) const
{
	if (_meshAsset == nullptr)
	{
		return false;
	}
	return _meshAsset->GetBounds(outBounds);
}

HSceneMeshID JGStaticMeshComponent::GetSceneMeshID() const
{
	return _sceneMeshID;
}

void JGStaticMeshComponent::OnBeginPlay()
{
	createSceneMesh();
}

void JGStaticMeshComponent::OnTick(float32 deltaSeconds)
{
	if (_sceneMeshID.IsValid() == false)
	{
		return;
	}

	PSharedPtr<PWorld>  world = GetWorld();
	PSharedPtr<JGActor> owner = GetOwner();
	if (world == nullptr || owner == nullptr)
	{
		return;
	}
	world->GetScene()->SetMeshWorldMatrix(_sceneMeshID, owner->GetWorldMatrix());
}

void JGStaticMeshComponent::OnEndPlay()
{
	destroySceneMesh();
}

void JGStaticMeshComponent::createSceneMesh()
{
	if (_sceneMeshID.IsValid() == true || _mesh == nullptr || HasBegunPlay() == false)
	{
		return;
	}

	PSharedPtr<PWorld>  world = GetWorld();
	PSharedPtr<JGActor> owner = GetOwner();
	if (world == nullptr || owner == nullptr)
	{
		return;
	}

	HSceneMesh sceneMesh;
	sceneMesh.Mesh        = _mesh;
	sceneMesh.Materials   = _materials;
	sceneMesh.WorldMatrix = owner->GetWorldMatrix();
	_sceneMeshID = world->GetScene()->CreateMesh(sceneMesh);
}

void JGStaticMeshComponent::destroySceneMesh()
{
	if (_sceneMeshID.IsValid() == false)
	{
		return;
	}

	PSharedPtr<PWorld> world = GetWorld();
	if (world != nullptr)
	{
		world->GetScene()->DestroyMesh(_sceneMeshID);
	}
	_sceneMeshID = HSceneMeshID();
}

void JGStaticMeshComponent::recreateSceneMesh()
{
	destroySceneMesh();
	createSceneMesh();
}

void JGStaticMeshComponent::onMeshLoaded(const HAssetPath& assetPath, PWeakPtr<JGAsset> asset)
{
	// 요청 뒤에 다른 에셋을 요청했으면 옛 결과는 버린다.
	if (_bLoading == false || _loadingPath.GetRawAssetPath() != assetPath.GetRawAssetPath())
	{
		return;
	}
	_bLoading = false;

	PSharedPtr<JGAsset> loaded = asset.Pin();
	if (loaded == nullptr)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Error, "JGStaticMeshComponent: fail to load %s", assetPath.GetRawAssetPath().ToString());
		return;
	}

	// Cast<> 는 정적 캐스트라 타입을 먼저 확인한다.
	if ((loaded->GetType() == JGType::GenerateType<JGStaticMesh>()) == false)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Error, "JGStaticMeshComponent: %s is not a JGStaticMesh (%s)", assetPath.GetRawAssetPath().ToString(), loaded->GetType().GetName().ToString());
		return;
	}

	PSharedPtr<JGStaticMesh> meshAsset = Cast<JGStaticMesh>(loaded);
	if (meshAsset == nullptr || meshAsset->IsValid() == false)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Error, "JGStaticMeshComponent: loaded mesh is invalid (%s)", assetPath.GetRawAssetPath().ToString());
		return;
	}

	SetStaticMesh(meshAsset);
}
