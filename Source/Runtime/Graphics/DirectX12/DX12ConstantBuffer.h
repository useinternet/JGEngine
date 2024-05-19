#pragma once
#include "Classes/ConstantBuffer.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "Classes/DescriptionAllocator.h"

class PDX12ConstantBuffer
	: public IMemoryObject
	, public IConstantBuffer
{

	PName _name;

	uint64 _elementSize = 0;
	void* _cpuData = nullptr;

	HDX12ComPtr<HDX12Resource> _dx12Resource;

	mutable std::mutex _mutex;

	mutable HDescriptionAllocation _csv;
public:
	virtual ~PDX12ConstantBuffer();

public:
	// IJGGraphicsObject
	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;
	// ~IJGGraphicsObject

	// IConstantBuffer
	virtual void  SetData(const void* inData, uint64 elementSize) override;
	virtual void SetData(const void* inData, uint64 inOffset, uint64 inElementSize) override;
	virtual void* GetData(uint64 inOffset = 0) const override;

	virtual uint64 GetDataSize() const override;

	virtual void Reset() override;
	virtual bool IsValid() const override;
	// ~IConstantBuffer

	HDX12Resource* Get() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCSV() const;
};