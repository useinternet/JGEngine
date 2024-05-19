#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"

class GRAPHICS_API IConstantBuffer : public IJGGraphicsObject
{
public:
	virtual void SetData(const void* inData, uint64 inOffset, uint64 inElementSize) = 0;
	virtual void  SetData(const void* inData, uint64 elementSize) = 0;
	virtual void* GetData(uint64 inOffset = 0) const = 0;

	virtual uint64 GetDataSize() const = 0;

	virtual void Reset() = 0;
	virtual bool IsValid() const = 0;
};