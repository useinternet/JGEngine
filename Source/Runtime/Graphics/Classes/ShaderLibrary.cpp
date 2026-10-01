#include "PCH/PCH.h"
#include "ShaderLibrary.h"

PString GShaderLibrary::GraphicsShaderTemplate = "graphics_shader_template";
PString GShaderLibrary::MaterialConstantBufferContentsScript = "__PS_CONSTANT_BUFFER_CONTENTS_SCRIPT__";
PString GShaderLibrary::MaterialSurfaceContentsScript        = "__PS_SURFACE_CONTENTS_SCRIPT__";
PString GShaderLibrary::MaterialScreenContentsScript         = "__PS_SCREEN_CONTENTS_SCRIPT__";

void GShaderLibrary::Start()
{
	loadTemplates();
}

void GShaderLibrary::Destroy()
{
	_shaderTemplates.clear();
	_bLoaded = false;
}

const PString& GShaderLibrary::GetGraphicsShaderTemplateCode() const
{
	return GetShaderCode(GraphicsShaderTemplate);
}

const PString& GShaderLibrary::GetShaderCode(const PString& inName) const
{
	if (_bLoaded == false)
	{
		loadTemplates();
	}

	const PName name(inName);
	if (_shaderTemplates.contains(name))
	{
		return _shaderTemplates.at(name);
	}

	static PString nullCode;
	return nullCode;
}

void GShaderLibrary::loadTemplates() const
{
	if (_bLoaded)
	{
		return;
	}
	_bLoaded = true;

	const PString& shaderDirPath = HFileHelper::EngineShaderDirectory();
	JG_LOG(Graphics, ELogLevel::Info, "Start Collect Shader in (%s)", shaderDirPath);

	HList<PString> shaderFileList;
	HFileHelper::FileListInDirectory(shaderDirPath, &shaderFileList, true, { ".hlsl" });

	for (const PString& shaderTemplatePath : shaderFileList)
	{
		PString shaderCode;
		PString shaderTemplateName;
		HFileHelper::FileNameOnly(shaderTemplatePath, &shaderTemplateName);

		if (HFileHelper::ReadAllText(shaderTemplatePath, &shaderCode))
		{
			_shaderTemplates.emplace(shaderTemplateName, shaderCode);
			JG_LOG(Graphics, ELogLevel::Trace, "Collect %s", shaderTemplateName);
		}
		else
		{
			JG_LOG(Graphics, ELogLevel::Error, "Fail %s", shaderTemplateName);
		}
	}

	JG_LOG(Graphics, ELogLevel::Info, "Complete Collect Shader");
}
