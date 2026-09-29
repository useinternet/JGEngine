#pragma once
#include "GameMaster/State/GameplayEntityId.h"

// 이벤트. "무슨 일이 일어났나". 프레젠테이션이 연출에 쓰는 유일한 통로이므로
// 이전 · 이후 · 원인 · 위치처럼 연출에 필요한 정보를 담는다. 직렬화 가능.
struct GAMEFRAMEWORKS_API HGameplayEvent : public IJsonable
{
	PName               Kind;
	HGameplayEntityId Subject;
	HGameplayEntityId Target;
	int32               Before = 0;
	int32               After  = 0;
	int32               Amount = 0;
	HGameplayCoord    From;
	HGameplayCoord    To;
	PName               Tag;      // 종류별 부가 식별자 (도착 영역 · 효과 이름 …)
	PName               Tag2;     // 두 번째 식별자 (출발 영역 …)

	// 인과. 엔진이 채운다. 같은 CauseSequence 는 같은 명령에서 파생된 것이고, Depth 는 효과 연쇄 깊이다.
	uint32 CauseSequence = 0;
	int32  Depth         = 0;

	HList<PName> ExtraKeys;
	HList<int32> ExtraValues;

	HGameplayEvent() = default;
	explicit HGameplayEvent(const PName& inKind);
	HGameplayEvent(const PName& inKind, const HGameplayEntityId& inSubject, const HGameplayEntityId& inTarget);

	void  SetExtra(const PName& key, int32 value);
	bool  GetExtra(const PName& key, int32* outValue) const;
	int32 GetExtra(const PName& key, int32 defaultValue) const;

	PString ToString() const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
