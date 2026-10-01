#pragma once
#include "Core/GameFrameWorksDefines.h"

class JGActor;
class JGClass;
class JGCameraComponent;
class PScene;
class HRay;
struct HWorldPickHit;

// 월드. 액터를 소유하고 스폰 · 파괴 · 틱을 돌린다. GameMaster 를 모른다 (GameMaster 는 Actors/ 의 GameMasterActor 가 든다).
// 장면(PScene)을 하나 가진다. 메시 · 카메라 컴포넌트가 장면에 배치를 등록하고, 월드를 그리는 쪽(씬 뷰포트)이 활성 카메라로 장면을 그린다.
class GAMEFRAMEWORKS_API PWorld : public IMemoryObject
{
	PName                      _name;
	HList<PSharedPtr<JGActor>> _actors;
	HList<PSharedPtr<JGActor>> _pendingSpawn;
	HList<PWeakPtr<JGActor>>   _pendingDestroy;
	PSharedPtr<PScene>         _scene;
	PWeakPtr<JGCameraComponent> _activeCamera;
	bool                       _bBegun    = false;
	bool                       _bTicking  = false;
	float32                    _time      = 0.0f;
	uint64                     _frame     = 0;

public:
	PWorld();
	explicit PWorld(const PName& name);
	virtual ~PWorld();

	const PName& GetName() const;
	float32      GetTime() const;
	uint64       GetFrame() const;
	bool         HasBegun() const;

	// 스폰. BeginPlay 이후라면 다음 틱 시작 시 OnBeginPlay 가 불린다.
	template<class T>
	PSharedPtr<T> SpawnActor(const PName& name = PName())
	{
		PSharedPtr<T> actor = Allocate<T>();
		registerSpawned(actor, name);
		return actor;
	}

	PSharedPtr<JGActor> SpawnActorByClass(PSharedPtr<JGClass> actorClass, const PName& name = PName());

	// 파괴 예약. 틱 끝에 OnEndPlay 후 제거된다.
	void DestroyActor(PSharedPtr<JGActor> actor);

	void BeginPlay();
	void Tick(float32 deltaSeconds);
	void EndPlay();

	const HList<PSharedPtr<JGActor>>& GetActors() const;
	PSharedPtr<JGActor> FindActorByGuid(const HGuid& guid) const;
	PSharedPtr<JGActor> FindActorByName(const PName& name) const;

	// T 로 다운캐스트되는 액터를 모은다 (동적 캐스트).
	template<class T>
	void FindActors(HList<PSharedPtr<T>>& outActors) const
	{
		for (const PSharedPtr<JGActor>& actor : _actors)
		{
			PSharedPtr<T> typed = RawDynamicCast<T>(actor);
			if (typed != nullptr)
			{
				outActors.push_back(typed);
			}
		}
	}

	// 장면. 처음 부를 때 만든다. 그래픽 백엔드 없이도 쓸 수 있는 데이터다 (헤드리스 자체 테스트가 쓴다). 메인 스레드 전용.
	PSharedPtr<PScene> GetScene();

	// 활성 카메라. 씬 뷰포트가 이 카메라로 그리고 피킹한다. 카메라 컴포넌트는 활성 카메라가 없을 때 BeginPlay 에서 스스로 활성이 된다.
	void                          SetActiveCamera(PSharedPtr<JGCameraComponent> camera);
	PSharedPtr<JGCameraComponent> GetActiveCamera() const;

	// 피킹. 광선이 맞은 가장 가까운 판정 모양(JGPickShapeComponent). 맞은 것이 없으면 false. HWorldPickHit 는 Components/PickShapeComponent.h.
	bool PickActor(const HRay& ray, HWorldPickHit* outHit) const;

private:
	void registerSpawned(PSharedPtr<JGActor> actor, const PName& name);
	void flushPendingSpawn();
	void flushPendingDestroy();
};
