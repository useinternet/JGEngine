#include "PCH/PCH.h"

#include <functional>
#include <iostream>
#include "Core.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "Misc/Delegate.h"

// JGConsole.exe delegate.selftest — Core 대리자(HDelegate · HMulticastDelegate) 자체 검사.
//   주인 찾기(GetOwner · IsBoundTo · ClearIfBoundTo — Raw · SP · 람다), 주인 기준 해제(RemoveObject),
//   방송 중 해제 · 추가(Remove · RemoveObject · RemoveAll · 중첩 방송), 주인이 죽은 뒤의 SP 구독,
//   람다 저장(왼값은 사본 · 오른값은 이동 · 덧붙인 인자 · += — ER-023).
namespace
{
	int32 GCheckCount   = 0;
	int32 GFailureCount = 0;

	void check(bool bCondition, const char* caseName)
	{
		++GCheckCount;
		if (bCondition == false)
		{
			++GFailureCount;
			std::cout << "delegate.selftest FAILED: " << caseName << std::endl;
		}
	}

	using PTestEvent    = HMulticastDelegate<int32>;
	using PTestDelegate = HDelegate<void, int32>;

	// SP 구독 주인. PSharedPtr 로 잡으므로 IMemoryObject 뿌리가 하나다
	class PDelegateTestListener : public IMemoryObject
	{
	public:
		int32 Calls = 0;

		void OnEvent(int32 inValue)
		{
			Calls += inValue;
		}
	};

	// Raw 구독 주인
	struct HRawTestListener
	{
		int32 Calls = 0;

		void OnEvent(int32 inValue)
		{
			Calls += inValue;
		}
	};

	void staticTestFunction(int32 inValue)
	{
	}

	void testOwnerLookup()
	{
		PSharedPtr<PDelegateTestListener> owner = Allocate<PDelegateTestListener>();
		PTestDelegate spDelegate = PTestDelegate::CreateSP(owner, &PDelegateTestListener::OnEvent);
		check(spDelegate.GetOwner() == owner.GetRawPointer(), "owner: SP delegate reports its living owner");
		check(spDelegate.IsBoundTo(owner.GetRawPointer()), "owner: IsBoundTo(owner) is true for an SP delegate");

		PTestDelegate copied = spDelegate;
		check(copied.GetOwner() == owner.GetRawPointer(), "owner: a copied SP delegate keeps the owner");

		spDelegate.ClearIfBoundTo(owner.GetRawPointer());
		check(spDelegate.IsBound() == false, "owner: ClearIfBoundTo(owner) clears an SP delegate");

		HRawTestListener rawOwner;
		PTestDelegate rawDelegate = PTestDelegate::CreateRaw(&rawOwner, &HRawTestListener::OnEvent);
		check(rawDelegate.GetOwner() == &rawOwner && rawDelegate.IsBoundTo(&rawOwner), "owner: raw delegate reports its object");

		PTestDelegate lambdaDelegate = PTestDelegate::CreateLambda([](int32)
			{
			});
		PTestDelegate staticDelegate = PTestDelegate::CreateStatic(&staticTestFunction);
		check(lambdaDelegate.GetOwner() == nullptr && staticDelegate.GetOwner() == nullptr, "owner: lambda and static delegates have no owner");
		check(lambdaDelegate.IsBoundTo(owner.GetRawPointer()) == false, "owner: IsBoundTo(other object) is false");
	}

