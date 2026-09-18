#include "PCH/PCH.h"
#include "Misc/Module.h"
#include "JGGraphics.h"
#include "DirectX12API.h"
#include <d3d12sdklayers.h>
#include "Classes/CommandQueue.h"
#include "Classes/ResourceStateTracker.h"
#include "Classes/DescriptionAllocator.h"
#include "Classes/CommandList.h"
#include "DirectX12/DX12FrameBuffer.h"
#include "DirectX12/DX12Texture.h"
#include "DirectX12/DX12GraphicsCommand.h"
#include "DirectX12/DX12VertexBuffer.h"
#include "DirectX12/DX12IndexBuffer.h"
#include "DirectX12/DX12Material.h"

PDirectX12API::~PDirectX12API()
{
	// Destroy() 를 거치지 않고 파괴되는 경로가 생기더라도 캐시는 반드시 끊는다.
	HDirectXAPI::invalidateCache(this);
}

void PDirectX12API::Initialize(const HJGGraphicsArguments& args)
{
	// 이전 인스턴스가 남긴 차단 상태를 푼다. (모듈 재연결 대응)
	HDirectXAPI::resetCache();

	JG_LOG(Graphics, ELogLevel::Trace, "DirectX12 Init Start");
	_arguments   = args;
	_dx12Factory = HDirectX12Helper::CreateDXGIFactory();

	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer(); // 꼭 device 만들기 전에 호출!
	}

	DXGI_ADAPTER_DESC1 adapterDesc = {};
	_bIsSupportedRayTracing = false;
	_dx12Device = HDirectX12Helper::CreateD3DDevice(_dx12Factory, false, &adapterDesc, &_bIsSupportedRayTracing);

	PString adapterDescription;
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		HRawString str = converterX.to_bytes(adapterDesc.Description);
		adapterDescription = str.c_str();
	}

	if (_dx12Device)
	{
		JG_LOG(Graphics, ELogLevel::Trace, "Success Create D3D12Device");
		JG_LOG(Graphics, ELogLevel::Info, "Description : %s", adapterDescription);
		JG_LOG(Graphics, ELogLevel::Info, "VideoMemory : %d  MB", adapterDesc.DedicatedVideoMemory / 1024 / 1024);
	}
	else
	{
		JG_LOG(Graphics, ELogLevel::Critical, "Failed Create D3D12Device");
	}

	JG_LOG(Graphics, ELogLevel::Trace, "Create DescriptorAllocator...");
	_csuAllocator = Allocate<PDescriptionAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_rtvAllocator = Allocate<PDescriptionAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_dsvAllocator = Allocate<PDescriptionAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	JG_LOG(Graphics, ELogLevel::Trace, "Create CommandQueue...");
	_commandQueue = Allocate<PCommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);

	JG_LOG(Graphics, ELogLevel::Trace, "Create FrameBuffer...");
	HFrameBufferInfo frameBufferInfo;
	frameBufferInfo.Handle = args.Handle;
	frameBufferInfo.ClearColor = args.ClearColor;
	frameBufferInfo.FrameBufferCount = args.BufferCount;
	frameBufferInfo.Width  = args.Width;
	frameBufferInfo.Height = args.Height;
	frameBufferInfo.Format = ETextureFormat::R16G16B16A16_Float;
	_frameBuffer = Allocate<PDX12FrameBuffer>();
	_frameBuffer->Initialize(frameBufferInfo);

	JG_LOG(Graphics, ELogLevel::Trace, "Create Default Resources...");
	createDefaultResources();

	JG_LOG(Graphics, ELogLevel::Trace, "DirectX12 Init End");
}

