#include "PCH/PCH.h"
#include "MemoryStatistics.h"
#include "GUI.h"

namespace
{
	// 화면에 보일 색(sRGB 0xRRGGBB)을 GUI 에 넘길 선형 색으로 바꾼다.
	// GUI 는 선형(FP16) 버퍼에 그려진 뒤 sRGB 로 인코딩되어 나간다(ImGui 기본 창 배경 0.06 이 #454545 로 보인다).
	// sRGB 값을 그대로 넘기면 옅고 밝게 뜬다. 출력 색 공간이 바뀌면(Graphics 5-14) 이 변환만 고친다.
	float32 srgbToLinear(float32 InValue)
	{
		if (InValue <= 0.04045f)
		{
			return InValue / 12.92f;
		}
		return powf((InValue + 0.055f) / 1.055f, 2.4f);
	}

	HLinearColor displayColor(uint32 InRGB)
	{
		const float32 R = static_cast<float32>((InRGB >> 16) & 0xFF) / 255.0f;
		const float32 G = static_cast<float32>((InRGB >> 8) & 0xFF) / 255.0f;
		const float32 B = static_cast<float32>(InRGB & 0xFF) / 255.0f;
		return HLinearColor(srgbToLinear(R), srgbToLinear(G), srgbToLinear(B), 1.0f);
	}

	// 두 sRGB 색을 섞는다. 막대의 빈 칸은 채움 색을 표면 쪽으로 섞은 같은 계열의 어두운 색이다.
	uint32 mixRGB(uint32 InFrom, uint32 InTo, float32 InAmount)
	{
		uint32 Result = 0;
		for (uint32 Shift = 0; Shift <= 16; Shift += 8)
		{
			const float32 From = static_cast<float32>((InFrom >> Shift) & 0xFF);
			const float32 To   = static_cast<float32>((InTo >> Shift) & 0xFF);
			Result |= static_cast<uint32>(From + (To - From) * InAmount + 0.5f) << Shift;
		}
		return Result;
	}

	// 팔레트 (어두운 표면). 계열 색 파랑 · 주황은 색각 이상 검사를 통과한 조합이고, 상태 색은 글자 딱지와 함께만 쓴다.
	const uint32 SurfaceRGB  = 0x1a1a19;
	const uint32 InUseRGB    = 0x3987e5;
	const uint32 CriticalRGB = 0xd03b3b;

	const HLinearColor ColorPage          = displayColor(0x0d0d0d);
	const HLinearColor ColorSurface       = displayColor(SurfaceRGB);
	const HLinearColor ColorRaised        = displayColor(0x242423);     // 선택된 탭 · 풍선 도움말
	const HLinearColor ColorRaisedHover   = displayColor(0x2e2e2c);
	const HLinearColor ColorBorder        = displayColor(0x313130);
	const HLinearColor ColorGrid          = displayColor(0x2c2c2a);
	const HLinearColor ColorTextPrimary   = displayColor(0xffffff);
	const HLinearColor ColorTextSecondary = displayColor(0xc3c2b7);
	const HLinearColor ColorTextMuted     = displayColor(0x898781);
	const HLinearColor ColorInUse         = displayColor(InUseRGB);     // 할당된 블록
	const HLinearColor ColorHeader        = displayColor(0xd95926);     // 블록 · 페이지 헤더
	const HLinearColor ColorCommitted     = displayColor(0x898781);     // 추이의 맥락선 (강조하지 않는다)
	const HLinearColor ColorInUseTrack    = displayColor(mixRGB(SurfaceRGB, InUseRGB, 0.22f));
	const HLinearColor ColorWarning       = displayColor(0xfab219);     // 페이지가 늘어난 클래스
	const HLinearColor ColorCritical      = displayColor(CriticalRGB);  // 사용률 90% 이상
	const HLinearColor ColorCriticalTrack = displayColor(mixRGB(SurfaceRGB, CriticalRGB, 0.22f));
	const HLinearColor ColorInk           = displayColor(0x0d0d0d);     // 밝은 딱지 위 글자

