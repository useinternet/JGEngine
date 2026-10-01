#include "PCH/PCH.h"
#include "GfwViewEntryActor.h"
#include "Core/World.h"
#include "Core/GameInstance.h"
#include "Core/GameplayBoardLayout.h"
#include "Actors/CameraActor.h"
#include "Actors/GameMasterActor.h"
#include "Actors/GameplayControllerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PickShapeComponent.h"
#include "GameMaster/GameMaster.h"
#include "Classes/StaticMesh.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

// GameFrameWorks Phase 2 검증용 게임 코드 (엔진 밖, 스크래치패드 프로젝트).
// 6x6 사각 보드, 엔티티 4개(X Bot 메시 + 메시 경계 상자 판정), 칸 타일, 궤도 카메라, 컨트롤러.
namespace
{
	constexpr int32       GfwBoardSize    = 6;
	constexpr float32     GfwCellSize     = 200.0f;
	constexpr const char* GfwUnitMeshPath = "/JGEngine/TempAsset/Sample.jgasset";

	// 칸 타일: 한 칸보다 조금 작은 납작한 사각형 (양면).
	PSharedPtr<JGStaticMesh> makeTileMesh(float32 halfSize)
	{
		const HVector3 corners[4] = {
			HVector3(-halfSize, 0.0f,  halfSize),
			HVector3( halfSize, 0.0f,  halfSize),
			HVector3( halfSize, 0.0f, -halfSize),
			HVector3(-halfSize, 0.0f, -halfSize),
		};
		const HVector2 uvs[4] = { HVector2(0.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(1.0f, 1.0f), HVector2(0.0f, 1.0f) };

		HList<HVertex> vertices;
		for (int32 i = 0; i < 4; ++i)
		{
			HVertex vertex;
			vertex.Position  = corners[i];
			vertex.Texcoord  = uvs[i];
			vertex.Normal    = HVector3(0.0f, 1.0f, 0.0f);
			vertex.Tangent   = HVector3(1.0f, 0.0f, 0.0f);
			vertex.Bitangent = HVector3(0.0f, 0.0f, 1.0f);
			vertices.push_back(vertex);
		}

		HList<uint32> indices = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };

		HList<PName>          names        = { PName("Tile") };
		HList<HList<HVertex>> vertexLists  = { vertices };
		HList<HList<uint32>>  indexLists   = { indices };

		PSharedPtr<JGStaticMesh> mesh = Allocate<JGStaticMesh>();
		mesh->SetData(names, vertexLists, indexLists);
		return mesh;
	}

	// 게임이 정하는 컴포넌트 예 (DevView 인스펙터가 JSON 으로 보여 준다).
	struct HGfwViewUnit : public IJsonable
	{
		int32 Health = 10;
		int32 Team   = 0;

	protected:
		virtual void WriteJson(PJsonData& json) const override
		{
			json.AddMember("Health", Health);
			json.AddMember("Team", Team);
		}

		virtual void ReadJson(const PJsonData& json) override
		{
			json.GetData("Health", &Health);
			json.GetData("Team", &Team);
		}
	};

	// 검증 스크립트: 3초 뒤 EndTurn 한 번(DevView 이벤트 로그 · 되돌리기 확인), 5초 뒤 에디터 안에서 gmtest(게임 월드를 건드리지 않는지 확인).
	class PGfwViewScriptActor : public JGActor
	{
	public:
		PWeakPtr<JGGameMasterActor> GameMasterActor;

	private:
		float32 _elapsed      = 0.0f;
		bool    _bEndTurnDone = false;
		bool    _bSelfTestDone = false;

	protected:
		virtual void OnTick(float32 deltaSeconds) override
		{
			_elapsed += deltaSeconds;

			if (_bEndTurnDone == false && _elapsed >= 3.0f)
			{
				_bEndTurnDone = true;
				PSharedPtr<JGGameMasterActor> gameMasterActor = GameMasterActor.Pin();
				PSharedPtr<PGameMaster>       gameMaster      = gameMasterActor != nullptr ? gameMasterActor->GetGameMaster() : nullptr;
				if (gameMaster != nullptr)
				{
					const HGameplayEntityId actor = gameMaster->GetState().Turn.CurrentActor;
					PString reason;
					const EGameMasterActorSubmit result = gameMasterActor->Submit(HGameplayCommand(PName(HGameplayBuiltin::CommandEndTurn), actor), &reason);
					JG_LOG(GfwView, ELogLevel::Info, "GfwView script: EndTurn by %s -> result %d (%s)", actor.ToString(), (int32)result, reason);
				}
			}

			if (_bSelfTestDone == false && _elapsed >= 5.0f)
			{
				_bSelfTestDone = true;
				PSharedPtr<PWorld> world = GetWorld();
				const int32 actorsBefore = world != nullptr ? (int32)world->GetActors().size() : -1;
				const bool bPassed = GConsoleCommandGlobalSystem::GetInstance().Execute(PString("gmtest"));
				PSharedPtr<PWorld> worldAfter = JGGameInstance::Get().GetWorld();
				const int32 actorsAfter = worldAfter != nullptr ? (int32)worldAfter->GetActors().size() : -1;
				JG_LOG(GfwView, ELogLevel::Info, "GfwView script: gmtest in editor -> %s, game world %s, actors %d -> %d",
					PString(bPassed ? "OK" : "FAILED"), PString(worldAfter == world ? "kept" : "REPLACED"), actorsBefore, actorsAfter);
			}
		}
	};

