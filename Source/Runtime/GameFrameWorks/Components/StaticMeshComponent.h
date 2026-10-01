#pragma once
#include "Components/ActorComponent.h"
#include "Classes/Scene.h"
#include "AssetPath.h"
#include "StaticMeshComponent.generation.h"

class JGAsset;
class JGStaticMesh;
class IMesh;
class IRawMaterial;

// 정적 메시 하나를 월드 장면(PWorld::GetScene)에 배치한다. 배치는 장면이 번호로 들고, 이 컴포넌트는 번호만 든다.
//   BeginPlay → CreateMesh,  틱마다 → SetMeshWorldMatrix(소유 액터의 월드 행렬),  EndPlay → DestroyMesh
// 메시는 에셋(JGStaticMesh, 경계 상자 포함)으로 주거나, 렌더 메시(IMesh)를 직접 준다(절차 생성 · 헤드리스 테스트).
// 메인 스레드 전용 (PScene 규칙).
JGCLASS()
class GAMEFRAMEWORKS_API JGStaticMeshComponent : public JGActorComponent
{
	JG_GENERATED_CLASS_BODY

private:
	PSharedPtr<JGStaticMesh>        _meshAsset;
	PSharedPtr<IMesh>               _mesh;          // 장면에 넘기는 렌더 메시. 에셋이 있으면 에셋의 것
	HList<PSharedPtr<IRawMaterial>> _materials;     // 슬롯별 덮어쓰기. 비었거나 null 인 칸은 메시의 머터리얼
	HSceneMeshID                    _sceneMeshID;
	HAssetPath                      _loadingPath;   // 읽는 중인 에셋. 늦게 온 옛 요청을 거른다
	bool                            _bLoading = false;

public:
	JGStaticMeshComponent() = default;
	virtual ~JGStaticMeshComponent() = default;

	void                     SetStaticMesh(PSharedPtr<JGStaticMesh> meshAsset);
	PSharedPtr<JGStaticMesh> GetStaticMesh() const;

	// 에셋을 비동기로 읽어 SetStaticMesh 한다. 에셋 데이터베이스가 없으면(헤드리스 JGConsole) false.
	bool LoadStaticMesh(const HAssetPath& assetPath);
	bool IsLoading() const;

	// 렌더 메시를 직접 준다. 에셋은 비운다 (경계 상자가 없다).
	void              SetMesh(PSharedPtr<IMesh> mesh);
	PSharedPtr<IMesh> GetMesh() const;

	void SetMaterial(uint32 slot, PSharedPtr<IRawMaterial> material);

	// 에셋 메시의 로컬 경계 상자. 에셋이 없거나 정점이 없으면 false.
	bool GetLocalBounds(HBBox& outBounds) const;

	// 장면의 배치 번호. BeginPlay 전이나 메시가 없으면 무효.
	HSceneMeshID GetSceneMeshID() const;

protected:
	virtual void OnBeginPlay() override;
	virtual void OnTick(float32 deltaSeconds) override;
	virtual void OnEndPlay() override;

private:
	void createSceneMesh();
	void destroySceneMesh();
	void recreateSceneMesh();
	void onMeshLoaded(const HAssetPath& assetPath, PWeakPtr<JGAsset> asset);
};
