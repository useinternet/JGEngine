#include "PCH/PCH.h"
#include "MemoryPool.h"
#include "CoreSystem.h"
#include "Misc/Log.h"
#include "Math/Math.h"

// 풀 할당/해제 트레이스 로그 스위치 (Memory_TODO 1-1). 기본 꺼짐.
// 켜려면 이 값을 1로 바꾸고 다시 빌드한다(이 파일만 다시 컴파일된다). 켠 빌드에서만 Document/Memory/tools/memlog_stats.sh 가 데이터를 얻는다.
// 켜면 프레임당 수백 줄이 파일·콘솔 두 싱크에 쓰이므로 측정용으로만 쓴다.
#ifndef JG_MEMORY_TRACE
#define JG_MEMORY_TRACE 0
#endif

void HMemoryChunk::Initialize(const HMemoryChunkArguments& InArguments)
{
	ChunkID = InArguments.ChunkID;

	TotalMemorySize = 0;
	for (const std::pair<EMemorySize, uint64>& Pair : InArguments.MemoryBlockCountMap)
	{
		uint64 MemorySize = ( static_cast<uint64>(Pair.first) + sizeof(HMemoryHeader) ) * Pair.second;
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


	JG_LOG(Memory, ELogLevel::Info, "[ID: %llu] Memory Chunk Initizlie. TotalMemory : %0.3f", GetID(), (float)TotalMemorySize / 1024);
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
	
	JG_LOG(Memory, ELogLevel::Info, "[ID: %llu] Memory Chunk Shutdown.", GetID());
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

// 주의: Allocate/Deallocate 안에서는 트레이스 스위치 밖의 JG_LOG를 부르지 않는다.
// GLogGlobalSystem::AddLog 가 PString::Format 으로 문자열을 만들며 힙을 쓰므로, 문자열이나 로깅 경로가 이 풀을 쓰게 되면
// Allocate → JG_LOG → Format → Allocate 로 무한 재귀한다. 실패·성장 로그(Memory_TODO 2-1)는 실패 분기에서만 부른다.
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

#if JG_MEMORY_TRACE
	// 주소는 memlog_stats.sh 가 소문자 16진수로 읽으므로 %p(MSVC는 대문자·고정폭) 대신 %llx 를 쓴다.
	JG_LOG(Memory, ELogLevel::Info, "[ID: %llu] Allocated %llu byte, address : %llx", GetID(), AdjustedMemorySize, (uint64)pResult);
#endif
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

#if JG_MEMORY_TRACE
	JG_LOG(Memory, ELogLevel::Info, "[ID: %llu] Deallocated %llu byte, address : %llx", GetID(), (uint64)MemoryHeader->MemorySize, (uint64)InMemory);
#endif
}

void HMemoryChunk::GetStatInfo(HMemoryChunkStatInfo& OutStatInfo)  const
{
	OutStatInfo = HMemoryChunkStatInfo();
	OutStatInfo.ChunkID = GetID();
	OutStatInfo.TotalMemorySize = TotalMemorySize;
	OutStatInfo.AllocatedMemorySize = 0;
	OutStatInfo.HeaderMemorySize = 0;

	uint64 OffsetPos = 0;
	while (OffsetPos < OutStatInfo.TotalMemorySize)
	{
		void* CurrentPos = (void*)((uint64)pMemory + OffsetPos);

		HMemoryHeader* pMemoryHeader = static_cast<HMemoryHeader*>(CurrentPos);
		OutStatInfo.HeaderMemorySize += sizeof(HMemoryHeader);

		HMemroyBlockStat BlockStat;
		BlockStat.MemorySize = pMemoryHeader->MemorySize;
		BlockStat.Ptr = (void*)((uint64)CurrentPos + sizeof(HMemoryHeader));

		if (pMemoryHeader->MemeoryState == EMemoryState::Allocated)
		{
			OutStatInfo.AllocatedMemorySize += (uint64)BlockStat.MemorySize;
			OutStatInfo.AllocatedMemories[BlockStat.MemorySize].push_back(BlockStat);
		}
		else
		{
			OutStatInfo.EmptyMemories[BlockStat.MemorySize].push_back(BlockStat);
		}

		OffsetPos += ((uint64)pMemoryHeader->MemorySize + sizeof(HMemoryHeader));
	}
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

		JG_LOG(Memory, ELogLevel::Warning, "Dismatch Allocated Thread(%llu) And Deallocated Thread(%llu), Memory Location : %llx", ID, ThisThreadID, (uint64)InMemory);
	}
}

void HMemoryPool::GetStatInfo(HMemoryPoolStatInfo& OutStatInfo)
{
	std::shared_lock<std::shared_mutex> lock(RWLock);

	for (const std::pair<const ThreadID, HMemoryChunk>& Pair : MemoryChunkMap)
	{
		const HMemoryChunk& MemoryChunk = Pair.second;
		HMemoryChunkStatInfo MemoryChunkStatInfo;
		MemoryChunk.GetStatInfo(MemoryChunkStatInfo);

		OutStatInfo.ChunkStatInfos.push_back(MemoryChunkStatInfo);
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
