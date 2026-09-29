#include "PCH/PCH.h"
#include "MemoryStatistics.h"
#include "GUI.h"

namespace
{
	PString formatBytes(uint64 InBytes)
	{
		if (InBytes >= 1024ull * 1024ull)
		{
			return PString::Format("%.2f MB", static_cast<double>(InBytes) / (1024.0 * 1024.0));
		}
		if (InBytes >= 1024ull)
		{
			return PString::Format("%.1f KB", static_cast<double>(InBytes) / 1024.0);
		}
		return PString::Format("%llu B", InBytes);
	}

	const HLinearColor MemoryStatColorDim (0.62f, 0.62f, 0.62f, 1.0f);
	const HLinearColor MemoryStatColorWarn(1.0f, 0.78f, 0.25f, 1.0f);   // 성장이 있었던 클래스
	const HLinearColor MemoryStatColorBad (1.0f, 0.40f, 0.40f, 1.0f);   // 사용률 90% 이상 (다음 할당이 페이지를 추가한다)
}

void JGMemoryStatistcs::OnInitialize()
{

}
void JGMemoryStatistcs::OnLayout(const HWidgetLayout& InLayout)
{
	HVector2 ContentSize = InLayout.ContentSize;
}

// 풀 통계는 O(1) 카운터 읽기다 (Memory_TODO 2-2). 청크 전체를 걷지 않으므로 위젯 개폐가 프레임 시간에 영향을 주지 않는다.
void JGMemoryStatistcs::OnGenerateGUI()
{
	HMemoryPoolStatInfo StatInfo;
	HMemoryPool* MemoryPool = GMemoryGlobalSystem::GetInstance().GetMemoryPool();
	MemoryPool->GetStatInfo(StatInfo);

	const ThreadID MainThreadID = GCoreSystem::GetMainThreadID();

	// 메인 스레드 청크를 맨 앞에
	HList<HMemoryChunkStatInfo> Chunks;
	for (const HMemoryChunkStatInfo& ChunkStatInfo : StatInfo.ChunkStatInfos)
	{
		if (ChunkStatInfo.OwnerThreadID == MainThreadID)
		{
			Chunks.insert(Chunks.begin(), ChunkStatInfo);
		}
		else
		{
			Chunks.push_back(ChunkStatInfo);
		}
	}

	// 풀 전체
	HGUI::Text(PString::Format("Pool: %u chunks, committed %s, allocated %s, large live %u (%s)",
		StatInfo.ChunkCount, formatBytes(StatInfo.TotalCommittedBytes).GetCStr(), formatBytes(StatInfo.TotalAllocatedBytes).GetCStr(),
		StatInfo.LargeLive, formatBytes(StatInfo.LargeBytes).GetCStr()));
	HGUI::Text(PString::Format("Large cache: %u blocks / %s, reuse %llu (waste %s), budget evict %llu, idle release %llu",
		StatInfo.LargeCachedCount, formatBytes(StatInfo.LargeCachedBytes).GetCStr(), StatInfo.LargeReuseCount, formatBytes(StatInfo.LargeReuseWasteBytes).GetCStr(),
		StatInfo.LargeEvictCount, StatInfo.LargeIdleReleaseCount));

	// 스레드별 막대: 커밋 대비 헤더 / 할당 / 빈 블록 비율
	HPlotBarGroupsArguments Args;
	Args.PlotSize   = HVector2(1000.0f, static_cast<float32>(Chunks.size()) * 60.0f + 80.0f);
	Args.TitleName  = "Memory";
	Args.DataLabels = { "MemoryHeader", "Allocated", "Empty" };

	HList<double> MemHeaderMemRatios;
	HList<double> AllocatedMemRatios;
	HList<double> EmptyMemRatios;

	int32 Idx = 0;
	for (const HMemoryChunkStatInfo& ChunkStatInfo : Chunks)
	{
		const double TotalMemory     = static_cast<double>(ChunkStatInfo.TotalMemorySize);
		const double MemHeaderMemory = static_cast<double>(ChunkStatInfo.HeaderMemorySize);
		const double AllocatedMemory = static_cast<double>(ChunkStatInfo.AllocatedMemorySize);
		const double EmptyMemory     = (TotalMemory > MemHeaderMemory + AllocatedMemory) ? TotalMemory - MemHeaderMemory - AllocatedMemory : 0.0;

		if (Idx == 0)
		{
			Args.GroupLabels.push_back(PString::Format("Main Thread (%s / %s)", formatBytes(ChunkStatInfo.AllocatedMemorySize + ChunkStatInfo.HeaderMemorySize).GetCStr(), formatBytes(ChunkStatInfo.TotalMemorySize).GetCStr()));
		}
		else
		{
			Args.GroupLabels.push_back(PString::Format("Thread %d (%s / %s)", Idx, formatBytes(ChunkStatInfo.AllocatedMemorySize + ChunkStatInfo.HeaderMemorySize).GetCStr(), formatBytes(ChunkStatInfo.TotalMemorySize).GetCStr()));
		}

		MemHeaderMemRatios.push_back((TotalMemory > 0.0) ? MemHeaderMemory / TotalMemory : 0.0);
		AllocatedMemRatios.push_back((TotalMemory > 0.0) ? AllocatedMemory / TotalMemory : 0.0);
		EmptyMemRatios.push_back((TotalMemory > 0.0) ? EmptyMemory / TotalMemory : 0.0);

		++Idx;
	}

	for (double Value : MemHeaderMemRatios)
	{
		Args.Datas.push_back(Value);
	}

	for (double Value : AllocatedMemRatios)
	{
		Args.Datas.push_back(Value);
	}

	for (double Value : EmptyMemRatios)
	{
		Args.Datas.push_back(Value);
	}

	HGUI::PlotBarGroups(Args);

	// 청크별 클래스 표 (크기 / live / total / 페이지(빈 페이지) / 성장 / 회수 / 사용률 / 피크)
	Idx = 0;
	for (const HMemoryChunkStatInfo& ChunkStatInfo : Chunks)
	{
		HGUI::Text(PString::Format("[chunk %llu] %s   committed %s   allocated %s   remote free pending %u / total %llu   large live %u (%s, peak %s, total %llu)",
			ChunkStatInfo.ChunkID, (Idx == 0) ? "Main Thread" : PString::Format("Thread %d", Idx).GetCStr(),
			formatBytes(ChunkStatInfo.TotalMemorySize).GetCStr(), formatBytes(ChunkStatInfo.AllocatedMemorySize).GetCStr(),
			ChunkStatInfo.RemoteFreePending, ChunkStatInfo.RemoteFreeTotal,
			ChunkStatInfo.LargeLive, formatBytes(ChunkStatInfo.LargeBytes).GetCStr(), formatBytes(ChunkStatInfo.LargePeakBytes).GetCStr(), ChunkStatInfo.LargeTotal));
		HGUI::Text("      class        live /   total    pages (empty)    growth   release    usage    peak", MemoryStatColorDim);

		for (const HMemoryClassStatInfo& ClassStatInfo : ChunkStatInfo.Classes)
		{
			if (ClassStatInfo.Pages == 0 && ClassStatInfo.Growths == 0 && ClassStatInfo.Releases == 0 && ClassStatInfo.Peak == 0)
			{
				continue;
			}

			const double Usage = (ClassStatInfo.Total > 0) ? static_cast<double>(ClassStatInfo.Live) * 100.0 / static_cast<double>(ClassStatInfo.Total) : 0.0;
			const PString Line = PString::Format("%9u B   %7u / %7u   %5u (%3u)     %6u   %7u   %5.1f%%   %7u",
				ClassStatInfo.BlockSize, ClassStatInfo.Live, ClassStatInfo.Total, ClassStatInfo.Pages, ClassStatInfo.EmptyPages,
				ClassStatInfo.Growths, ClassStatInfo.Releases, Usage, ClassStatInfo.Peak);

			if (Usage >= 90.0)
			{
				HGUI::Text(Line, MemoryStatColorBad);
			}
			else if (ClassStatInfo.Growths > 0)
			{
				HGUI::Text(Line, MemoryStatColorWarn);
			}
			else
			{
				HGUI::Text(Line);
			}
		}

		++Idx;
	}
}

PString JGMemoryStatistcs::GetTitleName() const
{
	return "MemoryStatistcs";
}

const HGuid& JGMemoryStatistcs::GetGUID() const
{
	return GetStaticGUID();
}
