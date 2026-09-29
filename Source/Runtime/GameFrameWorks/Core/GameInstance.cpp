#include "PCH/PCH.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Actors/GameEntryActor.h"

JGGameInstance* JGGameInstance::s_instance = nullptr;

bool JGGameInstance::HasInstance()
{
	return s_instance != nullptr;
}

JGGameInstance& JGGameInstance::Get()
{
	JG_CHECK(s_instance != nullptr);
	return *s_instance;
}

void JGGameInstance::SetEntryClass(PSharedPtr<JGClass> entryClass)
{
	_entryClass = entryClass;
}

PSharedPtr<JGClass> JGGameInstance::GetEntryClass() const
{
	return _entryClass;
}

PSharedPtr<PWorld> JGGameInstance::LoadWorld(const PName& name)
{
	if (_world != nullptr)
	{
		UnloadWorld();
	}

	PSharedPtr<PWorld> world = (name == NAME_NONE) ? Allocate<PWorld>() : Allocate<PWorld>(name);
	_world = world;

	if (_entryClass != nullptr)
	{
		PSharedPtr<JGActor> actor = world->SpawnActorByClass(_entryClass, PName("GameEntry"));
		PSharedPtr<JGGameEntryActor> entry = RawDynamicCast<JGGameEntryActor>(actor);
		if (entry != nullptr)
		{
			entry->OnEnterWorld();
		}
		else
		{
			JG_LOG(GameFrameWorks, ELogLevel::Warning, "JGGameInstance: entry class is not a JGGameEntryActor");
		}
	}

	OnWorldReady(world);
	OnWorldLoaded.BroadCast(world);

	world->BeginPlay();
	JG_LOG(GameFrameWorks, ELogLevel::Info, "JGGameInstance: world loaded (%s)", world->GetName().ToString());
	return world;
}

void JGGameInstance::UnloadWorld()
{
	if (_world == nullptr)
	{
		return;
	}

	PSharedPtr<PWorld> world = _world;

	PSharedPtr<JGGameEntryActor> entry = GetEntryActor();
	if (entry != nullptr)
	{
		entry->OnExitWorld();
	}

	OnWorldUnloading.BroadCast(world);
	OnWorldUnload(world);

	world->EndPlay();
	_world.Reset();
	JG_LOG(GameFrameWorks, ELogLevel::Info, "JGGameInstance: world unloaded (%s)", world->GetName().ToString());
}

PSharedPtr<PWorld> JGGameInstance::GetWorld() const
{
	return _world;
}

bool JGGameInstance::HasWorld() const
{
	return _world != nullptr;
}

PSharedPtr<JGGameEntryActor> JGGameInstance::GetEntryActor() const
{
	if (_world == nullptr)
	{
		return nullptr;
	}

	HList<PSharedPtr<JGGameEntryActor>> entries;
	_world->FindActors<JGGameEntryActor>(entries);
	if (entries.empty() == true)
	{
		return nullptr;
	}
	return entries[0];
}

void JGGameInstance::Tick(float32 deltaSeconds)
{
	if (_world != nullptr && _world->HasBegun() == true)
	{
		_world->Tick(deltaSeconds);
	}
}

void JGGameInstance::init()
{
	if (_bInitialized == true)
	{
		return;
	}
	_bInitialized = true;
	s_instance = this;
	OnInit();
}

void JGGameInstance::shutdown()
{
	if (_bInitialized == false)
	{
		return;
	}
	UnloadWorld();
	OnShutdown();
	if (s_instance == this)
	{
		s_instance = nullptr;
	}
	_bInitialized = false;
}
