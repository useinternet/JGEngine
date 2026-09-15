#pragma once
#include "CoreDefines.h"

enum class EMemorySize
{
	Unknown = 0,
	_4_Byte = 4,
	_8_Byte = 8,
	_16_Byte = 16,
	_32_Byte = 32,
	_64_Byte = 64,
	_128_Byte = 128,
	_256_Byte = 256,
	_512_Byte = 512,
	_1_KB = 1024,
	_2_KB = 2048,
	_4_KB = 4096,
	_8_KB = 8192,
	_16_KB = 16384,
	_32_KB = 32768,
	_64_KB = 65536,
	_128_KB = 131072,
	_256_KB = 262144,
	_512_KB = 524288,
	_1_MB = 1048576,
	_1_GB = _1_MB * 2,
};

enum class EMemoryUnit
{
	Byte,
	KB,
	MB,
	GB,
};

enum class EMemoryState
{
	None,
	Allocated,
};

struct HMemoryHeader
{
	EMemorySize  MemorySize = EMemorySize::Unknown;
	EMemoryState MemeoryState = EMemoryState::None;
	uint64 OwnerChunkID = -1;
};

struct HMemroyBlockStat
{
	EMemorySize MemorySize;
	const void* Ptr;
};

struct HMemoryChunkStatInfo
{
	uint64 ChunkID;
	uint64 TotalMemorySize;
	uint64 AllocatedMemorySize;
	uint64 HeaderMemorySize;

	std::map<EMemorySize, std::vector<HMemroyBlockStat>> AllocatedMemories;
	std::map<EMemorySize, std::vector<HMemroyBlockStat>> EmptyMemories;
};

struct HMemoryPoolStatInfo
{
	std::vector<HMemoryChunkStatInfo> ChunkStatInfos;
};

struct HMemoryChunkArguments
{
	uint64 ChunkID;
	std::map<EMemorySize, uint64> MemoryBlockCountMap;
};

class HMemoryChunk
{
	std::map<EMemorySize, std::queue<void*>> MemoryQueues;
	uint64  ChunkID = 0;
	uint64 TotalMemorySize = 0;
	void* pMemory = nullptr;
public:
	void Initialize(const HMemoryChunkArguments& InArguments);
	void Shutdown();
	uint64  GetID() const;
	uint64 GetSize() const;
	uint64 HasSpace(uint64 InMemorySize) const;
	void* Allocate(uint64 InMemorySize);
	void  Deallocate(void* InMemory);

	void GetStatInfo(HMemoryChunkStatInfo& OutStatInfo) const;
};

class HMemoryPool
{
	std::shared_mutex RWLock;
	std::unordered_map<ThreadID, HMemoryChunk> MemoryChunkMap;
public:
	void  Initialize();
	void  Shutdown();
	void* Allocate(uint64 InMemorySize);
	void  Deallocate(void* InMemory);

	void GetStatInfo(HMemoryPoolStatInfo& OutStatInfo);
private:
	void MakeMemoryChunk(uint64 ChunkID);
};

inline HMemoryHeader* GetMemoryHeader(void* InMemory)
{
	void* pMmeoryBlockInfo = static_cast<char*>(InMemory) - sizeof(HMemoryHeader);
	return static_cast<HMemoryHeader*>(pMmeoryBlockInfo);
}

inline double ConvertMemory(EMemoryUnit SrcMemUnit, double SrcMemory, EMemoryUnit DstMemUnit)
{
	if (SrcMemUnit == DstMemUnit)
	{
		return SrcMemory;
	}

	switch (SrcMemUnit)
	{
	case EMemoryUnit::Byte:
		switch (DstMemUnit)
		{
		case EMemoryUnit::KB: return SrcMemory / 1024;
		case EMemoryUnit::MB: return SrcMemory / 1024 / 1024;
		case EMemoryUnit::GB: return SrcMemory / 1024 / 1024 / 1024;
		}
	case EMemoryUnit::KB:
		switch (DstMemUnit)
		{
		case EMemoryUnit::Byte: return SrcMemory * 1024;
		case EMemoryUnit::MB:	return SrcMemory / 1024;
		case EMemoryUnit::GB:	return SrcMemory / 1024 / 1024;
		}
	case EMemoryUnit::MB:
		switch (DstMemUnit)
		{
		case EMemoryUnit::Byte: return SrcMemory * 1024 * 1024;
		case EMemoryUnit::KB:	return SrcMemory * 1024;
		case EMemoryUnit::GB:	return SrcMemory / 1024;
		}
	case EMemoryUnit::GB:
		switch (DstMemUnit)
		{
		case EMemoryUnit::Byte: return SrcMemory * 1024 * 1024 * 1024;
		case EMemoryUnit::KB:	return SrcMemory * 1024 * 1024;
		case EMemoryUnit::MB:	return SrcMemory * 1024;
		}
	}

	return SrcMemory;
}