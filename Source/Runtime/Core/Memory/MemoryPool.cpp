#include "PCH/PCH.h"
#include "MemoryPool.h"
#include "CoreSystem.h"
#include "Misc/Log.h"
#include <intrin.h>

// 풀 할당/해제 트레이스 로그 스위치 (Memory_TODO 1-1). 기본 꺼짐.
// 켜려면 이 값을 1로 바꾸고 다시 빌드한다(이 파일만 다시 컴파일된다). 켠 빌드에서만 Document/Memory/tools/memlog_stats.sh 가 데이터를 얻는다.
// 켜면 프레임당 수백 줄이 파일·콘솔 두 싱크에 쓰이므로 측정용으로만 쓴다.
#ifndef JG_MEMORY_TRACE
#define JG_MEMORY_TRACE 0
#endif

// 디버그 채움·격리 (설계 §3-12, 결정 4). 빈 목록이 LIFO 라 방금 놓은 블록이 바로 다음 요청에 재배포된다.
// 디버그 빌드에서는 할당 시 0xCD, 해제 시 0xDD 로 채우고(MSVC 디버그 힙과 같은 값) 최근 해제 32개(4KB 이하 클래스)를 격리해
// use-after-free 가 값 손상으로 곧바로 드러나게 한다. 릴리스 경로에는 코드가 없다.
#ifdef _DEBUG
#define JG_MEMORY_DEBUG_FILL        1
#define JG_MEMORY_QUARANTINE_BLOCKS 32
#else
#define JG_MEMORY_DEBUG_FILL        0
#define JG_MEMORY_QUARANTINE_BLOCKS 0
#endif

namespace
{
	using HConfig = HMemoryPoolConfig;

	constexpr uint32 MinClassLog2           = 3;                                   // 8B
	constexpr uint64 CommitWarnLevels[]     = { 16ull * _MB, 32ull * _MB, 64ull * _MB, 128ull * _MB };   // 결정 3: 청크 커밋이 처음 넘을 때 Warning
	constexpr uint32 CommitWarnLevelCount   = 4;
	constexpr uint64 DebugFillMaxBytes      = 1ull * _MB;                          // 대형 블록 채움 상한 (매 프레임 오는 수 MB 리드백 버퍼를 통째로 채우지 않는다)
	constexpr uint32 QuarantineMaxBlockSize = 4096;                                // 이보다 큰 클래스는 격리하지 않는다 (1MB 블록 32개를 붙들지 않기 위해)
	constexpr uint32 ReleaseBatchPerTick    = 32;                                  // 한 틱에 클래스당 회수하는 최대 페이지 수 (락 보유 시간 제한)

	// 풀 안에서의 로그 재진입 가드 (설계 §3-8). GLogGlobalSystem::AddLog 가 PString::Format 으로 문자열을 만들며 힙을 쓰므로,
	// 문자열이나 로깅 경로가 이 풀을 쓰게 되는 날에도 Allocate → JG_LOG → Format → Allocate 로 무한 재귀하지 않는다.
	// 정상 경로(블록 pop/push)에는 로그가 없다. 청크 생성·페이지 성장·실패·종료에서만 부른다.
	thread_local bool tlsInPoolLog = false;

	struct HPoolLogScope
	{
		HPoolLogScope()
		{
			tlsInPoolLog = true;
		}
		~HPoolLogScope()
		{
			tlsInPoolLog = false;
		}
	};

	bool canPoolLog()
	{
		return tlsInPoolLog == false && GLogGlobalSystem::IsValid() == true;
	}

#define JG_POOL_LOG(Level, Text, ...)                     \
	do                                                    \
	{                                                     \
		if (canPoolLog() == true)                         \
		{                                                 \
			HPoolLogScope poolLogScope;                   \
			JG_LOG(Memory, Level, Text, __VA_ARGS__)      \
		}                                                 \
	} while (false)

	// 스레드 → 청크 캐시. Core 는 정적 라이브러리라 DLL(모듈)마다 사본이 있지만 각 사본이 같은 풀 객체에서 같은 청크 포인터를 얻으므로 문제없다.
	// 다른 모듈 사본의 TLS 가 아직 비어 있으면 Deallocate 가 스레드 id 비교로 한 번 채운다.
	struct HThreadChunkCache
	{
		HMemoryPool*  Pool  = nullptr;
		HMemoryChunk* Chunk = nullptr;
	};
	thread_local HThreadChunkCache tlsChunk;

	[[noreturn]] void poolFatal(const char* InWhat, uint64 InSize)
	{
		JG_POOL_LOG(ELogLevel::Critical, "%s (size %llu). aborting.", InWhat, InSize);
		std::abort();
	}

	double nowSeconds()
	{
		return std::chrono::duration<double>(HSteadyClock::now().time_since_epoch()).count();
	}

	void*& nextOf(void* InBlock)
	{
		return *static_cast<void**>(InBlock);
	}

	uint64 alignUp(uint64 InValue, uint64 InAlignment)
	{
		return (InValue + InAlignment - 1) & ~(InAlignment - 1);
	}

	// 요청 크기 → 클래스 인덱스 (올림 2의 제곱, 최소 8B). InSize <= MaxClassBytes.
	// HMath::AlignPowerOfTwo 는 `1 << Log2()` 가 int 시프트라 여기서는 uint64 전용으로 따로 계산한다.
	uint32 classIndexForRequest(uint64 InSize)
	{
		if (InSize <= HConfig::MinClassBytes)
		{
			return 0;
		}

		unsigned long msb = 0;
		_BitScanReverse64(&msb, InSize - 1);
		return static_cast<uint32>(msb) + 1 - MinClassLog2;
	}

	// 헤더의 MemorySize (정확한 2의 제곱) → 클래스 인덱스
	uint32 classIndexForBlockSize(uint64 InBlockSize)
	{
		unsigned long msb = 0;
		_BitScanReverse64(&msb, InBlockSize);
		return static_cast<uint32>(msb) - MinClassLog2;
	}

	bool isValidClassBlockSize(uint64 InBlockSize)
	{
		return InBlockSize >= HConfig::MinClassBytes && InBlockSize <= HConfig::MaxClassBytes && (InBlockSize & (InBlockSize - 1)) == 0;
	}

	void debugFill(void* InPayload, uint64 InBytes, int32 InValue)
	{
#if JG_MEMORY_DEBUG_FILL
		memset(InPayload, InValue, static_cast<size_t>((InBytes < DebugFillMaxBytes) ? InBytes : DebugFillMaxBytes));
#else
		(void)InPayload;
		(void)InBytes;
		(void)InValue;
#endif
	}
}

