#pragma once
#include "GameMaster/State/GameplayEntityId.h"
#include "GameMaster/State/GameplayEntityRegistry.h"
#include "GameMaster/State/GameplayComponentTable.h"
#include "GameMaster/State/GameplayZone.h"
#include "GameMaster/State/GameplayTurnState.h"
#include "GameMaster/State/GameplayBoardState.h"
#include "GameMaster/State/GameplayRandomStream.h"
#include "GameMaster/State/GameplayChoice.h"

// 되감기 지점 하나 (ER-027). 표시한 순간의 상태(지점 목록은 뺌)를 상태 JSON 글로 든다.
// 표시한 뒤 바뀌지 않으므로 해시는 표시 · 읽기 때 한 번 구하고, 상태 체크섬에는 이 해시만 섞는다.
struct GAMEFRAMEWORK_API HGameplayRewindPoint : public IJsonable
{
	PName   Name;
	PString StateText;   // HGameplayState JSON (SchemaVersion + State, 지점 목록 없음)
	uint64  Hash = 0;    // StateText 의 FNV-1a. 저장하지 않고 읽을 때 다시 구한다

	virtual ~HGameplayRewindPoint() = default;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

// Gameplay 상태 전체. 값 타입 — 복사가 곧 스냅샷이다.
// JGObject / PSharedPtr 을 안에 두지 않는다. 포인터 동일성이 없어야 결정론이 지켜지고 복사가 안전하다.
// const 로 받은 상태로는 값을 바꿀 수 없다 (Find · Get · Table 의 const 판은 const 를 돌려준다).
struct GAMEFRAMEWORK_API HGameplayState : public IJsonable
{
	// 2 (2026-10-17, ER-027): 되감기 지점(RewindPoints) · 등록부 세대 바닥(GenerationFloors). 둘 다 비면 JSON 에 쓰지 않는다.
	static constexpr uint32 SchemaVersion            = 2;
	// 이 판부터 지금 판까지는 마이그레이터 없이 그대로 읽는다 (1 문서 = 지점 · 세대 바닥이 없는 2 문서).
	static constexpr uint32 MinReadableSchemaVersion = 1;
	static constexpr int32  RandomStreamCount = (int32)EGameplayRandomStream::Count;

	HGameplayEntityRegistry Entities;
	HGameplayZoneSet        Zones;
	HGameplayBoardState     Board;
	HGameplayTurnState      Turn;
	HGameplayChoice         Choice;
	HGameplayRandomStream   Random[RandomStreamCount];
	uint32                    Sequence = 0;    // 실행된 명령 수. 이벤트의 CauseSequence 와 짝. 되감기로 되돌아가지 않는다
	uint64                    Seed     = 0;
	HList<HGameplayRewindPoint> RewindPoints;  // 되감기 지점 (ER-027). 표시 순서, 같은 이름은 하나. 되감기로 되돌아가지 않는다

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

	// 지금 명령을 낼 수 있는 행동자 (입력 행동자). 선택 대기면 선택자 한 명, 아니면 흐름이 정한 Turn.Actors. 시작 전 · 끝난 뒤는 없다.
	// 컨트롤러 입력 · 네트워크 세션 · AI 러너가 이것을 본다.
	void              CollectInputActors(HList<HGameplayEntityId>& outActors) const;
	bool              IsInputActor(const HGameplayEntityId& id) const;
	HGameplayEntityId FirstInputActor() const;   // 한 명씩 행동하는 흐름에서의 편의. 없으면 None

	// 영역
	HGameplayZone&       Zone(const PName& name);
	const HGameplayZone* FindZone(const PName& name) const;

	// 되감기 지점 (ER-027). 규칙 코드는 HGameplayContext 의 MarkRewindPoint · ClearRewindPoint · RewindTo 를 쓴다
	// (RewindTo 는 엔진이 남은 효과 · 반응도 버려야 해서 ctx 를 거친다). 지점은 상태 안이라 저장 · 문서 · Undo · Simulate 에 함께 간다.
	const HGameplayRewindPoint* FindRewindPoint(const PName& name) const;
	void MarkRewindPoint(const PName& name);    // 지금 상태(지점 목록 빼고)를 남긴다. 같은 이름은 새 것으로 바꾼다
	bool ClearRewindPoint(const PName& name);   // 없으면 false
	// 상태 쪽 되감기: 지점의 상태로 바꾼다. 되돌리지 않는 것 — Sequence · Seed · 지점 목록 · 지금까지 낸 엔티티 세대(지점 뒤에 생긴 번호는 죽은 번호).
	// random 이 KeepCurrent 면 난수 스트림도 되감기 직전 것을 둔다. 지점이 없거나 읽기에 실패하면 false 이고 아무것도 바꾸지 않는다.
	bool RewindTo(const PName& name, EGameplayRewindRandom random);

	// 직렬화 · 검증
	PString ToJsonString() const;
	bool    FromJsonString(const PString& text);
	uint64  Checksum() const;   // 직렬화 텍스트의 FNV-1a. 같은 명령열이면 같은 값. 되감기 지점은 사본 대신 지점 해시만 섞는다

	HList<HSTLUniquePtr<IGameplayComponentTable>> Tables;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;

private:
	// 되감기 지점을 JSON 에 어떻게 쓸지. Full = 사본 글까지(저장 · 문서 · 전송), HashOnly = 이름 · 해시만(체크섬), None = 쓰지 않음(지점 사본 자신)
	enum class ERewindPointJson : int32
	{
		Full = 0,
		HashOnly,
		None,
	};

	IGameplayComponentTable* findTable(uint64 typeId) const;
	void    copyFrom(const HGameplayState& rhs);
	void    moveFrom(HGameplayState& rhs) noexcept;
	void    writeState(PJsonData& json, ERewindPointJson points) const;
	PString toJsonText(ERewindPointJson points) const;

	struct HStateJsonWriter;   // writeState 를 IJsonable 로 감싼다 (JSON 트리를 복사하지 않고 "State" 로 쓰기)
};