	void testRemoveObject()
	{
		// 같은 주인이 둘(사이에 다른 주인) — 뒤 원소를 당겨 온 자리도 다시 본다
		PTestEvent spEvent;
		PSharedPtr<PDelegateTestListener> a = Allocate<PDelegateTestListener>();
		PSharedPtr<PDelegateTestListener> b = Allocate<PDelegateTestListener>();
		spEvent.AddSP(a, &PDelegateTestListener::OnEvent);
		spEvent.AddSP(b, &PDelegateTestListener::OnEvent);
		spEvent.AddSP(a, &PDelegateTestListener::OnEvent);
		spEvent.RemoveObject(a.GetRawPointer());
		spEvent.BroadCast(1);
		check(a->Calls == 0 && b->Calls == 1, "remove: RemoveObject(owner) removes every SP subscription of that owner");
		check(spEvent.GetSize() == 1, "remove: only the other owner's SP subscription is left");

		PTestEvent rawEvent;
		HRawTestListener r1;
		HRawTestListener r2;
		rawEvent.AddRaw(&r1, &HRawTestListener::OnEvent);
		rawEvent.AddRaw(&r2, &HRawTestListener::OnEvent);
		rawEvent.AddRaw(&r1, &HRawTestListener::OnEvent);
		rawEvent.RemoveObject(&r1);
		rawEvent.BroadCast(1);
		check(r1.Calls == 0 && r2.Calls == 1 && rawEvent.GetSize() == 1, "remove: RemoveObject(owner) removes both raw subscriptions of the same object");

		rawEvent.RemoveObject(nullptr);
		check(rawEvent.GetSize() == 1, "remove: RemoveObject(nullptr) removes nothing");
	}

	void testRemoveDuringBroadcast()
	{
		{
			// 앞 콜백이 뒤 구독을 핸들로 뺀다
			PTestEvent event;
			int32 firstCalls  = 0;
			int32 secondCalls = 0;
			int32 thirdCalls  = 0;
			HDelegateHandle second;
			event.AddLambda([&](int32)
				{
					++firstCalls;
					event.Remove(second);
				});
			second = event.AddLambda([&](int32)
				{
					++secondCalls;
				});
			event.AddLambda([&](int32)
				{
					++thirdCalls;
				});
			event.BroadCast(1);
			check(firstCalls == 1 && secondCalls == 0 && thirdCalls == 1, "broadcast: a subscription removed by an earlier callback is skipped in the same broadcast");
			event.BroadCast(1);
			check(firstCalls == 2 && secondCalls == 0 && thirdCalls == 2, "broadcast: the removed subscription is not called in the next broadcast either");
			check(event.GetSize() == 2, "broadcast: the removed entry is dropped when the broadcast ends");
		}
		{
			// 콜백이 자기 자신을 뺀다 — 실행 중인 콜백을 지우지 않는다
			PTestEvent event;
			int32 calls = 0;
			HDelegateHandle self;
			self = event.AddLambda([&](int32)
				{
					++calls;
					event.Remove(self);
				});
			event.BroadCast(1);
			event.BroadCast(1);
			check(calls == 1 && event.GetSize() == 0, "broadcast: a callback removing itself runs once and is gone");
		}
		{
			// 방송 중 RemoveObject(SP 주인)
			PTestEvent event;
			PSharedPtr<PDelegateTestListener> listener = Allocate<PDelegateTestListener>();
			event.AddLambda([&](int32)
				{
					event.RemoveObject(listener.GetRawPointer());
				});
			event.AddSP(listener, &PDelegateTestListener::OnEvent);
			event.BroadCast(1);
			check(listener->Calls == 0 && event.GetSize() == 1, "broadcast: RemoveObject(SP owner) during a broadcast skips and drops that owner");
		}
		{
			// 방송 중 RemoveAll
			PTestEvent event;
			int32 calls = 0;
			event.AddLambda([&](int32)
				{
					++calls;
					event.RemoveAll();
				});
			event.AddLambda([&](int32)
				{
					++calls;
				});
			event.BroadCast(1);
			check(calls == 1 && event.GetSize() == 0, "broadcast: RemoveAll during a broadcast stops the rest and empties the event");
			event.BroadCast(1);
			check(calls == 1, "broadcast: nothing is called after RemoveAll");
		}
		{
			// 중첩 방송 — 안쪽에서 뺀 구독은 바깥 방송에서도 건너뛰고, 바깥 방송이 끝난 뒤 정리된다
			PTestEvent event;
			int32 depth       = 0;
			int32 victimCalls = 0;
			HDelegateHandle victim;
			event.AddLambda([&](int32 inValue)
				{
					if (depth == 0)
					{
						++depth;
						event.BroadCast(inValue);
						--depth;
					}
				});
			event.AddLambda([&](int32)
				{
					if (depth == 1)
					{
						event.Remove(victim);
					}
				});
			victim = event.AddLambda([&](int32)
				{
					++victimCalls;
				});
			event.BroadCast(1);
			check(victimCalls == 0 && event.GetSize() == 2, "broadcast: removal inside a nested broadcast is honored by the outer broadcast");
		}
		{
			// 방송 중 추가 — 멈추지 않고, 남아서 다음 방송에 불린다
			PTestEvent event;
			int32 addedCalls = 0;
			bool  bAdded     = false;
			event.AddLambda([&](int32)
				{
					if (bAdded == false)
					{
						bAdded = true;
						event.AddLambda([&](int32)
							{
								++addedCalls;
							});
					}
				});
			event.BroadCast(1);
			const int32 callsAfterFirst = addedCalls;
			event.BroadCast(1);
			check(event.GetSize() == 2 && addedCalls == callsAfterFirst + 1, "broadcast: a subscription added during a broadcast stays and is called in the next broadcast");
		}
	}

