#include "PCH/PCH.h"
#include "Memory.h"
#include "Misc/Log.h"

GMemoryGlobalSystem::GMemoryGlobalSystem(int32 processBlockCountPerFrame)
{
	ProcessBlockCountPerFrame = processBlockCountPerFrame;
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
	HLockGuard<HMutex> lock(Mutex);
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