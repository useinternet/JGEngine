#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"

struct GRAPHICS_API HIndexBufferConstructArguments
{
	PName Name;
};

class GRAPHICS_API IIndexBuffer : public IJGGraphicsObject
{
public:

	virtual void SetDatas(const uint32* inDatas, uint64 inCount) = 0;
	virtual void SetData(uint32 inData, uint64 inIndex) = 0;
	virtual uint32* GetDatas() const = 0;
	virtual uint32 GetData(uint64 inIndex) const = 0;

	virtual uint64 GetIndexCount() const = 0;

	virtual void Reset() = 0;
	virtual bool IsValid() const = 0;
};