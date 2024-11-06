#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

JG_DECLARE_EVENT(HOnGUI, PGUIBackend);
JG_DECLARE_EVENT(HOnMainMenuGUI, PGUIBackend);

class PGUIBackend : public IMemoryObject
{
public:
	HOnGUI OnGUI;
	HOnMainMenuGUI OnMainMenuGUI;
public:
	virtual ~PGUIBackend() = default;
	virtual uint64 GPUAllocate(TextureID textureID) = 0;
	virtual void Initialize();
	virtual void Shutdown();

protected:
	void GenerateGUI();
	void GenerateMainMenuGUI();
};