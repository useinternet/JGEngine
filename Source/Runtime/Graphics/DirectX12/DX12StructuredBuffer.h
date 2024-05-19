#pragma once
#include "Classes/StructuredBuffer.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"

class PDX12StructuredBuffer
	: public IMemoryObject
	, public IStructuredBuffer
{

	PName _name;

	uint64 _elementSize = 0;
	uint64 _elementCount = 0;
	void* _cpuData = nullptr;

	HDX12ComPtr<HDX12Resource> _dx12Resource;

public:
	virtual ~PDX12StructuredBuffer();

public:
	// IJGGraphicsObject
	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;
	// ~IJGGraphicsObject

	// IStructuredBuffer
	virtual void SetDatas(const void* inDatas, uint64 inElementSize, uint64 inElementCount) override;
	virtual void SetData(const void* inData, uint64 inIndex) const override;
	virtual void* GetDatas() const override;
	virtual void* GetData(uint64 inIndex) const override;

	virtual uint64 GetDataSize() const override;
	virtual uint64 GetElementCount() const override;
	virtual uint64 GetElementSize() const override;

	virtual void Reset() override;
	virtual bool IsValid() const override;
	// ~IStructuredBuffer

	HDX12Resource* Get() const;
};