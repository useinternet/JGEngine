#include "PCH/PCH.h"
#include "Core/WorldSelfTest.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Actors/GameMasterActor.h"
#include "Actors/GameplayEntityActor.h"
#include "Actors/GameplayControllerActor.h"
#include "Actors/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CameraComponent.h"
#include "Components/PickShapeComponent.h"
#include "Core/GameplayBoardLayout.h"
#include "GameMaster/GameMaster.h"
#include "Classes/Scene.h"
#include "Classes/Mesh.h"
#include <iostream>

namespace
{
	struct HWorldCheck
	{
		int32 Failures = 0;
		int32 Passed   = 0;

		void operator()(bool bCondition, const PString& what)
		{
			if (bCondition == true)
			{
				++Passed;
				std::cout << "  [PASS] " << what.GetRawString() << std::endl;
				return;
			}
			++Failures;
			std::cout << "  [FAIL] " << what.GetRawString() << std::endl;
			JG_LOG(WorldSelfTest, ELogLevel::Error, "[FAIL] %s", what);
		}
	};

	// 대상 엔티티를 파괴하는 최소 명령. 엔티티 파괴가 액터 파괴로 이어지는지 보기 위한 것.
	class PWorldTestKillHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("WorldTestKill");
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			if (state.IsAlive(command.Target()) == false)
			{
				if (outReason != nullptr)
				{
					*outReason = "invalid target";
				}
				return false;
			}
			return true;
		}

		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayEffectRequest destroy(PName(HGameplayBuiltin::EffectDestroyEntity), command.Actor, command.Target());
			ctx.Enqueue(destroy);
		}
	};

	// 엔티티를 만들고 같은 명령 안에서 곧바로 파괴한다. 한 프레임에 EntitySpawned · EntityDestroyed 가 함께 온다 (R15).
	class PWorldTestSpawnAndKillHandler : public JGGameplayCommandHandler
	{
	public:
		virtual PName GetKind() const override
		{
			return PName("WorldTestSpawnAndKill");
		}

		virtual bool Validate(const HGameplayState& state, const HGameplayCommand& command, PString* outReason) const override
		{
			return true;
		}

		virtual void Execute(HGameplayContext& ctx, const HGameplayCommand& command) override
		{
			HGameplayEntityId id = ctx.SpawnEntity(PName("Units"));
			ctx.DestroyEntity(id);
		}
	};

	// 틱 안에서 액터를 스폰하고 같은 틱에 파괴한다 (R11).
	class PWorldTestSpawnerActor : public JGActor
	{
	public:
		PSharedPtr<JGActor> Spawned;

	protected:
		virtual void OnTick(float32 deltaSeconds) override
		{
			if (Spawned != nullptr)
			{
				return;
			}
			Spawned = GetWorld()->SpawnActor<JGActor>(PName("WorldTestZombie"));
			Spawned->Destroy();
		}
	};

	// 파괴 이벤트의 큐를 물어보는 순간 그 엔티티의 액터를 찾을 수 있는지 센다 (R13).
	class PWorldTestDeathCue : public JGGameplayCue
	{
	public:
		JGGameMasterActor* Owner = nullptr;
		mutable int32 Asked = 0;
		mutable int32 Found = 0;

		virtual bool Accepts(const HGameplayEvent& event) const override
		{
			if (event.Kind != PName(HGameplayBuiltin::EventEntityDestroyed))
			{
				return false;
			}
			++Asked;
			if (Owner != nullptr && Owner->FindActor(event.Subject) != nullptr)
			{
				++Found;
			}
			return true;
		}
	};

	// 그래픽 없이 쓰는 렌더 메시. PScene 은 메시를 참조만 하고 그리지 않으므로 헤드리스에서 배치 수명을 볼 수 있다.
	class PWorldTestMesh : public IMesh
	{
		PName        _name = PName("WorldTestMesh");
		HInputLayout _inputLayout;

	public:
		virtual const PName& GetName() const override
		{
			return _name;
		}

		virtual void SetName(const PName& inName) override
		{
			_name = inName;
		}

		virtual uint32 GetSubMeshCount() const override
		{
			return 0;
		}

		virtual PSharedPtr<IVertexBuffer> GetVertexBuffer(uint32 inSubMeshIndex) const override
		{
			return nullptr;
		}

		virtual PSharedPtr<IIndexBuffer> GetIndexBuffer(uint32 inSubMeshIndex) const override
		{
			return nullptr;
		}

		virtual PSharedPtr<IRawMaterial> GetMaterial(uint32 inSubMeshIndex) const override
		{
			return nullptr;
		}

		virtual const HInputLayout& GetInputLayout() const override
		{
			return _inputLayout;
		}

		virtual bool IsValid() const override
		{
			return true;
		}

		virtual void Reset() override
		{
		}
	};

	// 클릭이 OnClick 까지 오는지 센다.
	class PWorldTestController : public JGGameplayControllerActor
	{
	public:
		int32               Clicks = 0;
		HGameplayPickResult Last;

	protected:
		virtual void OnClick(const HGameplayPickResult& pick) override
		{
			++Clicks;
			Last = pick;
		}
	};

	bool nearlyEqual(float32 a, float32 b, float32 tolerance = 0.001f)
	{
		return HMath::Abs(a - b) <= tolerance;
	}

	bool nearlyEqual(const HVector3& a, const HVector3& b, float32 tolerance = 0.001f)
	{
		return nearlyEqual(a.x, b.x, tolerance) && nearlyEqual(a.y, b.y, tolerance) && nearlyEqual(a.z, b.z, tolerance);
	}

	HVector3 matrixTranslation(const HMatrix& m)
	{
		// 행 벡터 규약이라 이동 성분은 4행이다. Get_C(열, 행).
		return HVector3(m.Get_C(0, 3), m.Get_C(1, 3), m.Get_C(2, 3));
	}

	HRay makeRay(const HVector3& origin, const HVector3& dir)
	{
		HRay ray;
		ray.origin = origin;
		ray.dir    = HVector3::Normalize(dir);
		return ray;
	}

	int32 countEntityActors(PSharedPtr<PWorld> world)
	{
		HList<PSharedPtr<JGGameplayEntityActor>> actors;
		world->FindActors<JGGameplayEntityActor>(actors);
		return (int32)actors.size();
	}

	void tickFrames(PSharedPtr<PWorld> world, int32 frames)
	{
		for (int32 i = 0; i < frames; ++i)
		{
			world->Tick(1.0f / 60.0f);
		}
	}

	// 2-2: 월드의 장면과 정적 메시 컴포넌트의 배치 수명
	void runSceneChecks(HWorldCheck& check)
	{
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("SceneTestWorld"));
		PSharedPtr<PScene> scene = world->GetScene();
		check(scene != nullptr && world->GetScene() == scene, "2-2 the world owns one scene");

		// BeginPlay 전에 메시를 준 액터는 BeginPlay 에서 배치된다
		PSharedPtr<JGActor> early = world->SpawnActor<JGActor>(PName("EarlyMeshActor"));
		PSharedPtr<JGStaticMeshComponent> earlyMesh = early->AddComponent<JGStaticMeshComponent>();
		earlyMesh->SetMesh(Allocate<PWorldTestMesh>());
		check(earlyMesh->GetSceneMeshID().IsValid() == false, "2-2 no scene mesh before BeginPlay");
		world->BeginPlay();
		check(earlyMesh->GetSceneMeshID().IsValid() == true && scene->FindMesh(earlyMesh->GetSceneMeshID()) != nullptr, "2-2 BeginPlay places the mesh in the scene");

		// BeginPlay 뒤에 메시를 주면 바로 배치된다. 메시가 없으면 배치도 없다
		PSharedPtr<JGActor> actor = world->SpawnActor<JGActor>(PName("MeshActor"));
		PSharedPtr<JGStaticMeshComponent> mesh = actor->AddComponent<JGStaticMeshComponent>();
		check(mesh->GetSceneMeshID().IsValid() == false, "2-2 a mesh component without a mesh has no scene mesh");
		mesh->SetMesh(Allocate<PWorldTestMesh>());
		const HSceneMeshID meshID = mesh->GetSceneMeshID();
		check(meshID.IsValid() == true && scene->FindMesh(meshID) != nullptr, "2-2 SetMesh after BeginPlay places the mesh at once");
		check(scene->GetMeshes().size() == 2, PString::Format("2-2 the scene holds 2 meshes, got %d", (int32)scene->GetMeshes().size()));

		// 틱마다 소유 액터의 월드 행렬 (부모 포함)
		PSharedPtr<JGActor> parent = world->SpawnActor<JGActor>(PName("MeshParent"));
		parent->SetLocalPosition(HVector3(10.0f, 0.0f, 0.0f));
		actor->AttachTo(parent);
		actor->SetLocalPosition(HVector3(3.0f, 4.0f, 5.0f));
		tickFrames(world, 1);
		const HSceneMesh* placed = scene->FindMesh(meshID);
		const HVector3 placedPosition = placed != nullptr ? matrixTranslation(placed->WorldMatrix) : HVector3();
		check(placed != nullptr && nearlyEqual(placedPosition, HVector3(13.0f, 4.0f, 5.0f)),
			PString::Format("2-2 tick copies the world matrix (%.1f, %.1f, %.1f) == (13, 4, 5)", placedPosition.x, placedPosition.y, placedPosition.z));

		// 메시를 바꾸면 배치를 새로 만든다
		mesh->SetMesh(Allocate<PWorldTestMesh>());
		const HSceneMeshID replacedID = mesh->GetSceneMeshID();
		check(replacedID != meshID && scene->FindMesh(meshID) == nullptr && scene->FindMesh(replacedID) != nullptr, "2-2 replacing the mesh replaces the scene mesh");

		// 부모를 파괴하면 자식도 파괴되고 배치가 지워진다
		parent->Destroy();
		check(scene->FindMesh(replacedID) == nullptr, "2-2 destroying the actor (through its parent) removes its scene mesh");

		// 컴포넌트만 떼어도 배치가 지워진다
		PSharedPtr<JGActor> other = world->SpawnActor<JGActor>(PName("DetachMeshActor"));
		PSharedPtr<JGStaticMeshComponent> otherMesh = other->AddComponent<JGStaticMeshComponent>();
		otherMesh->SetMesh(Allocate<PWorldTestMesh>());
		const HSceneMeshID otherID = otherMesh->GetSceneMeshID();
		other->RemoveComponent(otherMesh);
		check(otherID.IsValid() == true && scene->FindMesh(otherID) == nullptr, "2-2 removing the component removes its scene mesh");

		// EndPlay 는 남은 배치를 모두 지운다
		world->EndPlay();
		check(scene->GetMeshes().empty() == true, PString::Format("2-2 EndPlay leaves no scene mesh, got %d", (int32)scene->GetMeshes().size()));
	}

	// 2-1: 카메라 컴포넌트 수명 · 활성 카메라 · 고정/궤도 모드 · 뷰포트 광선
	void runCameraChecks(HWorldCheck& check)
	{
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("CameraTestWorld"));
		PSharedPtr<PScene> scene = world->GetScene();
		world->BeginPlay();

		PSharedPtr<JGCameraActor>     first       = world->SpawnActor<JGCameraActor>(PName("FirstCamera"));
		PSharedPtr<JGCameraComponent> firstCamera = first->GetCameraComponent();
		check(firstCamera != nullptr && firstCamera->GetSceneCameraID().IsValid() == true && scene->FindCamera(firstCamera->GetSceneCameraID()) != nullptr,
			"2-1 a camera actor registers a scene camera");
		check(world->GetActiveCamera() == firstCamera && firstCamera->IsActive() == true, "2-1 the first camera becomes the active camera");

		PSharedPtr<JGCameraActor>     second       = world->SpawnActor<JGCameraActor>(PName("SecondCamera"));
		PSharedPtr<JGCameraComponent> secondCamera = second->GetCameraComponent();
		check(world->GetActiveCamera() == firstCamera, "2-1 a later camera does not take over by itself");
		secondCamera->Activate();
		check(world->GetActiveCamera() == secondCamera && firstCamera->IsActive() == false, "2-1 Activate switches the active camera");

		// 고정 모드: 액터 트랜스폼이 곧 카메라다. 틱에 장면 카메라로 옮겨진다
		first->SetLocalPosition(HVector3(1.0f, 2.0f, 3.0f));
		tickFrames(world, 1);
		const HSceneCamera* fixedCamera = scene->FindCamera(firstCamera->GetSceneCameraID());
		check(fixedCamera != nullptr && nearlyEqual(fixedCamera->Position, HVector3(1.0f, 2.0f, 3.0f)), "2-1 fixed mode follows the actor transform");

		// 궤도 모드: 대상 (0, 0, 0), yaw 0, pitch 45°, 거리 10 → (0, 7.071, -7.071)
		second->SetOrbit(HVector3(0.0f, 0.0f, 0.0f), 0.0f, HMath::ConvertToRadians(45.0f), 10.0f);
		tickFrames(world, 1);
		const HSceneCamera* orbitCamera   = scene->FindCamera(secondCamera->GetSceneCameraID());
		const HVector3      orbitPosition = orbitCamera != nullptr ? orbitCamera->Position : HVector3();
		check(orbitCamera != nullptr && nearlyEqual(orbitPosition, HVector3(0.0f, 7.0711f, -7.0711f), 0.01f),
			PString::Format("2-1 orbit places the camera at (%.3f, %.3f, %.3f) == (0, 7.071, -7.071)", orbitPosition.x, orbitPosition.y, orbitPosition.z));

		// 화면 가운데 광선은 궤도 대상을 향한다
		const float32  aspect    = 16.0f / 9.0f;
		const HRay     centerRay = secondCamera->ViewportPointToRay(HVector2(0.5f, 0.5f), aspect);
		const HVector3 toTarget  = HVector3::Normalize(HVector3(0.0f, 0.0f, 0.0f) - orbitPosition);
		check(nearlyEqual(centerRay.origin, orbitPosition, 0.01f) && nearlyEqual(centerRay.dir, toTarget), "2-1 the viewport center ray starts at the camera and points at the orbit target");

		// 화면 오른쪽 끝(세로 가운데)의 광선은 가로 시야각 절반만큼 +X 쪽으로 벌어진다
		const HRay    edgeRay   = secondCamera->ViewportPointToRay(HVector2(1.0f, 0.5f), aspect);
		const float32 edgeX     = tanf(secondCamera->GetFovY() * 0.5f) * aspect;
		const float32 expectCos = 1.0f / sqrtf(edgeX * edgeX + 1.0f);
		check(edgeRay.dir.x > 0.0f && nearlyEqual(HVector3::Dot(edgeRay.dir, toTarget), expectCos),
			PString::Format("2-1 the viewport edge ray opens by half the horizontal fov (cos %.4f == %.4f)", HVector3::Dot(edgeRay.dir, toTarget), expectCos));

		// 궤도 입력은 pitch · 거리를 범위 안으로 자른다
		second->AddOrbitInput(0.0f, HMath::ConvertToRadians(90.0f), 100000.0f);
		check(second->GetOrbitPitch() <= HMath::ConvertToRadians(89.0f) + 0.0001f && second->GetOrbitDistance() <= 100000.0f, "2-1 orbit input is clamped");

		// 활성 카메라를 파괴하면 활성 카메라가 비고 장면 카메라도 지워진다
		const HSceneCameraID secondID = secondCamera->GetSceneCameraID();
		second->Destroy();
		check(world->GetActiveCamera() == nullptr && scene->FindCamera(secondID) == nullptr, "2-1 destroying the active camera clears it and removes the scene camera");

		world->EndPlay();
	}

	// 2-5: 판정 모양 (상자 · 납작한 사각형 · 회전/스케일) 과 PWorld::PickActor
	void runPickChecks(HWorldCheck& check)
	{
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("PickTestWorld"));
		world->BeginPlay();

		PSharedPtr<JGActor> nearBox = world->SpawnActor<JGActor>(PName("NearBox"));
		nearBox->AddComponent<JGPickShapeComponent>()->SetLocalBox(HVector3(0.0f, 0.0f, 0.0f), HVector3(1.0f, 1.0f, 1.0f));
		PSharedPtr<JGActor> farBox = world->SpawnActor<JGActor>(PName("FarBox"));
		farBox->SetLocalPosition(HVector3(0.0f, 0.0f, 5.0f));
		farBox->AddComponent<JGPickShapeComponent>()->SetLocalBox(HVector3(0.0f, 0.0f, 0.0f), HVector3(1.0f, 1.0f, 1.0f));

		HWorldPickHit hit;
		bool bHit = world->PickActor(makeRay(HVector3(0.0f, 0.0f, -10.0f), HVector3(0.0f, 0.0f, 1.0f)), &hit);
		check(bHit == true && hit.Actor == nearBox && nearlyEqual(hit.Distance, 9.0f) && nearlyEqual(hit.Position, HVector3(0.0f, 0.0f, -1.0f)),
			PString::Format("2-5 the nearest box is picked (distance %.3f == 9)", hit.Distance));

		check(world->PickActor(makeRay(HVector3(5.0f, 0.0f, -10.0f), HVector3(0.0f, 0.0f, 1.0f)), &hit) == false, "2-5 a ray that misses every shape picks nothing");
		check(world->PickActor(makeRay(HVector3(0.0f, 0.0f, -10.0f), HVector3(0.0f, 0.0f, -1.0f)), &hit) == false, "2-5 shapes behind the ray origin are not picked");

		PSharedPtr<JGPickShapeComponent> nearShape = nearBox->FindComponent<JGPickShapeComponent>();
		nearShape->SetPickEnabled(false);
		bHit = world->PickActor(makeRay(HVector3(0.0f, 0.0f, -10.0f), HVector3(0.0f, 0.0f, 1.0f)), &hit);
		check(bHit == true && hit.Actor == farBox, "2-5 a disabled shape is skipped");
		nearShape->SetPickEnabled(true);

		// 두께 0 인 상자 = 납작한 사각형 (바닥에 놓인 카드 · 타일 같은 것)
		PSharedPtr<JGActor> flat = world->SpawnActor<JGActor>(PName("FlatRect"));
		flat->SetLocalPosition(HVector3(10.0f, 0.0f, 0.0f));
		flat->AddComponent<JGPickShapeComponent>()->SetLocalBox(HVector3(0.0f, 0.0f, 0.0f), HVector3(1.0f, 0.0f, 1.5f));
		bHit = world->PickActor(makeRay(HVector3(10.5f, 10.0f, 1.0f), HVector3(0.0f, -1.0f, 0.0f)), &hit);
		check(bHit == true && hit.Actor == flat && nearlyEqual(hit.Distance, 10.0f), "2-5 a zero-thickness rectangle is picked from above");
		check(world->PickActor(makeRay(HVector3(10.5f, 10.0f, 2.0f), HVector3(0.0f, -1.0f, 0.0f)), &hit) == false, "2-5 a ray outside the rectangle misses it");

		// 액터의 회전 · 스케일을 따라간다: yaw 45°, 스케일 2 → 모서리까지 2√2 ≈ 2.83
		// x = 2.5 는 돌리지 않은 2배 상자(반폭 2) 밖이지만 돈 상자 안이다. 맞는 z = 20 - (2√2 - 2.5)
		PSharedPtr<JGActor> turned = world->SpawnActor<JGActor>(PName("TurnedBox"));
		turned->SetLocalTransform(HTransform(HVector3(0.0f, 0.0f, 20.0f), HQuaternion::ToQuaternion(0.0f, HMath::ConvertToRadians(45.0f), 0.0f), HVector3(2.0f, 2.0f, 2.0f)));
		turned->AddComponent<JGPickShapeComponent>()->SetLocalBox(HVector3(0.0f, 0.0f, 0.0f), HVector3(1.0f, 1.0f, 1.0f));
		bHit = world->PickActor(makeRay(HVector3(2.5f, 0.0f, 10.0f), HVector3(0.0f, 0.0f, 1.0f)), &hit);
		const float32 expectTurned = 10.0f - (2.0f * 1.41421356f - 2.5f);
		check(bHit == true && hit.Actor == turned && nearlyEqual(hit.Distance, expectTurned, 0.01f),
			PString::Format("2-5 the shape follows the actor rotation and scale (distance %.3f == %.3f)", hit.Distance, expectTurned));

		world->EndPlay();
	}

	// 2-5: 보드 배치 (칸 좌표 ↔ 월드 위치 · 보드 평면 피킹)
	void runBoardLayoutChecks(HWorldCheck& check)
	{
		const HGameplayBoardLayout square(EGameplayBoardKind::Square, HVector3(10.0f, 1.0f, 20.0f), 2.0f);
		HGameplayCoord coord;
		HVector3       position;

		check(nearlyEqual(square.CoordToWorld(HGameplayCoord(3, 4)), HVector3(16.0f, 1.0f, 28.0f)), "2-5 square cell (3, 4) center is (16, 1, 28)");
		check(square.WorldToCoord(HVector3(16.9f, 0.0f, 27.1f), &coord) == true && coord == HGameplayCoord(3, 4), "2-5 a point inside a square cell maps to it");
		check(square.WorldToCoord(HVector3(17.1f, 0.0f, 28.0f), &coord) == true && coord == HGameplayCoord(4, 4), "2-5 past the square cell edge maps to the neighbor");
		check(square.WorldToCoord(HVector3(8.5f, 0.0f, 19.5f), &coord) == true && coord == HGameplayCoord(-1, 0), "2-5 negative square coordinates round down");

		const HGameplayBoardLayout hex(EGameplayBoardKind::Hex, HVector3(0.0f, 0.0f, 0.0f), 1.0f);
		bool bRoundTrip = true;
		for (int32 q = -4; q <= 4; ++q)
		{
			for (int32 r = -4; r <= 4; ++r)
			{
				HGameplayCoord back;
				if (hex.WorldToCoord(hex.CoordToWorld(HGameplayCoord(q, r)), &back) == false || back != HGameplayCoord(q, r))
				{
					bRoundTrip = false;
				}
			}
		}
		check(bRoundTrip == true, "2-5 hex cell centers map back to their cells (q, r in -4..4)");

		// PGameplayBoardHex::Neighbors 와 같은 여섯 방향
		const int32 dq[6] = { 1, 1, 0, -1, -1, 0 };
		const int32 dr[6] = { 0, -1, -1, 0, 1, 1 };
		const HGameplayCoord center(2, -1);
		const HVector3 centerPosition = hex.CoordToWorld(center);
		bool bNeighborDistance = true;
		bool bBoundary         = true;
		for (int32 i = 0; i < 6; ++i)
		{
			const HGameplayCoord neighbor(center.X + dq[i], center.Y + dr[i]);
			const HVector3 toNeighbor = hex.CoordToWorld(neighbor) - centerPosition;
			if (nearlyEqual(HVector3::Length(toNeighbor), 1.0f) == false)
			{
				bNeighborDistance = false;
			}

			// 두 칸의 경계는 중심 사이 절반이다
			HGameplayCoord inside;
			HGameplayCoord outside;
			hex.WorldToCoord(centerPosition + toNeighbor * 0.45f, &inside);
			hex.WorldToCoord(centerPosition + toNeighbor * 0.55f, &outside);
			if (inside != center || outside != neighbor)
			{
				bBoundary = false;
			}
		}
		check(bNeighborDistance == true, "2-5 all six hex neighbors are one CellSize away");
		check(bBoundary == true, "2-5 hex cell boundaries are halfway between neighbor centers");

		check(square.PickCoord(makeRay(HVector3(16.2f, 11.0f, 27.8f), HVector3(0.0f, -1.0f, 0.0f)), &coord, &position) == true
			&& coord == HGameplayCoord(3, 4) && nearlyEqual(position, HVector3(16.2f, 1.0f, 27.8f)), "2-5 a downward ray picks the square cell under it on the board plane");
		check(square.PickCoord(makeRay(HVector3(16.0f, 11.0f, 28.0f), HVector3(1.0f, 0.0f, 0.0f)), &coord, nullptr) == false, "2-5 a ray parallel to the board picks no cell");
		check(square.PickCoord(makeRay(HVector3(16.0f, 11.0f, 28.0f), HVector3(0.0f, 1.0f, 0.0f)), &coord, nullptr) == false, "2-5 a board behind the ray picks no cell");
		check(HGameplayBoardLayout().PickCoord(makeRay(HVector3(0.0f, 5.0f, 0.0f), HVector3(0.0f, -1.0f, 0.0f)), &coord, nullptr) == false, "2-5 without a board layout no cell is picked");
	}

	// 2-5: 컨트롤러 피킹 — 판정 모양 → 엔티티, 보드 평면 → 칸 (범위는 GameMaster 보드), 카메라 광선 → HandleClick → OnClick
	void runControllerPickChecks(HWorldCheck& check)
	{
		PSharedPtr<PWorld> world = Allocate<PWorld>(PName("ControllerPickTestWorld"));
		world->BeginPlay();

		PSharedPtr<JGGameMasterActor> gameMasterActor = world->SpawnActor<JGGameMasterActor>(PName("PickGameMasterActor"));
		PSharedPtr<PGameMaster> gameMaster = Allocate<PGameMaster>();
		gameMaster->RegisterZone(PName("Units"));
		gameMaster->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName("Units")));
		gameMaster->SetBoard(EGameplayBoardKind::Square, 4, 4);

		HGameplayState& initial = gameMaster->EditInitialState();
		const HGameplayEntityId unit = initial.CreateEntity();
		initial.Zone(PName("Units")).PushBack(unit);
		initial.SetBoardPosition(unit, HGameplayCoord(2, 1));

		gameMasterActor->SetGameMaster(gameMaster);
		gameMaster->Start(99);
		const HGameplayBoardLayout layout(EGameplayBoardKind::Square, HVector3(0.0f, 0.0f, 0.0f), 1.0f);
		gameMasterActor->SetBoardLayout(layout);
		tickFrames(world, 3);

		// 엔티티 액터를 칸 위에 세우고 판정 모양을 붙인다 (게임이 SpawnActorForEntity 에서 할 일)
		PSharedPtr<JGActor> unitActor = gameMasterActor->FindActor(unit);
		check(unitActor != nullptr, "2-5 the unit has an entity actor");
		if (unitActor == nullptr)
		{
			world->EndPlay();
			return;
		}
		unitActor->SetLocalPosition(layout.CoordToWorld(HGameplayCoord(2, 1)));
		unitActor->AddComponent<JGPickShapeComponent>()->SetLocalBox(HVector3(0.0f, 0.5f, 0.0f), HVector3(0.3f, 0.5f, 0.3f));

		PSharedPtr<PWorldTestController> controller = world->SpawnActor<PWorldTestController>(PName("PickController"));
		controller->SetGameMasterActor(gameMasterActor);

		HGameplayPickResult pick;
		bool bPicked = controller->Pick(makeRay(HVector3(2.0f, 10.0f, 1.0f), HVector3(0.0f, -1.0f, 0.0f)), &pick);
		check(bPicked == true && pick.bHitActor == true && pick.Entity == unit && pick.bHitBoard == true && pick.Coord == HGameplayCoord(2, 1),
			PString::Format("2-5 picking the unit returns its entity and cell (%s)", pick.ToString()));

		bPicked = controller->Pick(makeRay(HVector3(0.1f, 10.0f, 3.2f), HVector3(0.0f, -1.0f, 0.0f)), &pick);
		check(bPicked == true && pick.bHitActor == false && pick.bHitBoard == true && pick.Coord == HGameplayCoord(0, 3), "2-5 an empty cell returns only the cell");

		bPicked = controller->Pick(makeRay(HVector3(10.0f, 10.0f, 10.0f), HVector3(0.0f, -1.0f, 0.0f)), &pick);
		check(bPicked == false && pick.bHitBoard == false, "2-5 outside the 4x4 board returns no cell");

		// 판정 모양이 자식 액터에 있어도 부모 쪽 엔티티로 올라간다
		PSharedPtr<JGActor> attachment = world->SpawnActor<JGActor>(PName("UnitAttachment"));
		attachment->AttachTo(unitActor);
		attachment->SetLocalPosition(HVector3(0.0f, 2.0f, 0.0f));
		attachment->AddComponent<JGPickShapeComponent>()->SetLocalBox(HVector3(0.0f, 0.0f, 0.0f), HVector3(0.2f, 0.2f, 0.2f));
		bPicked = controller->Pick(makeRay(HVector3(-5.0f, 2.0f, 1.0f), HVector3(1.0f, 0.0f, 0.0f)), &pick);
		check(bPicked == true && pick.Actor == attachment && pick.Entity == unit, "2-5 a shape on a child actor resolves to the parent's entity");

		// 카메라 → 뷰포트 광선 → 클릭 → OnClick (씬 뷰포트가 하는 일)
		PSharedPtr<JGCameraActor> cameraActor = world->SpawnActor<JGCameraActor>(PName("PickCamera"));
		cameraActor->SetOrbit(layout.CoordToWorld(HGameplayCoord(0, 3)), 0.0f, HMath::ConvertToRadians(60.0f), 8.0f);
		tickFrames(world, 1);
		const HRay clickRay = cameraActor->GetCameraComponent()->ViewportPointToRay(HVector2(0.5f, 0.5f), 16.0f / 9.0f);
		controller->HandleClick(clickRay);
		check(controller->Clicks == 1 && controller->HasLastPick() == true && controller->Last.bHitBoard == true && controller->Last.Coord == HGameplayCoord(0, 3),
			PString::Format("2-5 a click through the camera center reaches OnClick with cell (0, 3) (%s)", controller->Last.ToString()));

		world->EndPlay();
	}

	// 게임 인스턴스: LoadWorld · GameMasterActor 바인딩 · 입력 버퍼링 · 되돌리기 · UnloadWorld. 모듈의 게임 인스턴스를 쓴다.
	void runGameInstanceChecks(HWorldCheck& check, JGGameInstance& gameInstance)
	{
		PSharedPtr<PWorld> world = gameInstance.LoadWorld(PName("SelfTestWorld"));
		check(world != nullptr && gameInstance.GetWorld() == world, "LoadWorld creates the active world");
		check(world != nullptr && world->HasBegun() == true, "loaded world has begun play");

		// GameMasterActor + GameMaster
		PSharedPtr<JGGameMasterActor> gameMasterActor = world->SpawnActor<JGGameMasterActor>(PName("GameMasterActor"));
		check(gameMasterActor != nullptr && gameMasterActor->GetWorld() == world, "gameMasterActor spawned into the world");

		PSharedPtr<PGameMaster> sim = Allocate<PGameMaster>();
		sim->RegisterZone(PName("Units"));
		sim->RegisterHandler(Allocate<PWorldTestKillHandler>());
		sim->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName("Units")));

		HGameplayState& initial = sim->EditInitialState();
		HList<HGameplayEntityId> units;
		for (int32 i = 0; i < 3; ++i)
		{
			HGameplayEntityId id = initial.CreateEntity();
			initial.Zone(PName("Units")).PushBack(id);
			units.push_back(id);
		}

		gameMasterActor->SetGameMaster(sim);
		sim->Start(777);

		tickFrames(world, 5);
		check(gameMasterActor->IsBusy() == false, "gameMasterActor drained the start events");
		check(countEntityActors(world) == 3, PString::Format("3 entity actors bound after start, got %d", countEntityActors(world)));
		check(gameMasterActor->FindActor(units[0]) != nullptr, "entity id resolves to an actor");

		PSharedPtr<JGGameplayEntityActor> bound = RawDynamicCast<JGGameplayEntityActor>(gameMasterActor->FindActor(units[1]));
		check(bound != nullptr && bound->GetEntityId() == units[1], "bound actor carries its entity id");

		// 명령 → 파괴 이벤트 → 액터 제거
		HGameplayCommand kill(PName("WorldTestKill"), units[0]);
		kill.Targets.push_back(units[2]);
		PString reason;
		EGameMasterActorSubmit result = gameMasterActor->Submit(kill, &reason);
		check(result == EGameMasterActorSubmit::Executed, PString::Format("gameMasterActor executed the command (%s)", reason));

		tickFrames(world, 5);
		check(countEntityActors(world) == 2, PString::Format("entity actor removed after EntityDestroyed, got %d", countEntityActors(world)));
		check(gameMasterActor->FindActor(units[2]) == nullptr, "destroyed entity no longer resolves");

		// 연출 중 입력 버퍼링: 이벤트가 큐에 있는 동안 제출하면 Buffered, 틱 후 실행
		HGameplayCommand endTurn(PName(HGameplayBuiltin::CommandEndTurn), units[0]);
		gameMasterActor->SetInputPolicy(EGameplayInputPolicy::Buffer);
		HGameplayCommand kill2(PName("WorldTestKill"), units[0]);
		kill2.Targets.push_back(units[1]);
		gameMasterActor->Submit(kill2, &reason);                                       // 이벤트가 큐에 들어간다 (아직 틱 전)
		EGameMasterActorSubmit buffered = gameMasterActor->Submit(endTurn, &reason);
		check(buffered == EGameMasterActorSubmit::Buffered, "command submitted while busy is buffered");
		tickFrames(world, 5);
		check(sim->GetState().Turn.TurnCount >= 2, "buffered command executed after the presentation drained");

		// 되돌리기 → 상태 교체 → 바인딩 재구성
		int32 before = countEntityActors(world);
		sim->Undo();
		sim->Undo();
		tickFrames(world, 5);
		check(countEntityActors(world) == before + 1, PString::Format("bindings rebuilt after undo (%d -> %d)", before, countEntityActors(world)));

		// 언로드
		gameInstance.UnloadWorld();
		check(gameInstance.GetWorld() == nullptr && world->HasBegun() == false, "UnloadWorld ends play and clears the active world");
	}
}