void PDirectX12API::Destroy()
{
	_commandQueue->Flush();
	_defaultMaterial = nullptr;
	_defaultTexture  = nullptr;
	_frameBuffer = nullptr;
	_csuAllocator = nullptr;
	_rtvAllocator = nullptr;
	_dsvAllocator = nullptr;

	_rootSignatureCache.clear();
	_graphicsPSOCache.clear();
	_computePSOCache.clear();
	_resourceRefCache.clear();

	// 반드시 마지막에. 이 시점부터 HDirectXAPI 의 모든 조회가 nullptr 로 떨어져서,
	// 뒤늦게 GC 되는 텍스처/버퍼의 소멸자가 이 객체를 건드리지 않는다.
	// (위 정리 작업들은 아직 this 를 써야 하므로 앞에서 부르면 안 된다.)
	HDirectXAPI::invalidateCache(this);
}

void PDirectX12API::createDefaultResources()
{
	// 1x1 흰색 텍스처. 텍스처 프로퍼티가 비어 있을 때의 대체값.
	{
		HTextureInfo texInfo;
		texInfo.Name       = "DefaultTexture";
		texInfo.Width      = 1;
		texInfo.Height     = 1;
		texInfo.Format     = ETextureFormat::R8G8B8A8_Unorm;
		texInfo.Flags      = ETextureFlags::None;
		texInfo.MipLevel   = 1;
		texInfo.ArraySize  = 1;
		texInfo.FilterMode = ETextureFilterMode::Point;
		texInfo.WrapMode   = ETextureWrapMode::Clamp;

		const uint8 whitePixel[4] = { 255, 255, 255, 255 };
		_defaultTexture = CreateRawTexture(whitePixel, texInfo);
		if (_defaultTexture.IsValid() == false || _defaultTexture->IsValid() == false)
		{
			JG_LOG(Graphics, ELogLevel::Error, "Fail Create Default Texture");
			_defaultTexture = nullptr;
		}
	}

	// 프로퍼티 없는 Surface 머터리얼. 템플릿의 기본 출력(알베도 흰색)을 그대로 쓴다.
	{
		HRawMaterialConstructArguments materialArgs;
		materialArgs.Name   = PName("DefaultMaterial");
		materialArgs.Domain = EMaterialDomain::Surface;

		PSharedPtr<IRawMaterial> material = CreateRawMaterial(materialArgs);

		HMaterialCompileArguments compileArgs;
		compileArgs.ShaderCode = "";
		if (material.IsValid() && material->Compile(compileArgs))
		{
			_defaultMaterial = material;
		}
		else
		{
			JG_LOG(Graphics, ELogLevel::Error, "Fail Create Default Material (shader compile failed)");
		}
	}
}

void PDirectX12API::BeginFrame()
{
	_commandQueue->Begin();
}

void PDirectX12API::EndFrame()
{
	_frameBuffer->Update();
	_commandQueue->End();
	_frameBuffer->Present();
	_csuAllocator->UpdatePage();
	_rtvAllocator->UpdatePage();
	_dsvAllocator->UpdatePage();

#ifdef _DEBUG
	flushDebugLayerMessages();
#endif
}

#ifdef _DEBUG
void PDirectX12API::flushDebugLayerMessages()
{
	HDX12ComPtr<ID3D12InfoQueue> infoQueue;
	if (_dx12Device == nullptr || FAILED(_dx12Device.As(&infoQueue)))
	{
		return;
	}

	const uint64 messageCount = infoQueue->GetNumStoredMessages();
	for (uint64 i = 0; i < messageCount; ++i)
	{
		SIZE_T messageLength = 0;
		if (FAILED(infoQueue->GetMessage(i, nullptr, &messageLength)) || messageLength == 0)
		{
			continue;
		}

		HList<uint8> buffer(messageLength);
		D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(buffer.data());
		if (FAILED(infoQueue->GetMessage(i, message, &messageLength)))
		{
			continue;
		}

		ELogLevel level = ELogLevel::Info;
		switch (message->Severity)
		{
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		case D3D12_MESSAGE_SEVERITY_ERROR:   level = ELogLevel::Error;   break;
		case D3D12_MESSAGE_SEVERITY_WARNING: level = ELogLevel::Warning; break;
		default:                             level = ELogLevel::Info;    break;
		}

		JG_LOG(Graphics, level, "D3D12 DebugLayer [%d] %s", (int32)message->ID, PString(message->pDescription));
	}

	infoQueue->ClearStoredMessages();
}
#endif

