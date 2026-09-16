#pragma once
#include "CoreDefines.h"
#include "MemoryPool.h"
#include "Memory.h"

template<typename T>
class HAllocator
{
public:
	HAllocator() {}

	using value_type = T;

	template<typename Other>
	HAllocator(const HAllocator<Other>&) {}

	//초기화되지 않은 메모리 공간을 할당하여 그 시작 주소를 반환하는 함수
	T* allocate(std::size_t count)
	{
		if (GMemoryGlobalSystem::IsValid() == false)
		{
			JG_ASSERT(false);
		}
		
		const int32 MemSize = static_cast<int32>(count * sizeof(T));
		HMemoryPool* MemoryPool = GMemoryGlobalSystem::GetInstance().GetMemoryPool();
		void* Result = MemoryPool->Allocate(MemSize);
		return (T*)Result;
	}

	//  메모리 공간을 해제하는 함수
	void deallocate(T* ptr, size_t count)
	{
		if (GMemoryGlobalSystem::IsValid())
		{
			HMemoryPool* MemoryPool = GMemoryGlobalSystem::GetInstance().GetMemoryPool();
			MemoryPool->Deallocate(ptr);
		}
	}

	template <class U, class... Args>
	void construct(U* p, Args&&... args) {
		new(p) U(std::forward<Args>(args)...);
	}

	template <class U>
	void destroy(U* p) {
		p->~U();
	}
};

template<class Key, class Value>
using HHashMap = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, HAllocator<std::pair<const Key,Value>>>;

template<class Key, class Value>
using HMap = std::map<Key, Value, std::less<Key>, HAllocator<std::pair<const Key, Value>>>;

template<class T>
using HHashSet = std::unordered_set<T, std::hash<T>, std::equal_to<T>, HAllocator<T>>;

template<class T>
using HSet = std::set<T, std::less<T>, HAllocator<T>>;

template<class T>
using HList = std::vector<T, HAllocator<T>>;

template<class T>
using HDeque = std::deque<T, HAllocator<T>>;

template<class T>
using HQueue = std::queue<T, HDeque<T>>;

template<class T>
using HStack = std::stack<T, HDeque<T>>;
