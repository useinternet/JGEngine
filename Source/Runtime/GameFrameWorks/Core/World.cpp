#include "PCH/PCH.h"
#include "Core/World.h"
#include "Actors/Actor.h"
#include "Components/CameraComponent.h"
#include "Components/PickShapeComponent.h"
#include "Classes/Scene.h"

PWorld::PWorld()
	: _name(PName("World"))
{
}

PWorld::PWorld(const PName& name)
	: _name(name)
{
}

PWorld::~PWorld()
{
	// 소유한 액터의 월드 참조를 끊는다. 종료 순서와 무관하게 안전하도록.
	for (PSharedPtr<JGActor>& actor : _actors)
	{
		if (actor != nullptr)
		{
			actor->_world.Reset();
		}
	}
}

const PName& PWorld::GetName() const
{
	return _name;
}

float32 PWorld::GetTime() const
{
	return _time;
}

uint64 PWorld::GetFrame() const
{
	return _frame;
}

bool PWorld::HasBegun() const
{
	return _bBegun;
}

PSharedPtr<JGActor> PWorld::SpawnActorByClass(PSharedPtr<JGClass> actorClass, const PName& name)
{
	if (actorClass == nullptr)
	{
		return nullptr;
	}
	PSharedPtr<JGActor> actor = RawDynamicCast<JGActor>(AllocateByClass(actorClass));
	if (actor == nullptr)
	{
		return nullptr;
	}
	registerSpawned(actor, name);
	return actor;
}

void PWorld::DestroyActor(PSharedPtr<JGActor> actor)
{
	if (actor == nullptr || actor->_bPendingDestroy == true)
	{
		return;
	}
	actor->_bPendingDestroy = true;
	_pendingDestroy.push_back(actor);

	if (_bTicking == false)
	{
		flushPendingDestroy();
	}
}

void PWorld::BeginPlay()
{
	if (_bBegun == true)
	{
		return;
	}
	_bBegun = true;

	flushPendingSpawn();

	HList<PSharedPtr<JGActor>> actors = _actors;
	for (PSharedPtr<JGActor>& actor : actors)
	{
		actor->beginPlay();
	}
}

void PWorld::Tick(float32 deltaSeconds)
{
	_bTicking = true;
	_time += deltaSeconds;
	++_frame;

	flushPendingSpawn();

	HList<PSharedPtr<JGActor>> actors = _actors;
	for (PSharedPtr<JGActor>& actor : actors)
	{
		if (actor->_bPendingDestroy == false && actor->GetParent() == nullptr)
		{
			// 루트 액터만 직접 틱한다. 자식은 부모의 tick 이 재귀한다.
			actor->tick(deltaSeconds);
		}
	}

	_bTicking = false;
	flushPendingDestroy();
}

void PWorld::EndPlay()
{
	if (_bBegun == false)
	{
		return;
	}

	HList<PSharedPtr<JGActor>> actors = _actors;
	for (PSharedPtr<JGActor>& actor : actors)
	{
		actor->endPlay();
	}
	_bBegun = false;
}

const HList<PSharedPtr<JGActor>>& PWorld::GetActors() const
{
	return _actors;
}

PSharedPtr<JGActor> PWorld::FindActorByGuid(const HGuid& guid) const
{
	for (const PSharedPtr<JGActor>& actor : _actors)
	{
		if (actor->GetGuid() == guid)
		{
			return actor;
		}
	}
	return nullptr;
}

PSharedPtr<JGActor> PWorld::FindActorByName(const PName& name) const
{
	for (const PSharedPtr<JGActor>& actor : _actors)
	{
		if (actor->GetName() == name)
		{
			return actor;
		}
	}
	return nullptr;
}

PSharedPtr<PScene> PWorld::GetScene()
{
	if (_scene == nullptr)
	{
		_scene = Allocate<PScene>();
	}
	return _scene;
}

void PWorld::SetActiveCamera(PSharedPtr<JGCameraComponent> camera)
{
	if (camera == nullptr)
	{
		_activeCamera.Reset();
		return;
	}
	_activeCamera = camera;
}

PSharedPtr<JGCameraComponent> PWorld::GetActiveCamera() const
{
	return _activeCamera.Pin();
}

bool PWorld::PickActor(const HRay& ray, HWorldPickHit* outHit) const
{
	bool    bHit        = false;
	float32 nearest     = 0.0f;
	HWorldPickHit best;

	for (const PSharedPtr<JGActor>& actor : _actors)
	{
		if (actor == nullptr || actor->IsPendingDestroy() == true)
		{
			continue;
		}

		for (const PSharedPtr<JGActorComponent>& component : actor->GetComponents())
		{
			PSharedPtr<JGPickShapeComponent> shape = RawDynamicCast<JGPickShapeComponent>(component);
			if (shape == nullptr)
			{
				continue;
			}

			float32  distance = 0.0f;
			HVector3 position;
			if (shape->IntersectRay(ray, &distance, &position) == false)
			{
				continue;
			}

			if (bHit == false || distance < nearest)
			{
				bHit          = true;
				nearest       = distance;
				best.Actor    = actor;
				best.Shape    = shape;
				best.Position = position;
				best.Distance = distance;
			}
		}
	}

	if (bHit == true && outHit != nullptr)
	{
		*outHit = best;
	}
	return bHit;
}

void PWorld::registerSpawned(PSharedPtr<JGActor> actor, const PName& name)
{
	if (actor == nullptr)
	{
		return;
	}

	if (name != NAME_NONE)
	{
		actor->SetName(name);
	}
	actor->_world = SharedWrap(this);
	actor->OnSpawned();

	if (_bTicking == true)
	{
		_pendingSpawn.push_back(actor);
		return;
	}

	_actors.push_back(actor);
	if (_bBegun == true)
	{
		actor->beginPlay();
	}
}

void PWorld::flushPendingSpawn()
{
	if (_pendingSpawn.empty() == true)
	{
		return;
	}

	HList<PSharedPtr<JGActor>> pending = _pendingSpawn;
	_pendingSpawn.clear();

	for (PSharedPtr<JGActor>& actor : pending)
	{
		// 스폰된 틱 안에 파괴가 예약된 액터는 월드에 넣지 않는다.
		if (actor->_bPendingDestroy == true)
		{
			actor->_world.Reset();
			continue;
		}

		_actors.push_back(actor);
		if (_bBegun == true)
		{
			actor->beginPlay();
		}
	}
}

void PWorld::flushPendingDestroy()
{
	if (_pendingDestroy.empty() == true)
	{
		return;
	}

	HList<PWeakPtr<JGActor>> pending = _pendingDestroy;
	_pendingDestroy.clear();

	for (PWeakPtr<JGActor>& weak : pending)
	{
		PSharedPtr<JGActor> actor = weak.Pin();
		if (actor == nullptr)
		{
			continue;
		}

		// 자식도 함께 파괴한다.
		HList<PSharedPtr<JGActor>> children = actor->GetChildren();
		for (PSharedPtr<JGActor>& child : children)
		{
			child->_bPendingDestroy = true;
			_pendingDestroy.push_back(child);
		}

		actor->endPlay();
		actor->Detach();

		for (auto iter = _actors.begin(); iter != _actors.end(); ++iter)
		{
			if (*iter == actor)
			{
				_actors.erase(iter);
				break;
			}
		}
		actor->_world.Reset();
	}

	if (_pendingDestroy.empty() == false)
	{
		flushPendingDestroy();
	}
}
