#pragma once
#include "JGGraphicsAPI.h"
#include "DirectX12Define.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "DirectX12/Classes/ResourceStateTracker.h"


class PCommandQueue;
class PResourceStagingManager;
class PDescriptionAllocator;
class PGraphicsCommandList;
class PComputeCommandList;
class PCommandList;
class PDX12FrameBuffer;
class HDescriptionAllocation;

class GRAPHICS_API PDirectX12API : public PJGGraphicsAPI
{
	HDX12ComPtr<HDX12Factory> _dx12Factory;
	HDX12ComPtr<HDX12Device>  _dx12Device;

	PSharedPtr<PCommandQueue> _commandQueue;
	// GPU <-> CPU 전송(업로드/리드백). 큐 바로 뒤에 만들고 Destroy에서 큐 Flush 뒤 해제한다.
	PSharedPtr<PResourceStagingManager> _resourceStagingManager;
	PSharedPtr<PDescriptionAllocator> _csuAllocator;
	PSharedPtr<PDescriptionAllocator> _rtvAllocator;
	PSharedPtr<PDescriptionAllocator> _dsvAllocator;

	HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>> _rootSignatureCache;
	HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>> _graphicsPSOCache;
	HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>> _computePSOCache;
	HHashMap<HDX12Resource*, HResourceInfo> _resourceRefCache;

	HMutex _deviceMutex;

	PSharedPtr<PDX12FrameBuffer> _frameBuffer;

	HJGGraphicsArguments _arguments;
	bool _bIsSupportedRayTracing;
	
	void createDefaultResources();
#ifdef _DEBUG
	// 디버거 없이 실행할 때도 디버그 레이어 메시지를 볼 수 있게 InfoQueue를 비워 로그로 옮긴다. (EndFrame마다)
	void flushDebugLayerMessages();
#endif
public:
	// = default 가 아니다. 파괴될 때 HDirectXAPI 캐시를 비워야 한다. (DirectX12API.cpp)
	virtual ~PDirectX12API();

protected:
	virtual void Initialize(const HJGGraphicsArguments& args) override;
	virtual void Destroy() override;
	virtual void BeginFrame() override;
	virtual void EndFrame() override;
public:
	virtual void SubmitFinalTexture(PSharedPtr<IRawTexture> inTexture) override;
public:
	virtual PSharedPtr<IJGGraphicsCommand> GetGraphicsCommand() const override;

	virtual PSharedPtr<IRawTexture> CreateRawTexture(const HTextureInfo& textureInfo) override;
	virtual PSharedPtr<IRawTexture> CreateRawTexture(const uint8* pixels, const HTextureInfo& textureInfo) override;
	virtual PSharedPtr<IVertexBuffer> CreateVertexBuffer(const HVertexBufferConstructArguments& inArgs) override;
	virtual PSharedPtr<IIndexBuffer>  CreateIndexBuffer(const HIndexBufferConstructArguments& inArgs) override;
	virtual PSharedPtr<IRawMaterial> CreateRawMaterial(const HRawMaterialConstructArguments& inArgs) override;

	virtual bool RequestTextureReadback(PSharedPtr<IRawTexture> inTexture, const HOnTextureReadbackComplete& inOnComplete) override;
	virtual bool ReadbackTextureImmediate(PSharedPtr<IRawTexture> inTexture, HTexturePixels& outPixels) override;


public:
	HDX12Device*  GetDevice() const { return _dx12Device.Get(); }
	HDX12Factory* GetFactory() const { return _dx12Factory.Get(); }

	const HJGGraphicsArguments& GetArguments() const;

	HDX12ComPtr<HDX12Resource> CreateCommittedResource(
		const PString& name,
		const D3D12_HEAP_PROPERTIES* pHeapProperties,
		D3D12_HEAP_FLAGS heapFlags,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES initialResourceState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue);

	void DestroyCommittedResource(HDX12ComPtr<HDX12Resource> resource);

	const HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& GetRootSignatureCache() const;
	HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& GetRootSignatureCacheRef();

	const HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& GetGraphicsPSOCache() const;
	HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& GetGraphicsPSOCacheRef();

	const HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& GetComputePSOCache() const;
	HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& GetComputePSOCacheRef();

	const HHashMap<HDX12Resource*, HResourceInfo>& GetResourceRefCache() const;
	HHashMap<HDX12Resource*, HResourceInfo>& GetResourceRefCacheRef();