	const float64 HistorySeconds  = 60.0;    // 추이 그래프 가로 범위
	const float64 HistoryInterval = 0.25;    // 표본 간격
	const float64 FullUsage       = 90.0;    // 이 사용률 이상이면 다음 할당이 페이지를 추가한다
	const float32 PanelGap        = 8.0f;

	float64 toMB(uint64 InBytes)
	{
		return static_cast<float64>(InBytes) / (1024.0 * 1024.0);
	}

	float64 percentOf(uint64 InPart, uint64 InWhole)
	{
		return (InWhole > 0) ? static_cast<float64>(InPart) * 100.0 / static_cast<float64>(InWhole) : 0.0;
	}

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

	// 크기 클래스는 2 의 거듭제곱이라 KB · MB 가 정수로 떨어진다.
	PString formatClassSize(uint32 InBytes)
	{
		if (InBytes >= 1024u * 1024u && InBytes % (1024u * 1024u) == 0)
		{
			return PString::Format("%u MB", InBytes / (1024u * 1024u));
		}
		if (InBytes >= 1024u && InBytes % 1024u == 0)
		{
			return PString::Format("%u KB", InBytes / 1024u);
		}
		return PString::Format("%u B", InBytes);
	}

	// 세 자리마다 쉼표 (1,284)
	PString formatCount(uint64 InValue)
	{
		const std::string Digits = std::to_string(InValue);
		std::string Result;
		Result.reserve(Digits.size() + Digits.size() / 3);
		for (size_t Index = 0; Index < Digits.size(); ++Index)
		{
			if (Index > 0 && (Digits.size() - Index) % 3 == 0)
			{
				Result.push_back(',');
			}
			Result.push_back(Digits[Index]);
		}
		return PString(Result.c_str());
	}

	// "1 page" / "26 pages"
	PString formatNoun(uint64 InCount, const char* InSingular, const char* InPlural)
	{
		return PString::Format("%s %s", formatCount(InCount).GetCStr(), (InCount == 1) ? InSingular : InPlural);
	}

	struct HPoolTotals
	{
		uint64 LiveBlocks  = 0;
		uint64 HeaderBytes = 0;
		uint64 Pages       = 0;
		uint64 EmptyPages  = 0;
		uint64 Growths     = 0;
		uint64 Releases    = 0;
		uint64 Allocations = 0;
	};

	HPoolTotals sumPool(const HMemoryPoolStatInfo& InStatInfo)
	{
		HPoolTotals Totals;
		Totals.LiveBlocks = InStatInfo.LargeLive;
		for (const HMemoryChunkStatInfo& Chunk : InStatInfo.ChunkStatInfos)
		{
			Totals.HeaderBytes += Chunk.HeaderMemorySize;
			for (const HMemoryClassStatInfo& SizeClass : Chunk.Classes)
			{
				Totals.LiveBlocks  += SizeClass.Live;
				Totals.Pages       += SizeClass.Pages;
				Totals.EmptyPages  += SizeClass.EmptyPages;
				Totals.Growths     += SizeClass.Growths;
				Totals.Releases    += SizeClass.Releases;
				Totals.Allocations += SizeClass.AllocCount;
			}
		}
		return Totals;
	}

	// 구역 제목: 왼쪽에 이름, 오른쪽에 흐린 보조 글자
	void drawPanelTitle(const PString& InTitle, const PString& InNote)
	{
		HGUI::Text(InTitle, ColorTextPrimary);
		if (InNote.Empty() == false)
		{
			HGUI::SameLine();
			HGUI::TextRight(InNote, ColorTextMuted);
		}
		HGUI::Spacing();
	}

	// 0 은 흐린 "-" 로 써서 눈이 0 이 아닌 값에 가게 한다.
	void drawCountCell(uint64 InValue)
	{
		if (InValue == 0)
		{
			HGUI::TextRight("-", ColorTextMuted);
		}
		else
		{
			HGUI::TextRight(formatCount(InValue));
		}
	}

