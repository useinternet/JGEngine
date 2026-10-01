#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class GShaderLibrary : public GGlobalSystemInstance<GShaderLibrary>
{
public:
	static PString GraphicsShaderTemplate;
	static PString MaterialConstantBufferContentsScript;
	static PString MaterialSurfaceContentsScript;
	static PString MaterialScreenContentsScript;
private:
	// 시스템 Start()는 모듈 StartupModule보다 늦게 불리므로, 그 전에 템플릿이 필요하면 첫 접근 시 지연 로드한다.
	mutable HHashMap<PName, PString> _shaderTemplates;
	mutable bool _bLoaded = false;

public:
	virtual void Start();
	virtual void Destroy();

public:
	const PString& GetGraphicsShaderTemplateCode() const;
	// 셰이더 폴더에 있는 .hlsl 파일의 내용. 이름은 확장자 없는 파일 이름(예: "draw2d"). 없으면 빈 문자열.
	const PString& GetShaderCode(const PString& inName) const;

private:
	void loadTemplates() const;
};