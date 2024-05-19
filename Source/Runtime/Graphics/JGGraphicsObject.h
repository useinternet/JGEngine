#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"


class GRAPHICS_API IJGGraphicsObject
{
public:
	virtual const PName& GetName() const = 0;
	virtual void SetName(const PName& inName) = 0;
};