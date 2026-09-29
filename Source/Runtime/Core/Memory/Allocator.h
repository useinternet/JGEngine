#pragma once
#include "CoreDefines.h"
#include "MemoryPool.h"
#include "Memory.h"

namespace HAllocatorPrivate
{
	// 메모리 시스템이 내려간 뒤의 deallocate 는 무시한다 (정적 컨테이너가 DLL 언로드 때 소멸하는 경우. JGGraphicsDefine.h 의 규칙 위반).
	// 디버그에서는 처음 한 번 디버거 출력으로 알린다 (Memory_TODO 2-4).
	inline void ReportDeallocateAfterShutdownOnce()
	{
#ifdef _DEBUG
		static bool bReported = false;
		if (bReported == false)
		{
			bReported = true;
			OutputDebugStringA("[JGEngine][Memory] HAllocator::deallocate was called after the memory system shut down (static container holding pool memory?). ignored.\n");
		}
#endif
	}
}

template<typename T>
class HAllocator
{
public:
	using value_type = T;

	// 상태가 없어 모든 인스턴스가 같다. HList::swap 과 이동 대입이 할당자 비교 없이 컴파일된다 (Memory_TODO 2-4).
	using is_always_equal = std::true_type;

	HAllocator() = default;

	template<typename Other>
	HAllocator(const HAllocator<Other>&) {}

	//초기화되지 않은 메모리 공간을 할당하여 그 시작 주소를 반환하는 함수
	// 정렬이 16을 넘는 타입(alignas(32) 등)은 풀이 대형(정렬) 경로로 보내 맞춘다.
	T* allocate(std::size_t count)
	{
		JG_CHECK(GMemoryGlobalSystem::IsValid() == true);

		HMemoryPool* MemoryPool = GMemoryGlobalSystem::GetInstance().GetMemoryPool();
		void* Result = MemoryPool->Allocate(static_cast<uint64>(count) * sizeof(T), alignof(T));
		JG_CHECK(Result != nullptr);
		return static_cast<T*>(Result);
	}

	//  메모리 공간을 해제하는 함수
	void deallocate(T* ptr, size_t count)
	{
		if (GMemoryGlobalSystem::IsValid())
		{
			HMemoryPool* MemoryPool = GMemoryGlobalSystem::GetInstance().GetMemoryPool();
			MemoryPool->Deallocate(ptr);
		}
		else
		{
			HAllocatorPrivate::ReportDeallocateAfterShutdownOnce();
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

	template<typename Other>
	bool operator==(const HAllocator<Other>&) const
	{
		return true;
	}

	template<typename Other>
	bool operator!=(const HAllocator<Other>&) const
	{
		return false;
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