	void testDeadOwner()
	{
		PTestEvent event;
		PSharedPtr<PDelegateTestListener> owner = Allocate<PDelegateTestListener>();
		PDelegateTestListener* address = owner.GetRawPointer();
		PTestDelegate single = PTestDelegate::CreateSP(owner, &PDelegateTestListener::OnEvent);
		event.AddSP(owner, &PDelegateTestListener::OnEvent);

		// 마지막 강참조를 놓는다 — 주인이 죽는다(메모리는 GC 가 나중에 회수)
		owner.Reset();
		check(single.GetOwner() == nullptr, "dead owner: GetOwner returns nullptr, not a stale address");
		check(single.IsBoundTo(address) == false, "dead owner: IsBoundTo(old address) is false");

		single.ExecuteIfBound(1);
		event.BroadCast(1);
		check(true, "dead owner: calling an SP delegate whose owner died does nothing and does not crash");

		// 옛 주소로는 맞지 않는다(같은 주소에 새 객체가 들어와도 남의 구독을 건드리지 않는다)
		event.RemoveObject(address);
		check(event.GetSize() == 1, "dead owner: RemoveObject(old address) does not match the dead owner");
		event.RemoveAll();
		check(event.GetSize() == 0, "dead owner: RemoveAll drops the dead owner's subscription");
	}

