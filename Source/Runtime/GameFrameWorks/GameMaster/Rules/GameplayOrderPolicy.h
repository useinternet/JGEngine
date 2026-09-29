#pragma once
#include "GameMaster/State/GameplayState.h"

// 라운드마다 행동 순서를 만든다. 게임이 구현하거나 아래 기본 정책을 쓴다.
// 결과는 결정론적이어야 한다: 동률 처리까지 정책 안에서 끝낸다 (안정 정렬).
class GAMEFRAMEWORKS_API IGameplayOrderPolicy : public IMemoryObject
{
public:
	virtual ~IGameplayOrderPolicy() = default;
	virtual void BuildOrder(const HGameplayState& state, HList<HGameplayEntityId>& outOrder) const = 0;
};

// 영역의 순서 그대로. 영역 이름이 비어 있으면 살아 있는 엔티티 전부를 인덱스 순으로.
class GAMEFRAMEWORKS_API PGameplayZoneOrderPolicy : public IGameplayOrderPolicy
{
	PName _zoneName;

public:
	PGameplayZoneOrderPolicy() = default;
	explicit PGameplayZoneOrderPolicy(const PName& zoneName);
	virtual ~PGameplayZoneOrderPolicy() = default;

	virtual void BuildOrder(const HGameplayState& state, HList<HGameplayEntityId>& outOrder) const override;
};

// 영역의 엔티티를 정수 키로 안정 정렬. 키 함수는 게임이 준다.
class GAMEFRAMEWORKS_API PGameplayKeyedOrderPolicy : public IGameplayOrderPolicy
{
public:
	using HKeyFunction = std::function<int32(const HGameplayState&, const HGameplayEntityId&)>;

private:
	PName        _zoneName;
	HKeyFunction _keyFunction;
	bool         _bAscending = true;

public:
	PGameplayKeyedOrderPolicy() = default;
	PGameplayKeyedOrderPolicy(const PName& zoneName, const HKeyFunction& keyFunction, bool bAscending = true);
	virtual ~PGameplayKeyedOrderPolicy() = default;

	virtual void BuildOrder(const HGameplayState& state, HList<HGameplayEntityId>& outOrder) const override;
};
