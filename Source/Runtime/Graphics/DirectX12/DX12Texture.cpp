#include "PCH/PCH.h"
#include "DX12Texture.h"
#include "JGGraphicsHelper.h"
#include "DirectX12/DirectX12API.h"
#include "Classes/CommandList.h"

PDX12Texture::~PDX12Texture()
{
	Reset();
}

const PName& PDX12Texture::GetName() const
{
	return _name;
}

void PDX12Texture::SetName(const PName& inName)
{
	_name = inName;
	if (IsValid())
	{
		_dx12Resource->SetName(inName.ToString().GetRawWString().c_str());
	}
}

uint64 PDX12Texture::GetTextureID() const
{
	return uint64();
	//D3D12_SHADER_RESOURCE_VIEW_DESC;
	//HDirectXAPI::GetDevice()->CreateShaderResourceView()
}

const HTextureInfo& PDX12Texture::GetTextureInfo() const
{
	return _textureInfo;
}

void PDX12Texture::AccessPixels(HOnAccessTexturePixels onAccessTexturePixels) const
{
	if (IsValid() == false)
	{
		return;
	}

	void* result = nullptr;
	_dx12Resource->Map(0, nullptr, &result);
	onAccessTexturePixels.ExecuteIfBound(result);
	_dx12Resource->Unmap(0, nullptr);
}

void PDX12Texture::Reset()
{
	if (_dx12Resource == nullptr)
	{
		return;
	}

	_rtvs.clear();
	_dsvs.clear();
	_srvs.clear();
	_uavs.clear();	

	if (HDirectXAPI::IsValid())
	{
		HDirectXAPI::DestroyCommittedResource(_dx12Resource);
	}

	_dx12Resource.Reset();
	_dx12Resource = nullptr;
}

bool PDX12Texture::IsValid() const
{
	return _dx12Resource != nullptr;
}

