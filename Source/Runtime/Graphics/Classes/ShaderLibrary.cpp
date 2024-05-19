#include "PCH/PCH.h"
#include "ShaderLibrary.h"

PString GShaderLibrary::GraphicsShaderTemplate = "graphics_shader_template";
PString GShaderLibrary::MaterialSurfaceContentScript = "__PS_SURFACE_CONTENTS_SCRIPT__";
PString GShaderLibrary::MaterialConstantBufferContentScript = "__PS_CONSTANT_BUFFER_CONTENTS_SCRIPT__";

void GShaderLibrary::Start()
{
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

void GShaderLibrary::Destroy()
{
	_shaderTemplates.clear();
}

const PString& GShaderLibrary::GetGraphicsShaderTemplateCode() const
{
	if (_shaderTemplates.contains(GraphicsShaderTemplate))
	{
		return _shaderTemplates.at(GraphicsShaderTemplate);
	}

	static PString nullCode;
	return nullCode;
}
