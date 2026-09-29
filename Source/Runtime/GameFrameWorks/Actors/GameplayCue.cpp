#include "PCH/PCH.h"
#include "Actors/GameplayCue.h"

bool JGGameplayCue::Accepts(const HGameplayEvent& event) const
{
	return false;
}

void JGGameplayCue::Begin(const HGameplayEvent& event, JGGameMasterActor& gameMasterActor)
{
}

void JGGameplayCue::Tick(float32 deltaSeconds)
{
}

bool JGGameplayCue::IsDone() const
{
	return true;
}

void JGGameplayCue::Skip()
{
}

PSharedPtr<JGGameplayCue> JGGameplayCue::CreateInstance() const
{
	PSharedPtr<JGClass> classObject = GetClass();
	if (classObject == nullptr)
	{
		return nullptr;
	}
	return RawDynamicCast<JGGameplayCue>(AllocateByClass(classObject));
}
