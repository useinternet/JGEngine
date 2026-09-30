// 실행: clang++ -std=c++17 -O0 -o sharedptr_semantics_test.exe sharedptr_semantics_test.cpp && ./sharedptr_semantics_test.exe   (clang++ 은 C:/Program Files/LLVM/bin 에 있다)
// Memory.h 의 PSharedPtr/PWeakPtr 대입 코드를 바꿀 때마다 아래 "Memory.h 와 같은 코드" 부분을 다시 옮겨 붙이고 ALL OK 를 확인한다. (Memory_TODO 1-2, 4-1)
// Memory_TODO 1-2 검증: Source/Runtime/Core/Memory/Memory.h 의 PSharedPtr/PWeakPtr 대입 코드(assign/copy/move/operator=(nullptr_t))를
// 그대로 옮기고, 같은 시나리오를 std::shared_ptr/weak_ptr 로도 돌려 참조 수가 같은지 비교한다. 카운터는 풀 대신 지역 atomic.
#include <atomic>
#include <cstdio>
#include <memory>
#include <type_traits>
#include <utility>

using int32 = int;
using HAtomicInt32 = std::atomic_int;

class IMemoryObject { protected: IMemoryObject() = default; public: virtual ~IMemoryObject() = default; };

template<class T> class PSharedPtr;
template<class T> class PWeakPtr;
namespace PMemoryPrivate {
	template<class T> class PTemporaryOwner : public IMemoryObject {
	public:
		T* ptr = nullptr; HAtomicInt32* pRefCount = nullptr; HAtomicInt32* pWeakCount = nullptr;
		PTemporaryOwner(const PSharedPtr<T>& s) : ptr(s._ptr), pRefCount(s._pRefCount), pWeakCount(s._pWeakCount) {}
		PTemporaryOwner(const PWeakPtr<T>& w) : ptr(w._ptr), pRefCount(w._pRefCount), pWeakCount(w._pWeakCount) {}
		PTemporaryOwner(PSharedPtr<T>&& s) : ptr(s._ptr), pRefCount(s._pRefCount), pWeakCount(s._pWeakCount) { s._ptr = nullptr; s._pRefCount = nullptr; s._pWeakCount = nullptr; }
		PTemporaryOwner(PWeakPtr<T>&& w) : ptr(w._ptr), pRefCount(w._pRefCount), pWeakCount(w._pWeakCount) { w._ptr = nullptr; w._pRefCount = nullptr; w._pWeakCount = nullptr; }
	};
}

template<class T>
class PSharedPtr : public IMemoryObject {
	friend PMemoryPrivate::PTemporaryOwner<T>;
	friend PWeakPtr<T>;
public:
	T* _ptr = nullptr; HAtomicInt32* _pRefCount = nullptr; HAtomicInt32* _pWeakCount = nullptr;

	PSharedPtr() = default;
	PSharedPtr(nullptr_t) {}
	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PSharedPtr(const PSharedPtr<U>& rhs) { copy<U>(rhs); }
	PSharedPtr(const PSharedPtr<T>& rhs) { copy<T>(rhs); }
	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PSharedPtr(PSharedPtr<U>&& rhs) { move<U>(std::move(rhs)); }
	PSharedPtr(PSharedPtr<T>&& rhs) { move<T>(std::move(rhs)); }
	virtual ~PSharedPtr() { if (IsValid() == false) return; subRefCount(); }

	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PSharedPtr<T>& operator=(const PSharedPtr<U>& rhs) { copy<U>(rhs); return *this; }
	PSharedPtr<T>& operator=(const PSharedPtr<T>& rhs) { copy<T>(rhs); return *this; }
	template <class U, std::enable_if_t<std::is_base_of<T, U>::value, int32> = 0>
	PSharedPtr<T>& operator=(PSharedPtr<U>&& rhs) { move<U>(std::move(rhs)); return *this; }
	PSharedPtr<T>& operator=(PSharedPtr<T>&& rhs) { move<T>(std::move(rhs)); return *this; }
	PSharedPtr<T>& operator=(nullptr_t) { Reset(); return *this; }

	bool IsValid() const { return _ptr != nullptr; }
	void Reset() { if (IsValid()) { subRefCount(); _ptr = nullptr; _pRefCount = nullptr; _pWeakCount = nullptr; } }
private:
	// ---- Memory.h 와 같은 코드 ----
	void assign(T* inPtr, HAtomicInt32* inRefCount, HAtomicInt32* inWeakCount)
	{
		if (inPtr == _ptr) { return; }
		if (inRefCount != nullptr) { inRefCount->fetch_add(1); }
		subRefCount();
		_ptr = inPtr; _pRefCount = inRefCount; _pWeakCount = inWeakCount;
	}
	template<class U> void copy(const PSharedPtr<U>& rhs) { PMemoryPrivate::PTemporaryOwner<U> owner(rhs); assign(owner.ptr, owner.pRefCount, owner.pWeakCount); }
	template<class U> void move(PSharedPtr<U>&& rhs)
	{
		if (static_cast<const void*>(&rhs) == static_cast<const void*>(this)) { return; }
		PMemoryPrivate::PTemporaryOwner<U> owner(std::move(rhs));
		if (owner.ptr == _ptr) { if (owner.pRefCount != nullptr) { owner.pRefCount->fetch_sub(1); } return; }
		subRefCount();
		_ptr = owner.ptr; _pRefCount = owner.pRefCount; _pWeakCount = owner.pWeakCount;
	}
	void subRefCount() { if (_pRefCount) _pRefCount->fetch_sub(1); }
};

