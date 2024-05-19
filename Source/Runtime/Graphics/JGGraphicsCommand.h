#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class IRawTexture;
class IConstantBuffer;
class IVertexBuffer;
class IIndexBuffer;
class IStructuredBuffer;
class IMesh;

class IJGGraphicsCommand : public IMemoryObject
{
public:
	virtual ~IJGGraphicsCommand() = default;


public:
	//
	virtual void BeginDraw() = 0;
	virtual void EndDraw() = 0;

	// Render Setting
	virtual void SetRenderTarget(const HRenderTarget& inRenderTarget) = 0;
	virtual void SetRenderPassData(const HRenderPassData& inData) = 0;
	virtual void Draw(const HDrawArguments& inArgs) = 0;
	virtual void Draw(const HSceneDrawArguments& inArgs) = 0;
	// Util
	virtual void ClearTexture(PSharedPtr<IRawTexture> InTexture) const = 0;
	virtual void ClearTexture(PSharedPtr<IRawTexture> InTexture, const HLinearColor& InClearColor) const = 0;

	//void BindMesh(PSharedPtr<PStaticMesh> inMesh);

	// Bind Data
protected:
	//virtual void BindTextures(uint32 rootParam, HList<PSharedPtr<ITexture>> inTextures) = 0;
	//virtual void BindConstantBuffer(uint32 rootParam, PSharedPtr<IConstantBuffer> inConstantBuffer) = 0;
	//virtual void BindStructuredBuffer(uint32 rootParam, PSharedPtr<IStructuredBuffer> inStructuredBuffer) = 0;
	//virtual void BindVertexBuffer(PSharedPtr<IVertexBuffer> inVertexBuffer) = 0;
	//virtual void BindIndexBuffer(PSharedPtr<IIndexBuffer> inIndexBuffer) = 0;

	//

//void BindConstantBuffer(uint32 rootParam, PUploadAllocator::HAllocation alloc);
//void BindConstantBuffer(uint32 rootParam, const void* data, uint64 dataSize);
//void BindConstantBuffer(uint32 rootParam, D3D12_GPU_VIRTUAL_ADDRESS gpu, ID3D12Resource* backUpResource = nullptr);
//void BindStructuredBuffer(uint32 rootParam, PUploadAllocator::HAllocation alloc);
//void BindStructuredBuffer(uint32 rootParam, const void* data, uint64 elementCount, uint64 elementSize);
//void BindStructuredBuffer(uint32 rootParam, D3D12_GPU_VIRTUAL_ADDRESS gpu, ID3D12Resource* backUpResource = nullptr);
//void BindConstants(uint32 rootparam, uint32 btSize, const void* data, uint32 offset = 0);
//void BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view, bool bFlush = true);


	//void ClearRenderTargetTexture(HDX12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, const HColor& clearColor);
	//void ClearDepthTexture(HDX12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
	//	float32 clearDepth = 1.0f, uint8 clearStencil = 0, D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);
	//void ClearUAVUint(D3D12_CPU_DESCRIPTOR_HANDLE handle, HDX12Resource* resource);
	//void ClearUAVFloat(D3D12_CPU_DESCRIPTOR_HANDLE handle, HDX12Resource* resource);
	//void SetRenderTarget(
	//	HDX12Resource** rtTextures, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, uint64 rtTextureCount,
	//	HDX12Resource* depthTexture, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);

	//void BindRootSignature(PSharedPtr<PRootSignature> rootSig);
	//void BindPipelineState(PSharedPtr<PGraphicsPipelineState> pso);

	//void BindTextures(uint32 rootParam, HList<D3D12_CPU_DESCRIPTOR_HANDLE> handles);
	//void BindConstantBuffer(uint32 rootParam, PUploadAllocator::HAllocation alloc);
	//void BindConstantBuffer(uint32 rootParam, const void* data, uint64 dataSize);
	//void BindConstantBuffer(uint32 rootParam, D3D12_GPU_VIRTUAL_ADDRESS gpu, ID3D12Resource* backUpResource = nullptr);
	//void BindStructuredBuffer(uint32 rootParam, PUploadAllocator::HAllocation alloc);
	//void BindStructuredBuffer(uint32 rootParam, const void* data, uint64 elementCount, uint64 elementSize);
	//void BindStructuredBuffer(uint32 rootParam, D3D12_GPU_VIRTUAL_ADDRESS gpu, ID3D12Resource* backUpResource = nullptr);
	//void BindConstants(uint32 rootparam, uint32 btSize, const void* data, uint32 offset = 0);
	//void BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view, bool bFlush = true);

	// Bind
};