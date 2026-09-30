#pragma once

#include "CoreDefines.h"
#include "CoreSystem.h"
#include "MemoryPool.h"

class IMemoryObject
{
protected:
	IMemoryObject() = default;
public:
	virtual ~IMemoryObject() = default;
	
protected:
	friend class GMemoryGlobalSystem;
	virtual void Construct() {}
	virtual void Destruction() {}
};

class PMemoryObject : public IMemoryObject
{
public:
	virtual ~PMemoryObject() = default;
};

template<class T>
class PWeakPtr;

template<class T>
class PSharedPtr;

namespace PMemoryPrivate
{
	// 참조 수 두 개(강·약)를 한 덩어리로 둔다 (Memory_TODO 4-1, 2026-09-29).
	// 강참조 수가 0 이 되면 GC 가 객체를 파괴하지만, 이 블록은 약참조가 모두 사라질 때까지 남는다(std::shared_ptr 의 제어 블록과 같은 규칙).
	// 그래서 만료된 PWeakPtr 가 IsValid·Reset·재대입·소멸에서 카운터를 만져도 해제된 메모리가 아니다.
	// 이전에는 GC 가 객체와 함께 카운터를 해제해서(WeakCount 무시) 남은 약참조가 해제된 메모리를 읽고 썼다.
	// WeakCount = 약참조 수 + 1(객체가 살아 있는 동안 GC 가 들고 있는 몫). 0 이 되는 순간 블록을 해제한다(ReleaseWeakReference).
	// CRT 힙(new/delete)에 둔다: 메모리 시스템(풀)이 내려간 뒤에 소멸하는 스마트 포인터도 카운터를 안전하게 만질 수 있게.
	struct HMemoryControlBlock
	{
		HAtomicInt32 RefCount  = 0;   // 첫 멤버여야 한다: 블록 주소 = RefCount 주소 (Memory.cpp static_assert)
		HAtomicInt32 WeakCount = 1;
	};

	// 약참조 몫 하나를 놓고, 마지막 몫이면 제어 블록을 해제한다. 어느 스레드에서든 부를 수 있다.
	void ReleaseWeakReference(HAtomicInt32* inRefCount, HAtomicInt32* inWeakCount);

	template<class T>
	class PTemporaryOwner : public IMemoryObject
	{
	public:
		T* ptr = nullptr;

		HAtomicInt32* pRefCount = nullptr;
		HAtomicInt32* pWeakCount = nullptr;

		PTemporaryOwner(const PSharedPtr<T>& sharedPtr)
			: ptr(sharedPtr._ptr)
			, pRefCount(sharedPtr._pRefCount)
			, pWeakCount(sharedPtr._pWeakCount)
		{}

		PTemporaryOwner(const PWeakPtr<T>& weakPtr)
			: ptr(weakPtr._ptr)
			, pRefCount(weakPtr._pRefCount)
			, pWeakCount(weakPtr._pWeakCount)
		{}

		PTemporaryOwner(PSharedPtr<T>&& sharedPtr)
			: ptr(sharedPtr._ptr)
			, pRefCount(sharedPtr._pRefCount)
			, pWeakCount(sharedPtr._pWeakCount)
		{
			sharedPtr._ptr = nullptr;
			sharedPtr._pRefCount = nullptr;
			sharedPtr._pWeakCount = nullptr;
		}
		
		PTemporaryOwner(PWeakPtr<T>&& weakPtr)
			: ptr(weakPtr._ptr)
			, pRefCount(weakPtr._pRefCount)
			, pWeakCount(weakPtr._pWeakCount)
		{
			weakPtr._ptr = nullptr;
			weakPtr._pRefCount  = nullptr;
			weakPtr._pWeakCount = nullptr;
		}

		virtual ~PTemporaryOwner() = default;
	};
};

template<class T>
class TRawPtr
{
public:

};


template<class T>
class PSharedPtr : public IMemoryObject
{
private:
	friend class GMemoryGlobalSystem;
	friend PMemoryPrivate::PTemporaryOwner<T>;
	friend PWeakPtr<T>;

	T* _ptr = nullptr;

	HAtomicInt32* _pRefCount = nullptr;
	HAtomicInt32* _pWeakCount = nullptr;

public:
	PSharedPtr()  = default;
	PSharedPtr(nullptr_t) {};

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PSharedPtr(const PSharedPtr<U>& rhs)
	{
		copy<U>(rhs);
	}

