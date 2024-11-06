#include "PCH/PCH.h"
#include "MemoryPool.h"
#include "CoreSystem.h"
#include "Misc/Log.h"
#include "Math/Math.h"

void HMemoryChunk::Initialize(const HMemoryChunkArguments& InArguments)
{
	ChunkID = InArguments.ChunkID;

	TotalMemorySize = 0;
	for (const std::pair<EMemorySize, uint64>& Pair : InArguments.MemoryBlockCountMap)
	{
		uint64 MemorySize = static_cast<uint64>(Pair.first) * Pair.second;
		TotalMemorySize += MemorySize;
	}

	pMemory = malloc(TotalMemorySize);

	char* pMemoryCurrPos = static_cast<char*>(pMemory);

	for (const std::pair<EMemorySize, uint64>& Pair : InArguments.MemoryBlockCountMap)
	{
		EMemorySize MemorySize = Pair.first;
		uint64 MemoryCount = Pair.second;

		for (uint64 i = 0; i < MemoryCount; ++i)
		{
			HMemoryHeader* MemoryHeader = (HMemoryHeader*)(pMemoryCurrPos);
			MemoryHeader->MemeoryState = EMemoryState::None;
			MemoryHeader->MemorySize = MemorySize;
			MemoryHeader->OwnerChunkID = GetID();

			pMemoryCurrPos += sizeof(HMemoryHeader);

			MemoryQueues[MemorySize].push(pMemoryCurrPos);

			pMemoryCurrPos += static_cast<uint64>(MemorySize);
		}
	}


	JG_LOG(Memory, ELogLevel::Info, "[ID: %u] Memory Chunk Initizlie. TotalMemory : %0.3f", GetID(), (float)TotalMemorySize / 1024);
}

void HMemoryChunk::Shutdown()
{
	if (pMemory == nullptr)
	{
		return;
	}

	free(pMemory);
	pMemory = nullptr;

	ChunkID = 0;
	TotalMemorySize = 0;
	MemoryQueues.clear();
	
	JG_LOG(Memory, ELogLevel::Info, "[ID: %u] Memory Chunk Shutdown.", GetID());
}

uint64 HMemoryChunk::GetID() const
{
	return ChunkID;
}

uint64 HMemoryChunk::GetSize() const
{
	return TotalMemorySize;
}

uint64 HMemoryChunk::HasSpace(uint64 InMemorySize) const
{
	return MemoryQueues.at(static_cast<EMemorySize>(InMemorySize)).empty() == false;
}

void* HMemoryChunk::Allocate(uint64 InMemorySize)
{
	uint64 AdjustedMemorySize = HMath::AlignPowerOfTwo(InMemorySize);
	if (AdjustedMemorySize < 4)
	{
		AdjustedMemorySize = 4;
	}

	if (HasSpace(AdjustedMemorySize) == false)
	{
		JG_ASSERT("no memory space left.");
		return nullptr;
	}

	EMemorySize MemorySize = static_cast<EMemorySize>(AdjustedMemorySize);

	void* pResult = MemoryQueues[MemorySize].front();
	MemoryQueues[MemorySize].pop();

	HMemoryHeader* MemoryHeader = GetMemoryHeader(pResult);
	MemoryHeader->MemeoryState = EMemoryState::Allocated;

	JG_LOG(Memory, ELogLevel::Info, "[ID: %u] Allocated %u byte, address : %x", GetID(), AdjustedMemorySize, pResult);
	return pResult;
}

void HMemoryChunk::Deallocate(void* InMemory)
{
	HMemoryHeader* MemoryHeader = GetMemoryHeader(InMemory);
	if (MemoryHeader->MemeoryState != EMemoryState::Allocated)
	{
		JG_ASSERT(true);
	}

	MemoryHeader->MemeoryState = EMemoryState::None;
	MemoryQueues[MemoryHeader->MemorySize].push(InMemory);

	JG_LOG(Memory, ELogLevel::Info, "[ID: %u] Deallocated %u byte, address : %x", GetID(), MemoryHeader->MemorySize, InMemory);
}

void HMemoryPool::Initialize()
{

}

void HMemoryPool::Shutdown()
{
	for (HPair<const ThreadID, HMemoryChunk>& Pair : MemoryChunkMap)
	{
		Pair.second.Shutdown();
	}
	MemoryChunkMap.clear();
}

void* HMemoryPool::Allocate(uint64 InMemorySize)
{
	ThreadID ID = std::hash<std::thread::id>()(std::this_thread::get_id());

	auto Itr = MemoryChunkMap.end();
	{
		std::shared_lock<std::shared_mutex> lock(RWLock);
		Itr = MemoryChunkMap.find(ID);
	}

	if (Itr == MemoryChunkMap.end())
	{
		std::lock_guard<std::shared_mutex> Lock(RWLock);
		MakeMemoryChunk(ID);
		Itr = MemoryChunkMap.find(ID);
	}

	void* AllocatedMemoryPtr = Itr->second.Allocate(InMemorySize);
	return AllocatedMemoryPtr;
}

void  HMemoryPool::Deallocate(void* InMemory)
{
	const HMemoryHeader* MemoryHeader = GetMemoryHeader(InMemory);
	uint64 ID = MemoryHeader->OwnerChunkID;
	uint64 ThisThreadID = std::hash<std::thread::id>()(std::this_thread::get_id());
	if (ID == ThisThreadID)
	{
		std::shared_lock<std::shared_mutex> lock(RWLock);

		auto Itr = MemoryChunkMap.find(ID);
		JG_ASSERT(Itr != MemoryChunkMap.end());

		Itr->second.Deallocate(InMemory);
	}
	else
	{
		std::lock_guard<std::shared_mutex> Lock(RWLock);

		auto Itr = MemoryChunkMap.find(ID);
		JG_ASSERT(Itr != MemoryChunkMap.end());

		Itr->second.Deallocate(InMemory);

		JG_LOG(Memory, ELogLevel::Warning, "Dismatch Allocated Thread(%u) And Deallocated Thread(%u), Memory Location : %x", ID, ThisThreadID, InMemory);
	}
}

void HMemoryPool::MakeMemoryChunk(uint64 ChunkID)
{
	if (MemoryChunkMap.find(ChunkID) != MemoryChunkMap.end())
	{
		return;
	}

	HMemoryChunkArguments ChunkArgs;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_4_Byte] = 1024;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_8_Byte] = 4096;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_16_Byte] = 4096;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_32_Byte] = 4096;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_64_Byte] = 4096;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_128_Byte] = 2048;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_256_Byte] = 2048;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_512_Byte] = 2048;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_1_KB] = 2048;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_2_KB] = 2048;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_4_KB] = 1024;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_8_KB] = 1024;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_16_KB] = 512;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_32_KB] = 256;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_64_KB] = 128;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_128_KB] = 64;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_256_KB] = 64;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_512_KB] = 32;
	ChunkArgs.MemoryBlockCountMap[EMemorySize::_1_MB] = 16;
	ChunkArgs.ChunkID = ChunkID; 

	MemoryChunkMap[ChunkArgs.ChunkID].Initialize(ChunkArgs);
}
