#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "Classes/Texture.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "Classes/DescriptionAllocator.h"

class PDX12Texture 
	: public IMemoryObject 
	, public IRawTexture
{
	friend class PDirectX12API;

	PName _name;
	HDX12ComPtr<HDX12Resource> _dx12Resource;
	HTextureInfo _textureInfo;


	mutable std::shared_mutex _srvMutex;
	mutable std::shared_mutex _uavMutex;
	mutable std::shared_mutex _rtvMutex;
	mutable std::shared_mutex _dsvMutex;

	mutable HHashMap<uint64, HDescriptionAllocation> _srvs;
	mutable HHashMap<uint64, HDescriptionAllocation> _uavs;
	mutable HHashMap<uint64, HDescriptionAllocation> _rtvs;
	mutable HHashMap<uint64, HDescriptionAllocation> _dsvs;
public:
	virtual ~PDX12Texture();

	// IJGGraphicsObject
	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;
	// ~IJGGraphicsObject
public:
	virtual uint64 GetTextureID() const override;
	virtual const HTextureInfo& GetTextureInfo() const override;
	virtual void AccessPixels(HOnAccessTexturePixels onAccessTexturePixels) const;

	virtual void Reset() override;
	virtual bool IsValid() const override;

	HDX12Resource* Get() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;

protected:
	virtual void Initialize(const HTextureInfo& inTextureInfo) override;
	virtual void InitializeByMemory(const uint8* pixels, const HTextureInfo& inTextureInfo) override;

private:
	bool createSRVDesc(ETextureFlags inTextureFlags, D3D12_SHADER_RESOURCE_VIEW_DESC& outDesc) const;
	bool CreateUAVDesc(ETextureFlags inTextureFlags, D3D12_UNORDERED_ACCESS_VIEW_DESC& outDesc) const;
	bool createRTVDesc(ETextureFlags inTextureFlags, D3D12_RENDER_TARGET_VIEW_DESC& outDesc) const;
	bool createDSVDesc(ETextureFlags inTextureFlags, D3D12_DEPTH_STENCIL_VIEW_DESC& outDesc) const;
};