	PSharedPtr(const PSharedPtr<T>& rhs)
	{
		copy<T>(rhs);
	}

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PSharedPtr(PSharedPtr<U>&& rhs)
	{
		move<U>(std::move(rhs));
	}

	PSharedPtr(PSharedPtr<T>&& rhs)
	{
		move<T>(std::move(rhs));
	}

	virtual ~PSharedPtr()
	{
		if (IsValid() == false)
		{
			return;
		}
		subRefCount();
	}
public:
	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PSharedPtr<T>& operator=(const PSharedPtr<U>& rhs)
	{
		copy<U>(rhs);

		return *this;
	}

	PSharedPtr<T>& operator=(const PSharedPtr<T>& rhs)
	{
		copy<T>(rhs);
		
		return *this;
	}

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PSharedPtr<T>& operator=(PSharedPtr<U>&& rhs)
	{
		move<U>(std::move(rhs));

		return *this;
	}

	PSharedPtr<T>& operator=(PSharedPtr<T>&& rhs)
	{
		move<T>(std::move(rhs));

		return *this;
	}

	// `p = nullptr` 은 참조를 놓는다. (2026-09-28 Memory_TODO 1-2. 이전에는 빈 임시 객체의 이동 대입으로 흘러가 아무 일도 하지 않았다)
	PSharedPtr<T>& operator=(nullptr_t)
	{
		Reset();

		return *this;
	}

	bool operator==(const PSharedPtr<T>& ptr) const
	{
		return _ptr == ptr._ptr;
	}

	bool operator!=(const PSharedPtr<T>& ptr) const
	{
		return _ptr != ptr._ptr;
	}

	bool operator==(nullptr_t) const
	{
		return _ptr == nullptr;
	}

	bool operator!=(nullptr_t) const
	{
		return _ptr != nullptr;
	}

	T* operator->()
	{
		return _ptr;
	}

	const T* operator->() const
	{
		return _ptr;
	}

	T& operator*()
	{
		return *_ptr;
	}

	const T& operator*() const
	{
		return *_ptr;
	}
public:
	bool IsUnique() const
	{
		if (IsValid() == false)
		{
			return false;
		}
		return _pRefCount->load() == 1;
	}

	bool IsValid() const 
	{
		return _ptr != nullptr;
	}

	T* GetRawPointer() const
	{
		return _ptr;
	}

	const T* GetRawConstPointer() const
	{
		return _ptr;
	}

	void Reset()
	{
		if (IsValid() == true)
		{
			subRefCount();
			_ptr = nullptr;
			_pRefCount  = nullptr;
			_pWeakCount = nullptr;
		}
	}
private:
	// 대입 규칙 (2026-09-28 Memory_TODO 1-2. 표준 shared_ptr 과 같다):
	//  - 빈 rhs 를 대입하면 놓는다(Reset 과 같다). 이전에는 early return 으로 아무 일도 하지 않아 `p = 빈포인터` 가 참조를 놓지 않았다.
	//  - 같은 객체(자기 대입 포함)면 참조 수를 바꾸지 않는다. 이전에는 Reset 을 먼저 해서 `p = p` 가 참조를 잃었다.
	//  - 새 참조를 먼저 잡고 옛 참조를 놓는다.
	void assign(T* inPtr, HAtomicInt32* inRefCount, HAtomicInt32* inWeakCount)
	{
		if (inPtr == _ptr)
		{
			return;
		}

		if (inRefCount != nullptr)
		{
			inRefCount->fetch_add(1);
		}

		subRefCount();

		_ptr        = inPtr;
		_pRefCount  = inRefCount;
		_pWeakCount = inWeakCount;
	}

	template<class U>
	void copy(const PSharedPtr<U>& rhs)
	{
		PMemoryPrivate::PTemporaryOwner<U> owner(rhs);
		assign(owner.ptr, owner.pRefCount, owner.pWeakCount);
	}

	template<class U>
	void move(PSharedPtr<U>&& rhs)
	{
		// 자기 자신을 이동 대입하면 그대로 둔다.
		if (static_cast<const void*>(&rhs) == static_cast<const void*>(this))
		{
			return;
		}

		// rhs 의 참조를 넘겨받는다. owner 를 만드는 순간 rhs 는 비워진다.
		PMemoryPrivate::PTemporaryOwner<U> owner(std::move(rhs));

		if (owner.ptr == _ptr)
		{
			// 같은 객체: rhs 가 들고 있던 참조 하나는 우리가 대신 놓는다.
			if (owner.pRefCount != nullptr)
			{
				owner.pRefCount->fetch_sub(1);
			}
			return;
		}

		subRefCount();

		_ptr        = owner.ptr;
		_pRefCount  = owner.pRefCount;
		_pWeakCount = owner.pWeakCount;
	}

