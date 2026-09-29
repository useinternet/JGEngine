#pragma once
#include "Actors/Actor.h"
#include "Actors/GameMasterActor.h"
#include "GameplayControllerActor.generation.h"

// 조작자. 입력(선택 · 타깃 지정)을 명령으로 조립해 GameMasterActor 에 제출한다. 피킹은 게임/뷰가 붙인다.
//   BeginCommand → AddTarget / AddParam → Commit   |   ResolveChoice   |   Cancel
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayControllerActor : public JGActor
{
	JG_GENERATED_CLASS_BODY

private:
	PWeakPtr<JGGameMasterActor> _gameMasterActor;
	HGameplayCommand                  _draft;
	bool                                _bDrafting = false;

public:
	JGGameplayControllerActor() = default;
	virtual ~JGGameplayControllerActor() = default;

	void SetGameMasterActor(PSharedPtr<JGGameMasterActor> gameMasterActor);
	PSharedPtr<JGGameMasterActor> GetGameMasterActor() const;
	PSharedPtr<PGameMaster>               GetGameMaster() const;

	// 명령 초안
	void BeginCommand(const PName& kind, const HGameplayEntityId& actor);
	void AddTarget(const HGameplayEntityId& target);
	void AddParam(int32 value);
	void AddPathCoord(const HGameplayCoord& coord);
	bool IsDrafting() const;
	const HGameplayCommand& GetDraft() const;
	// 초안이 지금 합법인가 (타깃을 다 채웠는지 확인용).
	bool ValidateDraft(PString* outReason) const;
	EGameMasterActorSubmit Commit(PString* outReason = nullptr);
	void Cancel();

	// 선택 대기 응답
	EGameMasterActorSubmit ResolveChoice(const HList<HGameplayEntityId>& selection, PString* outReason = nullptr);
	const HGameplayChoice*  GetPendingChoice() const;

	// 현재 행동자의 합법 명령
	void EnumerateLegal(HList<HGameplayCommand>& outCommands) const;
	HGameplayEntityId GetCurrentActor() const;
};
