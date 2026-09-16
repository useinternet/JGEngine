#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class GShaderLibrary : public GGlobalSystemInstance<GShaderLibrary>
{
public:
	static PString GraphicsShaderTemplate;
	static PString MaterialConstantBufferContentsScript;
	static PString MaterialSurfaceContentsScript;
	static PString MaterialSceneContentsScript;
private:
	// 시스템 Start()는 모듈 StartupModule보다 늦게 불리므로, 그 전에 템플릿이 필요하면 첫 접근 시 지연 로드한다.
	mutable HHashMap<PName, PString> _shaderTemplates;
	mutable bool _bLoaded = false;

public:
	virtual void Start();
	virtual void Destroy();

public:
	const PString& GetGraphicsShaderTemplateCode() const;

private:
	void loadTemplates() const;
};