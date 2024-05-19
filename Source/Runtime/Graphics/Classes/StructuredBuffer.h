#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"

class GRAPHICS_API IStructuredBuffer : public IJGGraphicsObject
{
public:
	virtual void SetDatas(const void* initDatas, uint64 elementSize, uint64 elementCount) = 0;
	virtual void SetData(const void* inData, uint64 inIndex) const = 0;

	virtual void* GetDatas() const = 0;
	virtual void* GetData(uint64 inIndex) const = 0;

	virtual uint64 GetDataSize() const = 0;
	virtual uint64 GetElementCount() const = 0;
	virtual uint64 GetElementSize() const = 0;

	virtual void Reset() = 0;
	virtual bool IsValid() const = 0;
};