	void addRefCount()
	{
		if (_pRefCount == nullptr)
		{
			return;
		}

		_pRefCount->fetch_add(1);
	}

	void subRefCount()
	{
		if (_pRefCount == nullptr)
		{
			return;
		}

		_pRefCount->fetch_sub(1);
	}
};

template<class T>
class PWeakPtr : public IMemoryObject
{
	friend PMemoryPrivate::PTemporaryOwner<T>;
	friend class GMemoryGlobalSystem;

	T* _ptr = nullptr;

	HAtomicInt32* _pRefCount   = nullptr;
	HAtomicInt32* _pWeakCount  = nullptr;

public:
	PWeakPtr() = default;

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PWeakPtr(const PWeakPtr<U>& rhs)
	{
		copy<U>(rhs);
	}

	PWeakPtr(const PWeakPtr<T>& rhs)
	{
		copy<T>(rhs);
	}

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PWeakPtr(PWeakPtr<U>&& rhs)
	{
		move<U>(std::move(rhs));
	}

	PWeakPtr(PWeakPtr<T>&& rhs)
	{
		move<T>(std::move(rhs));
	}

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PWeakPtr(const PSharedPtr<U>& rhs)
	{
		set<U>(rhs);
	}

	PWeakPtr(const PSharedPtr<T>& rhs)
	{
		set<T>(rhs);
	}

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PWeakPtr(PSharedPtr<U>&& rhs)
	{
		set<U>(rhs);
	}

	PWeakPtr(PSharedPtr<T>&& rhs)
	{
		set<T>(rhs);
	}

	// 객체가 이미 죽었어도 약참조 몫은 놓는다. 제어 블록은 약참조가 모두 사라질 때까지 살아 있다 (Memory_TODO 4-1).
	// 이전에는 IsValid() 로 해제된 카운터를 읽었고, 죽은 객체면 몫을 놓지 않았다.
	virtual ~PWeakPtr()
	{
		subWeakCount();
	}
public:
	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PWeakPtr<T>& operator=(const PWeakPtr<U>& rhs)
	{
		copy<U>(rhs);

		return *this;
	}

	PWeakPtr<T>& operator=(const PWeakPtr<T>& rhs)
	{
		copy<T>(rhs);

		return *this;
	}

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PWeakPtr<T>& operator=(PWeakPtr<U>&& rhs)
	{
		move<U>(std::move(rhs));

		return *this;
	}

	PWeakPtr<T>& operator=(PWeakPtr<T>&& rhs)
	{
		move<T>(std::move(rhs));

		return *this;
	}

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PWeakPtr<T>& operator=(const PSharedPtr<U>& rhs)
	{
		set<U>(rhs);

		return *this;
	}

	PWeakPtr<T>& operator=(const PSharedPtr<T>& rhs)
	{
		set<T>(rhs);

		return *this;
	}

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PWeakPtr<T>& operator=(PSharedPtr<U>&& rhs)
	{
		set<U>(rhs);

		return *this;
	}

	PWeakPtr<T>& operator=(PSharedPtr<T>&& rhs)
	{
		set<T>(rhs);

		return *this;
	}

	// `w = nullptr` 은 약참조를 놓는다. (2026-09-28 Memory_TODO 1-2. 이전에는 빈 PSharedPtr 대입으로 흘러가 아무 일도 하지 않았다)
	PWeakPtr<T>& operator=(nullptr_t)
	{
		Reset();

		return *this;
	}

	void Reset()
	{
		if (isValidPointer() == true)
		{
			subWeakCount();
			_ptr = nullptr;
			_pRefCount  = nullptr;
			_pWeakCount = nullptr;
		}
	}
	
