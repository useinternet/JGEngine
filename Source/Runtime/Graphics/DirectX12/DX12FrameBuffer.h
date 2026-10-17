#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "Classes/FrameBuffer.h"
#include "Classes/DescriptionAllocator.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"

class PDX12Texture;
class PDX12FrameBuffer
	: public IFrameBuffer
{
	friend class PDirectX12API;

	PName _name;
	HDX12ComPtr<HDX12SwapChain>		  _dx12SwapChain;
	HList<HDX12ComPtr<HDX12Resource>> _dx12BackBuffers;
	HList<HDescriptionAllocation>     _rtvs;

	PSharedPtr<PDX12Texture>   _submittedTexture;
	HFrameBufferInfo _frameBufferInfo;

	bool _bMinimized    = false;   // 창이 0 x 0(최소화). 크기 변경과 Present 를 건너뛴다 (GP-G1)
	bool _bVSyncEnabled = false;   // Present(1, 0). 기본 꺼짐 = 에디터 그대로 (GP-G3)
	bool _bSubmitMismatchLogged = false;   // 제출 텍스처 크기 · 포맷 불일치 로그를 한 번만 (GP-G3)
public:
	JG_DECLARE_MULTICAST_DELEGATE(HOnPresent);
	JG_DECLARE_MULTICAST_DELEGATE(HOnUpdate, HDX12CommandList*, HDX12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_RESOURCE_STATES&);
	
	HOnPresent OnPresent;
	HOnUpdate  OnUpdate;
public:
	virtual ~PDX12FrameBuffer();

	// IJGGraphicsObject
	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;
	// ~IJGGraphicsObject

protected:
	virtual bool Initialize(const HFrameBufferInfo& info) override;
	virtual void Update() override;
	virtual bool Present() override;

public:
	virtual void SubmitTexture(PSharedPtr<IRawTexture> inTexture) override;
	// 0 x 0(최소화)이면 버퍼는 그대로 두고 Present 만 멈춘다. 복원 때 크기가 같으면 버퍼도 그대로다.
	virtual GRAPHICS_API void Resize(uint32 width, uint32 height) override;
	virtual const HFrameBufferInfo& GetInfo() const override;

	void SetVSyncEnabled(bool inEnabled);
	bool IsVSyncEnabled() const;
	bool IsMinimized() const;
};