void PDirectX12API::SubmitFinalTexture(PSharedPtr<IRawTexture> inTexture)
{
	GetFrameBuffer()->SubmitTexture(inTexture);
}

PSharedPtr<IJGGraphicsCommand> PDirectX12API::GetGraphicsCommand() const
{
	PSharedPtr<PDX12GraphicsCommand> command = Allocate<PDX12GraphicsCommand>();
	return command;
}

PSharedPtr<IRawTexture> PDirectX12API::CreateRawTexture(const HTextureInfo& textureInfo)
{
	PSharedPtr<PDX12Texture> texture = Allocate<PDX12Texture>();
	texture->Initialize(textureInfo);

	return texture;
}

PSharedPtr<IRawTexture> PDirectX12API::CreateRawTexture(const uint8* pixels, const HTextureInfo& textureInfo)
{
	PSharedPtr<PDX12Texture> texture = Allocate<PDX12Texture>();
	texture->InitializeByMemory(pixels, textureInfo);
	
	return texture;
}

PSharedPtr<IVertexBuffer> PDirectX12API::CreateVertexBuffer(const HVertexBufferConstructArguments& inArgs)
{
	PSharedPtr<PDX12VertexBuffer> vertexBuffer = Allocate<PDX12VertexBuffer>();
	vertexBuffer->SetName(inArgs.Name);

	return vertexBuffer;
}

PSharedPtr<IIndexBuffer>  PDirectX12API::CreateIndexBuffer(const HIndexBufferConstructArguments& inArgs)
{
	PSharedPtr<PDX12IndexBuffer> indexBuffer = Allocate<PDX12IndexBuffer>();
	indexBuffer->SetName(inArgs.Name);

	return indexBuffer;
}

PSharedPtr<IRawMaterial> PDirectX12API::CreateRawMaterial(const HRawMaterialConstructArguments& inArgs)
{
	PSharedPtr<PDX12Material> material = Allocate<PDX12Material>();
	material->Initialize(inArgs);

	return material;
}

const HJGGraphicsArguments& PDirectX12API::GetArguments() const
{
	return _arguments;
}

HDX12ComPtr<HDX12Resource> PDirectX12API::CreateCommittedResource(
	const PString& name,
	const D3D12_HEAP_PROPERTIES* pHeapProperties,
	D3D12_HEAP_FLAGS heapFlags,
	const D3D12_RESOURCE_DESC* pDesc,
	D3D12_RESOURCE_STATES initialResourceState,
	const D3D12_CLEAR_VALUE* pOptimizedClearValue)
{
	HDX12ComPtr<HDX12Resource> resultResource;
	HRESULT hResult = S_OK;
	{
		HLockGuard<HMutex> lock(_deviceMutex);
		hResult = GetDevice()->CreateCommittedResource(pHeapProperties, heapFlags, pDesc, initialResourceState, pOptimizedClearValue, IID_PPV_ARGS(resultResource.GetAddressOf()));
	}
	if (SUCCEEDED(hResult))
	{
		PResourceStateTracker::RegisterResource(name, resultResource.Get(), initialResourceState);
	}

	return resultResource;
}

void PDirectX12API::DestroyCommittedResource(HDX12ComPtr<HDX12Resource> resource)
{
	if (resource == nullptr)
	{
		return;
	}

	PResourceStateTracker::UnRegisterResource(resource.Get());
}

const HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& PDirectX12API::GetRootSignatureCache() const
{
	return _rootSignatureCache;
}

HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& PDirectX12API::GetRootSignatureCacheRef()
{
	return _rootSignatureCache;
}

const HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& PDirectX12API::GetGraphicsPSOCache() const
{
	return _graphicsPSOCache;
}

HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& PDirectX12API::GetGraphicsPSOCacheRef()
{
	return _graphicsPSOCache;
}

const HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& PDirectX12API::GetComputePSOCache() const
{
	return _computePSOCache;
}

HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& PDirectX12API::GetComputePSOCacheRef()
{
	return _computePSOCache;
}

const HHashMap<HDX12Resource*, HResourceInfo>& PDirectX12API::GetResourceRefCache() const
{
	return _resourceRefCache;
}

HHashMap<HDX12Resource*, HResourceInfo>& PDirectX12API::GetResourceRefCacheRef()
{
	return _resourceRefCache;
}

PSharedPtr<PGraphicsCommandList> PDirectX12API::RequestGraphicsCommandList()
{
	return Cast<PGraphicsCommandList>(_commandQueue->RequestCommandList(ECommandListType::Graphics));
}

PSharedPtr<PComputeCommandList>  PDirectX12API::RequestComputeCommandList()
{
	return Cast<PComputeCommandList>(_commandQueue->RequestCommandList(ECommandListType::Compute));
}

PSharedPtr<PCommandList> PDirectX12API::RequestCommandList()
{
	return _commandQueue->RequestCommandList(ECommandListType::Base);
}

HDescriptionAllocation PDirectX12API::RTVAllocate()
{
	return std::move(_rtvAllocator->Allocate());
}

HDescriptionAllocation PDirectX12API::DSVAllocate()
{
	return std::move(_dsvAllocator->Allocate());
}

HDescriptionAllocation PDirectX12API::CSUAllocate()
{
	return std::move(_csuAllocator->Allocate());
}

PSharedPtr<PCommandQueue> PDirectX12API::GetCommandQueue() const
{
	return _commandQueue;
}

PSharedPtr<PDX12FrameBuffer> PDirectX12API::GetFrameBuffer() const
{
	return _frameBuffer;
}

// ----------------------------------------------------------------------------
// HDirectXAPI
//
// @NOTE
// 아래 정적 래퍼들은 전부 PDirectX12API 인스턴스를 거쳐 동작한다.
// 그런데 PDirectX12API 는 GC(GMemoryGlobalSystem) 가 관리하는 객체다.
// 모듈 Shutdown 이 참조를 끊으면 곧바로 이어지는 GC Flush 에서 파괴되는데,
// 텍스처/버퍼 같은 GPU 리소스도 같은 Flush 에서 파괴된다.
// 할당 큐가 FIFO 라서 먼저 할당된 PDirectX12API 가 리소스들보다 항상 먼저 죽는다.
// 즉 "그래픽스 API 가 이미 없는 상태에서 리소스 소멸자가 도는" 구간이 반드시 생긴다.
//
// 그래서 두 가지를 지킨다.
//   1. 캐시는 PDirectX12API 가 죽기 전에 스스로 비운다. (resetCache / invalidateCache)
//   2. 모든 래퍼는 getDX12API() 가 nullptr 을 줄 수 있다고 보고 방어한다.
// 둘 중 하나만 해서는 안 된다. 1 만 하면 nullptr 역참조로 죽고, 2 만 하면 캐시가 썩는다.
// ----------------------------------------------------------------------------

static std::atomic<PDirectX12API*> GCachedDX12API{ nullptr };
static HAtomicBool GIsDX12APIAvailable{ true };

// 참조를 반환해야 하는 래퍼용 폴백. nullptr 을 돌려줄 수 없으니 빈 인스턴스를 내준다.
// 조회는 전부 없음으로 떨어지고, 쓰기는 버려진다.
//
// 함수 지역 static 인 이유
//  - 폴백 경로를 한 번도 타지 않으면 아예 생성되지 않는다. (정상 실행에서는 만들지 않는다)
//  - 파일 스코프 static 컨테이너의 초기화 순서 문제를 피한다.
// const / non-const 게터가 같은 인스턴스를 봐야 하므로 접근자로 감싼다.
static HJGGraphicsArguments& fallbackArguments()
{
	static HJGGraphicsArguments arguments;
	return arguments;
}

static HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& fallbackRootSignatureCache()
{
	static HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>> cache;
	return cache;
}

static HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& fallbackGraphicsPSOCache()
{
	static HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>> cache;
	return cache;
}

static HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& fallbackComputePSOCache()
{
	static HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>> cache;
	return cache;
}

static HHashMap<HDX12Resource*, HResourceInfo>& fallbackResourceRefCache()
{
	static HHashMap<HDX12Resource*, HResourceInfo> cache;
	return cache;
}

// 종료 시 리소스 수천 개가 이 경로를 타므로 한 번만 남긴다.
static void logFallbackOnce(const char* funcName)
{
	static HAtomicBool bLogged{ false };
	if (bLogged.exchange(true) == true)
	{
		return;
	}
	JG_LOG(Graphics, ELogLevel::Trace, "DirectX12 API is not available. HDirectXAPI::%s falls back.", PString(funcName));
}

PDirectX12API* HDirectXAPI::getDX12API()
{
	if (GIsDX12APIAvailable.load(std::memory_order_relaxed) == false)
	{
		return nullptr;
	}

	PDirectX12API* cachedAPI = GCachedDX12API.load(std::memory_order_relaxed);
	if (cachedAPI != nullptr)
	{
		return cachedAPI;
	}

	HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>();
	if (graphicsModule == nullptr)
	{
		return nullptr;
	}

	PSharedPtr<PDirectX12API> dx12API = RawFastCast<PDirectX12API>(graphicsModule->GetGraphicsAPI());
	if (dx12API.IsValid() == false)
	{
		return nullptr;
	}

	cachedAPI = dx12API.GetRawPointer();
	GCachedDX12API.store(cachedAPI, std::memory_order_relaxed);

	return cachedAPI;
}

void HDirectXAPI::resetCache()
{
	GCachedDX12API.store(nullptr, std::memory_order_relaxed);
	GIsDX12APIAvailable.store(true, std::memory_order_relaxed);
}

void HDirectXAPI::invalidateCache(const PDirectX12API* owner)
{
	// 죽는 인스턴스가 이미 교체된 새 인스턴스의 캐시까지 지우지 않도록 확인한다.
	PDirectX12API* cachedAPI = GCachedDX12API.load(std::memory_order_relaxed);
	if (owner != nullptr && cachedAPI != nullptr && cachedAPI != owner)
	{
		return;
	}

	GCachedDX12API.store(nullptr, std::memory_order_relaxed);
	GIsDX12APIAvailable.store(false, std::memory_order_relaxed);
}

HDX12Device* HDirectXAPI::GetDevice()
{
	PDirectX12API* dx12API = getDX12API();
	return (dx12API != nullptr) ? dx12API->GetDevice() : nullptr;
}

HDX12Factory* HDirectXAPI::GetFactory()
{
	PDirectX12API* dx12API = getDX12API();
	return (dx12API != nullptr) ? dx12API->GetFactory() : nullptr;
}

const HJGGraphicsArguments& HDirectXAPI::GetArguments()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetArguments");
		return fallbackArguments();
	}
	return dx12API->GetArguments();
}

HDX12ComPtr<HDX12Resource> HDirectXAPI::CreateCommittedResource(const PString& name, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS heapFlags, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES initialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue)
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		return nullptr;
	}
	return dx12API->CreateCommittedResource(name, pHeapProperties, heapFlags, pDesc, initialResourceState, pOptimizedClearValue);
}

void HDirectXAPI::DestroyCommittedResource(HDX12ComPtr<HDX12Resource> resource)
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		return;
	}
	dx12API->DestroyCommittedResource(resource);
}

const HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& HDirectXAPI::GetRootSignatureCache()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetRootSignatureCache");
		return fallbackRootSignatureCache();
	}
	return dx12API->GetRootSignatureCache();
}

HHashMap<uint64, HDX12ComPtr<HDX12RootSignature>>& HDirectXAPI::GetRootSignatureCacheRef()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetRootSignatureCacheRef");
		return fallbackRootSignatureCache();
	}
	return dx12API->GetRootSignatureCacheRef();
}

const HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& HDirectXAPI::GetGraphicsPSOCache()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetGraphicsPSOCache");
		return fallbackGraphicsPSOCache();
	}
	return dx12API->GetGraphicsPSOCache();
}

HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& HDirectXAPI::GetGraphicsPSOCacheRef()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetGraphicsPSOCacheRef");
		return fallbackGraphicsPSOCache();
	}
	return dx12API->GetGraphicsPSOCacheRef();
}

const HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& HDirectXAPI::GetComputePSOCache()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetComputePSOCache");
		return fallbackComputePSOCache();
	}
	return dx12API->GetComputePSOCache();
}

HHashMap<uint64, HDX12ComPtr<HDX12Pipeline>>& HDirectXAPI::GetComputePSOCacheRef()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetComputePSOCacheRef");
		return fallbackComputePSOCache();
	}
	return dx12API->GetComputePSOCacheRef();
}

const HHashMap<HDX12Resource*, HResourceInfo>& HDirectXAPI::GetResourceRefCache()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetResourceRefCache");
		return fallbackResourceRefCache();
	}
	return dx12API->GetResourceRefCache();
}

HHashMap<HDX12Resource*, HResourceInfo>& HDirectXAPI::GetResourceRefCacheRef()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		logFallbackOnce("GetResourceRefCacheRef");
		return fallbackResourceRefCache();
	}
	return dx12API->GetResourceRefCacheRef();
}

PSharedPtr<PGraphicsCommandList> HDirectXAPI::RequestGraphicsCommandList()
{
	PDirectX12API* dx12API = getDX12API();
	return (dx12API != nullptr) ? dx12API->RequestGraphicsCommandList() : nullptr;
}

PSharedPtr<PComputeCommandList> HDirectXAPI::RequestComputeCommandList()
{
	PDirectX12API* dx12API = getDX12API();
	return (dx12API != nullptr) ? dx12API->RequestComputeCommandList() : nullptr;
}

PSharedPtr<PCommandList> HDirectXAPI::RequestCommandList()
{
	PDirectX12API* dx12API = getDX12API();
	return (dx12API != nullptr) ? dx12API->RequestCommandList() : nullptr;
}

HDescriptionAllocation HDirectXAPI::RTVAllocate()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		return HDescriptionAllocation();
	}
	return dx12API->RTVAllocate();
}

HDescriptionAllocation HDirectXAPI::DSVAllocate()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		return HDescriptionAllocation();
	}
	return dx12API->DSVAllocate();
}

HDescriptionAllocation HDirectXAPI::CSUAllocate()
{
	PDirectX12API* dx12API = getDX12API();
	if (dx12API == nullptr)
	{
		return HDescriptionAllocation();
	}
	return dx12API->CSUAllocate();
}

PSharedPtr<PCommandQueue> HDirectXAPI::GetCommandQueue()
{
	PDirectX12API* dx12API = getDX12API();
	return (dx12API != nullptr) ? dx12API->GetCommandQueue() : nullptr;
}

PSharedPtr<PDX12FrameBuffer> HDirectXAPI::GetFrameBuffer()
{
	PDirectX12API* dx12API = getDX12API();
	return (dx12API != nullptr) ? dx12API->GetFrameBuffer() : nullptr;
}

PSharedPtr<IRawTexture> HDirectXAPI::GetDefaultTexture()
{
	PDirectX12API* dx12API = getDX12API();
	return (dx12API != nullptr) ? dx12API->GetDefaultTexture() : nullptr;
}