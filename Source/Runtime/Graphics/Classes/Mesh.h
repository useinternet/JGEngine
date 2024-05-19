#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class IVertexBuffer;
class IIndexBuffer;
class GRAPHICS_API IMesh
{
public:
	virtual bool IsValid() const = 0;
	virtual void Reset() = 0;
};