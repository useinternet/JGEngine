#pragma once
#include "Classes/VertexBuffer.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "Classes/DescriptionAllocator.h"

class PDX12VertexBuffer 
	: public IMemoryObject
	, public IVertexBuffer
{

	PName _name;

	uint64 _elementSize  = 0;
	uint64 _elementCount = 0;
	void* _cpuData = nullptr;

	HDX12ComPtr<HDX12Resource> _dx12Resource;

	mutable std::mutex _mutex;

	mutable HDescriptionAllocation _srv;
	mutable HDescriptionAllocation _uav;
public:
	virtual ~PDX12VertexBuffer();

public:
	// IJGGraphicsObject
	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;
	// ~IJGGraphicsObject

	// IVertexBuffer
	virtual void SetDatas(const void* inDatas, uint64 inElementSize, uint64 inElementCount) override;
	virtual void SetData(const void* inData, uint64 inIndex) override;
	virtual void* GetDatas() const override;
	virtual void* GetData(uint64 inIndex) const override;

	virtual uint64 GetVertexCount() const override;
	virtual uint64 GetVertexSize() const override;

	virtual void Reset() override;
	virtual bool IsValid() const override;
	// ~IVertexBuffer

	HDX12Resource* Get() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const;
};