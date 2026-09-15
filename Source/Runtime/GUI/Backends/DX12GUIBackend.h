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
	uint32 SrvStartIndex   = 1;
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