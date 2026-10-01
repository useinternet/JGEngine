#include "PCH/PCH.h"
#include "DX12GUIBackend.h"
#include "JGGraphics.h"


#ifdef _PLATFORM_WINDOWS
#include "Imgui/imgui_impl_win32.h"
#include "Platform/JWindow.h"
#endif // _PLATFORM_WINDOWS


#ifdef _DIRECTX12
#include "DirectX12/DirectX12API.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "DirectX12/DX12FrameBuffer.h"
#include "Imgui/imgui_impl_dx12.h"
#endif // _DIRECTX12

#ifdef _DIRECTX12

namespace
{
	// 에디터 기본 글꼴(ProggyClean)에 한글을 합친다(GUI BL-3). 데이터 테이블 칸 · 설명 같은 에디터 글에 한글이 나온다.
	// 엔진 Content/Fonts/EditorKorean.ttf 가 있으면 그것, 없으면 시스템 맑은 고딕. 둘 다 없으면 영문만(경고 한 줄).
	// 한글 음절 11,172자가 아틀라스에 들어간다 — 큰 글꼴(숫자용)에는 합치지 않는다.
	void AddKoreanGlyphsToDefaultFont()
	{
		ImGuiIO& io = ImGui::GetIO();

		PString contentFont;
		HFileHelper::CombinePath(HFileHelper::EngineContentDirectory(), "Fonts/EditorKorean.ttf", &contentFont);
		const PString systemFont = "C:/Windows/Fonts/malgun.ttf";

		PString fontPath;
		if (HFileHelper::Exists(contentFont))
		{
			fontPath = contentFont;
		}
		else if (HFileHelper::Exists(systemFont))
		{
			fontPath = systemFont;
		}
		if (fontPath.Empty())
		{
			JG_LOG(GUI, ELogLevel::Warning, "Korean font not found (Content/Fonts/EditorKorean.ttf, %s). Editor text shows Latin only", systemFont);
			return;
		}

		ImFontConfig config;
		config.MergeMode   = true;
		config.PixelSnapH  = true;
		config.OversampleH = 1;
		config.OversampleV = 1;
		io.Fonts->AddFontFromFileTTF(fontPath.GetCStr(), 14.0f, &config, io.Fonts->GetGlyphRangesKorean());
	}

	PSharedPtr<PDirectX12API> findDX12API()
	{
		PSharedPtr<PJGGraphicsAPI> graphicsAPI;
		if (HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>())
		{
			graphicsAPI = graphicsModule->GetGraphicsAPI();
		}
		return Cast<PDirectX12API>(graphicsAPI);
	}
}

PDX12GUIBackend::PDX12GUIBackend() : PGUIBackend()
{

}

PDX12GUIBackend::~PDX12GUIBackend()
{

}

void PDX12GUIBackend::Initialize()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
//	io.Fonts->AddFontFromFileTTF("../../Source/Font/Consolas.ttf", 16.0f);
	// 글꼴 순서 = EGUIFont (HGUI::PushFont). 0 = 기본(ProggyClean 13px), 1 = 큰 글꼴(같은 글꼴 26px — 픽셀 글꼴이라 정수 배만 선명하다)
	io.Fonts->AddFontDefault();
	AddKoreanGlyphsToDefaultFont();
	ImFontConfig LargeFontConfig;
	LargeFontConfig.SizePixels = 26.0f;
	io.Fonts->AddFontDefault(&LargeFontConfig);
	ImGui::StyleColorsDark();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	HCoreSystemGlobalValues& globalValues = GCoreSystem::GetGlobalValues();
#ifdef _PLATFORM_WINDOWS
	globalValues.WindowCallBacks->WndProc.AddSP(SharedWrap(this), &PDX12GUIBackend::OnWndProc);
	globalValues.WindowCallBacks->OnResize.AddSP(SharedWrap(this), &PDX12GUIBackend::OnResize);

	ImGui_ImplWin32_Init((void*)globalValues.MainWindow->GetHandle());
