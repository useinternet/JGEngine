#pragma once
#include "Core.h"
#include "DirectX12Helper.h"
#include "UploadAllocator.h"

class PResourceStateTracker;
class PDynamicDescriptionAllocator;
class PGraphicsPipelineState;
class PComputePipelineState;
class PComputeCommandList;
class PRootSignature;

class PCommandList : public IMemoryObject
{
protected:
	HDX12ComPtr<HDX12Pipeline>		 _bindedPipelineState   = nullptr;
	HDX12ComPtr<HDX12RootSignature>  _bindedGraphicsRootSig = nullptr;
	HDX12ComPtr<HDX12RootSignature>  _bindedComputeRootSig  = nullptr;
	HDX12ComPtr<HDX12DescriptorHeap> _bindedDescriptorHeap  = nullptr;

	D3D12_COMMAND_LIST_TYPE				_commandListType;            // 공용
	HDX12ComPtr<HDX12CommandList>		_dx12CommandList;     // 공용
	HDX12ComPtr<HDX12CommandAllocator>  _dx12Allocator;       // 공용

	HList<HDX12ComPtr<HDX12Object>>     _tempObjectList;       // 공용
	PSharedPtr<PResourceStateTracker> _resourceStateTracker; // 공용
	PSharedPtr<PUploadAllocator>      _uploadAllocator;      // 공용
	PSharedPtr<PDynamicDescriptionAllocator> _dynamicDescriptionAllocator; // 따로

public:
	PCommandList() = default;
	PCommandList(D3D12_COMMAND_LIST_TYPE d3dType);
	virtual ~PCommandList() = default;
public:
	HDX12CommandList* Get() const { return _dx12CommandList.Get(); }

	void BackupResource(HDX12Object* d3dObj);
	virtual void Reset();
	virtual void Close();
	virtual bool Close(PCommandList* commandList);

	void TransitionBarrier(HDX12Resource* d3dResource, D3D12_RESOURCE_STATES state, uint32 subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool bFlush = false);
	void UAVBarrier(HDX12Resource* d3dResource, bool bFlush = false);
	void AliasBarrier(HDX12Resource* beforeResource, HDX12Resource* afterResource, bool bFlush = false);
	D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void FlushResourceBarrier();

	void CopyResource(HDX12Resource* dest, HDX12Resource* src);
	void CopyTextrueFromMemory(HDX12Resource* resource, const void* pixels, int32 width, int32 height, int32 channels, uint64 arraySize);
	void CopyBuffer(HDX12Resource* buffer, const void* data, uint64 elementSize, uint64 elementCount);
	void CopyBufferRegion(HDX12Resource* dest, uint64 destOffset, HDX12Resource* src, uint64 srcOffset, uint64 byteSize);
	void CopyTextureRegion(HDX12Resource* dest, HDX12Resource* src, const HRect& srcBox, D3D12_RESOURCE_STATES inDestState, D3D12_RESOURCE_STATES inSrcState);

	// -- 전송 관리자용 (PTransferManager) --
	// 스테이징(UPLOAD 힙) -> 버퍼 복사 뒤 finalState로 전이한다. 스테이징은 UPLOAD 힙이라 배리어를 걸지 않는다.
	// 복사와 같은 제출 안에서 바로 쓸 수 있도록 마지막 전이를 명시한다. (버퍼는 제출이 끝나면 COMMON으로 자연 감쇠하지만 같은 제출 안에서는 아니다)
	void UploadBuffer(HDX12Resource* dest, HDX12Resource* stagingBuffer, uint64 byteSize, D3D12_RESOURCE_STATES finalState);
	// 스테이징(UPLOAD 힙, 풋프린트 배치) -> 텍스처 서브리소스 복사 뒤 finalState로 전이한다.
	void UploadTexture(HDX12Resource* dest, HDX12Resource* stagingBuffer, const HList<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>& footprints, const HList<uint32>& subresources, D3D12_RESOURCE_STATES finalState);
	// 텍스처 서브리소스 -> 스테이징(READBACK 힙, 풋프린트 배치) 복사. 소스는 COPY_SOURCE로 전이하고,
	// restoreState가 주어지면 복사 뒤 그 상태로 되돌린다. (즉시 경로: 열려 있는 프레임 리스트의 상태 계산을 어긋나게 하지 않기 위해)
	void ReadbackTexture(HDX12Resource* stagingBuffer, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint, HDX12Resource* src, uint32 subresource, const D3D12_RESOURCE_STATES* restoreState);
};