template<class T>
class PWeakPtr : public IMemoryObject {
	friend PMemoryPrivate::PTemporaryOwner<T>;
public:
	T* _ptr = nullptr; HAtomicInt32* _pRefCount = nullptr; HAtomicInt32* _pWeakCount = nullptr;
	PWeakPtr() = default;
	PWeakPtr(const PWeakPtr<T>& rhs) { copy<T>(rhs); }
	PWeakPtr(PWeakPtr<T>&& rhs) { move<T>(std::move(rhs)); }
	PWeakPtr(const PSharedPtr<T>& rhs) { set<T>(rhs); }
	virtual ~PWeakPtr() { if (_ptr == nullptr) return; subWeakCount(); }
	PWeakPtr<T>& operator=(const PWeakPtr<T>& rhs) { copy<T>(rhs); return *this; }
	PWeakPtr<T>& operator=(PWeakPtr<T>&& rhs) { move<T>(std::move(rhs)); return *this; }
	PWeakPtr<T>& operator=(const PSharedPtr<T>& rhs) { set<T>(rhs); return *this; }
	PWeakPtr<T>& operator=(nullptr_t) { Reset(); return *this; }
	void Reset() { if (_ptr != nullptr) { subWeakCount(); _ptr = nullptr; _pRefCount = nullptr; _pWeakCount = nullptr; } }
private:
	// ---- Memory.h 와 같은 코드 ----
	void assign(T* inPtr, HAtomicInt32* inRefCount, HAtomicInt32* inWeakCount)
	{
		if (inPtr == _ptr) { return; }
		if (inWeakCount != nullptr) { inWeakCount->fetch_add(1); }
		subWeakCount();
		_ptr = inPtr; _pRefCount = inRefCount; _pWeakCount = inWeakCount;
	}
	template <class U> void set(const PSharedPtr<U>& ptr) { PMemoryPrivate::PTemporaryOwner<U> owner(ptr); assign(owner.ptr, owner.pRefCount, owner.pWeakCount); }
	template <class U> void copy(const PWeakPtr<U>& rhs) { PMemoryPrivate::PTemporaryOwner<U> owner(rhs); assign(owner.ptr, owner.pRefCount, owner.pWeakCount); }
	template <class U> void move(PWeakPtr<U>&& rhs)
	{
		if (static_cast<const void*>(&rhs) == static_cast<const void*>(this)) { return; }
		PMemoryPrivate::PTemporaryOwner<U> owner(std::move(rhs));
		if (owner.ptr == _ptr) { if (owner.pWeakCount != nullptr) { owner.pWeakCount->fetch_sub(1); } return; }
		subWeakCount();
		_ptr = owner.ptr; _pRefCount = owner.pRefCount; _pWeakCount = owner.pWeakCount;
	}
	void subWeakCount() { if (_pWeakCount) _pWeakCount->fetch_sub(1); }
};

struct A : IMemoryObject { int v = 7; };

// 객체 하나 = (A, ref, weak) 묶음. Allocate<T>() 가 하는 일(참조 1로 시작)을 흉내낸다.
struct Obj { A a; HAtomicInt32 ref{0}; HAtomicInt32 weak{0}; PSharedPtr<A> make() { PSharedPtr<A> p; p._ptr = &a; p._pRefCount = &ref; p._pWeakCount = &weak; ref.fetch_add(1); return p; } };

static int failures = 0;
static void check(const char* name, long jg, long std_, bool jgValid, bool stdValid) {
	bool ok = (jg == std_) && (jgValid == stdValid);
	std::printf("%-46s jg ref=%d valid=%d | std use=%d valid=%d  %s\n", name, jg, (int)jgValid, std_, (int)stdValid, ok ? "OK" : "MISMATCH");
	if (!ok) failures++;
}