	// 람다 저장(ER-023): 왼값(이름 있는 람다 · std::function 변수)은 사본으로, 오른값은 이동으로 갖는다. 참조로 가지면 원래 객체가 사라진 뒤 매달린다
	void testLambdaStorage()
	{
		{
			// 이름 있는 람다를 AddLambda — 사본이면 델리게이트를 불러도 원래 람다의 상태는 그대로다
			int32 seen = 0;
			auto counter = [count = 0, &seen](int32) mutable
				{
					++count;
					seen = count;
				};
			PTestEvent event;
			event.AddLambda(counter);
			event.BroadCast(1);
			event.BroadCast(1);
			counter(0);
			check(seen == 1, "lambda: AddLambda copies an lvalue lambda (calling the subscription does not change the original's state)");
		}
		{
			// std::function 변수를 AddLambda 한 뒤 그 변수에 다른 함수를 넣어도 구독은 처음 함수를 부른다
			int32 firstCalls  = 0;
			int32 secondCalls = 0;
			std::function<void(int32)> function = [&firstCalls](int32)
				{
					++firstCalls;
				};
			PTestEvent event;
			event.AddLambda(function);
			function = [&secondCalls](int32)
				{
					++secondCalls;
				};
			event.BroadCast(1);
			check(firstCalls == 1 && secondCalls == 0, "lambda: AddLambda copies an lvalue std::function (reassigning the variable later does not change the subscription)");
		}
		{
			// 요청서 완료 기준: 원래 std::function 이 사라진 뒤에도 정상으로 불린다
			int32 calls = 0;
			PTestEvent event;
			{
				std::function<void(int32)> scoped = [&calls](int32 inValue)
					{
						calls += inValue;
					};
				event.AddLambda(scoped);
			}
			bool bThrown = false;
			try
			{
				event.BroadCast(2);
			}
			catch (...)
			{
				bThrown = true;
			}
			check(bThrown == false && calls == 2, "lambda: a subscription made from an lvalue std::function still works after the variable is gone");
		}
		{
			// HDelegate::CreateLambda · BindLambda 도 같다
			int32 createdSeen = 0;
			auto createdCounter = [count = 0, &createdSeen](int32) mutable
				{
					++count;
					createdSeen = count;
				};
			PTestDelegate created = PTestDelegate::CreateLambda(createdCounter);
			created.Execute(1);
			created.Execute(1);
			createdCounter(0);
			check(createdSeen == 1, "lambda: CreateLambda copies an lvalue lambda");

			int32 boundSeen = 0;
			auto boundCounter = [count = 0, &boundSeen](int32) mutable
				{
					++count;
					boundSeen = count;
				};
			PTestDelegate bound;
			bound.BindLambda(boundCounter);
			bound.Execute(1);
			bound.Execute(1);
			boundCounter(0);
			check(boundSeen == 1, "lambda: BindLambda copies an lvalue lambda");

			// 델리게이트 사본(Clone)은 자기 람다를 갖는다
			PTestDelegate copied = created;
			created.Clear();
			copied.Execute(1);
			check(createdSeen == 3, "lambda: a copied delegate keeps working after the source is cleared");
		}
		{
			// 덧붙인 인자(payload): 왼값 · 오른값 람다 모두 그대로
			int32 lvalueReceived = 0;
			int32 rvalueReceived = 0;
			auto withPayload = [&lvalueReceived](int32 inValue, int32 inExtra)
				{
					lvalueReceived = inValue * 100 + inExtra;
				};
			PTestEvent event;
			event.AddLambda(withPayload, 7);
			event.AddLambda([&rvalueReceived](int32 inValue, int32 inExtra)
				{
					rvalueReceived = inValue * 100 + inExtra;
				}, 9);
			event.BroadCast(3);
			check(lvalueReceived == 307 && rvalueReceived == 309, "lambda: payload arguments reach both lvalue and rvalue lambdas");
		}
		{
			// += 는 왼값을 옮겨 가지 않는다(원래 변수는 그대로 쓸 수 있다)
			int32 calls = 0;
			std::function<void(int32)> function = [&calls](int32)
				{
					++calls;
				};
			PTestEvent event;
			event += function;
			check(static_cast<bool>(function) == true, "lambda: += leaves an lvalue std::function usable (it is copied, not moved)");
			event.BroadCast(1);
			function(1);
			check(calls == 2, "lambda: the += subscription and the original both work");
		}
	}

	bool executeSelfTest(const HConsoleCommandArgs& args)
	{
		GCheckCount   = 0;
		GFailureCount = 0;

		testOwnerLookup();
		testRemoveObject();
		testRemoveDuringBroadcast();
		testDeadOwner();
		testLambdaStorage();

		std::cout << "delegate.selftest: " << (GFailureCount == 0 ? "OK" : "FAILED")
			<< " (" << (GCheckCount - GFailureCount) << "/" << GCheckCount << ")" << std::endl;
		return GFailureCount == 0;
	}

	HAutoConsoleCommand DelegateSelfTestCommand(
		"delegate.selftest",
		"delegate.selftest",
		"Run the Core delegate self tests (owner lookup, RemoveObject, removal and add during a broadcast, dead SP owners, lambda storage)",
		&executeSelfTest);
}
