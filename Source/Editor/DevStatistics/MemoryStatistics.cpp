#include "PCH/PCH.h"
#include "MemoryStatistics.h"
#include "GUI.h"


void JGMemoryStatistcs::OnInitialize()
{

}
void JGMemoryStatistcs::OnLayout(const HWidgetLayout& InLayout)
{
	HVector2 ContentSize = InLayout.ContentSize;
}

void JGMemoryStatistcs::OnGenerateGUI()
{
	HList<PString> GroupLabels = { "Main Thread", "Thread 1", "Thread 2", "Thread 3" };
	HList<PString> DataLabels = { "Memory Header", "Allocated", "Empty" };

	HMemoryPoolStatInfo StatInfo;
	HMemoryPool* MemoryPool = GMemoryGlobalSystem::GetInstance().GetMemoryPool();
	MemoryPool->GetStatInfo(StatInfo);

	ThreadID MainThreadID = GCoreSystem::GetInstance().GetMainThreadID();

	HList<HMemoryChunkStatInfo> MemoryChunkStateInfos;
	for (const HMemoryChunkStatInfo& ChunkStatInfo : StatInfo.ChunkStatInfos)
	{
		if (ChunkStatInfo.ChunkID == MainThreadID)
		{
			MemoryChunkStateInfos.insert(MemoryChunkStateInfos.begin(), ChunkStatInfo);
		}
		else
		{
			MemoryChunkStateInfos.push_back(ChunkStatInfo);
		}
	}

	HPlotBarGroupsArguments Args;
	float32 HeightSize = MemoryChunkStateInfos.size() * 100;

	Args.PlotSize = HVector2(1000, HeightSize);
	Args.TitleName   = "Memory";
	Args.DataLabels  = { "MemoryHeader", "Allocated", "Empty" };

	HList<double> MemHeaderMemRatios;
	HList<double> AllocatedMemRatios;
	HList<double> EmptyMemRatios;

	int32 Idx = 0;
	for (const HMemoryChunkStatInfo& ChunkStatInfo : MemoryChunkStateInfos)
	{
		EMemoryUnit TargetUnit = EMemoryUnit::MB;
		if (Idx == 0)
		{
			TargetUnit = EMemoryUnit::GB;
		}

		double TotalMemory = ConvertMemory(EMemoryUnit::Byte, MemoryChunkStateInfos[0].TotalMemorySize, TargetUnit);
		double MemHeaderMemory = ConvertMemory(EMemoryUnit::Byte, MemoryChunkStateInfos[0].HeaderMemorySize, TargetUnit);
		double AllocatedMemory = ConvertMemory(EMemoryUnit::Byte, MemoryChunkStateInfos[0].AllocatedMemorySize, TargetUnit);
		double EmptyMemory = TotalMemory - MemHeaderMemory - AllocatedMemory;
		if (Idx == 0)
		{
			Args.GroupLabels.push_back(PString::Format("Main Thread (%.2f GB / %2f GB)", AllocatedMemory + MemHeaderMemory, TotalMemory));
		}
		else
		{
			Args.GroupLabels.push_back(PString::Format("Thread %d (%.2f MB / %2f MB)", Idx, AllocatedMemory + MemHeaderMemory, TotalMemory));
		}

		MemHeaderMemRatios.push_back(MemHeaderMemory / TotalMemory);
		AllocatedMemRatios.push_back(AllocatedMemory / TotalMemory);
		EmptyMemRatios.push_back(EmptyMemory / TotalMemory);

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
}

PString JGMemoryStatistcs::GetTitleName() const
{
	return "MemoryStatistcs";
}

const HGuid& JGMemoryStatistcs::GetGUID() const
{
	return GetStaticGUID();
}
