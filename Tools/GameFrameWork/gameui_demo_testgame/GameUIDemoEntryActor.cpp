#include "PCH/PCH.h"
#include "GameUIDemoEntryActor.h"
#include "Core/World.h"
#include "Actors/CameraActor.h"
#include "Components/StaticMeshComponent.h"

void JGGameUIDemoEntryActor::OnEnterWorld()
{
	// 게임 UI 검증용 배경: 엔진 샘플 메시(X Bot) 하나와 그것을 도는 궤도 카메라. 화면(HUD)은 게임 모듈이 게임 인스턴스 UI 에 올린다.
	PSharedPtr<PWorld> world = GetWorld();

	PSharedPtr<JGActor> bot = world->SpawnActor<JGActor>(PName("XBot"));
	bot->AddComponent<JGStaticMeshComponent>()->LoadStaticMesh(HAssetPath("/JGEngine/TempAsset/Sample.jgasset"));

	PSharedPtr<JGCameraActor> camera = world->SpawnActor<JGCameraActor>(PName("Camera"));
	camera->SetOrbit(HVector3(0.0f, 90.0f, 0.0f), HMath::ConvertToRadians(0.0f), HMath::ConvertToRadians(10.0f), 420.0f);

	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo entry actor entered world: X Bot + orbit camera");
}
