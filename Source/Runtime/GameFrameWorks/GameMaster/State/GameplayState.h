#pragma once
#include "GameMaster/State/GameplayEntityId.h"
#include "GameMaster/State/GameplayEntityRegistry.h"
#include "GameMaster/State/GameplayComponentTable.h"
#include "GameMaster/State/GameplayZone.h"
#include "GameMaster/State/GameplayTurnState.h"
#include "GameMaster/State/GameplayBoardState.h"
#include "GameMaster/State/GameplayRandomStream.h"
#include "GameMaster/State/GameplayChoice.h"

// Gameplay 상태 전체. 값 타입 — 복사가 곧 스냅샷이다.
// JGObject / PSharedPtr 을 안에 두지 않는다. 포인터 동일성이 없어야 결정론이 지켜지고 복사가 안전하다.
// const 로 받은 상태로는 값을 바꿀 수 없다 (Find · Get · Table 의 const 판은 const 를 돌려준다).
struct GAMEFRAMEWORKS_API HGameplayState : public IJsonable
{
	static constexpr uint32 SchemaVersion     = 1;
	static constexpr int32  RandomStreamCount = (int32)EGameplayRandomStream::Count;

	HGameplayEntityRegistry Entities;
	HGameplayZoneSet        Zones;
	HGameplayBoardState     Board;
	HGameplayTurnState      Turn;
	HGameplayChoice         Choice;
	HGameplayRandomStream   Random[RandomStreamCount];
	uint32                    Sequence = 0;    // 실행된 명령 수. 이벤트의 CauseSequence 와 짝
	uint64                    Seed     = 0;

	HGameplayState();
	HGameplayState(const HGameplayState& rhs);
	HGameplayState(HGameplayState&& rhs) noexcept;
	HGameplayState& operator=(const HGameplayState& rhs);
	HGameplayState& operator=(HGameplayState&& rhs) noexcept;
	virtual ~HGameplayState() = default;

	// 난수
	void SeedAll(uint64 seed);
	HGameplayRandomStream& Rng(EGameplayRandomStream stream);

	// 엔티티
	HGameplayEntityId CreateEntity();
	bool DestroyEntity(const HGameplayEntityId& id);   // 모든 테이블 · 영역 · 보드에서 제거
	bool IsAlive(const HGameplayEntityId& id) const;

	// 컴포넌트 테이블
	template<class T>
	HGameplayComponentTable<T>* RegisterTable(const PName& typeName)
	{
		HGameplayComponentTable<T>* existing = Table<T>();
		if (existing != nullptr)
		{
			return existing;
		}
		HGameplayComponentTable<T>* table = new HGameplayComponentTable<T>(typeName);
		Tables.push_back(HSTLUniquePtr<IGameplayComponentTable>(table));
		return table;
	}

	template<class T>
	HGameplayComponentTable<T>* Table()
	{
		return static_cast<HGameplayComponentTable<T>*>(findTable(JGType::GenerateTypeID<T>()));
	}

	template<class T>
	const HGameplayComponentTable<T>* Table() const
	{
		return static_cast<const HGameplayComponentTable<T>*>(findTable(JGType::GenerateTypeID<T>()));
	}

	template<class T>
	bool Has(const HGameplayEntityId& id) const
	{
		const HGameplayComponentTable<T>* table = Table<T>();
		if (table == nullptr)
		{
			return false;
		}
		return table->Has(id);
	}

	template<class T>
	T* Find(const HGameplayEntityId& id)
	{
		HGameplayComponentTable<T>* table = Table<T>();
		if (table == nullptr)
		{
			return nullptr;
		}
		return table->Find(id);
	}

	template<class T>
	const T* Find(const HGameplayEntityId& id) const
	{
		const HGameplayComponentTable<T>* table = Table<T>();
		if (table == nullptr)
		{
			return nullptr;
		}
		return table->Find(id);
	}

	// 없으면 assert. 있는지 모르면 Find 를 쓴다.
	template<class T>
	T& Get(const HGameplayEntityId& id)
	{
		T* found = Find<T>(id);
		JG_CHECK(found != nullptr);
		return *found;
	}

	template<class T>
	const T& Get(const HGameplayEntityId& id) const
	{
		const T* found = Find<T>(id);
		JG_CHECK(found != nullptr);
		return *found;
	}

	// 없으면 만들고, 있으면 값을 덮어쓴다. 죽은 ID(파괴 뒤 늦게 온 요청)는 nullptr — 같은 번호를 재사용한 엔티티의 값을 덮지 않는다.
	template<class T>
	T* Add(const HGameplayEntityId& id, const T& value = T())
	{
		if (IsAlive(id) == false)
		{
			return nullptr;
		}
		HGameplayComponentTable<T>* table = Table<T>();
		JG_CHECK(table != nullptr);
		return &table->Add(id, value);
	}

	template<class T>
	bool Remove(const HGameplayEntityId& id)
	{
		HGameplayComponentTable<T>* table = Table<T>();
		if (table == nullptr)
		{
			return false;
		}
		return table->Remove(id);
	}

	// fn(const HGameplayEntityId&, const T&). 인덱스 오름차순.
	template<class T, class Fn>
	void Each(Fn fn) const
	{
		const HGameplayComponentTable<T>* table = Table<T>();
		if (table == nullptr)
		{
			return;
		}
		table->Each(fn);
	}

	template<class T, class Fn>
	void EachMutable(Fn fn)
	{
		HGameplayComponentTable<T>* table = Table<T>();
		if (table == nullptr)
		{
			return;
		}
		table->EachMutable(fn);
	}

	// 보드 · 영역 쓰기. 죽은 ID 는 false. Board · Zones 를 직접 쓰는 함수는 생존을 보지 않으므로 규칙 코드는 이쪽을 쓴다.
	bool SetBoardPosition(const HGameplayEntityId& id, const HGameplayCoord& coord);
	bool MoveToZone(const HGameplayEntityId& id, const PName& zoneName);

	// 영역
	HGameplayZone&       Zone(const PName& name);
	const HGameplayZone* FindZone(const PName& name) const;

	// 직렬화 · 검증
	PString ToJsonString() const;
	bool    FromJsonString(const PString& text);
	uint64  Checksum() const;   // 직렬화 텍스트의 FNV-1a. 같은 명령열이면 같은 값

	HList<HSTLUniquePtr<IGameplayComponentTable>> Tables;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;

private:
	IGameplayComponentTable* findTable(uint64 typeId) const;
	void copyFrom(const HGameplayState& rhs);
	void moveFrom(HGameplayState& rhs) noexcept;
};