class PGraphicsCommandList : public PCommandList
{
private:
	HList<D3D12_VERTEX_BUFFER_VIEW> _vertexViews;
	//SharedPtr<DynamicDescriptorAllocator> mComputeDynamicDescriptorAllocator;
public:
	PGraphicsCommandList(D3D12_COMMAND_LIST_TYPE d3dType);
	virtual ~PGraphicsCommandList() = default;
public:
	virtual void Reset() override;
public:
	void SetViewport(const HViewport& viewport);
	void SetViewports(const HList<HViewport>& viewports);
	void SetScissorRect(const HScissorRect& rect);
	void SetScissorRects(const HList<HScissorRect>& rects);
	void ClearRenderTargetTexture(HDX12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, const HLinearColor& clearColor);
	void ClearDepthTexture(HDX12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
		float32 clearDepth = 1.0f, uint8 clearStencil = 0, D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);
	void ClearUAVUint(D3D12_CPU_DESCRIPTOR_HANDLE handle, HDX12Resource* resource);
	void ClearUAVFloat(D3D12_CPU_DESCRIPTOR_HANDLE handle, HDX12Resource* resource);
	void SetRenderTarget(
		HDX12Resource** rtTextures, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, uint64 rtTextureCount,
		HDX12Resource* depthTexture, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);

	void BindRootSignature(PSharedPtr<PRootSignature> rootSig);
	void BindPipelineState(PSharedPtr<PGraphicsPipelineState> pso);

	void BindTextures(uint32 rootParam, HList<D3D12_CPU_DESCRIPTOR_HANDLE> handles);
	void BindConstantBuffer(uint32 rootParam, PUploadAllocator::HAllocation alloc);
	void BindConstantBuffer(uint32 rootParam, const void* data, uint64 dataSize);
	void BindConstantBuffer(uint32 rootParam, D3D12_GPU_VIRTUAL_ADDRESS gpu, ID3D12Resource* backUpResource = nullptr);
	void BindStructuredBuffer(uint32 rootParam, PUploadAllocator::HAllocation alloc);
	void BindStructuredBuffer(uint32 rootParam, const void* data, uint64 elementCount, uint64 elementSize);
	void BindStructuredBuffer(uint32 rootParam, D3D12_GPU_VIRTUAL_ADDRESS gpu, ID3D12Resource* backUpResource = nullptr);
	void BindConstants(uint32 rootparam, uint32 btSize, const void* data, uint32 offset = 0);
	void BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view, bool bFlush = true);
	void FlushVertexBuffer();

	void BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& view);
	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);

	void DrawIndexed(uint32 indexCount, uint32 instancedCount = 1, uint32 startIndexLocation = 0, uint32 startVertexLocation = 0, uint32 startInstanceLocation = 0);
	void Draw(uint32 vertexPerInstance, uint32 instanceCount = 1, uint32 startVertexLocation = 0, uint32 startInstanceLocation = 0);

	// PSharedPtr<PComputeCommandList> QueryInterfaceAsComputeCommandList();
};

class PComputeCommandList : public PCommandList
{
	friend PGraphicsCommandList;
public:
	PComputeCommandList() : PCommandList() {}
	PComputeCommandList(D3D12_COMMAND_LIST_TYPE d3dType) : PCommandList(d3dType) {};
	virtual ~PComputeCommandList() = default;
public:
	void ClearUAVUint(D3D12_CPU_DESCRIPTOR_HANDLE handle, HDX12Resource* resource);
	void ClearUAVFloat(D3D12_CPU_DESCRIPTOR_HANDLE handle, HDX12Resource* resource);

	void BindRootSignature(PSharedPtr<PRootSignature> rootSig);
	void BindPipelineState(PSharedPtr<PComputePipelineState> pso);
	void BindPipelineState(HDX12StateObject* pso);

	void BindTextures(uint32 rootParam, HList<D3D12_CPU_DESCRIPTOR_HANDLE> handles);
	void BindConstantBuffer(uint32 rootParam, PUploadAllocator::HAllocation alloc);
	void BindConstantBuffer(uint32 rootParam, const void* data, uint64 dataSize);
	void BindConstantBuffer(uint32 rootParam, D3D12_GPU_VIRTUAL_ADDRESS gpu, HDX12Resource* backUpResource);
	void BindStructuredBuffer(uint32 rootParam, PUploadAllocator::HAllocation alloc);
	void BindStructuredBuffer(uint32 rootParam, const void* data, uint64 elementCount, uint64 elementSize);
	void BindStructuredBuffer(uint32 rootParam, D3D12_GPU_VIRTUAL_ADDRESS gpu, HDX12Resource* backUpResource);
	void BindStructuredBuffer(uint32 rootParam, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void BindConstants(uint32 rootparam, uint32 btSize, const void* data, uint32 offset = 0);
	void Dispatch(uint32 groupX, uint32 groupY, uint32 groupZ);
	void DispatchRays(const D3D12_DISPATCH_RAYS_DESC& desc);

};