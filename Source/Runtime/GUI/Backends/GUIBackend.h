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

	// 글꼴 · 스타일에 쓴 화면 배율(1.0 = 96 DPI). HGUI::GetDpiScale 이 읽는다.
	float32 GetDpiScale() const { return DpiScale; }

protected:
	float32 DpiScale = 1.0f;

	void GenerateGUI();
	void GenerateMainMenuGUI();
};