#pragma once
#include "GameMaster/State/GameplayEntityId.h"

// 명령. 플레이어 · AI · 리플레이 · 테스트가 모두 이 형식으로 GameMaster 에 들어간다. 직렬화 가능.
struct GAMEFRAMEWORKS_API HGameplayCommand : public IJsonable
{
	PName                      Kind;
	HGameplayEntityId        Actor;
	HList<HGameplayEntityId> Targets;
	HList<int32>               Params;
	HList<HGameplayCoord>    Path;

	// 게임 확장 필드. 이름-값 병렬 배열 (순서 고정 = 결정론 · 직렬화 안정)
	HList<PName> ExtraKeys;
	HList<int32> ExtraValues;

	HGameplayCommand() = default;
	HGameplayCommand(const PName& inKind, const HGameplayEntityId& inActor);

	HGameplayEntityId Target() const;                       // Targets[0] 또는 None
	int32 Param(int32 index, int32 defaultValue = 0) const;

	void  SetExtra(const PName& key, int32 value);
	bool  GetExtra(const PName& key, int32* outValue) const;
	int32 GetExtra(const PName& key, int32 defaultValue) const;

	PString ToString() const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
