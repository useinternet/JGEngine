#include "PCH/PCH.h"
#include "Actors/GameplayControllerActor.h"

void JGGameplayControllerActor::SetGameMasterActor(PSharedPtr<JGGameMasterActor> gameMasterActor)
{
	_gameMasterActor = gameMasterActor;
}

PSharedPtr<JGGameMasterActor> JGGameplayControllerActor::GetGameMasterActor() const
{
	return _gameMasterActor.Pin();
}

PSharedPtr<PGameMaster> JGGameplayControllerActor::GetGameMaster() const
{
	PSharedPtr<JGGameMasterActor> gameMasterActor = _gameMasterActor.Pin();
	if (gameMasterActor == nullptr)
	{
		return nullptr;
	}
	return gameMasterActor->GetGameMaster();
}

void JGGameplayControllerActor::BeginCommand(const PName& kind, const HGameplayEntityId& actor)
{
	_draft = HGameplayCommand(kind, actor);
	_bDrafting = true;
}

void JGGameplayControllerActor::AddTarget(const HGameplayEntityId& target)
{
	_draft.Targets.push_back(target);
}

void JGGameplayControllerActor::AddParam(int32 value)
{
	_draft.Params.push_back(value);
}

void JGGameplayControllerActor::AddPathCoord(const HGameplayCoord& coord)
{
	_draft.Path.push_back(coord);
}

bool JGGameplayControllerActor::IsDrafting() const
{
	return _bDrafting;
}

const HGameplayCommand& JGGameplayControllerActor::GetDraft() const
{
	return _draft;
}

bool JGGameplayControllerActor::ValidateDraft(PString* outReason) const
{
	PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
	if (gameMaster == nullptr || _bDrafting == false)
	{
		if (outReason != nullptr)
		{
			*outReason = "no draft";
		}
		return false;
	}
	return gameMaster->Validate(_draft, outReason);
}

EGameMasterActorSubmit JGGameplayControllerActor::Commit(PString* outReason)
{
	PSharedPtr<JGGameMasterActor> gameMasterActor = _gameMasterActor.Pin();
	if (gameMasterActor == nullptr || _bDrafting == false)
	{
		if (outReason != nullptr)
		{
			*outReason = "no gameMasterActor or draft";
		}
		return EGameMasterActorSubmit::Rejected;
	}

	HGameplayCommand command = _draft;
	_bDrafting = false;
	_draft = HGameplayCommand();
	return gameMasterActor->Submit(command, outReason);
}

void JGGameplayControllerActor::Cancel()
{
	_bDrafting = false;
	_draft = HGameplayCommand();
}

EGameMasterActorSubmit JGGameplayControllerActor::ResolveChoice(const HList<HGameplayEntityId>& selection, PString* outReason)
{
	PSharedPtr<JGGameMasterActor> gameMasterActor = _gameMasterActor.Pin();
	PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
	if (gameMasterActor == nullptr || gameMaster == nullptr)
	{
		if (outReason != nullptr)
		{
			*outReason = "no gameMasterActor";
		}
		return EGameMasterActorSubmit::Rejected;
	}

	const HGameplayChoice* choice = gameMaster->GetPendingChoice();
	if (choice == nullptr)
	{
		if (outReason != nullptr)
		{
			*outReason = "no pending choice";
		}
		return EGameMasterActorSubmit::Rejected;
	}

	HGameplayCommand resolve(PName(HGameplayBuiltin::CommandResolveChoice), choice->Chooser);
	resolve.Targets = selection;
	return gameMasterActor->Submit(resolve, outReason);
}

const HGameplayChoice* JGGameplayControllerActor::GetPendingChoice() const
{
	PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
	if (gameMaster == nullptr)
	{
		return nullptr;
	}
	return gameMaster->GetPendingChoice();
}

void JGGameplayControllerActor::EnumerateLegal(HList<HGameplayCommand>& outCommands) const
{
	PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
	if (gameMaster == nullptr)
	{
		return;
	}
	gameMaster->EnumerateLegal(gameMaster->GetState().Turn.CurrentActor, outCommands);
}

HGameplayEntityId JGGameplayControllerActor::GetCurrentActor() const
{
	PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
	if (gameMaster == nullptr)
	{
		return HGameplayEntityId::None();
	}
	return gameMaster->GetState().Turn.CurrentActor;
}
