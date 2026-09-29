#pragma once
#include "CoreDefines.h"

// 메모리 풀 (Memory_TODO Phase 2 재설계, 2026-09-28). 상세: Document/Memory_풀재설계_설계_2026-09-28.md
//  - 스레드마다 청크 하나. 청크는 크기 클래스(8B~1MB, 2의 제곱 18개)마다 페이지 목록을 갖고, 클래스가 비면 페이지를 하나 더 만든다(고갈 없음).
//  - 1MB 초과 또는 정렬 16 초과 요청은 요청 크기 그대로 잡는 대형 블록(청크가 소유하는 블록 하나짜리 페이지)이다.
//    놓으면 풀 전체 대형 캐시에 두어 비슷한 크기의 요청에 재사용한다(용량 ≤ 요청 × 1.25, 총량 64MB, 유휴 10초).
//  - 모든 블록의 payload 바로 앞 16B 는 HMemoryHeader 이고 OwnerChunkID(청크 테이블 인덱스)로 소유 청크를 찾는다. 대형 블록은 MemorySize = Large.
//  - 소유 스레드만 자기 청크의 블록 목록을 만진다(무락). 다른 스레드의 반납은 lock-free 스택으로 받아 소유 스레드가 다음 할당 때 회수한다.
//  - 완전히 빈 페이지와 캐시된 대형 블록은 10초 동안 다시 쓰이지 않으면 프레임 틱(Tick, 메인 스레드)이 시스템에 돌려준다.
//    Empty·All 페이지 목록만 청크별 뮤텍스(페이지가 생기고·비고·회수될 때만)로 지키고, 블록 할당·해제의 정상 경로에는 락이 없다.

enum class EMemorySize
{
	Unknown = 0,
	Large   = 1,          // 대형 블록 표식 (1MB 초과 또는 정렬 16 초과 요청). 실제 크기는 HLargeHeader::Capacity
	_4_Byte = 4,          // 더 쓰지 않는다 (최소 클래스 8B). 열거자는 호환을 위해 남긴다
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
	_1_MB = 1048576,      // 최대 클래스
	_1_GB = _1_MB * 2,    // 더 쓰지 않는다 (실제 값은 2MB). 호환을 위해 남긴다
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

// 풀 상수. 결정 사항은 설계 문서 §6.
struct HMemoryPoolConfig
{
	static constexpr uint64 HeaderSize       = 16;                    // HMemoryHeader
	static constexpr uint64 MinClassBytes    = 8;                     // 빈 블록 안에 next 포인터(8B)를 두므로 4B 클래스는 없다
	static constexpr uint64 MaxClassBytes    = 1024ull * 1024ull;     // 결정 2: 이보다 큰 요청은 대형 블록 경로
	static constexpr uint32 ClassCount       = 18;                    // log2(1MB) - log2(8B) + 1
	static constexpr uint64 PageBytes        = 64ull * 1024ull;       // 결정 1: 페이지 크기. 여러 블록을 담는 페이지는 이 크기로 정렬된다
	static constexpr uint64 PageHeaderSize   = 64;                    // HPage. 첫 블록은 이 뒤에서 시작한다 (16B 정렬 유지)
	static constexpr uint32 MaxChunks        = 256;                   // 청크 테이블 고정 크기 (하드웨어 스레드 28 + 여유). 넘으면 Critical + 중단
	static constexpr uint64 LargeHeaderSize  = 32;                    // HLargeHeader (뒤 16B 가 HMemoryHeader 와 같은 자리)
	static constexpr uint64 LargeCacheBudget = 64ull * 1024ull * 1024ull;   // 결정 2-b: 대형 캐시 총량. 넘으면 오래된 것부터 해제
	static constexpr double IdleSeconds      = 10.0;                  // 결정 2-c: 빈 페이지·캐시 블록이 이 시간 쓰이지 않으면 회수
	static constexpr uint32 RemoteFreeDrainThreshold = 256;           // 원격 반납 대기가 이만큼 쌓이면 소유 스레드가 다음 할당 때 무조건 회수
	// 결정 2-a: 대형 캐시 재사용 허용 낭비 1.25배 는 정수 연산(요청 + 요청 / 4)으로 적용한다
};

// 모든 블록 payload 바로 앞 16B. 레이아웃은 재설계 전과 같다.
struct HMemoryHeader
{
	EMemorySize  MemorySize = EMemorySize::Unknown;   // 클래스 블록 크기 (2의 제곱) 또는 Large
	EMemoryState MemeoryState = EMemoryState::None;
	uint64 OwnerChunkID = -1;                          // 소유 청크의 테이블 인덱스
};

// 통계 (O(1) 카운터를 relaxed 로 읽기만 한다. 다른 스레드 청크 값은 한 프레임 늦을 수 있다)
struct HMemoryClassStatInfo
{
	uint32 BlockSize     = 0;
	uint32 BlocksPerPage = 0;    // 1 이면 블록 하나짜리 페이지 (32KB 이상 클래스)
	uint32 Pages         = 0;    // 현재 페이지 수 (빈 페이지 포함)
	uint32 EmptyPages    = 0;    // 완전히 비어 회수 대기 중인 페이지 수
	uint32 Total         = 0;    // 블록 수 = Pages × BlocksPerPage
	uint32 Live          = 0;    // 나가 있는 블록 수
	uint32 Peak          = 0;
	uint32 Growths       = 0;    // 첫 페이지 이후 추가된 페이지 수 (누적)
	uint32 Releases      = 0;    // 유휴 회수로 시스템에 돌려준 페이지 수 (누적)
	uint64 AllocCount    = 0;    // 누적 할당 횟수
};

struct HMemoryChunkStatInfo
{
	uint64   ChunkID             = 0;    // 청크 테이블 인덱스 (= 헤더 OwnerChunkID)
	ThreadID OwnerThreadID       = 0;    // std::hash<std::thread::id>. GCoreSystem::GetMainThreadID() 와 비교
	uint64   TotalMemorySize     = 0;    // 페이지 커밋 합계 + 살아 있는 대형 블록
	uint64   AllocatedMemorySize = 0;    // Σ Live × BlockSize + 대형 블록 바이트
	uint64   HeaderMemorySize    = 0;    // 블록 헤더 + 페이지 헤더 + 대형 헤더
	uint32   RemoteFreePending   = 0;    // 아직 회수되지 않은 타 스레드 반납
	uint64   RemoteFreeTotal     = 0;    // 누적 (예전 Dismatch 경고를 대신함)
	uint32   LargeLive           = 0;
	uint64   LargeBytes          = 0;
	uint64   LargePeakBytes      = 0;
	uint64   LargeTotal          = 0;
	HMemoryClassStatInfo Classes[HMemoryPoolConfig::ClassCount];
};

struct HMemoryPoolStatInfo
{
	std::vector<HMemoryChunkStatInfo> ChunkStatInfos;
	uint32 ChunkCount            = 0;
	uint64 TotalCommittedBytes   = 0;
	uint64 TotalAllocatedBytes   = 0;
	uint32 LargeLive             = 0;
	uint64 LargeBytes            = 0;
	uint32 LargeCachedCount      = 0;    // 대형 캐시 (풀 전체)
	uint64 LargeCachedBytes      = 0;
	uint64 LargeReuseCount       = 0;
	uint64 LargeReuseWasteBytes  = 0;    // 재사용 때 용량 - 요청 누적
	uint64 LargeEvictCount       = 0;    // 예산 초과로 해제
	uint64 LargeIdleReleaseCount = 0;    // 유휴 해제
};

class  HMemoryChunk;
struct HSizeClass;
struct HPage;
struct HLargeCacheNode;

class HMemoryPool
{
	std::atomic<HMemoryChunk*> Chunks[HMemoryPoolConfig::MaxChunks] = {};   // 인덱스 = OwnerChunkID. 생성 뒤 불변, 읽기는 락 없음
	std::atomic<uint32>        ChunkCount = 0;
	HMutex                     CreateMutex;                                   // 청크 생성 때만
	std::unordered_map<std::thread::id, uint32> ChunkIndexByThread;         // CreateMutex 아래에서만. CRT 힙 (풀 자신을 쓸 수 없다)

