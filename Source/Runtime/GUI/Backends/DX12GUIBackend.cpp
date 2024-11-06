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
	ImGuiIO& io = ImGui::GetIO(); (void)io;
//	io.Fonts->AddFontFromFileTTF("../../Source/Font/Consolas.ttf", 16.0f);
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

	ImGui_ImplDX12_Init(
		DX12API->GetDevice(), DX12API->GetArguments().BufferCount, DXGI_FORMAT_R16G16B16A16_FLOAT,
		SrvDescriptorHeap.Get(),
		SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

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
	CurrentSrvIndex = SrvStartIndex;
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
	SrvDescriptorHeap.Reset(); SrvDescriptorHeap = nullptr;
	CommandList.Reset();  CommandList = nullptr;
	CommandAlloc.Reset(); CommandAlloc = nullptr;
	ImGui_ImplDX12_Shutdown();

#ifdef _PLATFORM_WINDOWS
	ImGui_ImplWin32_Shutdown();
#endif
	ImGui::DestroyContext();
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

	CurrentSrvIndex = SrvStartIndex;
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE CPU(SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPU(SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	CPU.Offset((int32)CurrentSrvIndex, (uint32)IncreaseSize);
	GPU.Offset((int32)CurrentSrvIndex, (uint32)IncreaseSize);
	CurrentSrvIndex++;

	DX12API->GetDevice()->CopyDescriptorsSimple(1, CPU, { id }, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return (ImTextureID)GPU.ptr;
}

#endif // _DIRECTX12

