#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"

struct GRAPHICS_API HVertexBufferConstructArguments
{
	PName Name;
};

class GRAPHICS_API IVertexBuffer : public IJGGraphicsObject
{
public:
	virtual void SetDatas(const void* inDatas, uint64 inElementSize, uint64 inElementCount) = 0; 
	virtual void SetData(const void* inData, uint64 inIndex) = 0;
	virtual void* GetDatas() const = 0;
	virtual void* GetData(uint64 inIndex) const = 0;

	template<class T>
	const T& GetData(uint64 inIndex) const
	{
		return *((T*)GetData(inIndex));
	}

	virtual uint64 GetVertexCount() const = 0;
	virtual uint64 GetVertexSize() const = 0;

	virtual void Reset() = 0;
	virtual bool IsValid() const = 0;
};