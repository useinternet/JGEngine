#include "PCH/PCH.h"
#include "Memory.h"
#include "Misc/Log.h"

GMemoryGlobalSystem::GMemoryGlobalSystem(int32 processBlockCountPerFrame)
{
	ProcessBlockCountPerFrame = processBlockCountPerFrame;
	bProcessingGarbageCollection = false;
	MemoryPool.Initialize();
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

		bool bIsClass   = memoryBlock.bIsClass;
		int32 refCount  = memoryBlock.RefCount->load();
		int32 weakCount = memoryBlock.WeakCount->load();
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

			++deleteCount;
		}
		else
		{
			AllocatedMemoryBlockQueue.push(ptr);
		}
	}

	return deleteCount;
}