// ----------------------------------------------------------------------------------------------------------------------------
// 자료구조 (설계 §3-3 ~ §3-5, §3-9)
// ----------------------------------------------------------------------------------------------------------------------------

enum class EPageList : uint32
{
	None,      // 어느 목록에도 없다 (가득 찬 페이지)
	Partial,   // 빈 블록이 남은 페이지 (소유 스레드 전용, 무락)
	Empty,     // 완전히 빈 페이지, 회수 대기 (PageListMutex)
};

// 페이지 맨 앞 64B. 여러 블록을 담는 페이지는 64KB 정렬(VirtualAlloc)이라 블록 주소를 마스크하면 나온다.
// 블록 하나짜리 페이지(32KB 이상 클래스)는 정렬하지 않고 블록 주소 - 80(페이지 헤더 64 + 블록 헤더 16)으로 찾는다.
struct HPage
{
	uint32    ClassIndex;
	uint32    Live;          // 나가 있는 블록 수. 0 이면 Empty 목록으로
	uint32    BlockCount;
	EPageList ListKind;
	void*     FreeHead;      // 이 페이지의 빈 블록 목록 (intrusive, payload 첫 8B = next)
	HPage*    Prev;          // Partial 또는 Empty 목록 링크. 한 페이지는 둘 중 한 곳에만 있다
	HPage*    Next;
	HPage*    NextAll;       // 클래스의 모든 페이지 (종료·회수용). PageListMutex 아래에서만
	double    EmptySince;    // Empty 목록에 들어간 시각 (steady_clock 초)
	uint64    Reserved;
};
static_assert(sizeof(HPage) == HMemoryPoolConfig::PageHeaderSize, "HPage must be 64 bytes (first block starts right after it, 16B aligned)");

// 크기 클래스 하나. 블록 목록(PartialHead)은 소유 스레드만 만진다. Empty·All 목록과 PageCount·EmptyCount 갱신은 PageListMutex 아래에서.
struct HSizeClass
{
	uint32 BlockSize     = 0;    // 8 .. MaxClassBytes
	uint32 Stride        = 0;    // BlockSize + 16
	uint32 BlocksPerPage = 0;    // 1 이면 블록 하나짜리 페이지

	HPage* PartialHead = nullptr;
	HPage* EmptyHead   = nullptr;    // head = 가장 오래 비어 있던 것
	HPage* EmptyTail   = nullptr;    // tail = 가장 최근에 비워진 것 (재사용은 여기서)
	HPage* AllHead     = nullptr;

	std::atomic<uint32> PageCount  = 0;
	std::atomic<uint32> EmptyCount = 0;    // Tick 이 락 없이 읽어 0 이면 건너뛴다
	std::atomic<uint32> Live       = 0;    // 통계 (원격 반납 스레드도 내린다)
	std::atomic<uint32> Peak       = 0;
	std::atomic<uint32> Total      = 0;    // 페이지 수 × BlocksPerPage
	std::atomic<uint32> Growths    = 0;    // 첫 페이지 이후 추가된 페이지 수
	std::atomic<uint32> Releases   = 0;    // 회수해 시스템에 돌려준 페이지 수
	std::atomic<uint64> AllocCount = 0;

#if JG_MEMORY_QUARANTINE_BLOCKS > 0
	void*  Quarantine[JG_MEMORY_QUARANTINE_BLOCKS] = {};    // 최근 해제 링. 소유 스레드만
	uint32 QuarantineNext = 0;
#endif
};

class HMemoryChunk
{
public:
	uint64          Index           = 0;    // 청크 테이블 인덱스 = 헤더 OwnerChunkID
	std::thread::id OwnerThread;
	ThreadID        OwnerThreadHash = 0;    // std::hash<std::thread::id>. 통계·로그용

	HSizeClass Classes[HConfig::ClassCount];

	std::atomic<void*>  RemoteFreeHead    = nullptr;    // 타 스레드 반납 스택 (lock-free push / 소유자 pop-all)
	std::atomic<uint32> RemoteFreePending = 0;
	std::atomic<uint64> RemoteFreeTotal   = 0;
	std::atomic<uint64> CommittedBytes    = 0;          // 페이지 합계
	uint32              CommitWarnIndex   = 0;          // 다음에 경고할 문턱 (소유 스레드만)

	// Empty·All 페이지 목록과 PageCount 전용. 페이지가 생기고·비고·회수될 때만 잡는다(드문 이벤트).
	// Tick(메인 스레드)이 다른 청크의 빈 페이지를 회수할 수 있게 하기 위한 것. Partial 목록과 블록 경로는 무락.
	HMutex PageListMutex;

	std::atomic<uint32> LargeLive      = 0;    // 대형 블록 통계. 어느 스레드에서든 놓이므로 atomic
	std::atomic<uint64> LargeBytes     = 0;
	std::atomic<uint64> LargePeakBytes = 0;
	std::atomic<uint64> LargeTotal     = 0;
};

// 대형 블록 헤더 32B. Header 가 payload 바로 앞 = 일반 블록과 같은 자리 (GetMemoryHeader 가 그대로 통한다).
struct HLargeHeader
{
	void*         Raw;         // malloc 이 준 주소 (정렬 여유 포함)
	uint64        Capacity;    // payload 크기 (요청 크기 그대로)
	HMemoryHeader Header;      // MemorySize = Large
};
static_assert(sizeof(HLargeHeader) == HMemoryPoolConfig::LargeHeaderSize, "HLargeHeader must be 32 bytes");

// 캐시에 있는 동안 빈 payload 첫 24B 에 둔다.
struct HLargeCacheNode
{
	HLargeCacheNode* Prev;
	HLargeCacheNode* Next;
	double           FreedSeconds;
};

namespace
{
	uint64 pageBytesOf(const HSizeClass& InClass)
	{
		if (InClass.BlocksPerPage > 1)
		{
			return HConfig::PageBytes;
		}
		return HConfig::PageHeaderSize + InClass.Stride;
	}

	HPage* pageOf(void* InBlock, const HSizeClass& InClass)
	{
		if (InClass.BlocksPerPage > 1)
		{
			return reinterpret_cast<HPage*>(reinterpret_cast<uintptr_t>(InBlock) & ~static_cast<uintptr_t>(HConfig::PageBytes - 1));
		}
		return reinterpret_cast<HPage*>(static_cast<char*>(InBlock) - HConfig::PageHeaderSize - HConfig::HeaderSize);
	}

