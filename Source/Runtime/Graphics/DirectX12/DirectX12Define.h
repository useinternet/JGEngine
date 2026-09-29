#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
#include "DirectX12/Utill/d3d12shader.h"
#include "DirectX12/Utill/d3dx12.h"
#include "DirectX12/Utill/dxcapi.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"dxguid.lib")

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif
#if defined(far)
#undef far
#endif

#if defined(near)
#undef near
#endif

using namespace Microsoft::WRL;

template<class T>
using HDX12ComPtr = ComPtr<T>;

using HDX12Factory			= IDXGIFactory4;
using HDX12Adapter			= IDXGIAdapter1;
using HDX12Device			= ID3D12Device5;
using HDX12SwapChain		= IDXGISwapChain4;
using HDX12CommandQueue		= ID3D12CommandQueue;
using HDX12CommandList		= ID3D12GraphicsCommandList4;
using HDX12CommandAllocator = ID3D12CommandAllocator;
using HDX12DescriptorHeap	= ID3D12DescriptorHeap;
using HDX12RootSignature	= ID3D12RootSignature;
using HDXC12Blob			= IDxcBlob;
using HDX12Blob				= ID3DBlob;
using HDX12Resource			= ID3D12Resource;
using HDX12Fence			= ID3D12Fence;
using HDX12Pipeline			= ID3D12PipelineState;
using HDX12Object			= ID3D12Object;
using HDX12StateObject		= ID3D12StateObject;

enum class ECommandListType
{
	Base,
	Graphics,
	Compute,
};

namespace HHLSL
{
	constexpr const char* VSEntry = "vs_main";
	constexpr const char* DSEntry = "ds_main";
	constexpr const char* HSEntry = "hs_main";
	constexpr const char* GSEntry = "gs_main";
	constexpr const char* PSEntry = "ps_main";
	constexpr const char* CSEntry = "cs_main";
	constexpr const char* VSTarget = "vs_5_1";
	constexpr const char* DSTarget = "ds_5_1";
	constexpr const char* HSTarget = "hs_5_1";
	constexpr const char* GSTarget = "gs_5_1";
	constexpr const char* PSTarget = "ps_5_1";
	// 컴파일러는 FXC(D3DCompile2)라 셰이더 모델 5.1까지만 된다. cs_6_0이면 컴퓨트 셰이더가 전부 컴파일에 실패했다. (5-4)
	constexpr const char* CSTarget = "cs_5_1";
	constexpr const char* RTTarget = "lib_6_3";   // DXC 전용(셰이더 모델 6.3). 지금 쓰는 곳은 없다.
}