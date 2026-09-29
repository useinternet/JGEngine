# PScene · 카메라 설계 방안 (Graphics 5-7) — 2026-09-29

## 결론
- 제안하신 구조로 간다. 밖(DevScene, 나중에 `PWorld`)이 `PScene`을 만들고, 구성물의 생성 · 조회 · 수정 · 삭제는 모두 `PScene`을 거친다.
- 다듬을 점 둘. (1) 장면이 **가지는** 것은 카메라와 메시 배치(메시 + 머터리얼 + 월드 행렬)뿐이고, 메시 · 머터리얼 · 텍스처 자체는 **참조만** 한다. (2) 장면은 **데이터만** 들고, 그리는 일은 장면을 읽는 `PSceneRenderer`가 한다(DevScene의 G버퍼 · 합성 패스를 옮긴다, 5-22).
- 그래서 GameFrameWorks는 "그리는" 것이 아니라 자기 컴포넌트가 받은 ID로 장면 데이터를 고칠 뿐이고, 그리기는 Graphics가 한다.

## 구조
| 누가 | 하는 일 | 방법 |
|---|---|---|
| DevScene / `PWorld`(GFW) | `PScene`을 만들고 소유 | `Allocate<PScene>()`. 백엔드와 무관한 데이터라 API 팩토리가 필요 없다 |
| DevScene / GFW 컴포넌트 | 카메라 · 메시 배치를 만들고 고치고 지운다 | 받은 ID로 `PScene` 함수 호출 |
| `PScene` | 카메라 · 메시 배치 목록(ID → 데이터) | 메인 스레드 전용 |
| 에셋(`JGStaticMesh`, 머터리얼 · 텍스처) | 리소스 | 장면은 `PSharedPtr`로 참조만 |
| `PSceneRenderer` | 매 프레임 장면을 읽어 G버퍼 → 합성 → 출력 텍스처 | `Render(scene, cameraID)` |

## API 초안 (`Graphics/Classes/Scene.h`, `SceneRenderer.h`)
```cpp
struct HSceneMeshID   { uint64 Value = 0; bool IsValid() const; };   // 0 = 없음. 다시 쓰지 않는 증가 번호
struct HSceneCameraID { uint64 Value = 0; bool IsValid() const; };

struct HSceneMesh                                    // 장면에 놓인 메시 하나(배치)
{
	PSharedPtr<IMesh> Mesh;                          // 에셋의 렌더 메시 (JGStaticMesh::GetMesh)
	HList<PSharedPtr<IRawMaterial>> Materials;       // 서브메시별 머터리얼. 비면 메시의 것
	HMatrix WorldMatrix = HMatrix::Identity();
};

struct HSceneCamera                                  // 원근 카메라. 화면 비율은 그리는 쪽(출력 크기)이 정한다
{
	HMatrix WorldMatrix = HMatrix::Identity();       // 카메라의 위치 · 방향. 뷰 행렬은 이것의 역행렬
	float32 FovY  = HMath::ConvertToRadians(45.0f);
	float32 NearZ = 0.1f;
	float32 FarZ  = 5000.0f;
	void SetLookAt(const HVector3& inEye, const HVector3& inTarget, const HVector3& inUp);   // DevScene처럼 바라볼 점으로 정할 때
};

class GRAPHICS_API PScene : public IMemoryObject
{
public:
	HSceneMeshID      CreateMesh(const HSceneMesh& inMesh);
	void              DestroyMesh(HSceneMeshID inID);
	const HSceneMesh* FindMesh(HSceneMeshID inID) const;          // 읽기 전용. 포인터는 그 자리에서만 쓴다
	bool              SetMeshWorldMatrix(HSceneMeshID inID, const HMatrix& inWorldMatrix);
	bool              SetMeshMaterial(HSceneMeshID inID, uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial);

	HSceneCameraID      CreateCamera(const HSceneCamera& inCamera);
	void                DestroyCamera(HSceneCameraID inID);
	const HSceneCamera* FindCamera(HSceneCameraID inID) const;
	bool                SetCamera(HSceneCameraID inID, const HSceneCamera& inCamera);

	const HHashMap<uint64, HSceneMesh>& GetMeshes() const;          // 렌더러가 순회
};

class GRAPHICS_API PSceneRenderer : public IMemoryObject             // DevScene의 G버퍼 · 합성 코드를 옮긴 것
{
public:
	void Initialize(uint32 inWidth, uint32 inHeight);                // G버퍼 · 깊이 · 출력 텍스처 · 합성 머터리얼
	void Render(const PScene& inScene, HSceneCameraID inCamera);     // 지오메트리 패스 → 합성 패스
	PSharedPtr<IRawTexture> GetOutputTexture() const;
	PSharedPtr<IRawTexture> GetGBufferTexture(uint32 inIndex) const; // DevScene 리드백 검증용
};
```

## GameFrameWorks에서 쓰는 모습 (GFW 2-1 · 2-2가 구현한다)
```cpp
// PWorld가 PScene을 하나 가진다: PWorld::GetScene()
void JGStaticMeshComponent::OnBeginPlay()
{
	HSceneMesh mesh;
	mesh.Mesh        = _meshAsset->GetMesh();
	mesh.WorldMatrix = GetOwner()->GetWorldMatrix();
	_meshID = GetWorld()->GetScene()->CreateMesh(mesh);
}
void JGStaticMeshComponent::OnTick(float32) { GetWorld()->GetScene()->SetMeshWorldMatrix(_meshID, GetOwner()->GetWorldMatrix()); }
void JGStaticMeshComponent::OnEndPlay()     { GetWorld()->GetScene()->DestroyMesh(_meshID); }
// JGCameraComponent도 같다 (CreateCamera / SetCamera / DestroyCamera).
// 어느 카메라로 그릴지(활성 카메라)는 GFW가 정해서 PSceneRenderer::Render에 넘긴다.
```