	// 요약 카드: 이름 · 큰 값 · 보조 줄. InMeter 가 0 이상이면 값 아래에 비율 막대를 그린다.
	void drawStatCard(const PString& InName, const HVector2& InSize, const PString& InLabel, const PString& InValue,
		const PString& InDetail, float32 InMeter, const PString& InTooltip)
	{
		HGUI::BeginPanel(InName, InSize, ColorSurface, ColorBorder);
		HGUI::Text(InLabel, ColorTextSecondary);
		HGUI::PushFont(EGUIFont::Large);
		HGUI::Text(InValue, ColorTextPrimary);
		HGUI::PopFont();
		if (InMeter >= 0.0f)
		{
			HGUI::SegmentedBar({ { InMeter, ColorInUse } }, HVector2(0.0f, 6.0f), ColorInUseTrack);
		}
		else
		{
			// 막대가 없는 카드도 보조 줄 높이를 맞춘다.
			HGUI::Text(" ");
		}
		HGUI::Text(InDetail, ColorTextMuted);
		HGUI::EndPanel();
		HGUI::ItemTooltip(InTooltip);
	}

	void drawSummary(const HMemoryPoolStatInfo& InStatInfo, const HPoolTotals& InTotals)
	{
		// 좁으면 두 줄로 (2 × 2)
		const float32  Avail    = HGUI::GetContentRegionAvail().x;
		const int32    Columns  = (Avail >= 640.0f) ? 4 : 2;
		const HVector2 CardSize((Avail - PanelGap * static_cast<float32>(Columns - 1)) / static_cast<float32>(Columns), 100.0f);
		const float64  InUsePercent = percentOf(InStatInfo.TotalAllocatedBytes, InStatInfo.TotalCommittedBytes);

		drawStatCard("##memory_card_committed", CardSize, "Committed", formatBytes(InStatInfo.TotalCommittedBytes),
			PString::Format("%s, %s", formatNoun(InStatInfo.ChunkCount, "thread", "threads").GetCStr(), formatNoun(InTotals.Pages, "page", "pages").GetCStr()), -1.0f,
			PString::Format("Pages the pool holds from the OS, plus live large blocks.\nPages %s (%s empty). Grew %s, released %s since start.",
				formatCount(InTotals.Pages).GetCStr(), formatCount(InTotals.EmptyPages).GetCStr(),
				formatCount(InTotals.Growths).GetCStr(), formatCount(InTotals.Releases).GetCStr()));
		HGUI::SameLine(PanelGap);

		drawStatCard("##memory_card_in_use", CardSize, "In use", formatBytes(InStatInfo.TotalAllocatedBytes),
			PString::Format("%.1f%% of committed", InUsePercent), static_cast<float32>(InUsePercent / 100.0),
			PString::Format("Bytes handed out: live blocks x block size, plus large blocks.\nHeaders take %s on top (block, page and large headers).",
				formatBytes(InTotals.HeaderBytes).GetCStr()));
		if (Columns == 4)
		{
			HGUI::SameLine(PanelGap);
		}

		drawStatCard("##memory_card_blocks", CardSize, "Live blocks", formatCount(InTotals.LiveBlocks),
			PString::Format("%s allocations so far", formatCount(InTotals.Allocations).GetCStr()), -1.0f,
			"Blocks allocated right now on every thread, large blocks included.");
		HGUI::SameLine(PanelGap);

		drawStatCard("##memory_card_large", CardSize, "Large blocks", formatCount(InStatInfo.LargeLive),
			PString::Format("%s live, %u cached", formatBytes(InStatInfo.LargeBytes).GetCStr(), InStatInfo.LargeCachedCount), -1.0f,
			PString::Format("Requests over 1 MB or aligned over 16 B.\nCache %u blocks (%s), reused %s times (waste %s).\nEvicted by budget %s, released after idle %s.",
				InStatInfo.LargeCachedCount, formatBytes(InStatInfo.LargeCachedBytes).GetCStr(),
				formatCount(InStatInfo.LargeReuseCount).GetCStr(), formatBytes(InStatInfo.LargeReuseWasteBytes).GetCStr(),
				formatCount(InStatInfo.LargeEvictCount).GetCStr(), formatCount(InStatInfo.LargeIdleReleaseCount).GetCStr()));
	}

