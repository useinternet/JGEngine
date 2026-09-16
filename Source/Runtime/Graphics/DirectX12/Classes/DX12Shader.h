#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "Classes/Shader.h"
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

// 그래픽스 셰이더 한 벌(VS/DS/HS/GS/PS)의 컴파일된 바이트코드를 소유한다.
// 컴파일은 이 객체의 연산이다. 컴파일러는 Compile() 안에서만 쓰고 결과 바이트코드만 남기므로
// 바이트코드의 원천은 _byteCodes 하나다. PSO 바인드 시 GetByteCodes()를 PGraphicsPipelineState::BindShader에 넘긴다.
// IMemoryObject는 IRawShader 사슬을 통해 한 번만 상속한다. (메모리 시스템 규칙: 뿌리 하나, 오프셋 0)
class PDX12GraphicsShader
	: public IRawGraphicsShader
{
private:
	HHashMap<EShaderDomain, HList<uint8>> _byteCodes;

public:
	virtual ~PDX12GraphicsShader() = default;

	// IRawShader / IRawGraphicsShader
	virtual bool IsValid() const override;
	virtual bool Compile(const HGraphicsShaderCompileArguments& inArgs, PString* outError) override;
	// ~IRawShader / IRawGraphicsShader

	// 외부(캐시 파일 등)에서 가져온 바이트코드를 직접 넣는다.
	void SetByteCode(EShaderDomain inDomain, const void* inData, uint64 inSize);

	const HHashMap<EShaderDomain, HList<uint8>>& GetByteCodes() const;
	const HList<uint8>* GetByteCode(EShaderDomain inDomain) const;
	bool HasByteCode(EShaderDomain inDomain) const;

	void Reset();

private:
	void storeByteCode(EShaderDomain inDomain, HDX12Blob* inBlob);
};