## 방안별 판단
| 판단 | 권장 | 이유 | 다른 안의 문제 |
|---|---|---|---|
| 메시 · 머터리얼을 장면이 소유할지 | **참조만** | 같은 메시 · 머터리얼을 여러 배치와 여러 장면(에디터 미리보기, 게임)이 나눠 쓰고 수명은 에셋이 정한다 | 장면이 소유하면 지울 때마다 다른 배치가 쓰는지 따져야 한다. 지금 서브메시 머터리얼은 약참조(`StaticMesh.h:29`)라 주인이 없는데, 배치가 강참조로 들면 해결된다 |
| 구성물을 다루는 방법 | **ID + `PScene` 함수** | 컴포넌트가 ID만 들면 장면 · 컴포넌트 중 어느 쪽이 먼저 없어져도 조회가 실패할 뿐이다 | 객체 강참조는 GC 강제 파괴 순서 문제(종료 크래시 이력), 약참조는 5-18(약참조가 남은 채 카운터 해제) 문제를 안는다 |
| 그리기를 어디에 둘지 | **`PSceneRenderer` 분리** | GFW가 패스를 몰라도 되고, DevScene에만 있는 G버퍼 · 합성 코드(약 200줄)를 재사용할 곳으로 옮긴다(5-22) | `PScene::Render`로 두면 데이터와 패스가 한 클래스에 섞여 GFW가 렌더 설정까지 보게 된다 |
| 카메라 타입 | **`HSceneCamera`(값)** | 장면이 값으로 들고 밖에서는 ID로만 다룬다. `P` 접두는 `IMemoryObject` 클래스 규칙 | `PCamera` 클래스로 두면 카메라마다 GC 객체가 생기는데 얻는 것이 없다. 스텁 `Classes/Camera.h`는 지운다 |

## 이름 (결정 필요)
- 새 타입: `PScene`, `HSceneMesh`, `HSceneCamera`, `HSceneMeshID`, `HSceneCameraID`, `PSceneRenderer`. "장면에 놓인 ~"으로 읽힌다. `HSceneMesh`가 에셋 메시와 헷갈리면 `HSceneMeshInstance`.
- **충돌:** 기존 `EMaterialDomain::Scene`(`Material.h:12`)과 `HSceneDrawArguments`(`JGGraphicsDefine.h:447`)는 "화면 전체에 그리는 머터리얼"이다. `PScene`이 생기면 Scene이 두 뜻이 된다. 후보는 `Screen`(권장, 화면 공간이라는 뜻 그대로) 또는 `PostProcess`(합성 같은 용도에는 맞지만 첫 패스 이름으로는 어색). 바꾸면 `EMaterialDomain::Screen`, `HScreenDrawArguments`가 된다.

## 넣지 않는 것
조명(지금은 합성 셰이더의 고정 방향), 컬링 · 정렬 · 배칭, 변경 추적, 멀티스레드, 장면 저장(월드 · 레벨이 할 일), 스켈레탈 메시. 지금 이것 때문에 막힌 곳이 없다.

## 진행 순서
| 단계 | 내용 | 확인 |
|---|---|---|
| 1 | `PScene` · `HSceneCamera` · `HSceneMesh` · `PSceneRenderer`를 추가하고 DevScene을 이것으로 바꾼다(5-7 + 5-22). 새 파일이라 PreBuild(JGBuildTool)가 필요하다 | 캡처와 리드백(덮인 픽셀 65,587개)이 이전과 같음, 오류 0, PSO 2 |
| 2 | 5-26: 정점 CPU 사본을 버퍼에서 에셋으로, 경계 상자를 임포트 때 계산해 저장 | 메시 로드 · 경계 상자 · 저장 결과가 같음 |
| 3 | GFW 2-1 · 2-2(`JGCameraComponent`, `JGStaticMeshComponent`, `PWorld::GetScene`) | GFW 트랙에서 |

## 근거 위치
- 지금 카메라 · 패스가 있는 곳: `Devkit/DevScene.h:32-45`(G버퍼 · 카메라 멤버), `DevScene.cpp:210` `CreateRenderTargets`, `:279` `CreateCompositeMaterial`, `:378` `FitCameraToMesh`, `:406` `FillRenderPassData`, `:438` `RenderGeometryPass`, `:474` `RenderCompositePass`.
- 스텁: `Graphics/Classes/Scene.h`(구현 없는 `AddMaterial/RemoveMaterial`), `Classes/Camera.h`(빈 클래스).
- GFW: `Core/World.h`(월드가 액터 소유), `Components/ActorComponent.h:33-37`(`OnBeginPlay/OnTick/OnEndPlay`), `Actors/Actor.h:56`(`GetWorldMatrix`), `Core/GameInstance.h:19`(월드 소유), `Document/GameFrameWorks_TODO.md:32-33`(2-1 · 2-2가 이 API를 기다림). GFW 모듈은 이미 Graphics에 의존한다(`GameFrameWorks.module.json`).