	void drawTrend(const HList<float64>& InTimes, const HList<float64>& InCommittedMB, const HList<float64>& InAllocatedMB, float64 InNow)
	{
		// 범례는 제목 줄에 둔다 (그래프 안에 두면 선이 범례 높이를 지날 때 겹친다).
		HGUI::BeginPanel("##memory_trend", HVector2(0.0f, 0.0f), ColorSurface, ColorBorder);
		HGUI::Text("Pool over time", ColorTextPrimary);
		HGUI::SameLine(20.0f);
		HGUI::PushStyleColor(EGUIColor::Text, ColorTextSecondary);
		HGUI::LegendItem("Committed", ColorCommitted);
		HGUI::SameLine(16.0f);
		HGUI::LegendItem("In use", ColorInUse);
		HGUI::PopStyleColor();
		HGUI::SameLine();
		HGUI::TextRight(PString::Format("last %.0f s", HistorySeconds), ColorTextMuted);
		HGUI::Spacing();

		HPlotLinesArguments Args;
		Args.TitleName = "memory_trend_plot";
		Args.PlotSize  = HVector2(-1.0f, 150.0f);
		Args.XMin      = -HistorySeconds;
		Args.XMax      = 0.0;
		for (float64 Time : InTimes)
		{
			Args.XValues.push_back(Time - InNow);
		}

		// 커밋은 맥락(회색 선), 사용 중이 주인공(파란 선 + 옅은 면). 면 불투명도는 선형 버퍼에서 섞이므로 sRGB 10% 쯤으로 보이는 값이다.
		Args.SeriesLabels     = { "Committed", "In use" };
		Args.SeriesColors     = { ColorCommitted, ColorInUse };
		Args.SeriesFillAlphas = { 0.0f, 0.035f };
		Args.Datas.insert(Args.Datas.end(), InCommittedMB.begin(), InCommittedMB.end());
		Args.Datas.insert(Args.Datas.end(), InAllocatedMB.begin(), InAllocatedMB.end());

		float64 PeakMB = 0.0;
		for (float64 Value : InCommittedMB)
		{
			if (Value > PeakMB)
			{
				PeakMB = Value;
			}
		}
		Args.XAxisFormat   = "%.0fs";
		Args.YAxisFormat   = (PeakMB >= 10.0) ? "%.0f MB" : "%.1f MB";
		Args.ValueFormat   = "%.2f MB";
		Args.bShowLegend   = false;
		Args.GridColor     = ColorGrid;
		Args.AxisTextColor = ColorTextMuted;
		Args.SurfaceColor  = ColorSurface;
		HGUI::PlotLines(Args);

		HGUI::EndPanel();
	}