	// 엔티티의 몸을 정하는 게임 쪽 GameMasterActor: 칸 위에 X Bot 을 세우고 메시 경계 상자로 판정한다.
	class PGfwViewGameMasterActor : public JGGameMasterActor
	{
	protected:
		virtual PSharedPtr<JGGameplayEntityActor> SpawnActorForEntity(const HGameplayEntityId& id, const HGameplayEvent& event) override
		{
			PSharedPtr<JGGameplayEntityActor> actor = JGGameMasterActor::SpawnActorForEntity(id, event);
			if (actor == nullptr)
			{
				return nullptr;
			}

			PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
			const HGameplayCoord*  coord      = gameMaster != nullptr ? gameMaster->GetState().Board.FindPosition(id) : nullptr;
			if (coord != nullptr)
			{
				actor->SetLocalPosition(GetBoardLayout().CoordToWorld(*coord));
			}

			actor->AddComponent<JGStaticMeshComponent>()->LoadStaticMesh(HAssetPath(GfwUnitMeshPath));
			actor->AddComponent<JGPickShapeComponent>();   // 상자를 정하지 않으면 메시 경계 상자를 쓴다
			return actor;
		}
	};
}

void JGGfwViewEntryActor::OnEnterWorld()
{
	PSharedPtr<PWorld> world = GetWorld();
	const HGameplayBoardLayout layout(EGameplayBoardKind::Square, HVector3(0.0f, 0.0f, 0.0f), GfwCellSize);

	// 규칙: 6x6 사각 보드, 엔티티 4개
	PSharedPtr<PGfwViewGameMasterActor> gameMasterActor = world->SpawnActor<PGfwViewGameMasterActor>(PName("GameMaster"));
	gameMasterActor->SetBoardLayout(layout);

	PSharedPtr<PGameMaster> gameMaster = Allocate<PGameMaster>();
	gameMaster->RegisterZone(PName("Units"));
	gameMaster->SetOrderPolicy(Allocate<PGameplayZoneOrderPolicy>(PName("Units")));
	gameMaster->SetBoard(EGameplayBoardKind::Square, GfwBoardSize, GfwBoardSize);
	gameMaster->RegisterComponent<HGfwViewUnit>(PName("GfwViewUnit"));

	HGameplayState& initial = gameMaster->EditInitialState();
	const HGameplayCoord unitCells[4] = { HGameplayCoord(1, 1), HGameplayCoord(4, 1), HGameplayCoord(1, 4), HGameplayCoord(3, 3) };
	for (int32 i = 0; i < 4; ++i)
	{
		const HGameplayEntityId id = initial.CreateEntity();
		initial.Zone(PName("Units")).PushBack(id);
		initial.SetBoardPosition(id, unitCells[i]);

		HGfwViewUnit unit;
		unit.Health = 10 + i * 5;
		unit.Team   = i % 2;
		initial.Add<HGfwViewUnit>(id, unit);
	}

	gameMasterActor->SetGameMaster(gameMaster);
	gameMaster->Start(20260930);

	PSharedPtr<PGfwViewScriptActor> script = world->SpawnActor<PGfwViewScriptActor>(PName("VerifyScript"));
	script->GameMasterActor = gameMasterActor;

	// 연출: 칸마다 타일
	PSharedPtr<JGStaticMesh> tile = makeTileMesh(GfwCellSize * 0.45f);
	for (int32 y = 0; y < GfwBoardSize; ++y)
	{
		for (int32 x = 0; x < GfwBoardSize; ++x)
		{
			PSharedPtr<JGActor> tileActor = world->SpawnActor<JGActor>(PName(PString::Format("Tile_%d_%d", x, y)));
			tileActor->SetLocalPosition(layout.CoordToWorld(HGameplayCoord(x, y)));
			tileActor->AddComponent<JGStaticMeshComponent>()->SetStaticMesh(tile);
		}
	}

	// 카메라: 칸 (3, 3) 을 도는 궤도. 월드 뷰 가운데가 칸 (3, 3) 이다
	PSharedPtr<JGCameraActor> cameraActor = world->SpawnActor<JGCameraActor>(PName("BoardCamera"));
	cameraActor->SetOrbit(layout.CoordToWorld(HGameplayCoord(3, 3)), HMath::ConvertToRadians(20.0f), HMath::ConvertToRadians(50.0f), 1800.0f);

	// 조작자: 월드 뷰의 클릭을 받는다
	PSharedPtr<JGGameplayControllerActor> controller = world->SpawnActor<JGGameplayControllerActor>(PName("Controller"));
	controller->SetGameMasterActor(gameMasterActor);

	JG_LOG(GfwView, ELogLevel::Info, "GfwView entry actor entered world: board %dx%d, units %d, tiles %d, orbit camera, controller",
		GfwBoardSize, GfwBoardSize, 4, GfwBoardSize * GfwBoardSize);
}