	// 대형 블록 캐시 (설계 §3-9). 드문 경로라 뮤텍스 하나. 노드는 빈 payload 안에 둔다. 카운터는 락 없이 읽을 수 있게 atomic.
	HMutex              LargeCacheMutex;
	HLargeCacheNode*    LargeCacheHead = nullptr;    // 최근 해제 순 (head = 최신, tail = 최고참)
	HLargeCacheNode*    LargeCacheTail = nullptr;
	std::atomic<uint64> LargeCachedBytes      = 0;
	std::atomic<uint32> LargeCachedCount      = 0;
	std::atomic<uint64> LargeReuseCount       = 0;
	std::atomic<uint64> LargeReuseWasteBytes  = 0;
	std::atomic<uint64> LargeEvictCount       = 0;
	std::atomic<uint64> LargeIdleReleaseCount = 0;

public:
	HMemoryPool() = default;
	~HMemoryPool();
	HMemoryPool(const HMemoryPool&) = delete;
	HMemoryPool& operator=(const HMemoryPool&) = delete;

	void  Initialize();
	void  Shutdown();

	// 정렬 16 초과(alignas(32) 등)는 크기와 무관하게 대형 경로로 간다. 실패는 시스템 힙 실패 하나뿐이며 Critical 로그 뒤 중단한다 (nullptr 을 돌려주지 않는다).
	void* Allocate(uint64 InMemorySize, uint64 InAlignment = HMemoryPoolConfig::HeaderSize);
	void  Deallocate(void* InMemory);

	// 매 프레임 (GMemoryGlobalSystem::Update). 10초 동안 쓰이지 않은 빈 페이지와 캐시 블록을 시스템에 돌려준다.
	void  Tick();

	void  GetStatInfo(HMemoryPoolStatInfo& OutStatInfo);

private:
	HMemoryChunk& thisThreadChunk();
	uint32        createChunkLocked(const std::thread::id& InThreadID);
	HMemoryChunk* chunkOf(uint64 InChunkID) const;

	HPage* takeEmptyOrGrow(HMemoryChunk& InChunk, uint32 InClassIndex);
	HPage* growClass(HMemoryChunk& InChunk, uint32 InClassIndex);
	void   moveToEmpty(HMemoryChunk& InChunk, HSizeClass& InClass, HPage* InPage);
	void   releaseIdlePages(double InNow);
	void   freeBlockLocal(HMemoryChunk& InChunk, void* InBlock);
	void   drainRemoteFrees(HMemoryChunk& InChunk);

	void* allocateLarge(HMemoryChunk& InChunk, uint64 InMemorySize, uint64 InAlignment);
	void  deallocateLarge(void* InMemory);
	void* tryReuseLarge(uint64 InMemorySize);
	void  evictLargeTailLocked();
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
