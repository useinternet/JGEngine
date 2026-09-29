#pragma once
#include "GUIBackend.h"
#include "Imgui/imgui.h"
#include "Imgui/implot.h"

#ifdef _DIRECTX12
#include "DirectX12/DirectX12Define.h"

class PDX12GUIBackend : public PGUIBackend
{
	HDX12ComPtr<HDX12DescriptorHeap>   SrvDescriptorHeap;
	HDX12ComPtr<HDX12CommandAllocator> CommandAlloc;
	HDX12ComPtr<HDX12CommandList>      CommandList;

	uint32 IncreaseSize    = 0;
	uint32 CurrentSrvIndex = 0;
	uint32 SrvStartIndex   = 1;      // 0번은 ImGui 폰트 텍스처
	// 프레임 파이프라이닝(5-5): SrvStartIndex 뒤 슬롯을 프레임 인덱스마다 나눠 쓴다. 이번 프레임의 구간은 [SrvRegionStart, SrvRegionEnd).
	uint32 SrvRegionStart  = 1;
	uint32 SrvRegionEnd    = 1024;
	const uint32 MaxSrvCount = 1024;

public:
	PDX12GUIBackend();
	virtual ~PDX12GUIBackend();
	
	virtual uint64 GPUAllocate(TextureID textureID) override;
public:
	void NewFrame();
	virtual void Initialize() override;
	virtual void Shutdown() override;

private:
	void OnUpdate(HDX12CommandList* cmdList, HDX12Resource* backBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_RESOURCE_STATES& outState);
	void OnPresent();
	void OnResize(uint32 inWidth, uint32 inHeight);

#ifdef _PLATFORM_WINDOWS
	void OnWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // _PLATFORM_WINDOWS

	ImTextureID ConvertImGuiTextureID(TextureID id);
};

#endif // _DIRECTX12