	void drawThreads(const HList<HMemoryChunkStatInfo>& InChunks, const HList<PString>& InNames)
	{
		HGUI::BeginPanel("##memory_threads", HVector2(0.0f, 0.0f), ColorSurface, ColorBorder);
		drawPanelTitle("Threads", formatNoun(InChunks.size(), "chunk", "chunks"));

		if (HGUI::BeginTable("##memory_thread_table", 5))
		{
			HGUI::TableSetupColumn("Thread", 96.0f);
			HGUI::TableSetupColumn("Usage");
			HGUI::TableSetupColumn("In use / Committed", 150.0f, true);
			HGUI::TableSetupColumn("Remote frees", 96.0f, true);
			HGUI::TableSetupColumn("Large live", 110.0f, true);
			HGUI::PushStyleColor(EGUIColor::Text, ColorTextMuted);
			HGUI::TableHeadersRow();
			HGUI::PopStyleColor();

			for (size_t Index = 0; Index < InChunks.size(); ++Index)
			{
				const HMemoryChunkStatInfo& Chunk = InChunks[Index];
				const uint64 Committed = Chunk.TotalMemorySize;
				const uint64 InUse     = Chunk.AllocatedMemorySize;
				const uint64 Headers   = Chunk.HeaderMemorySize;
				const uint64 Free      = (Committed > InUse + Headers) ? Committed - InUse - Headers : 0;

				HGUI::TableNextRow();
				HGUI::TableNextColumn();
				HGUI::Text(InNames[Index]);
				HGUI::ItemTooltip(PString::Format("Chunk %llu", Chunk.ChunkID));

				HGUI::TableNextColumn();
				HGUI::SegmentedBar({ { static_cast<float32>(percentOf(InUse, Committed) / 100.0), ColorInUse },
					{ static_cast<float32>(percentOf(Headers, Committed) / 100.0), ColorHeader } }, HVector2(0.0f, 8.0f), ColorInUseTrack);
				HGUI::ItemTooltip(PString::Format("In use     %s  (%.1f%%)\nHeaders    %s  (%.1f%%)\nFree       %s  (%.1f%%)\nCommitted  %s",
					formatBytes(InUse).GetCStr(), percentOf(InUse, Committed),
					formatBytes(Headers).GetCStr(), percentOf(Headers, Committed),
					formatBytes(Free).GetCStr(), percentOf(Free, Committed),
					formatBytes(Committed).GetCStr()));

				HGUI::TableNextColumn();
				HGUI::TextRight(PString::Format("%s / %s", formatBytes(InUse).GetCStr(), formatBytes(Committed).GetCStr()));

				HGUI::TableNextColumn();
				HGUI::TextRight(PString::Format("%s / %s", formatCount(Chunk.RemoteFreePending).GetCStr(), formatCount(Chunk.RemoteFreeTotal).GetCStr()),
					(Chunk.RemoteFreePending > 0) ? ColorTextPrimary : ColorTextSecondary);
				HGUI::ItemTooltip("Frees of this thread's blocks made on other threads: waiting / total.\nWaiting ones are reclaimed on this thread's next allocation.");

				HGUI::TableNextColumn();
				if (Chunk.LargeLive > 0)
				{
					HGUI::TextRight(PString::Format("%u  (%s)", Chunk.LargeLive, formatBytes(Chunk.LargeBytes).GetCStr()));
				}
				else
				{
					HGUI::TextRight("-", ColorTextMuted);
				}
				HGUI::ItemTooltip(PString::Format("Large blocks: %u live (%s), peak %s, %s allocated so far",
					Chunk.LargeLive, formatBytes(Chunk.LargeBytes).GetCStr(), formatBytes(Chunk.LargePeakBytes).GetCStr(), formatCount(Chunk.LargeTotal).GetCStr()));
			}
			HGUI::EndTable();
		}

		HGUI::Spacing();
		HGUI::PushStyleColor(EGUIColor::Text, ColorTextSecondary);
		HGUI::LegendItem("In use", ColorInUse);
		HGUI::SameLine(16.0f);
		HGUI::LegendItem("Headers", ColorHeader);
		HGUI::SameLine(16.0f);
		HGUI::LegendItem("Free", ColorInUseTrack);
		HGUI::PopStyleColor();

		HGUI::EndPanel();
	}

