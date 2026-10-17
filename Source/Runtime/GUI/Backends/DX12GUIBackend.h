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

	// 화면 배율(D-22 ②): 글꼴 · 스타일은 DpiScale(PGUIBackend) 로 만든다. 주 창이 배율이 다른 모니터로 옮겨 가면(WM_DPICHANGED)
	// 새 배율을 PendingDpiScale 에 두고 다음 NewFrame(프레임 밖)에서 글꼴 아틀라스 · 스타일을 다시 만든다.
	float32    PendingDpiScale = 0.0f;   // 0 이면 바뀐 것 없음
	ImGuiStyle BaseStyle;                // 배율을 곱하기 전 스타일

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
	// 글꼴(아틀라스)과 스타일을 Scale 로 다시 만든다. 프레임 밖에서만 부른다 — 아틀라스 텍스처는 다음 ImGui_ImplDX12_NewFrame 이 올린다.
	void ApplyDpiScale(float32 Scale);

#ifdef _PLATFORM_WINDOWS
	void OnWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // _PLATFORM_WINDOWS

	ImTextureID ConvertImGuiTextureID(TextureID id);
};

#endif // _DIRECTX12