#endif
	PSharedPtr<PJGGraphicsAPI> GraphicsAPI;
	if (HJGGraphicsModule* GraphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>())
	{
		GraphicsAPI = GraphicsModule->GetGraphicsAPI();
	}
	JG_CHECK(GraphicsAPI.IsValid());

	PSharedPtr<PDirectX12API> DX12API = Cast<PDirectX12API>(GraphicsAPI);
	JG_CHECK(DX12API.IsValid());

	SrvDescriptorHeap = HDirectX12Helper::CreateD3DDescriptorHeap(DX12API->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, MaxSrvCount);
	CommandAlloc = HDirectX12Helper::CreateD3DCommandAllocator(DX12API->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	CommandList = HDirectX12Helper::CreateD3DCommandList(DX12API->GetDevice(), CommandAlloc, D3D12_COMMAND_LIST_TYPE_DIRECT);
	CommandList->Close();

	// ImGui PSO의 렌더 타깃 포맷 = 프레임버퍼 포맷(8비트 sRGB 출력, Graphics 5-14). 멀티 뷰포트 창의 스왑체인도 이 포맷으로 만든다.
	// ImGui 색(sRGB로 정한 값)은 변환 없이 그대로 쓰이고, 반투명도 디자인 도구처럼 sRGB 공간에서 섞인다.
	const DXGI_FORMAT FrameBufferFormat = HDirectX12Helper::ConvertDXGIFormat(DX12API->GetFrameBuffer()->GetInfo().Format);
	ImGui_ImplDX12_Init(
		DX12API->GetDevice(), DX12API->GetArguments().BufferCount, FrameBufferFormat,
		SrvDescriptorHeap.Get(),
		SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// ImGui 메인 뷰포트의 정점/인덱스 버퍼는 BufferCount개를 돌려 쓴다. 엔진이 동시에 GPU에 올리는 프레임 수보다 적으면 안 된다. (5-5)
	JG_CHECK((uint32)DX12API->GetArguments().BufferCount >= DX12API->GetFramesInFlight());

	CurrentSrvIndex = SrvStartIndex;
	IncreaseSize = DX12API->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	PSharedPtr<PDX12FrameBuffer> FrameBuffer = DX12API->GetFrameBuffer();
	FrameBuffer->OnUpdate.AddSP(SharedWrap(this), &PDX12GUIBackend::OnUpdate);
	FrameBuffer->OnPresent.AddSP(SharedWrap(this), &PDX12GUIBackend::OnPresent);

	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::GraphicsBegin, PTaskDelegate::CreateSP(SharedWrap(this), &PDX12GUIBackend::NewFrame));
}

uint64 PDX12GUIBackend::GPUAllocate(TextureID textureID)
{
	return (uint64)ConvertImGuiTextureID(textureID);
}

void PDX12GUIBackend::NewFrame()
{
	// 프레임 파이프라이닝(5-5): 앞 프레임의 GUI 드로우가 GPU에서 아직 이 힙의 슬롯을 읽고 있을 수 있다.
	// 프레임 인덱스마다 다른 구간을 써서 그 프레임이 끝나기 전에는 덮지 않는다. (NewFrame은 그래픽 BeginFrame의 펜스 대기 뒤에 불린다)
	uint32 framesInFlight = 1;
	uint32 frameIndex     = 0;
	PSharedPtr<PDirectX12API> DX12API = findDX12API();
	if (DX12API.IsValid())
	{
		framesInFlight = DX12API->GetFramesInFlight();
		frameIndex     = DX12API->GetFrameIndex();
	}
	const uint32 regionSize = (MaxSrvCount - SrvStartIndex) / framesInFlight;
	SrvRegionStart  = SrvStartIndex + frameIndex * regionSize;
	SrvRegionEnd    = SrvRegionStart + regionSize;
	CurrentSrvIndex = SrvRegionStart;

	ImGui_ImplDX12_NewFrame();

#ifdef _PLATFORM_WINDOWS
	ImGui_ImplWin32_NewFrame();
#endif
	ImGui::NewFrame();

	ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	ImGuiWindowFlags   WindowFlags    = ImGuiWindowFlags_NoDocking;
	const ImGuiViewport* Viewport      = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(Viewport->WorkPos);
	ImGui::SetNextWindowSize(Viewport->WorkSize);
	ImGui::SetNextWindowViewport(Viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	WindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	WindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	WindowFlags |= ImGuiWindowFlags_NoBackground;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DEFAULT_UI_SYSTEM_LAYER", nullptr, WindowFlags);
	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2);
	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("UISystemLayer DockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), DockspaceFlags);
	}

	ImGui::End();

	GenerateMainMenuGUI();
	GenerateGUI();
}

