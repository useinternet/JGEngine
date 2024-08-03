#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class GShaderLibrary : public GGlobalSystemInstance<GShaderLibrary>
{
public:
	static PString GraphicsShaderTemplate;
	static PString MaterialSurfaceContentcript;
	static PString MaterialConstantBufferContentcript;
private:
	HHashMap<PName, PString> _shaderTemplates;

public:
	virtual void Start();
	virtual void Destroy();

public:
	const PString& GetGraphicsShaderTemplateCode() const;
};