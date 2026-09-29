#pragma once
#include "Core/GameFrameWorksDefines.h"

class JGActor;
class JGClass;

// 월드. 액터를 소유하고 스폰 · 파괴 · 틱을 돌린다. GameMaster 를 모른다 (GameMaster 는 Actors/ 의 GameMasterActor 가 든다).
class GAMEFRAMEWORKS_API PWorld : public IMemoryObject
{
	PName                      _name;
	HList<PSharedPtr<JGActor>> _actors;
	HList<PSharedPtr<JGActor>> _pendingSpawn;
	HList<PWeakPtr<JGActor>>   _pendingDestroy;
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

private:
	void registerSpawned(PSharedPtr<JGActor> actor, const PName& name);
	void flushPendingSpawn();
	void flushPendingDestroy();
};
