#pragma once
#include "GameMaster/State/GameplayEntityId.h"

// 효과 요청. 명령 핸들러 · 트리거가 큐에 넣고, 효과가 하나씩 해결한다. 직렬화 가능 (선택 대기 중 큐 보존).
struct GAMEFRAMEWORKS_API HGameplayEffectRequest : public IJsonable
{
	PName                      Kind;
	HGameplayEntityId        Subject;
	HGameplayEntityId        Target;
	HList<HGameplayEntityId> Targets;
	HList<int32>               Params;
	HGameplayCoord           Coord;
	PName                      Tag;              // 영역 이름 등 부가 식별자

	// 원인. 엔진이 채운다.
	uint32 CauseSequence = 0;
	int32  Depth         = 0;

	// 선택 대기 재진입. ResolveChoice 가 오면 엔진이 채워서 같은 효과를 다시 부른다.
	bool                       bHasChoice = false;
	HList<HGameplayEntityId> Choice;

	HGameplayEffectRequest() = default;
	HGameplayEffectRequest(const PName& inKind, const HGameplayEntityId& inSubject, const HGameplayEntityId& inTarget);

	bool  HasChoice() const;
	int32 Param(int32 index, int32 defaultValue = 0) const;

	PString ToString() const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
