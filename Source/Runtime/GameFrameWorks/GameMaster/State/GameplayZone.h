#pragma once
#include "GameMaster/State/GameplayEntityId.h"
#include "GameMaster/State/GameplayRandomStream.h"

// 영역. 엔티티 ID 의 순서 있는 컬렉션 (덱 · 손 · 대기열 · 진영 …). 이름은 게임이 정한다.
struct GAMEFRAMEWORKS_API HGameplayZone : public IJsonable
{
	PName                      Name;
	HList<HGameplayEntityId> Entities;

	HGameplayZone() = default;
	explicit HGameplayZone(const PName& inName);

	int32 Count() const;
	bool  Contains(const HGameplayEntityId& id) const;
	int32 IndexOf(const HGameplayEntityId& id) const;

	void PushBack(const HGameplayEntityId& id);
	void PushFront(const HGameplayEntityId& id);
	void Insert(int32 index, const HGameplayEntityId& id);
	bool Remove(const HGameplayEntityId& id);
	void Clear();

	HGameplayEntityId Front() const;
	HGameplayEntityId Back() const;
	HGameplayEntityId PopFront();
	HGameplayEntityId PopBack();

	// Fisher-Yates. 스트림은 상태의 것을 넘긴다.
	void Shuffle(HGameplayRandomStream& rng);

	void CollectTo(HList<HGameplayEntityId>& outIds) const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

// 이름 → 영역. 등록 순서를 유지한다 (결정론 · 직렬화 안정).
struct GAMEFRAMEWORKS_API HGameplayZoneSet : public IJsonable
{
	HList<HGameplayZone> Zones;

	HGameplayZone*       Find(const PName& name);
	const HGameplayZone* Find(const PName& name) const;
	HGameplayZone&       FindOrAdd(const PName& name);
	bool                   Has(const PName& name) const;

	// 어떤 영역이든 들어 있으면 그 영역 이름을 돌려준다.
	bool FindZoneOf(const HGameplayEntityId& id, PName* outZoneName) const;
	// 모든 영역에서 제거.
	void RemoveEverywhere(const HGameplayEntityId& id);
	// 다른 영역으로 이동 (뒤에 붙인다). 원래 영역이 없어도 대상에 넣는다.
	// 엔티티 생존을 모른다. 규칙 코드는 HGameplayState::MoveToZone(죽은 ID 거부)을 쓴다.
	bool MoveTo(const HGameplayEntityId& id, const PName& toZone);

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
