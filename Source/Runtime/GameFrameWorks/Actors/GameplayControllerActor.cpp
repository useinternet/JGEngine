#include "PCH/PCH.h"
#include "Actors/GameplayControllerActor.h"
#include "Core/World.h"
#include "Components/PickShapeComponent.h"
#include "Network/Session/GameplaySession.h"

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

bool JGGameplayControllerActor::IsLocallyControlled(const HGameplayEntityId& actor) const
{
	PSharedPtr<JGGameMasterActor> gameMasterActor = _gameMasterActor.Pin();
	PSharedPtr<PGameplaySession>  session         = gameMasterActor != nullptr ? gameMasterActor->GetSession() : nullptr;
	if (session == nullptr)
	{
		return true;
	}
	return session->IsLocallyControlled(actor);
}

HGameplayEntityId JGGameplayControllerActor::GetInputActor() const
{
	PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
	if (gameMaster == nullptr)
	{
		return HGameplayEntityId::None();
	}
	return gameMaster->GetState().FirstInputActor();
}

void JGGameplayControllerActor::CollectInputActors(HList<HGameplayEntityId>& outActors) const
{
	PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
	if (gameMaster == nullptr)
	{
		return;
	}
	gameMaster->GetState().CollectInputActors(outActors);
}

bool JGGameplayControllerActor::BeginCommand(const PName& kind, const HGameplayEntityId& actor)
{
	if (IsLocallyControlled(actor) == false)
	{
		JG_LOG(GameFrameWorks, ELogLevel::Warning, "%s: %s is not controlled by the local player", GetName().ToString(), actor.ToString());
		return false;
	}

	_draft = HGameplayCommand(kind, actor);
	_bDrafting = true;
	return true;
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
	if (IsLocallyControlled(choice->Chooser) == false)
	{
		if (outReason != nullptr)
		{
			*outReason = "not your choice";
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
	EnumerateLegal(GetInputActor(), outCommands);
}

void JGGameplayControllerActor::EnumerateLegal(const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const
{
	PSharedPtr<PGameMaster> gameMaster = GetGameMaster();
	if (gameMaster == nullptr)
	{
		return;
	}
	gameMaster->EnumerateLegal(actor, outCommands);
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

bool JGGameplayControllerActor::Pick(const HRay& ray, HGameplayPickResult* outResult) const
{
	HGameplayPickResult result;

	// 1. 월드 판정 모양. 맞은 액터에 엔티티가 없으면 부모 쪽으로 올라가며 찾는다 (메시가 자식 액터에 있는 경우).
	PSharedPtr<PWorld> world = GetWorld();
	HWorldPickHit hit;
	if (world != nullptr && world->PickActor(ray, &hit) == true)
	{
		result.bHitActor     = true;
		result.Actor         = hit.Actor;
		result.ActorPosition = hit.Position;
		result.ActorDistance = hit.Distance;

		PSharedPtr<JGActor> cursor = hit.Actor;
		while (cursor != nullptr)
		{
			PSharedPtr<JGGameplayEntityComponent> entity = cursor->FindComponent<JGGameplayEntityComponent>();
			if (entity != nullptr && entity->IsBound() == true)
			{
				result.Entity = entity->GetEntityId();
				break;
			}
			cursor = cursor->GetParent();
		}
	}

	// 2. 보드. 칸 범위는 GameMaster 상태의 보드로 거른다.
	PSharedPtr<JGGameMasterActor> gameMasterActor = _gameMasterActor.Pin();
	if (gameMasterActor != nullptr)
	{
		const HGameplayBoardLayout& layout = gameMasterActor->GetBoardLayout();
		HGameplayCoord coord;
		HVector3       position;
		if (layout.PickCoord(ray, &coord, &position) == true)
		{
			PSharedPtr<PGameMaster> gameMaster = gameMasterActor->GetGameMaster();
			if (gameMaster == nullptr || gameMaster->GetState().Board.InBounds(coord) == true)
			{
				result.bHitBoard     = true;
				result.Coord         = coord;
				result.BoardPosition = position;
			}
		}
	}

	if (outResult != nullptr)
	{
		*outResult = result;
	}
	return result.bHitActor == true || result.bHitBoard == true;
}

void JGGameplayControllerActor::HandleClick(const HRay& ray)
{
	HGameplayPickResult pick;
	Pick(ray, &pick);
	_lastPick     = pick;
	_bHasLastPick = true;

	JG_LOG(GameFrameWorks, ELogLevel::Info, "%s click: %s", GetName().ToString(), pick.ToString());
	OnClick(pick);
}

bool JGGameplayControllerActor::HasLastPick() const
{
	return _bHasLastPick;
}

const HGameplayPickResult& JGGameplayControllerActor::GetLastPick() const
{
	return _lastPick;
}

PString HGameplayPickResult::ToString() const
{
	PString actorText = "actor none";
	if (bHitActor == true && Actor != nullptr)
	{
		actorText = PString::Format("actor %s entity %s at (%.2f, %.2f, %.2f) distance %.2f",
			Actor->GetName().ToString(), Entity.ToString(), ActorPosition.x, ActorPosition.y, ActorPosition.z, ActorDistance);
	}

	PString boardText = "board none";
	if (bHitBoard == true)
	{
		boardText = PString::Format("board cell %s at (%.2f, %.2f, %.2f)", Coord.ToString(), BoardPosition.x, BoardPosition.y, BoardPosition.z);
	}

	return PString::Format("%s / %s", actorText, boardText);
}
