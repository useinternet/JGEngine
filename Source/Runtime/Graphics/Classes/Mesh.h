#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class IVertexBuffer;
class IIndexBuffer;
class GRAPHICS_API IMesh : public IMemoryObject
{
public:
	virtual ~IMesh() = default;

	virtual bool IsValid() const = 0;
	virtual void Reset() = 0;
};