	// 강참조 수가 0 보다 클 때만 1 올린다(CAS). 0 이면 GC 가 파괴했거나 파괴할 객체라 되살리지 않는다. (Memory_TODO 4-1)
	// 이전에는 주소로 다시 찾아서(Wrap(_ptr)) 같은 주소에 새 객체가 들어오면 그 객체를 잡을 수 있었고, 검사와 증가 사이에 경합이 있었다.
	PSharedPtr<T> Pin() const
	{
		if (_ptr == nullptr || _pRefCount == nullptr)
		{
			return nullptr;
		}

		int32 refCount = _pRefCount->load();
		while (refCount > 0)
		{
			if (_pRefCount->compare_exchange_weak(refCount, refCount + 1) == true)
			{
				PSharedPtr<T> result;
				result._ptr        = _ptr;
				result._pRefCount  = _pRefCount;
				result._pWeakCount = _pWeakCount;
				return result;
			}
		}

		return nullptr;
	}

public:
	bool IsValid() const
	{
		if (_ptr == nullptr)
		{
			return false;
		}
		return _pRefCount->load() > 0;
	}

private:
	bool isValidPointer() const 
	{
		return _ptr != nullptr;
	}

	// 대입 규칙은 PSharedPtr 와 같다 (2026-09-28 Memory_TODO 1-2): 빈 rhs = Reset, 같은 객체면 변화 없음, 새 약참조를 먼저 잡고 옛 것을 놓는다.
	// 만료된 약참조(참조 수 0)도 표준 weak_ptr 처럼 그대로 복사한다. 이전에는 IsValid 검사에 걸려 대입이 통째로 무시됐다.
	void assign(T* inPtr, HAtomicInt32* inRefCount, HAtomicInt32* inWeakCount)
	{
		if (inPtr == _ptr)
		{
			return;
		}

		if (inWeakCount != nullptr)
		{
			inWeakCount->fetch_add(1);
		}

		subWeakCount();

		_ptr        = inPtr;
		_pRefCount  = inRefCount;
		_pWeakCount = inWeakCount;
	}

	template <class U>
	void set(const PSharedPtr<U>& ptr)
	{
		PMemoryPrivate::PTemporaryOwner<U> owner(ptr);
		assign(owner.ptr, owner.pRefCount, owner.pWeakCount);
	}

	template <class U>
	void copy(const PWeakPtr<U>& rhs)
	{
		PMemoryPrivate::PTemporaryOwner<U> owner(rhs);
		assign(owner.ptr, owner.pRefCount, owner.pWeakCount);
	}

	template <class U>
	void move(PWeakPtr<U>&& rhs)
	{
		// 자기 자신을 이동 대입하면 그대로 둔다.
		if (static_cast<const void*>(&rhs) == static_cast<const void*>(this))
		{
			return;
		}

		// rhs 의 약참조를 넘겨받는다. owner 를 만드는 순간 rhs 는 비워진다.
		PMemoryPrivate::PTemporaryOwner<U> owner(std::move(rhs));

		if (owner.ptr == _ptr)
		{
			// 같은 객체: rhs 가 들고 있던 약참조 하나는 우리가 대신 놓는다.
			if (owner.pWeakCount != nullptr)
			{
				PMemoryPrivate::ReleaseWeakReference(owner.pRefCount, owner.pWeakCount);
			}
			return;
		}

		subWeakCount();

		_ptr        = owner.ptr;
		_pRefCount  = owner.pRefCount;
		_pWeakCount = owner.pWeakCount;
	}

	void addWeakCount()
	{
		if (_pWeakCount == nullptr)
		{
			return;
		}

		_pWeakCount->fetch_add(1);
	}

	void subWeakCount()
	{
		if (_pWeakCount == nullptr)
		{
			return;
		}

		PMemoryPrivate::ReleaseWeakReference(_pRefCount, _pWeakCount);
	}
};