	PSharedPtr<PGraphicsCommandList> RequestGraphicsCommandList();
	PSharedPtr<PComputeCommandList>  RequestComputeCommandList();
	PSharedPtr<PCommandList> RequestCommandList();

	HDescriptionAllocation RTVAllocate();
	HDescriptionAllocation DSVAllocate();
	HDescriptionAllocation CSUAllocate();

	PSharedPtr<PCommandQueue> GetCommandQueue() const;
	PSharedPtr<PDX12FrameBuffer> GetFrameBuffer() const;
	PSharedPtr<PResourceStagingManager> GetResourceStagingManager() const;

	// 프레임 파이프라이닝(5-5): 지금 기록 중인 프레임의 인덱스(0 ~ GetFramesInFlight()-1)와 동시에 GPU에 올라갈 수 있는 프레임 수.
	// 프레임마다 CPU가 새로 쓰는 GPU 가시 데이터(예: GUI의 SRV 슬롯)는 이 인덱스로 나눠 써야 앞 프레임이 읽는 중인 데이터를 덮지 않는다.
	uint32 GetFrameIndex() const;
	uint32 GetFramesInFlight() const;
	// 큐에 들어간 GPU 작업이 모두 끝날 때까지 기다린다. GPU가 쓰는 리소스를 이 API 밖에서 직접 해제하기 전에 부른다(예: GUI 종료).
	void WaitForGPUIdle();
};


class HDirectXAPI
{
public:
	static HDX12Device*  GetDevice();
	static HDX12Factory* GetFactory();
	static const HJGGraphicsArguments& GetArguments();

	static 	HDX12ComPtr<HDX12Resource> CreateCommittedResource(
		const PString& name,
		const D3D12_HEAP_PROPERTIES* pHeapProperties,
		D3D12_HEAP_FLAGS heapFlags,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES initialResourceState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue);

	static void DestroyCommittedResource(HDX12ComPtr<HDX12Resource> resource);

	static const HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& GetRootSignatureCache();
	static HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& GetRootSignatureCacheRef();
	static const HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& GetGraphicsPSOCache();
	static HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& GetGraphicsPSOCacheRef();
	static const HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& GetComputePSOCache();
	static HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& GetComputePSOCacheRef();
	static const HHashMap<HDX12Resource*, HResourceInfo>& GetResourceRefCache();
	static HHashMap<HDX12Resource*, HResourceInfo>& GetResourceRefCacheRef();
	
	static PSharedPtr<PGraphicsCommandList> RequestGraphicsCommandList();
	static PSharedPtr<PComputeCommandList>  RequestComputeCommandList();
	static PSharedPtr<PCommandList>  RequestCommandList();
	static HDescriptionAllocation RTVAllocate();
	static HDescriptionAllocation DSVAllocate();
	static HDescriptionAllocation CSUAllocate();
	static PSharedPtr<PCommandQueue> GetCommandQueue();
	static PSharedPtr<PDX12FrameBuffer> GetFrameBuffer();
	// GPU <-> CPU 스테이징 관리자. 버퍼/텍스처가 업로드를 요청할 때 쓴다. API가 없으면 nullptr.
	static PSharedPtr<PResourceStagingManager> GetResourceStagingManager();
	// API가 소유한 기본 텍스처(1x1 흰색). 머터리얼의 빈 Texture 슬롯 대체값. API가 없으면 nullptr.
	static PSharedPtr<IRawTexture> GetDefaultTexture();
private:
	// 캐시 수명은 PDirectX12API 자신만 조작한다. (resetCache / invalidateCache)
	friend class PDirectX12API;

	static PDirectX12API* getDX12API();

	// PDirectX12API::Initialize 에서 호출. 캐시를 비우고 조회를 다시 허용한다.
	static void resetCache();

	// PDirectX12API::Destroy 와 소멸자에서 호출. 캐시를 버리고 이후 조회를 전부 막는다.
	// PDirectX12API 는 GC(GMemoryGlobalSystem) 관리 객체라 모듈 Shutdown 직후
	// GC Flush 에서 파괴된다. 그때까지 캐시가 남아 있으면 파괴된 객체에 접근하게 된다.
	// owner 를 주면 캐시가 그 인스턴스를 가리킬 때만 비운다.
	// (교체된 새 인스턴스의 캐시를 죽는 인스턴스가 지우는 것을 막는다)
	static void invalidateCache(const PDirectX12API* owner = nullptr);
};