	void drawClassTable(const HMemoryChunkStatInfo& InChunk)
	{
		if (HGUI::BeginTable(PString::Format("##memory_class_table_%llu", InChunk.ChunkID), 9) == false)
		{
			return;
		}

		HGUI::TableSetupColumn("Class", 64.0f, true);
		HGUI::TableSetupColumn("Usage");
		HGUI::TableSetupColumn("Live / Total", 120.0f, true);
		HGUI::TableSetupColumn("Pages", 56.0f, true);
		HGUI::TableSetupColumn("Grew", 44.0f, true);
		HGUI::TableSetupColumn("Released", 64.0f, true);
		HGUI::TableSetupColumn("Peak", 64.0f, true);
		HGUI::TableSetupColumn("Allocs", 88.0f, true);
		HGUI::TableSetupColumn("", 40.0f);
		HGUI::PushStyleColor(EGUIColor::Text, ColorTextMuted);
		HGUI::TableHeadersRow();
		HGUI::PopStyleColor();

		for (const HMemoryClassStatInfo& SizeClass : InChunk.Classes)
		{
			if (SizeClass.Pages == 0 && SizeClass.Growths == 0 && SizeClass.Releases == 0 && SizeClass.Peak == 0)
			{
				continue;
			}

			// 블록 하나짜리 페이지(32KB 이상)는 늘 100% 이고 할당마다 페이지가 느는 게 정상이라 상태를 붙이지 않는다.
			const float64 Usage             = percentOf(SizeClass.Live, SizeClass.Total);
			const bool    bSingleBlockPages = SizeClass.BlocksPerPage == 1;
			const bool    bFull             = (bSingleBlockPages == false) && Usage >= FullUsage;
			const bool    bGrew             = (bSingleBlockPages == false) && SizeClass.Growths > 0;

			HGUI::TableNextRow();
			HGUI::TableNextColumn();
			HGUI::TextRight(formatClassSize(SizeClass.BlockSize));

			// 사용률: 막대 + 오른쪽 숫자
			HGUI::TableNextColumn();
			float32 BarWidth = HGUI::GetContentRegionAvail().x - 56.0f;
			if (BarWidth < 24.0f)
			{
				BarWidth = 24.0f;
			}
			HGUI::SegmentedBar({ { static_cast<float32>(Usage / 100.0), bFull ? ColorCritical : ColorInUse } },
				HVector2(BarWidth, 6.0f), bFull ? ColorCriticalTrack : ColorInUseTrack);
			HGUI::ItemTooltip(PString::Format("%s blocks: %s of %s live (%.1f%%), peak %s\n%s (%s empty), %s per page\nGrew %s, released %s, %s so far",
				formatClassSize(SizeClass.BlockSize).GetCStr(), formatCount(SizeClass.Live).GetCStr(), formatCount(SizeClass.Total).GetCStr(), Usage,
				formatCount(SizeClass.Peak).GetCStr(), formatNoun(SizeClass.Pages, "page", "pages").GetCStr(), formatCount(SizeClass.EmptyPages).GetCStr(),
				formatNoun(SizeClass.BlocksPerPage, "block", "blocks").GetCStr(),
				formatNoun(SizeClass.Growths, "page", "pages").GetCStr(), formatCount(SizeClass.Releases).GetCStr(),
				formatNoun(SizeClass.AllocCount, "allocation", "allocations").GetCStr()));
			HGUI::SameLine(8.0f);
			HGUI::TextRight(PString::Format("%.1f%%", Usage));

			HGUI::TableNextColumn();
			HGUI::TextRight(PString::Format("%s / %s", formatCount(SizeClass.Live).GetCStr(), formatCount(SizeClass.Total).GetCStr()));

			HGUI::TableNextColumn();
			if (SizeClass.EmptyPages > 0)
			{
				HGUI::TextRight(PString::Format("%u (%u)", SizeClass.Pages, SizeClass.EmptyPages));
			}
			else
			{
				HGUI::TextRight(PString::Format("%u", SizeClass.Pages));
			}

			HGUI::TableNextColumn();
			drawCountCell(SizeClass.Growths);
			HGUI::TableNextColumn();
			drawCountCell(SizeClass.Releases);
			HGUI::TableNextColumn();
			HGUI::TextRight(formatCount(SizeClass.Peak));
			HGUI::TableNextColumn();
			HGUI::TextRight(formatCount(SizeClass.AllocCount), ColorTextSecondary);

			HGUI::TableNextColumn();
			if (bFull)
			{
				HGUI::Badge("FULL", ColorCritical, ColorTextPrimary);
			}
			else if (bGrew)
			{
				HGUI::Badge("GREW", ColorWarning, ColorInk);
			}
		}

		HGUI::EndTable();
	}