HDX12Resource* PDX12Texture::Get() const
{
	return _dx12Resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12Texture::GetSRV() const
{
	if (IsValid() == false) return { 0 };

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;

	bool bIsDefaultDesc = createSRVDesc(_textureInfo.Flags, srvDesc);

	uint64 hash = 0;
	if (bIsDefaultDesc == false)
	{
		hash = std::hash<D3D12_SHADER_RESOURCE_VIEW_DESC>{}(srvDesc);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = { 0 };
	bool isFind = false;
	{
		std::shared_lock<std::shared_mutex> lock(_srvMutex);
		isFind = _srvs.find(hash) != _srvs.end();
		if (isFind == true)
		{
			handle = _srvs[hash].CPU();
		}
	}
	if (isFind == false)
	{
		std::lock_guard<std::shared_mutex> lock(_srvMutex);
		HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
		HDirectXAPI::GetDevice()->CreateShaderResourceView(Get(), bIsDefaultDesc ? nullptr : &srvDesc, alloc.CPU());
		_srvs.emplace(hash, std::move(alloc));

		handle = _srvs[hash].CPU();
	}

	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12Texture::GetUAV() const
{
	if (IsValid() == false) return { 0 };

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;

	bool bIsDefaultDesc = CreateUAVDesc(_textureInfo.Flags, uavDesc);

	uint64 hash = 0;
	if (bIsDefaultDesc == false)
	{
		hash = std::hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>{}(uavDesc);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = { 0 };
	bool isFind = false;
	{
		std::shared_lock<std::shared_mutex> lock(_uavMutex);
		isFind = _uavs.find(hash) != _uavs.end();
		if (isFind == true)
		{
			handle = _uavs[hash].CPU();
		}
	}
	if (isFind == false)
	{
		std::lock_guard<std::shared_mutex> lock(_uavMutex);
		HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
		HDirectXAPI::GetDevice()->CreateUnorderedAccessView(Get(), nullptr, bIsDefaultDesc ? nullptr : &uavDesc, alloc.CPU());
		_uavs.emplace(hash, std::move(alloc));
		handle = _uavs[hash].CPU();
	}

	return handle;
}
D3D12_CPU_DESCRIPTOR_HANDLE PDX12Texture::GetRTV() const
{
	if (IsValid() == false) return { 0 };

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	if ((_textureInfo.Flags & ETextureFlags::Allow_RenderTarget) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "not supported RenderTarget because does not include  ETextureFlags::Allow_RenderTarget Flag");
		return { 0 };
	}

	bool bIsDefaultDesc = createRTVDesc(_textureInfo.Flags, rtvDesc) == false;

	uint64 hash = 0;
	if (bIsDefaultDesc == false)
	{
		hash = std::hash<D3D12_RENDER_TARGET_VIEW_DESC>{}(rtvDesc);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = { 0 };
	bool isFind = false;
	{
		std::shared_lock<std::shared_mutex> lock(_rtvMutex);
		isFind = _rtvs.find(hash) != _rtvs.end();
		if (isFind == true)
		{
			handle = _rtvs[hash].CPU();
		}
	}
	if (isFind == false)
	{
		std::lock_guard<std::shared_mutex> lock(_rtvMutex);
		HDescriptionAllocation alloc = HDirectXAPI::RTVAllocate();
		HDirectXAPI::GetDevice()->CreateRenderTargetView(Get(), bIsDefaultDesc ? nullptr : &rtvDesc, alloc.CPU());
		_rtvs.emplace(hash, std::move(alloc));
		handle = _rtvs[hash].CPU();
	}

	return handle;
}
D3D12_CPU_DESCRIPTOR_HANDLE PDX12Texture::GetDSV() const
{
	if (IsValid() == false) return { 0 };
	if ((_textureInfo.Flags & ETextureFlags::Allow_DepthStencil) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "not supported DepthStencil because does not include  ETextureFlags::Allow_DepthStencil Flag");
		return { 0 };
	}
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;

	bool bIsDefaultDesc = createDSVDesc(_textureInfo.Flags, dsvDesc);

	uint64 hash = 0;
	if (bIsDefaultDesc == false)
	{
		hash = std::hash<D3D12_DEPTH_STENCIL_VIEW_DESC>{}(dsvDesc);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE handle = { 0 };
	bool isFind = false;
	{
		std::shared_lock<std::shared_mutex> lock(_dsvMutex);
		isFind = _dsvs.find(hash) != _dsvs.end();
		if (isFind == true)
		{
			handle = _dsvs[hash].CPU();
		}
	}
	if (isFind == false)
	{
		std::lock_guard<std::shared_mutex> lock(_dsvMutex);
		HDescriptionAllocation alloc = HDirectXAPI::RTVAllocate();
		HDirectXAPI::GetDevice()->CreateDepthStencilView(Get(), bIsDefaultDesc ? nullptr : &dsvDesc, alloc.CPU());
		_dsvs.emplace(hash, std::move(alloc));
		handle = _dsvs[hash].CPU();
	}

	return handle;
}

void PDX12Texture::Initialize(const HTextureInfo& inTextureInfo)
{
	Reset();

	_textureInfo = inTextureInfo;

	ETextureFlags flags = inTextureInfo.Flags;

	D3D12_RESOURCE_FLAGS d3dRscFlags = D3D12_RESOURCE_FLAG_NONE;
	if (flags & ETextureFlags::Allow_RenderTarget)
	{
		d3dRscFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	else if (flags & ETextureFlags::Allow_DepthStencil)
	{
		d3dRscFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	if (flags & ETextureFlags::Allow_UnorderedAccessView)
	{
		d3dRscFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_RESOURCE_DESC rscDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		HDirectX12Helper::ConvertDXGIFormat(_textureInfo.Format), _textureInfo.Width, _textureInfo.Height,
		_textureInfo.ArraySize, _textureInfo.MipLevel, 1, 0,
		d3dRscFlags, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0);

	bool bUseClearValue = false;

	D3D12_CLEAR_VALUE clearValue;
	if (flags & ETextureFlags::Allow_RenderTarget)
	{
		bUseClearValue = true;

		clearValue.Color[0] = _textureInfo.ClearColor.R;
		clearValue.Color[1] = _textureInfo.ClearColor.G;
		clearValue.Color[2] = _textureInfo.ClearColor.B;
		clearValue.Color[3] = _textureInfo.ClearColor.A;
		clearValue.Format = HDirectX12Helper::ConvertDXGIFormat(_textureInfo.Format);
	}
	else if (flags & ETextureFlags::Allow_DepthStencil)
	{
		bUseClearValue = true;

		clearValue.DepthStencil.Depth   = _textureInfo.ClearDepth;
		clearValue.DepthStencil.Stencil = _textureInfo.ClearStencil;
		clearValue.Format = HDirectX12Helper::ConvertDXGIFormat(_textureInfo.Format);
	}

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	_dx12Resource = HDirectXAPI::CreateCommittedResource(
		_textureInfo.Name,
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		bUseClearValue ? &clearValue : nullptr);

	SetName(_textureInfo.Name);
}

void PDX12Texture::InitializeByMemory(const uint8* pixels, const HTextureInfo& inTextureInfo)
{
	Initialize(inTextureInfo);

	uint8 channels = HJGGraphicsHelper::GetTextureFormatChannels(_textureInfo.Format);

	PSharedPtr<PCommandList> commandList = HDirectXAPI::RequestCommandList();
	commandList->CopyTextrueFromMemory(_dx12Resource.Get(), pixels, _textureInfo.Width, _textureInfo.Height, channels, _textureInfo.ArraySize);
}

bool PDX12Texture::createSRVDesc(ETextureFlags inTextureFlags, D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) const
{
	JG_CHECK(IsValid());

	D3D12_RESOURCE_DESC desc = Get()->GetDesc();

	if (inTextureFlags & ETextureFlags::SRV_TextureCube)
	{
		outDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		outDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		outDesc.TextureCube.MostDetailedMip = 0;
		outDesc.TextureCube.MipLevels = desc.MipLevels;
		outDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		outDesc.Format = desc.Format;

		return true;
	}

	return false;
}

bool PDX12Texture::CreateUAVDesc(ETextureFlags inTextureFlags, D3D12_UNORDERED_ACCESS_VIEW_DESC& outDesc) const
{
	return false;
}
bool PDX12Texture::createRTVDesc(ETextureFlags inTextureFlags, D3D12_RENDER_TARGET_VIEW_DESC& outDesc) const
{
	return false;
}
bool PDX12Texture::createDSVDesc(ETextureFlags inTextureFlags, D3D12_DEPTH_STENCIL_VIEW_DESC& outDesc) const
{
	return false;
}
