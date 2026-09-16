#pragma once
#include "JGGraphicsCommand.h"

class PDX12ConstantBuffer;
class PRootSignature;
class PGraphicsPipelineState;
class PDX12Texture;

// 한 프레임당 요놈 한개 만 사용
class PDX12GraphicsCommand : public IJGGraphicsCommand
{
	enum ERootParam : uint32
	{
		RootParam_RenderPassCB,
		RootParam_ObjectCB,
		RootParam_MaterialCB,
		RootParam_Texture,
		RootParam_TextureCube
	};
	
	PSharedPtr<PRootSignature>		   _rootSignature;
	PSharedPtr<PGraphicsPipelineState> _graphicsPSO;
	PSharedPtr<PDX12ConstantBuffer>    _renderPassConstantBuffer;
	// SetRenderTarget에서 바인드한 렌더 타깃. EndDraw에서 셰이더 리소스 상태로 넘긴다.
	HList<PSharedPtr<PDX12Texture>>    _boundRenderTextures;
public:
	virtual ~PDX12GraphicsCommand() = default;

public:

	// Clear Resource
	virtual void BeginDraw() override;
	virtual void EndDraw() override;

	virtual void SetRenderTarget(const HRenderTarget& inRenderTarget) override;
	virtual void SetRenderPassData(const HRenderPassCBData& inData) override;
	virtual void Draw(const HDrawArguments& inArgs) override;
	virtual void Draw(const HSceneDrawArguments& inArgs) override;
	
	virtual void ClearTexture(PSharedPtr<IRawTexture> inTexture) const override;
	virtual void ClearTexture(PSharedPtr<IRawTexture> inTexture, const HLinearColor& inClearColor) const override;


private:
	virtual void BindTextures(uint32 rootParam, HList<PSharedPtr<IRawTexture>> inTextures) override;
	virtual void BindConstantBuffer(uint32 rootParam, PSharedPtr<IConstantBuffer> inConstantBuffer) override;
	virtual void BindStructuredBuffer(uint32 rootParam, PSharedPtr<IStructuredBuffer> inStructuredBuffer) override;
	virtual void BindVertexBuffer(PSharedPtr<IVertexBuffer> inVertexBuffer) override;
	virtual void BindIndexBuffer(PSharedPtr<IIndexBuffer> inIndexBuffer) override;
	virtual void BindShader(PSharedPtr<IRawGraphicsShader> inGraphicsShader) override;

private:
	void createRootSignature();

	//void SetViewport(const HViewport& viewport);
	//void SetViewports(const HList<HViewport>& viewports);
	//void SetScissorRect(const HScissorRect& rect);
	//void SetScissorRects(const HList<HScissorRect>& rects);
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

	// RootSig 바인드
	// 파이프 라인 바인드

	// 머터리얼 = 파이프라인
	// 임의로 설정한 텍스쳐나 각종 리소스를 인덱스화 시킨다.
};