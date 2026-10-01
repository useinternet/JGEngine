#pragma once
#include "Actors/Actor.h"
#include "Actors/GameMasterActor.h"
#include "GameplayControllerActor.generation.h"

// 피킹 결과. 월드 판정(액터)과 보드 판정(칸)을 함께 담는다. 둘 다 맞을 수 있다 (보드 위의 말).
struct GAMEFRAMEWORKS_API HGameplayPickResult
{
	bool                bHitActor = false;
	PSharedPtr<JGActor> Actor;                      // 가장 가까운 판정 모양의 액터
	HGameplayEntityId   Entity;                     // 그 액터(없으면 부모 쪽)가 묶인 엔티티. 없으면 None
	HVector3            ActorPosition;
	float32             ActorDistance = 0.0f;

	bool                bHitBoard = false;
	HGameplayCoord      Coord;                      // 보드 범위 안의 칸
	HVector3            BoardPosition;

	PString ToString() const;
};

// 조작자. 입력(선택 · 타깃 지정)을 명령으로 조립해 GameMasterActor 에 제출한다.
//   BeginCommand → AddTarget / AddParam → Commit   |   ResolveChoice   |   Cancel
// 이 기계의 로컬 플레이어가 조작하는 행동자만 명령을 시작하고 선택에 답할 수 있다 (세션 슬롯 기준. AI 가 맡은 조작 주체는 아니다).
// 피킹: 씬 뷰포트가 클릭을 광선으로 바꿔 HandleClick 을 부른다 → Pick(판정 모양 · 보드) → OnClick.
// 클릭을 어떤 명령으로 바꿀지는 게임이 OnClick 을 재정의해 정한다 (엔진은 무엇을 눌렀는지만 알려 준다).
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayControllerActor : public JGActor
{
	JG_GENERATED_CLASS_BODY

private:
	PWeakPtr<JGGameMasterActor> _gameMasterActor;
	HGameplayCommand                  _draft;
	bool                                _bDrafting = false;
	HGameplayPickResult                 _lastPick;
	bool                                _bHasLastPick = false;

public:
	JGGameplayControllerActor() = default;
	virtual ~JGGameplayControllerActor() = default;

	void SetGameMasterActor(PSharedPtr<JGGameMasterActor> gameMasterActor);
	PSharedPtr<JGGameMasterActor> GetGameMasterActor() const;
	PSharedPtr<PGameMaster>               GetGameMaster() const;

	// 이 행동자를 이 기계의 로컬 플레이어가 조작하는가. 세션 밖(자체 테스트 월드)이면 전부 로컬이다.
	bool IsLocallyControlled(const HGameplayEntityId& actor) const;
	// 지금 입력을 받는 행동자 (HGameplayState::CollectInputActors): 선택 대기 중이면 선택자, 아니면 흐름이 정한 Turn.Actors.
	// 여러 명이 동시에 낼 수 있는 흐름이면 목록을 쓰고 IsLocallyControlled 로 거른다. GetInputActor 는 첫 행동자.
	HGameplayEntityId GetInputActor() const;
	void              CollectInputActors(HList<HGameplayEntityId>& outActors) const;

	// 명령 초안. 로컬 플레이어가 조작하지 않는 행동자면 시작하지 않고 false.
	bool BeginCommand(const PName& kind, const HGameplayEntityId& actor);
	void AddTarget(const HGameplayEntityId& target);
	void AddParam(int32 value);
	void AddPathCoord(const HGameplayCoord& coord);
	bool IsDrafting() const;
	const HGameplayCommand& GetDraft() const;
	// 초안이 지금 합법인가 (타깃을 다 채웠는지 확인용).
	bool ValidateDraft(PString* outReason) const;
	EGameMasterActorSubmit Commit(PString* outReason = nullptr);
	void Cancel();

	// 선택 대기 응답. 선택자가 로컬 플레이어의 것이 아니면 거절.
	EGameMasterActorSubmit ResolveChoice(const HList<HGameplayEntityId>& selection, PString* outReason = nullptr);
	const HGameplayChoice*  GetPendingChoice() const;

	// 합법 명령. 행동자를 주지 않으면 첫 입력 행동자.
	void EnumerateLegal(HList<HGameplayCommand>& outCommands) const;
	void EnumerateLegal(const HGameplayEntityId& actor, HList<HGameplayCommand>& outCommands) const;
	// 기본 흐름의 현재 행동자(Turn.CurrentActor). 흐름과 상관없이 "지금 누가 입력하나" 는 GetInputActor.
	HGameplayEntityId GetCurrentActor() const;

	// 피킹. 광선을 월드 판정 모양(PWorld::PickActor)과 GameMasterActor 의 보드 배치에 쏜다. 어느 쪽이든 맞으면 true.
	bool Pick(const HRay& ray, HGameplayPickResult* outResult) const;
	// 뷰포트 클릭 (씬 뷰포트가 부른다). Pick → 마지막 결과 저장 → OnClick.
	void HandleClick(const HRay& ray);
	bool HasLastPick() const;
	const HGameplayPickResult& GetLastPick() const;

protected:
	// 클릭을 명령으로 바꾸는 자리. 기본은 아무것도 하지 않는다 (게임이 재정의).
	virtual void OnClick(const HGameplayPickResult& pick) {}
};