int main() {
	// 1. p = nullptr
	{ Obj o; auto p = o.make(); p = nullptr;
	  auto s = std::make_shared<int>(1); std::weak_ptr<int> ws = s; s = nullptr;
	  check("1 p = nullptr", o.ref.load(), ws.use_count(), p.IsValid(), (bool)s); }
	// 2. p = empty
	{ Obj o; auto p = o.make(); PSharedPtr<A> e; p = e;
	  auto s = std::make_shared<int>(1); std::weak_ptr<int> ws = s; std::shared_ptr<int> se; s = se;
	  check("2 p = empty", o.ref.load(), ws.use_count(), p.IsValid(), (bool)s); }
	// 3. p = p (self)
	{ Obj o; auto p = o.make(); PSharedPtr<A>& alias = p; p = alias;
	  auto s = std::make_shared<int>(1); std::shared_ptr<int>& sa = s; s = sa;
	  check("3 p = p (self)", o.ref.load(), s.use_count(), p.IsValid(), (bool)s); }
	// 4. p = move(empty)
	{ Obj o; auto p = o.make(); PSharedPtr<A> e; p = std::move(e);
	  auto s = std::make_shared<int>(1); std::weak_ptr<int> ws = s; std::shared_ptr<int> se; s = std::move(se);
	  check("4 p = move(empty)", o.ref.load(), ws.use_count(), p.IsValid(), (bool)s); }
	// 5. p = q, 둘이 같은 객체
	{ Obj o; auto p = o.make(); auto q = p; p = q;
	  auto s = std::make_shared<int>(1); auto s2 = s; s = s2;
	  check("5 p = q (same object, both live)", o.ref.load(), s.use_count(), p.IsValid(), (bool)s); }
	// 6. p = q, 다른 객체 (A 는 놓이고 B 는 2)
	{ Obj oa, ob; auto p = oa.make(); auto q = ob.make(); p = q;
	  auto sa = std::make_shared<int>(1); std::weak_ptr<int> wa = sa; auto sb = std::make_shared<int>(2); sa = sb;
	  check("6 p = q (other object) : A", oa.ref.load(), wa.use_count(), false, false);
	  check("6 p = q (other object) : B", ob.ref.load(), sb.use_count(), p.IsValid(), (bool)sa); }
	// 7. p = move(q), 같은 객체 (2 -> 1, q 비어야 함)
	{ Obj o; auto p = o.make(); auto q = p; p = std::move(q);
	  auto s = std::make_shared<int>(1); auto s2 = s; s = std::move(s2);
	  check("7 p = move(q) (same object)", o.ref.load(), s.use_count(), p.IsValid(), (bool)s);
	  check("7 ... q emptied", q.IsValid() ? 1 : 0, s2 ? 1 : 0, false, false); }
	// 8. p = move(q), 다른 객체
	{ Obj oa, ob; auto p = oa.make(); auto q = ob.make(); p = std::move(q);
	  auto sa = std::make_shared<int>(1); std::weak_ptr<int> wa = sa; auto sb = std::make_shared<int>(2); std::weak_ptr<int> wb = sb; sa = std::move(sb);
	  check("8 p = move(q) (other object) : A", oa.ref.load(), wa.use_count(), false, false);
	  check("8 p = move(q) (other object) : B", ob.ref.load(), wb.use_count(), p.IsValid(), (bool)sa); }
	// W1. w = nullptr 이 약참조를 놓는가
	{ Obj o; auto p = o.make(); PWeakPtr<A> w = p; long before = o.weak.load(); w = nullptr;
	  auto s = std::make_shared<int>(1); std::weak_ptr<int> ws = s; ws.reset();
	  std::printf("%-46s jg weak %d -> %d | std weak reset ok\n", "W1 w = nullptr", before, o.weak.load()); if (o.weak.load() != 0) failures++; }
	// W2. w = w (self) 유지
	{ Obj o; auto p = o.make(); PWeakPtr<A> w = p; PWeakPtr<A>& wa = w; w = wa;
	  std::printf("%-46s jg weak=%d (expect 1)\n", "W2 w = w (self)", o.weak.load()); if (o.weak.load() != 1) failures++; }
	// W3. w2 = move(w1) 뒤 w1 비어 있고 weak 1
	{ Obj o; auto p = o.make(); PWeakPtr<A> w1 = p; PWeakPtr<A> w2; w2 = std::move(w1);
	  std::printf("%-46s jg weak=%d w1.empty=%d (expect 1, 1)\n", "W3 w2 = move(w1)", o.weak.load(), (int)(w1._ptr == nullptr)); if (o.weak.load() != 1 || w1._ptr != nullptr) failures++; }
	// W4. 만료된 약참조 복사도 그대로 복사되는가 (표준과 같게)
	{ Obj o; PWeakPtr<A> w1; { auto p = o.make(); w1 = p; } PWeakPtr<A> w2 = w1;
	  auto s = std::make_shared<int>(1); std::weak_ptr<int> x1 = s; s.reset(); std::weak_ptr<int> x2 = x1;
	  std::printf("%-46s jg weak=%d ref=%d | std expired=%d copied=%d (expect weak 2)\n", "W4 copy of expired weak", o.weak.load(), o.ref.load(), (int)x1.expired(), (int)!x2.owner_before(x1) && !x1.owner_before(x2)); if (o.weak.load() != 2) failures++; }
	std::printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL OK" : "FAILED", failures, failures == 1 ? "" : "s");
	return failures == 0 ? 0 : 1;
}
