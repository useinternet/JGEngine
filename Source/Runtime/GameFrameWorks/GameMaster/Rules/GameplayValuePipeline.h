#pragma once
#include "GameMaster/Rules/GameplayModifier.h"
#include "GameMaster/Rules/GameplayRegistry.h"

// 수치 파이프라인. 값 종류마다 게임이 정의한 단계 순서대로 수정자를 적용한다.
// 단계가 정의되지 않은 값 종류는 등록 순서대로 모든 수정자를 적용한다.
class GAMEFRAMEWORKS_API PGameplayValuePipeline
{
	HList<HPair<PName, HList<PName>>> _stagesByKind;

public:
	void DefineStages(const PName& valueKind, const HList<PName>& stages);
	const HList<PName>* FindStages(const PName& valueKind) const;
	const HList<HPair<PName, HList<PName>>>& All() const;
	void Clear();

	int32 Compute(const HGameplayContext& ctx, const PGameplayRegistry<JGGameplayModifier>& modifiers, const HGameplayValueQuery& query) const;
};
