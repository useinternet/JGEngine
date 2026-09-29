#pragma once
#include "GameMaster/State/GameplayEntityId.h"

// 컴포넌트 타입별 저장소의 공통 면. HGameplayState 가 값 복사 · 직렬화 · 엔티티 파괴 정리에 쓴다.
class GAMEFRAMEWORKS_API IGameplayComponentTable
{
public:
	virtual ~IGameplayComponentTable() = default;

	virtual uint64       GetTypeId() const = 0;
	virtual const PName& GetTypeName() const = 0;
	virtual HSTLUniquePtr<IGameplayComponentTable> Clone() const = 0;

	virtual bool   HasEntity(const HGameplayEntityId& id) const = 0;
	virtual bool   RemoveEntity(const HGameplayEntityId& id) = 0;
	virtual uint32 Count() const = 0;
	virtual void   Clear() = 0;

	virtual void Write(PJsonData& json) const = 0;
	virtual bool Read(const PJsonData& json) = 0;
};

// 타입 T 의 컴포넌트를 엔티티 인덱스로 보관한다. T 는 값 타입이고 IJsonable 을 구현해야 한다.
// 순회는 인덱스 오름차순 (결정론).
template<class T>
class HGameplayComponentTable : public IGameplayComponentTable
{
	static_assert(std::is_base_of<IJsonable, T>::value, "HGameplayComponentTable<T>: T must derive from IJsonable");

	PName          _typeName;
	HList<T>       _values;
	HList<uint8>   _present;
	HList<uint32>  _generations;
	uint32         _count = 0;

public:
	explicit HGameplayComponentTable(const PName& typeName)
		: _typeName(typeName)
	{
	}

	virtual ~HGameplayComponentTable() = default;

	// IGameplayComponentTable
	virtual uint64 GetTypeId() const override
	{
		return JGType::GenerateTypeID<T>();
	}

	virtual const PName& GetTypeName() const override
	{
		return _typeName;
	}

	virtual HSTLUniquePtr<IGameplayComponentTable> Clone() const override
	{
		HGameplayComponentTable<T>* copy = new HGameplayComponentTable<T>(_typeName);
		copy->_values      = _values;
		copy->_present     = _present;
		copy->_generations = _generations;
		copy->_count       = _count;
		return HSTLUniquePtr<IGameplayComponentTable>(copy);
	}

	virtual bool HasEntity(const HGameplayEntityId& id) const override
	{
		return Has(id);
	}

	virtual bool RemoveEntity(const HGameplayEntityId& id) override
	{
		return Remove(id);
	}

	virtual uint32 Count() const override
	{
		return _count;
	}

	virtual void Clear() override
	{
		_values.clear();
		_present.clear();
		_generations.clear();
		_count = 0;
	}

	virtual void Write(PJsonData& json) const override
	{
		HList<HGameplayEntityId> ids;
		HList<T> values;

		uint32 capacity = (uint32)_present.size();
		for (uint32 i = 0; i < capacity; ++i)
		{
			if (_present[i] != 0)
			{
				ids.push_back(HGameplayEntityId(i, _generations[i]));
				values.push_back(_values[i]);
			}
		}

		json.AddMember("Ids", ids);
		json.AddMember("Values", values);
	}

	virtual bool Read(const PJsonData& json) override
	{
		Clear();

		HList<HGameplayEntityId> ids;
		HList<T> values;
		if (json.GetData("Ids", &ids) == false)
		{
			return false;
		}
		if (json.GetData("Values", &values) == false)
		{
			return false;
		}
		if (ids.size() != values.size())
		{
			return false;
		}

		uint64 count = ids.size();
		for (uint64 i = 0; i < count; ++i)
		{
			Add(ids[i], values[i]);
		}
		return true;
	}
	// ~IGameplayComponentTable

	bool Has(const HGameplayEntityId& id) const
	{
		if (id.IsValid() == false)
		{
			return false;
		}
		if (id.Index >= (uint32)_present.size())
		{
			return false;
		}
		if (_present[id.Index] == 0)
		{
			return false;
		}
		return _generations[id.Index] == id.Generation;
	}

	T* Find(const HGameplayEntityId& id)
	{
		if (Has(id) == false)
		{
			return nullptr;
		}
		return &_values[id.Index];
	}

	const T* Find(const HGameplayEntityId& id) const
	{
		if (Has(id) == false)
		{
			return nullptr;
		}
		return &_values[id.Index];
	}

	// 없으면 만든다. 이미 있으면 값을 덮어쓴다.
	T& Add(const HGameplayEntityId& id, const T& value = T())
	{
		JG_CHECK(id.IsValid());
		ensureCapacity(id.Index + 1);

		if (_present[id.Index] == 0)
		{
			++_count;
		}

		_values[id.Index]      = value;
		_present[id.Index]     = 1;
		_generations[id.Index] = id.Generation;
		return _values[id.Index];
	}

	bool Remove(const HGameplayEntityId& id)
	{
		if (Has(id) == false)
		{
			return false;
		}
		_values[id.Index]      = T();
		_present[id.Index]     = 0;
		_generations[id.Index] = 0;
		--_count;
		return true;
	}

	// fn(const HGameplayEntityId&, const T&). 인덱스 오름차순.
	template<class Fn>
	void Each(Fn fn) const
	{
		uint32 capacity = (uint32)_present.size();
		for (uint32 i = 0; i < capacity; ++i)
		{
			if (_present[i] != 0)
			{
				fn(HGameplayEntityId(i, _generations[i]), _values[i]);
			}
		}
	}

	// fn(const HGameplayEntityId&, T&).
	template<class Fn>
	void EachMutable(Fn fn)
	{
		uint32 capacity = (uint32)_present.size();
		for (uint32 i = 0; i < capacity; ++i)
		{
			if (_present[i] != 0)
			{
				fn(HGameplayEntityId(i, _generations[i]), _values[i]);
			}
		}
	}

	void CollectIds(HList<HGameplayEntityId>& outIds) const
	{
		Each([&](const HGameplayEntityId& id, const T&)
		{
			outIds.push_back(id);
		});
	}

private:
	void ensureCapacity(uint32 capacity)
	{
		if ((uint32)_present.size() >= capacity)
		{
			return;
		}
		_values.resize(capacity);
		_present.resize(capacity, 0);
		_generations.resize(capacity, 0);
	}
};