	void drawClasses(const HList<HMemoryChunkStatInfo>& InChunks, const HList<PString>& InNames)
	{
		HGUI::BeginPanel("##memory_classes", HVector2(0.0f, 0.0f), ColorSurface, ColorBorder);
		drawPanelTitle("Size classes", "blocks by size, per thread");

		if (HGUI::BeginTabBar("##memory_class_tabs"))
		{
			for (size_t Index = 0; Index < InChunks.size(); ++Index)
			{
				// ### 뒤는 탭 ID. 이름이 바뀌어도 선택이 유지된다.
				if (HGUI::BeginTabItem(PString::Format("%s###memory_chunk_%llu", InNames[Index].GetCStr(), InChunks[Index].ChunkID)))
				{
					drawClassTable(InChunks[Index]);
					HGUI::EndTabItem();
				}
			}
			HGUI::EndTabBar();
		}

		HGUI::Spacing();
		HGUI::Badge("FULL", ColorCritical, ColorTextPrimary);
		HGUI::SameLine(6.0f);
		HGUI::Text("90% or more used: the next allocation adds a page", ColorTextMuted);
		HGUI::SameLine(18.0f);
		HGUI::Badge("GREW", ColorWarning, ColorInk);
		HGUI::SameLine(6.0f);
		HGUI::Text("added pages since start", ColorTextMuted);

		HGUI::EndPanel();
	}
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

	HList<PString> ChunkNames;
	for (size_t Index = 0; Index < Chunks.size(); ++Index)
	{
		if (Chunks[Index].OwnerThreadID == MainThreadID)
		{
			ChunkNames.push_back("Main thread");
		}
		else
		{
			ChunkNames.push_back(PString::Format("Thread %d", static_cast<int32>(Index)));
		}
	}

	// 추이 표본
	const float64 Now = HGUI::GetTime();
	if (LastSampleTime < 0.0 || Now - LastSampleTime >= HistoryInterval)
	{
		LastSampleTime = Now;
		HistoryTimes.push_back(Now);
		HistoryCommittedMB.push_back(toMB(StatInfo.TotalCommittedBytes));
		HistoryAllocatedMB.push_back(toMB(StatInfo.TotalAllocatedBytes));
	}
	while (HistoryTimes.empty() == false && Now - HistoryTimes.front() > HistorySeconds + HistoryInterval)
	{
		HistoryTimes.erase(HistoryTimes.begin());
		HistoryCommittedMB.erase(HistoryCommittedMB.begin());
		HistoryAllocatedMB.erase(HistoryAllocatedMB.begin());
	}

	// 이 창만 어두운 대시보드 표면. 풍선 도움말 · 표 · 탭 색도 같은 팔레트로.
	HGUI::FillWindowBackground(ColorPage);
	HGUI::PushStyleColor(EGUIColor::PopupBackground, ColorRaised);
	HGUI::PushStyleColor(EGUIColor::Border, ColorBorder);
	HGUI::PushStyleColor(EGUIColor::TableHeaderBackground, ColorSurface);
	HGUI::PushStyleColor(EGUIColor::TableBorderStrong, ColorBorder);
	HGUI::PushStyleColor(EGUIColor::TableBorderLight, ColorGrid);
	HGUI::PushStyleColor(EGUIColor::Tab, ColorSurface);
	HGUI::PushStyleColor(EGUIColor::TabHovered, ColorRaisedHover);
	HGUI::PushStyleColor(EGUIColor::TabSelected, ColorRaised);
	HGUI::PushStyleColor(EGUIColor::TabSelectedOverline, ColorInUse);
	HGUI::PushStyleColor(EGUIColor::TabDimmed, ColorSurface);
	HGUI::PushStyleColor(EGUIColor::TabDimmedSelected, ColorRaised);
	const int32 PushedColorCount = 11;

	drawSummary(StatInfo, sumPool(StatInfo));
	HGUI::Spacing();
	drawTrend(HistoryTimes, HistoryCommittedMB, HistoryAllocatedMB, Now);
	HGUI::Spacing();
	drawThreads(Chunks, ChunkNames);
	HGUI::Spacing();
	drawClasses(Chunks, ChunkNames);

	HGUI::PopStyleColor(PushedColorCount);
}

PString JGMemoryStatistcs::GetTitleName() const
{
	return "MemoryStatistcs";
}

const HGuid& JGMemoryStatistcs::GetGUID() const
{
	return GetStaticGUID();
}
