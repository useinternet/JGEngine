#pragma once
#include "Classes/IndexBuffer.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "Classes/DescriptionAllocator.h"

class PDX12IndexBuffer
	: public IMemoryObject
	, public IIndexBuffer
{

	PName _name;

	uint64 _indexCount = 0;
	uint32* _cpuData = nullptr;

	HDX12ComPtr<HDX12Resource> _dx12Resource;

	mutable std::mutex _mutex;

	mutable HDescriptionAllocation _srv;
	mutable HDescriptionAllocation _uav;
public:
	virtual ~PDX12IndexBuffer();

public:
	// IJGGraphicsObject
	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;
	// ~IJGGraphicsObject

	// IIndexBuffer
	virtual void SetDatas(const uint32* inDatas, uint64 inCount) override;
	virtual void SetData(uint32 inData, uint64 inIndex) override;
	virtual uint32* GetDatas() const override;
	virtual uint32 GetData(uint64 inIndex) const override;

	virtual uint64 GetIndexCount() const override;

	virtual void Reset() override;
	virtual bool IsValid() const override;
	// ~IIndexBuffer

	HDX12Resource* Get() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const;
};