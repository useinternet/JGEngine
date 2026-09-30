#pragma once
#include "GameMaster/State/GameplayState.h"
#include "GameMaster/Messages/GameplayCommand.h"
#include "GameMaster/Messages/GameplayEvent.h"

class PGameMaster;

// AI 용 상태 평가. 높을수록 actor 에 유리.
class GAMEFRAMEWORKS_API IGameplayEvaluator : public IMemoryObject
{
public:
	virtual ~IGameplayEvaluator() = default;
	virtual int32 Evaluate(const HGameplayState& state, const HGameplayEntityId& actor) const = 0;
};

// 행동자 하나의 명령 선택. 입력 대리 · AI · 스크립트가 구현한다.
// 에이전트는 상태를 직접 바꾸지 않는다. Submit 은 호출자(PGameplayAgentRunner 등)가 한다.
class GAMEFRAMEWORKS_API IGameplayAgent : public IMemoryObject
{
public:
	virtual ~IGameplayAgent() = default;

	virtual bool ChooseCommand(PGameMaster& gameMaster, const HGameplayEntityId& actor, HGameplayCommand& outCommand) = 0;
	virtual bool ChooseOption(PGameMaster& gameMaster, const HGameplayChoice& choice, HList<HGameplayEntityId>& outSelection) = 0;
};

// 합법 수 중 무작위. 자체 시드 스트림을 쓴다 (상태의 난수를 소비하면 "명령만 상태를 바꾼다" 규칙이 깨진다).
class GAMEFRAMEWORKS_API PGameplayRandomAgent : public IGameplayAgent
{
	HGameplayRandomStream _rng;

public:
	explicit PGameplayRandomAgent(uint64 seed = 1);
	virtual ~PGameplayRandomAgent() = default;

	virtual bool ChooseCommand(PGameMaster& gameMaster, const HGameplayEntityId& actor, HGameplayCommand& outCommand) override;
	virtual bool ChooseOption(PGameMaster& gameMaster, const HGameplayChoice& choice, HList<HGameplayEntityId>& outSelection) override;
};

// 1수 탐욕. 합법 수마다 상태 복사본에 적용해 평가하고 최고점을 고른다. 동점은 앞선 것.
class GAMEFRAMEWORKS_API PGameplayGreedyAgent : public IGameplayAgent
{
	PSharedPtr<IGameplayEvaluator> _evaluator;

public:
	PGameplayGreedyAgent() = default;
	explicit PGameplayGreedyAgent(PSharedPtr<IGameplayEvaluator> evaluator);
	virtual ~PGameplayGreedyAgent() = default;

	void SetEvaluator(PSharedPtr<IGameplayEvaluator> evaluator);

	virtual bool ChooseCommand(PGameMaster& gameMaster, const HGameplayEntityId& actor, HGameplayCommand& outCommand) override;
	virtual bool ChooseOption(PGameMaster& gameMaster, const HGameplayChoice& choice, HList<HGameplayEntityId>& outSelection) override;
};

// 현재 행동자(또는 선택 대기의 선택자)에 맞는 에이전트를 골라 명령을 얻고 Submit 한다.
class GAMEFRAMEWORKS_API PGameplayAgentRunner
{
public:
	// 명령만 고른다 (Submit 하지 않는다). 선택 대기 중이면 ResolveChoice 명령. 에이전트가 없거나 게임이 끝났으면 false.
	// 네트워크 호스트 세션은 이것으로 고른 명령을 자기 실행 경로(방송 포함)로 넣는다.
	static bool Choose(PGameMaster& gameMaster, HGameplayCommand& outCommand);
	// 한 걸음 진행 (Choose + Submit). 에이전트가 없거나 게임이 끝났으면 false.
	static bool Step(PGameMaster& gameMaster, HList<HGameplayEvent>& outEvents, PString* outReason = nullptr);
	// 최대 maxSteps 걸음. 실제 진행한 걸음 수.
	static int32 Run(PGameMaster& gameMaster, int32 maxSteps, HList<HGameplayEvent>* outEvents = nullptr);
};