class GMemoryGlobalSystem : public GGlobalSystemInstance<GMemoryGlobalSystem>
{
	struct HMemoryBlock
	{
		void*  Ptr  = nullptr;
		bool bIsClass = false;
		PMemoryPrivate::HMemoryControlBlock* Control = nullptr;   // 참조 수. 객체가 파괴돼도 약참조가 남아 있으면 살아 있다 (Memory_TODO 4-1)
	};
public:
	mutable std::unordered_map<const void*, HMemoryBlock> AllocatedMemoryBlocks;
	mutable std::queue<void*> AllocatedMemoryBlockQueue;
	mutable HRecursiveMutex Mutex;
	mutable bool bProcessingGarbageCollection;
	mutable HMemoryPool MemoryPool;
	int32 ProcessBlockCountPerFrame;

public:
	GMemoryGlobalSystem(int32 processBlockCountPerFrame = 10240);
	virtual ~GMemoryGlobalSystem();

protected:
	virtual void Update() override;

public:
	template<class T, class ...Args>
	PSharedPtr<T> Allocate(Args&& ... args) const
	{
		// 스마트 포인터로 다루는 클래스는 IMemoryObject 파생이어야 한다(GC 가 IMemoryObject* 로 Destruction()·소멸자를 부른다).
		// 위반은 컴파일 시점에 잡는다. (2026-09-28 Memory_TODO 1-3. 이전에는 런타임 JG_ASSERT 라 릴리스 구성에서 통과했다. Wrap<T> 와 같은 규칙)
		static_assert(std::is_class<T>::value == false || std::is_base_of<IMemoryObject, T>::value,
			"Allocate<T>: T must derive from IMemoryObject (classes held by PSharedPtr/PWeakPtr must be rooted at IMemoryObject)");

		void* MemPtr = MemoryPool.Allocate(sizeof(T), alignof(T));
		JG_CHECK(MemPtr != nullptr);   // 풀은 성장하므로 nullptr 을 돌려주지 않는다. 계약을 코드로 남긴다 (Memory_TODO 2-1)
		PSharedPtr<T> Result;
		Result._ptr = new(MemPtr) T(std::forward<Args>(args)...);


		// @NOTE
		// 아래 항목 HMemoryPool 로 이동
		HMemoryBlock memoryBlock;
		memoryBlock.Ptr = Result._ptr;
		memoryBlock.bIsClass = std::is_class<T>::value;
		memoryBlock.Control = new PMemoryPrivate::HMemoryControlBlock();
		memoryBlock.Control->RefCount.store(1);   // Result 가 첫 강참조. WeakCount 1 은 객체가 살아 있는 동안 GC 가 들고 있는 몫

		Result._pRefCount  = &memoryBlock.Control->RefCount;
		Result._pWeakCount = &memoryBlock.Control->WeakCount;

		{
			HLockGuard<HRecursiveMutex> lock(Mutex);
			AllocatedMemoryBlocks.emplace(Result._ptr, std::move(memoryBlock));
			AllocatedMemoryBlockQueue.push(Result._ptr);
		}

		// ~ @NOTE

		IMemoryObject* memObject = Result._ptr;
		memObject->Construct();

		return Result;
	}

	HMemoryPool* GetMemoryPool() const
	{
		return &MemoryPool;
	}

	template<class T>
	PSharedPtr<T> Wrap(const T* fromThis) const
	{
		// 규칙: 스마트 포인터로 다루는 클래스(인터페이스 포함)는 IMemoryObject 파생이어야 하고,
		// IMemoryObject 뿌리가 정확히 하나(오프셋 0)여야 한다. 그래야 인터페이스 타입 포인터도 할당 주소와 같아
		// 아래 블록 조회가 성립한다. 위반은 실행 시점이 아니라 컴파일 시점에 잡는다.
		static_assert(std::is_class<T>::value == false || std::is_base_of<IMemoryObject, T>::value,
			"Wrap<T>: T must derive from IMemoryObject (interfaces held by PSharedPtr/PWeakPtr must be rooted at IMemoryObject)");

		if (fromThis == nullptr)
		{
			return PSharedPtr<T>();
		}

		// @NOTE
		// 아래 항목 MemoryPool 로 이동
		HLockGuard<HRecursiveMutex> lock(Mutex);
		if (AllocatedMemoryBlocks.find(fromThis) == AllocatedMemoryBlocks.end())
		{
			return PSharedPtr<T>();
		}
		HMemoryBlock& memoryBlock = AllocatedMemoryBlocks[(const void*)fromThis];
		// 여기 까지


		PSharedPtr<T> Result;
		Result._ptr = static_cast<T*>(memoryBlock.Ptr);
		Result._pRefCount  = &memoryBlock.Control->RefCount;
		Result._pWeakCount = &memoryBlock.Control->WeakCount;   // 이전에는 비어 있어서, 이 강참조로 만든 약참조가 제어 블록을 붙들지 못했다 (4-1)
		Result._pRefCount->fetch_add(1);

		return Result;
	}

	template<class T, class U>
	PSharedPtr<T> RawFastCast(PSharedPtr<U> ptr)
	{
		if (std::is_base_of<T, U>::value == false && std::is_base_of<U, T>::value == false)
		{
			return nullptr;
		}

		return RawFastCastUnChecked<T, U>(ptr);
	}

	template<class T, class U>
	PWeakPtr<T> RawFastCast(PWeakPtr<U> ptr)
	{
		if (std::is_base_of<T, U>::value == false && std::is_base_of<U, T>::value == false)
		{
			return nullptr;
		}

		return RawFastCastUnChecked<T, U>(ptr);
	}