void PDX12GUIBackend::Shutdown()
{
	// 앞선 프레임의 GUI 드로우가 GPU에서 아직 이 힙과 ImGui 리소스(폰트 텍스처, 정점/인덱스 버퍼)를 쓰고 있을 수 있다. 끝난 뒤 놓는다.
	PSharedPtr<PDirectX12API> DX12API = findDX12API();
	if (DX12API.IsValid())
	{
		DX12API->WaitForGPUIdle();
	}

	SrvDescriptorHeap.Reset(); SrvDescriptorHeap = nullptr;
	CommandList.Reset();  CommandList = nullptr;
	CommandAlloc.Reset(); CommandAlloc = nullptr;
	ImGui_ImplDX12_Shutdown();

#ifdef _PLATFORM_WINDOWS
	ImGui_ImplWin32_Shutdown();
#endif

	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	PGUIBackend::Shutdown();
}


void PDX12GUIBackend::OnUpdate(HDX12CommandList* cmdList, HDX12Resource* backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_RESOURCE_STATES& outState)
{
	if (outState != D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, outState, D3D12_RESOURCE_STATE_RENDER_TARGET);

		cmdList->ResourceBarrier(1, &resourceBarrier);
		outState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	cmdList->OMSetRenderTargets(1, &rtv, FALSE, NULL);
	cmdList->SetDescriptorHeaps(1, SrvDescriptorHeap.GetAddressOf());
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

	CurrentSrvIndex = SrvRegionStart;
}

void PDX12GUIBackend::OnPresent()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(NULL, (void*)CommandList.Get());
	}
}

void PDX12GUIBackend::OnResize(uint32 inWidth, uint32 inHeight)
{
	PSharedPtr<PJGGraphicsAPI> GraphicsAPI;
	if (HJGGraphicsModule* GraphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>())
	{
		GraphicsAPI = GraphicsModule->GetGraphicsAPI();
	}
	JG_CHECK(GraphicsAPI.IsValid());

	PSharedPtr<PDirectX12API> DX12API = Cast<PDirectX12API>(GraphicsAPI);
	JG_CHECK(DX12API.IsValid());

	HCoreSystemGlobalValues& GlobalValues = GCoreSystem::GetGlobalValues();
	HVector2Int ClientSize = GlobalValues.MainWindow->GetClientSize();

	PSharedPtr<PDX12FrameBuffer> FrameBuffer = DX12API->GetFrameBuffer();
	FrameBuffer->Resize(ClientSize.x, ClientSize.y);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void PDX12GUIBackend::OnWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

ImTextureID PDX12GUIBackend::ConvertImGuiTextureID(TextureID id)
{
	PSharedPtr<PJGGraphicsAPI> GraphicsAPI;
	if (HJGGraphicsModule* GraphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>())
	{
		GraphicsAPI = GraphicsModule->GetGraphicsAPI();
	}
	JG_CHECK(GraphicsAPI.IsValid());

	PSharedPtr<PDirectX12API> DX12API = Cast<PDirectX12API>(GraphicsAPI);
	JG_CHECK(DX12API.IsValid());

	// 이번 프레임 구간을 다 쓰면 다른 프레임의 구간(또는 힙 밖)을 덮지 않도록 마지막 슬롯을 다시 쓴다. 넘친 이미지는 잘못 보인다.
	uint32 slot = CurrentSrvIndex++;
	if (slot >= SrvRegionEnd)
	{
		if (slot == SrvRegionEnd)
		{
			JG_LOG(GUI, ELogLevel::Error, "GUI SRV slots for this frame are exhausted (%d). Increase MaxSrvCount", (int32)(SrvRegionEnd - SrvRegionStart));
		}
		slot = SrvRegionEnd - 1;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE CPU(SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPU(SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	CPU.Offset((int32)slot, (uint32)IncreaseSize);
	GPU.Offset((int32)slot, (uint32)IncreaseSize);

	DX12API->GetDevice()->CopyDescriptorsSimple(1, CPU, { id }, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return (ImTextureID)GPU.ptr;
}

#endif // _DIRECTX12