int32 PWorldSelfTest::Run()
{
	HWorldCheck check;
	std::cout << "== World self test ==" << std::endl;

	check(JGGameInstance::HasInstance() == true, "game instance exists after module startup");
	if (JGGameInstance::HasInstance() == false)
	{
		return check.Failures;
	}

	JGGameInstance& gameInstance = JGGameInstance::Get();

	// 게임 인스턴스 절은 모듈의 게임 인스턴스로 월드를 로드 · 언로드한다. 이미 월드가 있으면(프로젝트 모드 에디터에서 게임이 도는 중)
	// 그 월드를 내리고 게임의 엔트리 액터를 시험 월드에 스폰하게 되므로 건너뛴다. 나머지 절은 별도 월드를 쓴다.
	if (gameInstance.HasWorld() == true)
	{
		const PString worldName = gameInstance.GetWorld()->GetName().ToString();
		std::cout << "  [SKIP] game instance section: world " << worldName.GetRawString() << " is already loaded" << std::endl;
		JG_LOG(WorldSelfTest, ELogLevel::Warning, "game instance section skipped: world %s is already loaded", worldName);
	}
	else
	{
		runGameInstanceChecks(check, gameInstance);
	}

	// 리뷰 재현 회귀 (R11 · R12 · R13 · R15). 게임 인스턴스와 상관없는 별도 월드에서 본다.
	{
		PSharedPtr<PWorld> reviewWorld = Allocate<PWorld>(PName("ReviewRegressionWorld"));
		reviewWorld->BeginPlay();

		// R11: 틱 안에서 스폰하고 같은 틱에 파괴한 액터는 월드에 들어가지 않는다
		PSharedPtr<PWorldTestSpawnerActor> spawner = reviewWorld->SpawnActor<PWorldTestSpawnerActor>(PName("Spawner"));
		tickFrames(reviewWorld, 3);
		PSharedPtr<JGActor> zombie = spawner->Spawned;
		bool bZombieInWorld = false;
		for (const PSharedPtr<JGActor>& actor : reviewWorld->GetActors())
		{
			if (actor == zombie)
			{
				bZombieInWorld = true;
			}
		}
		check(zombie != nullptr && bZombieInWorld == false && zombie->HasBegunPlay() == false, "R11 an actor spawned and destroyed in one tick never enters the world");

		// R12: 부모가 있는 액터의 월드 위치
		PSharedPtr<JGActor> parent = reviewWorld->SpawnActor<JGActor>(PName("Parent"));
		PSharedPtr<JGActor> child  = reviewWorld->SpawnActor<JGActor>(PName("Child"));
		parent->SetLocalPosition(HVector3(5.0f, 0.0f, 0.0f));
		child->SetLocalPosition(HVector3(1.0f, 2.0f, 3.0f));
		child->AttachTo(parent);
		HVector3 position = child->GetWorldPosition();
		check(position.x == 6.0f && position.y == 2.0f && position.z == 3.0f, PString::Format("R12 child world position (%.1f, %.1f, %.1f) == (6, 2, 3)", position.x, position.y, position.z));

		// R13 · R15 는 GameMasterActor 로 본다
		PSharedPtr<JGGameMasterActor> reviewMaster = reviewWorld->SpawnActor<JGGameMasterActor>(PName("ReviewGameMasterActor"));
		PSharedPtr<PGameMaster> reviewSim = Allocate<PGameMaster>();
		reviewSim->RegisterZone(PName("Units"));
		reviewSim->RegisterHandler(Allocate<PWorldTestKillHandler>());
		reviewSim->RegisterHandler(Allocate<PWorldTestSpawnAndKillHandler>());
		reviewSim->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName("Units")));

		HList<HGameplayEntityId> reviewUnits;
		HGameplayState& reviewInitial = reviewSim->EditInitialState();
		for (int32 i = 0; i < 2; ++i)
		{
			HGameplayEntityId id = reviewInitial.CreateEntity();
			reviewInitial.Zone(PName("Units")).PushBack(id);
			reviewUnits.push_back(id);
		}

		PSharedPtr<PWorldTestDeathCue> deathCue = Allocate<PWorldTestDeathCue>();
		deathCue->Owner = reviewMaster.GetRawPointer();
		reviewMaster->RegisterCue(deathCue);
		reviewMaster->SetGameMaster(reviewSim);
		reviewSim->Start(777);
		tickFrames(reviewWorld, 3);

		// R13: 사망 큐를 고르는 순간 액터가 아직 있고, 큐가 끝나면 없어진다
		bool bBoundBefore = reviewMaster->FindActor(reviewUnits[1]) != nullptr;
		HGameplayCommand reviewKill(PName("WorldTestKill"), reviewUnits[0]);
		reviewKill.Targets.push_back(reviewUnits[1]);
		PString reviewReason;
		reviewMaster->Submit(reviewKill, &reviewReason);
		tickFrames(reviewWorld, 3);
		check(bBoundBefore == true && deathCue->Asked == 1 && deathCue->Found == 1 && reviewMaster->FindActor(reviewUnits[1]) == nullptr,
			PString::Format("R13 the death cue finds its actor (asked %d, found %d) and the actor goes after the cue", deathCue->Asked, deathCue->Found));

		// R15: 한 프레임에 스폰 · 파괴가 함께 와도 좀비 액터가 남지 않는다
		reviewMaster->Submit(HGameplayCommand(PName("WorldTestSpawnAndKill"), reviewUnits[0]), &reviewReason);
		tickFrames(reviewWorld, 3);
		int32 zombies = 0;
		int32 entityActors = 0;
		for (const PSharedPtr<JGActor>& actor : reviewWorld->GetActors())
		{
			PSharedPtr<JGGameplayEntityActor> typed = RawDynamicCast<JGGameplayEntityActor>(actor);
			if (typed == nullptr)
			{
				continue;
			}
			++entityActors;
			if (typed->IsPendingDestroy() == true)
			{
				++zombies;
			}
		}
		check(zombies == 0 && entityActors == (int32)reviewSim->GetState().Entities.Count(),
			PString::Format("R15 spawn + destroy in one frame leaves no zombie (entity actors %d, alive entities %u)", entityActors, reviewSim->GetState().Entities.Count()));

		reviewWorld->EndPlay();
	}

	// Phase 2 (월드 · 연출): 장면 · 메시 · 카메라 · 피킹. 게임 인스턴스와 상관없는 별도 월드에서 본다.
	runSceneChecks(check);
	runCameraChecks(check);
	runPickChecks(check);
	runBoardLayoutChecks(check);
	runControllerPickChecks(check);

	std::cout << "== World self test: " << check.Passed << " passed, " << check.Failures << " failed ==" << std::endl;
	JG_LOG(WorldSelfTest, ELogLevel::Info, "World self test: %d passed, %d failed", check.Passed, check.Failures);
	return check.Failures;
}