	// 여러 블록을 담는 페이지는 VirtualAlloc 으로 잡는다. 시작 주소가 할당 단위(64KB)로 정렬되므로 블록 주소 마스크로 페이지를 찾을 수 있고,
	// _aligned_malloc(64KB, 64KB) 처럼 정렬 여유 64KB 를 더 쓰지 않는다. 블록 하나짜리 페이지는 정렬이 필요 없어 malloc.
	HPage* allocPageMemory(const HSizeClass& InClass)
	{
		if (InClass.BlocksPerPage > 1)
		{
			return static_cast<HPage*>(VirtualAlloc(nullptr, HConfig::PageBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		}
		return static_cast<HPage*>(malloc(pageBytesOf(InClass)));
	}

	void freePageMemory(const HSizeClass& InClass, HPage* InPage)
	{
		if (InClass.BlocksPerPage > 1)
		{
			VirtualFree(InPage, 0, MEM_RELEASE);
			return;
		}
		free(InPage);
	}

	HLargeHeader* largeHeaderOf(void* InPayload)
	{
		return reinterpret_cast<HLargeHeader*>(static_cast<char*>(InPayload) - HConfig::LargeHeaderSize);
	}

	// ---- 페이지 목록 (Prev/Next 이중 연결. Partial 은 소유 스레드 전용, Empty 는 PageListMutex) ----

	void listPushPartial(HSizeClass& InClass, HPage* InPage)
	{
		JG_CHECK(InPage->ListKind == EPageList::None);
		InPage->Prev = nullptr;
		InPage->Next = InClass.PartialHead;
		if (InClass.PartialHead != nullptr)
		{
			InClass.PartialHead->Prev = InPage;
		}
		InClass.PartialHead = InPage;
		InPage->ListKind = EPageList::Partial;
	}

	void listUnlinkPartial(HSizeClass& InClass, HPage* InPage)
	{
		JG_CHECK(InPage->ListKind == EPageList::Partial);
		if (InPage->Prev != nullptr)
		{
			InPage->Prev->Next = InPage->Next;
		}
		else
		{
			InClass.PartialHead = InPage->Next;
		}
		if (InPage->Next != nullptr)
		{
			InPage->Next->Prev = InPage->Prev;
		}
		InPage->Prev = nullptr;
		InPage->Next = nullptr;
		InPage->ListKind = EPageList::None;
	}

	void listAppendEmpty(HSizeClass& InClass, HPage* InPage)
	{
		JG_CHECK(InPage->ListKind == EPageList::None);
		InPage->Next = nullptr;
		InPage->Prev = InClass.EmptyTail;
		if (InClass.EmptyTail != nullptr)
		{
			InClass.EmptyTail->Next = InPage;
		}
		else
		{
			InClass.EmptyHead = InPage;
		}
		InClass.EmptyTail = InPage;
		InPage->ListKind = EPageList::Empty;
	}

	void listUnlinkEmpty(HSizeClass& InClass, HPage* InPage)
	{
		JG_CHECK(InPage->ListKind == EPageList::Empty);
		if (InPage->Prev != nullptr)
		{
			InPage->Prev->Next = InPage->Next;
		}
		else
		{
			InClass.EmptyHead = InPage->Next;
		}
		if (InPage->Next != nullptr)
		{
			InPage->Next->Prev = InPage->Prev;
		}
		else
		{
			InClass.EmptyTail = InPage->Prev;
		}
		InPage->Prev = nullptr;
		InPage->Next = nullptr;
		InPage->ListKind = EPageList::None;
	}

	// All 목록은 단일 연결. O(페이지 수)지만 회수 때만 걷고 클래스의 페이지는 보통 몇 개다.
	void listUnlinkAll(HSizeClass& InClass, HPage* InPage)
	{
		HPage** link = &InClass.AllHead;
		while (*link != nullptr)
		{
			if (*link == InPage)
			{
				*link = InPage->NextAll;
				InPage->NextAll = nullptr;
				return;
			}
			link = &(*link)->NextAll;
		}
		JG_CHECK(false);
	}
}

// ----------------------------------------------------------------------------------------------------------------------------
// 풀 수명
// ----------------------------------------------------------------------------------------------------------------------------

HMemoryPool::~HMemoryPool()
{
	if (ChunkCount.load(std::memory_order_acquire) > 0)
	{
		Shutdown();
	}
}

void HMemoryPool::Initialize()
{
	// lazy. 청크는 스레드의 첫 요청에서, 페이지는 클래스의 첫 요청에서 생긴다.
}

void HMemoryPool::Shutdown()
{
	// 대형 캐시의 블록은 모두 돌려준다.
	{
		HLockGuard<HMutex> lock(LargeCacheMutex);
		while (LargeCacheTail != nullptr)
		{
			evictLargeTailLocked();
		}
	}

	// 청크마다 페이지 전부 해제. 원격 스택의 블록은 페이지에 속하므로 따로 해제할 것이 없다.
	// 이 시점에는 워커 스레드가 모두 join 된 뒤여야 한다 (GScheduleGlobalSystem 이 먼저 내려간다).
	const uint32 chunkCount = ChunkCount.load(std::memory_order_acquire);
	uint32 largeLive  = 0;
	uint64 largeBytes = 0;
	for (uint32 c = 0; c < chunkCount; ++c)
	{
		HMemoryChunk* chunk = Chunks[c].exchange(nullptr, std::memory_order_acq_rel);
		if (chunk == nullptr)
		{
			continue;
		}

		uint32 liveBlocks = 0;
		uint32 pages      = 0;
		for (uint32 i = 0; i < HConfig::ClassCount; ++i)
		{
			HSizeClass& cls = chunk->Classes[i];
			liveBlocks += cls.Live.load(std::memory_order_relaxed);
			pages      += cls.PageCount.load(std::memory_order_relaxed);

			HPage* page = cls.AllHead;
			while (page != nullptr)
			{
				HPage* next = page->NextAll;
				freePageMemory(cls, page);
				page = next;
			}
			cls.AllHead     = nullptr;
			cls.PartialHead = nullptr;
			cls.EmptyHead   = nullptr;
			cls.EmptyTail   = nullptr;
		}

		largeLive  += chunk->LargeLive.load(std::memory_order_relaxed);
		largeBytes += chunk->LargeBytes.load(std::memory_order_relaxed);

		JG_POOL_LOG(ELogLevel::Info, "[ID: %llu] Memory Chunk Shutdown. pages %u, committed %.2f MB, live blocks %u, remote pending %u (total %llu), large live %u",
			chunk->Index, pages, static_cast<double>(chunk->CommittedBytes.load(std::memory_order_relaxed)) / (1024.0 * 1024.0), liveBlocks,
			chunk->RemoteFreePending.load(std::memory_order_relaxed), chunk->RemoteFreeTotal.load(std::memory_order_relaxed), chunk->LargeLive.load(std::memory_order_relaxed));

		delete chunk;
	}

	// 살아 있는 대형 블록은 목록에 없으므로 풀이 대신 해제하지 않는다. 누수 리포트만 남긴다.
	if (largeLive > 0)
	{
		JG_POOL_LOG(ELogLevel::Warning, "%u large blocks (%llu bytes) were still allocated at memory pool shutdown (leak)", largeLive, largeBytes);
	}

	{
		HLockGuard<HMutex> lock(CreateMutex);
		ChunkIndexByThread.clear();
		ChunkCount.store(0, std::memory_order_release);
	}

	// 이 스레드(메인)의 TLS 캐시는 지운다. 다른 스레드는 이미 끝났고, 남아 있어도 Pool != this 로 무효화된다.
	if (tlsChunk.Pool == this)
	{
		tlsChunk.Pool  = nullptr;
		tlsChunk.Chunk = nullptr;
	}
}

// ----------------------------------------------------------------------------------------------------------------------------
// 청크
// ----------------------------------------------------------------------------------------------------------------------------

HMemoryChunk* HMemoryPool::chunkOf(uint64 InChunkID) const
{
	if (InChunkID >= HConfig::MaxChunks)
	{
		return nullptr;
	}
	return Chunks[InChunkID].load(std::memory_order_acquire);
}

HMemoryChunk& HMemoryPool::thisThreadChunk()
{
	if (tlsChunk.Pool == this && tlsChunk.Chunk != nullptr)
	{
		return *tlsChunk.Chunk;
	}

	const std::thread::id threadID = std::this_thread::get_id();
	bool   bCreated = false;
	uint32 index    = 0;
	{
		HLockGuard<HMutex> lock(CreateMutex);
		auto iter = ChunkIndexByThread.find(threadID);
		if (iter != ChunkIndexByThread.end())
		{
			index = iter->second;
		}
		else
		{
			index    = createChunkLocked(threadID);
			bCreated = true;
		}
	}

	HMemoryChunk* chunk = Chunks[index].load(std::memory_order_acquire);
	tlsChunk.Pool  = this;
	tlsChunk.Chunk = chunk;

	// 로그는 락을 놓고 TLS 를 채운 뒤에. (로깅이 풀을 쓰게 되어도 CreateMutex 재진입이 없다)
	if (bCreated == true)
	{
		JG_POOL_LOG(ELogLevel::Info, "[ID: %llu] Memory Chunk Initialize. thread %llu. pages are created on first request", chunk->Index, chunk->OwnerThreadHash);
	}
	return *chunk;
}

uint32 HMemoryPool::createChunkLocked(const std::thread::id& InThreadID)
{
	const uint32 index = ChunkCount.load(std::memory_order_relaxed);
	if (index >= HConfig::MaxChunks)
	{
		poolFatal("memory pool: chunk table is full (too many threads)", index);
	}

	// 청크 객체 자체는 CRT 힙에 둔다 (풀 밖).
	HMemoryChunk* chunk = new HMemoryChunk();
	chunk->Index           = index;
	chunk->OwnerThread     = InThreadID;
	chunk->OwnerThreadHash = std::hash<std::thread::id>()(InThreadID);

	for (uint32 i = 0; i < HConfig::ClassCount; ++i)
	{
		HSizeClass& cls = chunk->Classes[i];
		cls.BlockSize = 1u << (i + MinClassLog2);
		cls.Stride    = cls.BlockSize + static_cast<uint32>(HConfig::HeaderSize);

		// 64KB 페이지에 블록이 두 개도 안 들어가는 클래스(32KB 이상)는 블록 하나짜리 페이지
		const uint64 blocksPerPage = (HConfig::PageBytes - HConfig::PageHeaderSize) / cls.Stride;
		cls.BlocksPerPage = (blocksPerPage >= 2) ? static_cast<uint32>(blocksPerPage) : 1;
	}

	Chunks[index].store(chunk, std::memory_order_release);
	ChunkIndexByThread.emplace(InThreadID, index);
	ChunkCount.store(index + 1, std::memory_order_release);
	return index;
}

// ----------------------------------------------------------------------------------------------------------------------------
// 페이지 성장·회수 (설계 §3-8, §3-8b)
// ----------------------------------------------------------------------------------------------------------------------------

HPage* HMemoryPool::growClass(HMemoryChunk& InChunk, uint32 InClassIndex)
{
	HSizeClass&  cls       = InChunk.Classes[InClassIndex];
	const uint64 pageBytes = pageBytesOf(cls);

	HPage* page = allocPageMemory(cls);
	if (page == nullptr)
	{
		return nullptr;
	}

	page->ClassIndex = InClassIndex;
	page->Live       = 0;
	page->BlockCount = cls.BlocksPerPage;
	page->ListKind   = EPageList::None;
	page->FreeHead   = nullptr;
	page->Prev       = nullptr;
	page->Next       = nullptr;
	page->NextAll    = nullptr;
	page->EmptySince = 0.0;
	page->Reserved   = 0;

	// 블록 헤더 초기화 + 페이지 빈 목록에 잇기. 뒤에서부터 넣어 주소 오름차순으로 배포된다.
	char* firstBlock = reinterpret_cast<char*>(page) + HConfig::PageHeaderSize;
	for (int32 i = static_cast<int32>(cls.BlocksPerPage) - 1; i >= 0; --i)
	{
		HMemoryHeader* header = reinterpret_cast<HMemoryHeader*>(firstBlock + static_cast<uint64>(i) * cls.Stride);
		header->MemorySize   = static_cast<EMemorySize>(cls.BlockSize);
		header->MemeoryState = EMemoryState::None;
		header->OwnerChunkID = InChunk.Index;

		void* payload   = header + 1;
		nextOf(payload) = page->FreeHead;
		page->FreeHead  = payload;
	}

	listPushPartial(cls, page);

	uint32 pageCount = 0;
	{
		HLockGuard<HMutex> lock(InChunk.PageListMutex);
		page->NextAll = cls.AllHead;
		cls.AllHead   = page;
		pageCount = cls.PageCount.fetch_add(1, std::memory_order_relaxed) + 1;
	}
	cls.Total.fetch_add(cls.BlocksPerPage, std::memory_order_relaxed);
	const uint64 committed = InChunk.CommittedBytes.fetch_add(pageBytes, std::memory_order_relaxed) + pageBytes;

	// 첫 페이지는 성장이 아니다. 회수됐다가 다시 만든 것은 성장으로 센다. (결정 3: 페이지 추가 = Info 한 줄)
	if (pageCount + cls.Releases.load(std::memory_order_relaxed) > 1)
	{
		const uint32 growths = cls.Growths.fetch_add(1, std::memory_order_relaxed) + 1;
		JG_POOL_LOG(ELogLevel::Info, "[ID: %llu] class %u B grew to %u pages (%u blocks, growth #%u, chunk committed %.2f MB)",
			InChunk.Index, cls.BlockSize, pageCount, cls.Total.load(std::memory_order_relaxed), growths, static_cast<double>(committed) / (1024.0 * 1024.0));
	}

	// 청크 커밋이 문턱(16·32·64·128MB)을 처음 넘으면 Warning 한 줄 (누수 신호. 결정 3)
	while (InChunk.CommitWarnIndex < CommitWarnLevelCount && committed >= CommitWarnLevels[InChunk.CommitWarnIndex])
	{
		JG_POOL_LOG(ELogLevel::Warning, "[ID: %llu] chunk committed memory passed %llu MB (now %.2f MB, class %u B has %u pages). check for a leak",
			InChunk.Index, CommitWarnLevels[InChunk.CommitWarnIndex] / _MB, static_cast<double>(committed) / (1024.0 * 1024.0), cls.BlockSize, pageCount);
		++InChunk.CommitWarnIndex;
	}

	return page;
}

HPage* HMemoryPool::takeEmptyOrGrow(HMemoryChunk& InChunk, uint32 InClassIndex)
{
	HSizeClass& cls = InChunk.Classes[InClassIndex];

	// 빈 페이지가 없으면 락을 잡지 않는다.
	if (cls.EmptyCount.load(std::memory_order_relaxed) > 0)
	{
		HPage* page = nullptr;
		{
			HLockGuard<HMutex> lock(InChunk.PageListMutex);
			// 가장 최근에 비워진 것을 다시 쓴다(LIFO). 오래된 것은 그대로 늙어 10초 뒤 회수되므로 쓰지 않는 페이지가 실제 사용량으로 내려온다.
			page = cls.EmptyTail;
			if (page != nullptr)
			{
				listUnlinkEmpty(cls, page);
				cls.EmptyCount.fetch_sub(1, std::memory_order_relaxed);
			}
		}
		if (page != nullptr)
		{
			listPushPartial(cls, page);
			return page;
		}
	}

	return growClass(InChunk, InClassIndex);
}

void HMemoryPool::moveToEmpty(HMemoryChunk& InChunk, HSizeClass& InClass, HPage* InPage)
{
	listUnlinkPartial(InClass, InPage);    // Partial 은 소유 스레드 전용이라 락 없음
	InPage->EmptySince = nowSeconds();

	HLockGuard<HMutex> lock(InChunk.PageListMutex);
	listAppendEmpty(InClass, InPage);
	InClass.EmptyCount.fetch_add(1, std::memory_order_relaxed);
}

void HMemoryPool::releaseIdlePages(double InNow)
{
	const uint32 chunkCount = ChunkCount.load(std::memory_order_acquire);
	for (uint32 c = 0; c < chunkCount; ++c)
	{
		HMemoryChunk* chunk = Chunks[c].load(std::memory_order_acquire);
		if (chunk == nullptr)
		{
			continue;
		}

		for (uint32 i = 0; i < HConfig::ClassCount; ++i)
		{
			HSizeClass& cls = chunk->Classes[i];

			// 정상 상태의 거의 모든 프레임: 빈 페이지가 없으면 락 없이 건너뛴다.
			if (cls.EmptyCount.load(std::memory_order_relaxed) == 0)
			{
				continue;
			}

			HPage* released[ReleaseBatchPerTick];
			uint32 releasedCount = 0;
			{
				HLockGuard<HMutex> lock(chunk->PageListMutex);
				while (cls.EmptyHead != nullptr && releasedCount < ReleaseBatchPerTick && InNow - cls.EmptyHead->EmptySince >= HConfig::IdleSeconds)
				{
					HPage* page = cls.EmptyHead;    // head = 가장 오래 비어 있던 것
					listUnlinkEmpty(cls, page);
					listUnlinkAll(cls, page);
					cls.EmptyCount.fetch_sub(1, std::memory_order_relaxed);
					cls.PageCount.fetch_sub(1, std::memory_order_relaxed);
					released[releasedCount] = page;
					++releasedCount;
				}
			}

			if (releasedCount == 0)
			{
				continue;
			}

			// 시스템 힙 호출은 락 밖에서.
			const uint64 pageBytes = pageBytesOf(cls);
			for (uint32 r = 0; r < releasedCount; ++r)
			{
				cls.Total.fetch_sub(cls.BlocksPerPage, std::memory_order_relaxed);
				cls.Releases.fetch_add(1, std::memory_order_relaxed);
				chunk->CommittedBytes.fetch_sub(pageBytes, std::memory_order_relaxed);
				freePageMemory(cls, released[r]);
			}
		}
	}
}

// ----------------------------------------------------------------------------------------------------------------------------
// 할당·해제 (설계 §3-6, §3-7)
// 주의: 정상 경로에서는 트레이스 스위치 밖의 JG_LOG 를 부르지 않는다 (재귀 규칙. 위 JG_POOL_LOG 주석 참조).
// ----------------------------------------------------------------------------------------------------------------------------

void* HMemoryPool::Allocate(uint64 InMemorySize, uint64 InAlignment)
{
	if (InMemorySize > HConfig::MaxClassBytes || InAlignment > HConfig::HeaderSize)
	{
		return allocateLarge(thisThreadChunk(), InMemorySize, InAlignment);
	}

	HMemoryChunk& chunk      = thisThreadChunk();
	const uint32  classIndex = classIndexForRequest(InMemorySize);
	HSizeClass&   cls        = chunk.Classes[classIndex];

	// 원격 반납이 많이 쌓였으면 먼저 회수한다.
	if (chunk.RemoteFreePending.load(std::memory_order_relaxed) >= HConfig::RemoteFreeDrainThreshold)
	{
		drainRemoteFrees(chunk);
	}

	HPage* page = cls.PartialHead;
	if (page == nullptr)
	{
		drainRemoteFrees(chunk);    // 원격 반납이 이 클래스의 페이지를 채웠을 수 있다
		page = cls.PartialHead;
		if (page == nullptr)
		{
			page = takeEmptyOrGrow(chunk, classIndex);    // 10초 안에 비워진 빈 페이지가 있으면 그것, 없으면 새 페이지
			if (page == nullptr)
			{
				poolFatal("memory pool: page allocation failed (system heap exhausted)", InMemorySize);
			}
		}
	}

	void* block    = page->FreeHead;
	page->FreeHead = nextOf(block);
	++page->Live;
	if (page->FreeHead == nullptr)
	{
		listUnlinkPartial(cls, page);    // 가득 찬 페이지는 Partial 에서 뺀다
	}

	HMemoryHeader* header = GetMemoryHeader(block);    // MemorySize / OwnerChunkID 는 페이지 생성 때 써 둔 값
	header->MemeoryState = EMemoryState::Allocated;

	const uint32 live = cls.Live.fetch_add(1, std::memory_order_relaxed) + 1;
	if (live > cls.Peak.load(std::memory_order_relaxed))
	{
		cls.Peak.store(live, std::memory_order_relaxed);
	}
	cls.AllocCount.fetch_add(1, std::memory_order_relaxed);

	debugFill(block, cls.BlockSize, 0xCD);

#if JG_MEMORY_TRACE
	// 주소는 memlog_stats.sh 가 소문자 16진수로 읽으므로 %p(MSVC는 대문자·고정폭) 대신 %llx 를 쓴다.
	JG_LOG(Memory, ELogLevel::Info, "[ID: %llu] Allocated %llu byte, address : %llx", chunk.Index, static_cast<uint64>(cls.BlockSize), reinterpret_cast<uint64>(block));
#endif
	return block;
}

void HMemoryPool::Deallocate(void* InMemory)
{
	if (InMemory == nullptr)
	{
		return;
	}

	HMemoryHeader* header = GetMemoryHeader(InMemory);
	if (header->MemorySize == EMemorySize::Large)
	{
		deallocateLarge(InMemory);
		return;
	}

	HMemoryChunk* owner = chunkOf(header->OwnerChunkID);
	if (owner == nullptr || header->MemeoryState != EMemoryState::Allocated || isValidClassBlockSize(static_cast<uint64>(header->MemorySize)) == false)
	{
		// 이중 해제 또는 풀 밖 포인터. 목록을 건드리지 않고 남긴다.
		JG_POOL_LOG(ELogLevel::Critical, "Deallocate: invalid block %llx (state %d, owner %llu, size %llu). double free or foreign pointer",
			reinterpret_cast<uint64>(InMemory), static_cast<int32>(header->MemeoryState), header->OwnerChunkID, static_cast<uint64>(header->MemorySize));
		JG_CHECK(false);
		return;
	}

	header->MemeoryState = EMemoryState::None;

	const uint32 classIndex = classIndexForBlockSize(static_cast<uint64>(header->MemorySize));
	HSizeClass&  cls        = owner->Classes[classIndex];
	cls.Live.fetch_sub(1, std::memory_order_relaxed);
	debugFill(InMemory, cls.BlockSize, 0xDD);

#if JG_MEMORY_TRACE
	JG_LOG(Memory, ELogLevel::Info, "[ID: %llu] Deallocated %llu byte, address : %llx", owner->Index, static_cast<uint64>(cls.BlockSize), reinterpret_cast<uint64>(InMemory));
#endif

	// 소유 스레드 판정은 TLS 캐시의 청크 포인터 비교라 비용이 없다. 이 모듈 사본의 TLS 가 아직 비어 있으면 스레드 id 로 한 번 채운다.
	bool bOwnerThread = (tlsChunk.Pool == this && tlsChunk.Chunk == owner);
	if (bOwnerThread == false && (tlsChunk.Pool != this || tlsChunk.Chunk == nullptr))
	{
		if (owner->OwnerThread == std::this_thread::get_id())
		{
			tlsChunk.Pool  = this;
			tlsChunk.Chunk = owner;
			bOwnerThread   = true;
		}
	}

	if (bOwnerThread == true)
	{
#if JG_MEMORY_QUARANTINE_BLOCKS > 0
		// 최근 해제 N개를 격리해 재사용을 늦춘다. 밀려 나온 블록을 대신 되돌린다.
		if (cls.BlockSize <= QuarantineMaxBlockSize)
		{
			void* evicted = cls.Quarantine[cls.QuarantineNext];
			cls.Quarantine[cls.QuarantineNext] = InMemory;
			cls.QuarantineNext = (cls.QuarantineNext + 1) % JG_MEMORY_QUARANTINE_BLOCKS;
			if (evicted == nullptr)
			{
				return;
			}
			InMemory = evicted;
		}
#endif
		freeBlockLocal(*owner, InMemory);
		return;
	}

	// 타 스레드: 소유 청크의 원격 스택에 lock-free push (MPSC). 소유 스레드가 다음 할당 때 회수한다.
	// Pending 은 push 전에 올린다 (소유자가 먼저 빼 가도 카운터가 음수로 돌지 않는다).
	owner->RemoteFreePending.fetch_add(1, std::memory_order_relaxed);
	owner->RemoteFreeTotal.fetch_add(1, std::memory_order_relaxed);
	void* head = owner->RemoteFreeHead.load(std::memory_order_relaxed);
	do
	{
		nextOf(InMemory) = head;
	} while (owner->RemoteFreeHead.compare_exchange_weak(head, InMemory, std::memory_order_release, std::memory_order_relaxed) == false);
}

// 소유 스레드에서 블록을 페이지의 빈 목록에 되돌린다. 통계 Live 는 Deallocate 가 이미 내렸다.
void HMemoryPool::freeBlockLocal(HMemoryChunk& InChunk, void* InBlock)
{
	HMemoryHeader* header = GetMemoryHeader(InBlock);
	HSizeClass&    cls    = InChunk.Classes[classIndexForBlockSize(static_cast<uint64>(header->MemorySize))];
	HPage*         page   = pageOf(InBlock, cls);

	const bool bWasFull = (page->FreeHead == nullptr);
	nextOf(InBlock) = page->FreeHead;
	page->FreeHead  = InBlock;
	--page->Live;

	if (bWasFull == true)
	{
		listPushPartial(cls, page);    // 다시 Partial 목록으로
	}
	if (page->Live == 0)
	{
		moveToEmpty(InChunk, cls, page);    // Partial 에서 빼 Empty 목록으로 (비워진 시각 기록)
	}
}

// 소유 스레드만 호출. 원격 스택을 한 번에 전부 가져온다 (단일 소비자라 ABA 없음).
void HMemoryPool::drainRemoteFrees(HMemoryChunk& InChunk)
{
	void*  block = InChunk.RemoteFreeHead.exchange(nullptr, std::memory_order_acquire);
	uint32 count = 0;
	while (block != nullptr)
	{
		void* next = nextOf(block);
		freeBlockLocal(InChunk, block);
		block = next;
		++count;
	}
	if (count > 0)
	{
		InChunk.RemoteFreePending.fetch_sub(count, std::memory_order_relaxed);
	}
}

// ----------------------------------------------------------------------------------------------------------------------------
// 대형 블록 (설계 §3-9, §3-10). 1MB 초과 또는 정렬 16 초과. 요청 크기 그대로 잡고, 놓으면 풀 전체 캐시에 둔다.
// ----------------------------------------------------------------------------------------------------------------------------

void* HMemoryPool::allocateLarge(HMemoryChunk& InChunk, uint64 InMemorySize, uint64 InAlignment)
{
	const uint64 alignment = (InAlignment > HConfig::HeaderSize) ? InAlignment : HConfig::HeaderSize;
	JG_CHECK((alignment & (alignment - 1)) == 0);

	// 정렬 16 초과 요청은 캐시를 보지 않는다 (캐시 블록은 16 정렬만 보장). 정렬 16 초과로 잡은 블록도 놓이면 캐시에 들어가 일반 요청에 재사용된다.
	char* payload = (alignment == HConfig::HeaderSize) ? static_cast<char*>(tryReuseLarge(InMemorySize)) : nullptr;
	if (payload == nullptr)
	{
		// malloc 은 16 정렬이므로 raw + 32 도 16 정렬. 그 위 정렬은 alignment - 16 만큼 여유를 둔다.
		const uint64 rawBytes = InMemorySize + HConfig::LargeHeaderSize + (alignment - HConfig::HeaderSize);
		char* raw = static_cast<char*>(malloc(rawBytes));
		if (raw == nullptr)
		{
			poolFatal("memory pool: large allocation failed (system heap exhausted)", InMemorySize);    // 유일하게 남는 실패 경로
		}

		payload = reinterpret_cast<char*>(alignUp(reinterpret_cast<uintptr_t>(raw) + HConfig::LargeHeaderSize, alignment));
		HLargeHeader* fresh = largeHeaderOf(payload);
		fresh->Raw      = raw;
		fresh->Capacity = InMemorySize;
	}

	HLargeHeader* header = largeHeaderOf(payload);
	header->Header.MemorySize   = EMemorySize::Large;
	header->Header.MemeoryState = EMemoryState::Allocated;
	header->Header.OwnerChunkID = InChunk.Index;    // 소유 청크 = 이번에 할당(재사용)한 스레드

	const uint64 bytes = InChunk.LargeBytes.fetch_add(header->Capacity, std::memory_order_relaxed) + header->Capacity;
	if (bytes > InChunk.LargePeakBytes.load(std::memory_order_relaxed))
	{
		InChunk.LargePeakBytes.store(bytes, std::memory_order_relaxed);
	}
	InChunk.LargeLive.fetch_add(1, std::memory_order_relaxed);
	InChunk.LargeTotal.fetch_add(1, std::memory_order_relaxed);

	debugFill(payload, InMemorySize, 0xCD);

#if JG_MEMORY_TRACE
	JG_LOG(Memory, ELogLevel::Info, "[ID: %llu] Allocated %llu byte, address : %llx", InChunk.Index, InMemorySize, reinterpret_cast<uint64>(payload));
#endif
	return payload;
}

void* HMemoryPool::tryReuseLarge(uint64 InMemorySize)
{
	if (LargeCachedCount.load(std::memory_order_relaxed) == 0)
	{
		return nullptr;
	}

	HLockGuard<HMutex> lock(LargeCacheMutex);

	// [요청, 요청 × 1.25] 안에서 가장 작은 것 (결정 2-a: 낭비 상한 25%). 경계는 정수 비교 `용량 × 4 <= 요청 × 5` 로 정확히 본다
	// (요청 + 요청 / 4 는 나눗셈 절사로 경계값을 놓친다). 캐시는 수십 개 이하다 (예산 64MB / MB 단위).
	HLargeCacheNode* best         = nullptr;
	uint64           bestCapacity = 0;
	for (HLargeCacheNode* node = LargeCacheHead; node != nullptr; node = node->Next)
	{
		const uint64 capacity = largeHeaderOf(node)->Capacity;
		if (capacity >= InMemorySize && capacity * 4 <= InMemorySize * 5 && (best == nullptr || capacity < bestCapacity))
		{
			best         = node;
			bestCapacity = capacity;
		}
	}
	if (best == nullptr)
	{
		return nullptr;
	}

	if (best->Prev != nullptr)
	{
		best->Prev->Next = best->Next;
	}
	else
	{
		LargeCacheHead = best->Next;
	}
	if (best->Next != nullptr)
	{
		best->Next->Prev = best->Prev;
	}
	else
	{
		LargeCacheTail = best->Prev;
	}

	LargeCachedBytes.fetch_sub(bestCapacity, std::memory_order_relaxed);
	LargeCachedCount.fetch_sub(1, std::memory_order_relaxed);
	LargeReuseCount.fetch_add(1, std::memory_order_relaxed);
	LargeReuseWasteBytes.fetch_add(bestCapacity - InMemorySize, std::memory_order_relaxed);
	return best;    // 노드 위치 = payload
}

// 어느 스레드에서든 호출 가능. 소유 스레드를 거치지 않는다.
void HMemoryPool::deallocateLarge(void* InMemory)
{
	HLargeHeader* header = largeHeaderOf(InMemory);
	HMemoryChunk* owner  = chunkOf(header->Header.OwnerChunkID);
	if (owner == nullptr || header->Header.MemeoryState != EMemoryState::Allocated)
	{
		JG_POOL_LOG(ELogLevel::Critical, "Deallocate: invalid large block %llx (state %d, owner %llu, capacity %llu). double free or foreign pointer",
			reinterpret_cast<uint64>(InMemory), static_cast<int32>(header->Header.MemeoryState), header->Header.OwnerChunkID, header->Capacity);
		JG_CHECK(false);
		return;
	}

	header->Header.MemeoryState = EMemoryState::None;
	const uint64 capacity = header->Capacity;
	owner->LargeBytes.fetch_sub(capacity, std::memory_order_relaxed);
	owner->LargeLive.fetch_sub(1, std::memory_order_relaxed);

	debugFill(InMemory, capacity, 0xDD);

#if JG_MEMORY_TRACE
	JG_LOG(Memory, ELogLevel::Info, "[ID: %llu] Deallocated %llu byte, address : %llx", owner->Index, capacity, reinterpret_cast<uint64>(InMemory));
#endif

	// 예산보다 큰 것은 캐시하지 않는다. 캐시 노드(24B)가 안 들어가는 작은 정렬 요청도 바로 돌려준다.
	if (capacity > HConfig::LargeCacheBudget || capacity < sizeof(HLargeCacheNode))
	{
		free(header->Raw);
		return;
	}

	HLargeCacheNode* node = static_cast<HLargeCacheNode*>(InMemory);
	node->FreedSeconds = nowSeconds();

	HLockGuard<HMutex> lock(LargeCacheMutex);
	node->Prev = nullptr;
	node->Next = LargeCacheHead;
	if (LargeCacheHead != nullptr)
	{
		LargeCacheHead->Prev = node;
	}
	else
	{
		LargeCacheTail = node;
	}
	LargeCacheHead = node;
	LargeCachedBytes.fetch_add(capacity, std::memory_order_relaxed);
	LargeCachedCount.fetch_add(1, std::memory_order_relaxed);

	// 예산을 넘으면 오래된 것부터 돌려준다.
	while (LargeCachedBytes.load(std::memory_order_relaxed) > HConfig::LargeCacheBudget && LargeCacheTail != nullptr)
	{
		evictLargeTailLocked();
		LargeEvictCount.fetch_add(1, std::memory_order_relaxed);
	}
}

void HMemoryPool::evictLargeTailLocked()
{
	HLargeCacheNode* node = LargeCacheTail;
	if (node == nullptr)
	{
		return;
	}

	LargeCacheTail = node->Prev;
	if (LargeCacheTail != nullptr)
	{
		LargeCacheTail->Next = nullptr;
	}
	else
	{
		LargeCacheHead = nullptr;
	}

	HLargeHeader* header = largeHeaderOf(node);
	LargeCachedBytes.fetch_sub(header->Capacity, std::memory_order_relaxed);
	LargeCachedCount.fetch_sub(1, std::memory_order_relaxed);
	free(header->Raw);
}

// ----------------------------------------------------------------------------------------------------------------------------
// 프레임 틱·통계
// ----------------------------------------------------------------------------------------------------------------------------

void HMemoryPool::Tick()
{
	const double now = nowSeconds();    // 엔진 타이머에 의존하지 않는다

	releaseIdlePages(now);

	if (LargeCachedCount.load(std::memory_order_relaxed) == 0)
	{
		return;
	}

	HLockGuard<HMutex> lock(LargeCacheMutex);
	while (LargeCacheTail != nullptr && now - LargeCacheTail->FreedSeconds >= HConfig::IdleSeconds)
	{
		evictLargeTailLocked();
		LargeIdleReleaseCount.fetch_add(1, std::memory_order_relaxed);
	}
}

// 카운터를 relaxed 로 읽기만 한다. 락을 잡지 않는다. 다른 스레드 청크 값은 한 프레임 늦을 수 있다 (통계 용도).
void HMemoryPool::GetStatInfo(HMemoryPoolStatInfo& OutStatInfo)
{
	OutStatInfo = HMemoryPoolStatInfo();

	const uint32 chunkCount = ChunkCount.load(std::memory_order_acquire);
	OutStatInfo.ChunkCount  = chunkCount;

	for (uint32 c = 0; c < chunkCount; ++c)
	{
		const HMemoryChunk* chunk = Chunks[c].load(std::memory_order_acquire);
		if (chunk == nullptr)
		{
			continue;
		}

		HMemoryChunkStatInfo info;
		info.ChunkID         = chunk->Index;
		info.OwnerThreadID   = chunk->OwnerThreadHash;
		info.TotalMemorySize = chunk->CommittedBytes.load(std::memory_order_relaxed);

		for (uint32 i = 0; i < HConfig::ClassCount; ++i)
		{
			const HSizeClass&     cls = chunk->Classes[i];
			HMemoryClassStatInfo& out = info.Classes[i];
			out.BlockSize     = cls.BlockSize;
			out.BlocksPerPage = cls.BlocksPerPage;
			out.Pages         = cls.PageCount.load(std::memory_order_relaxed);
			out.EmptyPages    = cls.EmptyCount.load(std::memory_order_relaxed);
			out.Total         = cls.Total.load(std::memory_order_relaxed);
			out.Live          = cls.Live.load(std::memory_order_relaxed);
			out.Peak          = cls.Peak.load(std::memory_order_relaxed);
			out.Growths       = cls.Growths.load(std::memory_order_relaxed);
			out.Releases      = cls.Releases.load(std::memory_order_relaxed);
			out.AllocCount    = cls.AllocCount.load(std::memory_order_relaxed);

			info.AllocatedMemorySize += static_cast<uint64>(out.Live) * out.BlockSize;
			info.HeaderMemorySize    += static_cast<uint64>(out.Total) * HConfig::HeaderSize + static_cast<uint64>(out.Pages) * HConfig::PageHeaderSize;
		}

		info.RemoteFreePending = chunk->RemoteFreePending.load(std::memory_order_relaxed);
		info.RemoteFreeTotal   = chunk->RemoteFreeTotal.load(std::memory_order_relaxed);
		info.LargeLive         = chunk->LargeLive.load(std::memory_order_relaxed);
		info.LargeBytes        = chunk->LargeBytes.load(std::memory_order_relaxed);
		info.LargePeakBytes    = chunk->LargePeakBytes.load(std::memory_order_relaxed);
		info.LargeTotal        = chunk->LargeTotal.load(std::memory_order_relaxed);

		info.TotalMemorySize     += info.LargeBytes + static_cast<uint64>(info.LargeLive) * HConfig::LargeHeaderSize;
		info.AllocatedMemorySize += info.LargeBytes;
		info.HeaderMemorySize    += static_cast<uint64>(info.LargeLive) * HConfig::LargeHeaderSize;

		OutStatInfo.TotalCommittedBytes += info.TotalMemorySize;
		OutStatInfo.TotalAllocatedBytes += info.AllocatedMemorySize;
		OutStatInfo.LargeLive           += info.LargeLive;
		OutStatInfo.LargeBytes          += info.LargeBytes;
		OutStatInfo.ChunkStatInfos.push_back(info);
	}

	OutStatInfo.LargeCachedCount      = LargeCachedCount.load(std::memory_order_relaxed);
	OutStatInfo.LargeCachedBytes      = LargeCachedBytes.load(std::memory_order_relaxed);
	OutStatInfo.LargeReuseCount       = LargeReuseCount.load(std::memory_order_relaxed);
	OutStatInfo.LargeReuseWasteBytes  = LargeReuseWasteBytes.load(std::memory_order_relaxed);
	OutStatInfo.LargeEvictCount       = LargeEvictCount.load(std::memory_order_relaxed);
	OutStatInfo.LargeIdleReleaseCount = LargeIdleReleaseCount.load(std::memory_order_relaxed);
}
