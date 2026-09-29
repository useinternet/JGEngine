#pragma once
#include "GameMaster/Rules/GameplayContext.h"
#include "GameplayModifier.generation.h"

// 수치 파이프라인 질의. 어떤 값을(Kind) 누가(Subject) 누구에게(Target) 기본값 얼마로(Base) 계산하나.
struct GAMEFRAMEWORKS_API HGameplayValueQuery
{
	PName               Kind;
	HGameplayEntityId Subject;
	HGameplayEntityId Target;
	int32               Base = 0;
};

// 수치 파이프라인의 한 단계에서 값을 바꾼다. 게임이 파생해 리플렉션으로 자동 등록된다.
// 배율은 정수 백분율로 다룬다 (결정론). "대신에" 류 규칙은 트리거가 아니라 여기로 온다.
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayModifier : public JGObject
{
	JG_GENERATED_CLASS_BODY

public:
	JGGameplayModifier() = default;
	virtual ~JGGameplayModifier() = default;

	virtual PName GetKind() const;
	virtual PName GetValueKind() const;   // 어떤 값에 적용되나. HGameplayValueQuery::Kind
	virtual PName GetStage() const;       // 어느 단계에서. PGameMaster::DefineValueStages 로 정의한 이름
	virtual bool  Applies(const HGameplayContext& ctx, const HGameplayValueQuery& query) const;
	virtual int32 Apply(const HGameplayContext& ctx, const HGameplayValueQuery& query, int32 value) const;
};