	template<class T, class U>
	PSharedPtr<T> RawDynamicCast(PSharedPtr<U> ptr)
	{
		if (std::is_base_of<T, U>::value == false && std::is_base_of<U, T>::value == false)
		{
			return nullptr;
		}

		return RawDynamicCastUnChecked<T, U>(ptr);
	}

	template<class T, class U>
	PWeakPtr<T> RawDynamicCast(PWeakPtr<U> ptr)
	{
		if (std::is_base_of<T, U>::value == false && std::is_base_of<U, T>::value == false)
		{
			return nullptr;
		}

		return RawDynamicCastUnChecked<T, U>(ptr);
	}

	template<class T, class U>
	PSharedPtr<T> RawFastCastUnChecked(PSharedPtr<U> ptr)
	{
		PSharedPtr<T> result;
		result._ptr = static_cast<T*>(ptr._ptr);
		result._pRefCount = ptr._pRefCount;
		result._pWeakCount = ptr._pWeakCount;
		result.addRefCount();

		return result;
	}

	template<class T, class U>
	PWeakPtr<T> RawFastCastUnChecked(PWeakPtr<U> ptr)
	{
		PWeakPtr<T> result;
		result._ptr = static_cast<T*>(ptr._ptr);
		result._pRefCount = ptr._pRefCount;
		result._pWeakCount = ptr._pWeakCount;
		result.addWeakCount();

		return result;
	}

	template<class T, class U>
	PWeakPtr<T> RawDynamicCastUnChecked(PWeakPtr<U> ptr)
	{
		T* result_ptr = dynamic_cast<T*>(ptr._ptr);
		if (result_ptr == nullptr)
		{
			return nullptr;
		}

		PWeakPtr<T> result;
		result._ptr = result_ptr;
		result._pRefCount  = ptr._pRefCount;
		result._pWeakCount = ptr._pWeakCount;
		result.addWeakCount();

		return result;
	}

	template<class T, class U>
	PSharedPtr<T> RawDynamicCastUnChecked(PSharedPtr<U> ptr)
	{
		T* result_ptr = dynamic_cast<T*>(ptr._ptr);
		if (result_ptr == nullptr)
		{
			return nullptr;
		}

		PSharedPtr<T> result;
		result._ptr = result_ptr;
		result._pRefCount = ptr._pRefCount;
		result._pWeakCount = ptr._pWeakCount;
		result.addRefCount();

		return result;
	}

	void Flush();
private:
	void forceFlush();
	void garbageCollection(int32 level);
	int32 garbageCollectionInternal(int32 countPerFrame, bool bForce = false);
};


template<class T, class ...Args>
inline PSharedPtr<T> Allocate(Args ...args)
{
	return GMemoryGlobalSystem::GetInstance().Allocate<T>(args...);
}


template<class T>
inline PSharedPtr<T> Allocate(const T& data)
{
	PSharedPtr<T> result = GMemoryGlobalSystem::GetInstance().Allocate<T>();
	*result = data;
	return result;
}

template<class T>
inline PSharedPtr<T> SharedWrap(const T* fromThis)
{
	return GMemoryGlobalSystem::GetInstance().Wrap(fromThis);
}

template<class T, class U>
inline PSharedPtr<T> RawFastCast(PSharedPtr<U> ptr)
{
	return GMemoryGlobalSystem::GetInstance().RawFastCast<T, U>(ptr);
}

template<class T, class U>
inline T* RawFastCast(U* ptr)
{
	if (std::is_base_of<T, U>::value == false && std::is_base_of<U, T>::value == false)
	{
		return nullptr;
	}

	return static_cast<T*>(ptr);
}

template<class T, class U>
inline PWeakPtr<T> RawFastCast(PWeakPtr<U> ptr)
{
	return GMemoryGlobalSystem::GetInstance().RawFastCast<T, U>(ptr);
}

template<class T, class U>
inline PSharedPtr<T> RawDynamicCast(PSharedPtr<U> ptr)
{
	return GMemoryGlobalSystem::GetInstance().RawDynamicCast<T, U>(ptr);
}

template<class T, class U>
inline PWeakPtr<T> RawDynamicCast(PWeakPtr<U> ptr)
{
	return GMemoryGlobalSystem::GetInstance().RawDynamicCast<T, U>(ptr);
}

