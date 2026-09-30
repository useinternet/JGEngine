#include "PCH/PCH.h"
#include "Memory.h"
#include "Misc/Log.h"

GMemoryGlobalSystem::GMemoryGlobalSystem(int32 processBlockCountPerFrame)
{
	ProcessBlockCountPerFrame = processBlockCountPerFrame;
	bProcessingGarbageCollection = false;
	MemoryPool.Initialize();
}

namespace PMemoryPrivate
{
	static_assert(offsetof(HMemoryControlBlock, RefCount) == 0, "HMemoryControlBlock::RefCount must be the first member (the block is found from the RefCount pointer)");

	void ReleaseWeakReference(HAtomicInt32* inRefCount, HAtomicInt32* inWeakCount)
	{
		if (inWeakCount->fetch_sub(1) != 1)
		{
			return;
		}

		HMemoryControlBlock* block = reinterpret_cast<HMemoryControlBlock*>(inRefCount);
		JG_CHECK(&block->WeakCount == inWeakCount);
		delete block;
	}
}

GMemoryGlobalSystem::~GMemoryGlobalSystem()
{
	forceFlush();
	AllocatedMemoryBlocks.clear();
	MemoryPool.Shutdown();
}

void GMemoryGlobalSystem::Update()
{
	garbageCollection(1);

	// 10초 동안 쓰이지 않은 빈 페이지와 대형 캐시 블록을 시스템에 돌려준다 (Memory_TODO 2-1, 설계 §3-8b·§3-9)
	MemoryPool.Tick();
}

void GMemoryGlobalSystem::Flush()
{
	garbageCollection(0);
}

void GMemoryGlobalSystem::forceFlush()
{
	garbageCollection(-1);
}

void GMemoryGlobalSystem::garbageCollection(int32 level)
{
	HLockGuard<HRecursiveMutex> lock(Mutex);
	bProcessingGarbageCollection = true;
	if (level < 0)
	{
		while (AllocatedMemoryBlockQueue.empty() == false)
		{
			int32 deleteCount = garbageCollectionInternal((int32)AllocatedMemoryBlocks.size());

			if (deleteCount <= 0)
			{
				if (AllocatedMemoryBlocks.size() > 0)
				{
					garbageCollectionInternal((int32)AllocatedMemoryBlocks.size(), true);
				}
				break;
			}
		}
	}
	else if (level == 0)
	{
		while (garbageCollectionInternal((int32)AllocatedMemoryBlocks.size())) {}
	}
	else
	{
		int32 tempCnt = 0;

		while (AllocatedMemoryBlockQueue.empty() == false && tempCnt <= level)
		{
			++tempCnt;
			garbageCollectionInternal(ProcessBlockCountPerFrame);
		}
	}
	bProcessingGarbageCollection = false;
}

int32 GMemoryGlobalSystem::garbageCollectionInternal(int32 countPerFrame, bool bForce)
{
	int32 deleteCount = 0;
	int32 tempCnt     = 0;

	while (AllocatedMemoryBlockQueue.empty() == false)
	{
		if (tempCnt > countPerFrame)
		{
			break;
		}
		++tempCnt;

		void* ptr = AllocatedMemoryBlockQueue.front();

		const HMemoryBlock& memoryBlock = AllocatedMemoryBlocks[ptr];

		bool bIsClass = memoryBlock.bIsClass;
		PMemoryPrivate::HMemoryControlBlock* control = memoryBlock.Control;
		int32 refCount = control->RefCount.load();
		AllocatedMemoryBlockQueue.pop();
		if (refCount == 0 || bForce)
		{
			if (bIsClass == true)
			{
				((IMemoryObject*)ptr)->Destruction();
				((IMemoryObject*)ptr)->~IMemoryObject();
			}

			MemoryPool.Deallocate(ptr);

			AllocatedMemoryBlocks.erase(ptr);
			ptr = nullptr;

			// 객체가 살아 있는 동안 GC 가 들고 있던 약참조 몫을 놓는다. 남은 약참조가 없으면 여기서 제어 블록이 해제되고,
			// 있으면 마지막 약참조가 사라질 때 해제된다 (Memory_TODO 4-1).
			// 종료 때 강참조가 남은 채 강제 파괴한 객체는 그 PSharedPtr 들이 나중에 참조 수를 내리므로 제어 블록을 남긴다(종료 시점의 의도된 누수).
			if (refCount == 0)
			{
				PMemoryPrivate::ReleaseWeakReference(&control->RefCount, &control->WeakCount);
			}

			++deleteCount;
		}
		else
		{
			AllocatedMemoryBlockQueue.push(ptr);
		}
	}

	return deleteCount;
}