#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "DirectX12Helper.h"

class IComputeBuffer;
class GraphicsPipelineState;
class RootSignature;
class IRawTexture;
class ShaderScriptCodeAnalyzer;

struct HCompileConfig
{
	PString Entry;
	PString Target;
	HCompileConfig(const PString& entry, const PString& target) : Entry(entry), Target(target) {}
};

class PDX12ShaderCompiler : public IMemoryObject
{
protected:
	EShaderCompileFlags _flags;
	bool _bSuccess = false;

public:
	virtual ~PDX12ShaderCompiler() = default;

	EShaderCompileFlags GetFlags() const
	{
		return _flags;
	}

	bool IsSuccessed() const
	{
		return _bSuccess;
	}

protected:
	bool compile(HDX12ComPtr<HDX12Blob>& blob, const PString& sourceCode, const HCompileConfig& config, const HList<HPair<PName, PName>>& inMacros, PString* error);
};

class PDX12GraphicsShaderCompiler : public PDX12ShaderCompiler
{
	HDX12ComPtr<HDX12Blob> _VSData;
	HDX12ComPtr<HDX12Blob> _DSData;
	HDX12ComPtr<HDX12Blob> _HSData;
	HDX12ComPtr<HDX12Blob> _GSData;
	HDX12ComPtr<HDX12Blob> _PSData;
public:
	virtual ~PDX12GraphicsShaderCompiler() = default;

	HDX12Blob* GetVSData() const {
		if (IsSuccessed() == false)
		{
			return nullptr;
		}
		return _VSData.Get();
	}
	HDX12Blob* GetDSData() const {
		if (IsSuccessed() == false)
		{
			return nullptr;
		}
		return _DSData.Get();
	}
	HDX12Blob* GetHSData() const {
		if (IsSuccessed() == false)
		{
			return nullptr;
		}
		return _HSData.Get();
	}
	HDX12Blob* GetGSData() const {
		if (IsSuccessed() == false)
		{
			return nullptr;
		}
		return _GSData.Get();
	}
	HDX12Blob* GetPSData() const {
		if (IsSuccessed() == false)
		{
			return nullptr;
		}
		return _PSData.Get();
	}

	bool Compile(const PString& code, EShaderCompileFlags flags, const HList<HPair<PName, PName>>& inMacros, PString* error);
};

class PDX12ComputeShaderCompiler : public PDX12ShaderCompiler
{
	HDX12ComPtr<HDX12Blob> _CSData;

public:
	virtual ~PDX12ComputeShaderCompiler() = default;

	bool Compile(const PString& code, PString* error);

	HDX12Blob* GetCSData() const {
		if (IsSuccessed() == false)
		{
			return nullptr;
		}

		return _CSData.Get();
	}
};
