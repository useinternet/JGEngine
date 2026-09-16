#include "PCH/PCH.h"
#include "DX12Shader.h"
#include "DirectX12/DirectX12API.h"

bool PDX12ShaderCompiler::compile(HDX12ComPtr<HDX12Blob>& blob, const PString& sourceCode, const HCompileConfig& config, const HList<HPair<PName, PName>>& inMacros, PString* error)
{
	// D3D_SHADER_MACRO는 문자열을 참조만 하므로 D3DCompile2가 끝날 때까지 문자열이 살아 있어야 한다.
	// (이전 코드는 임시 PString의 포인터를 넘겨 댕글링이었고, 배열 끝의 {nullptr, nullptr} 종료 항목도 없었다.)
	HList<PString> macroStrings;
	macroStrings.reserve(inMacros.size() * 2);
	for (const HPair<PName, PName>& _pair : inMacros)
	{
		macroStrings.push_back(_pair.first.ToString());
		macroStrings.push_back(_pair.second.ToString());
	}

	HList<D3D_SHADER_MACRO> d3dMacros;
	d3dMacros.reserve(inMacros.size() + 1);
	for (uint64 i = 0; i < inMacros.size(); ++i)
	{
		D3D_SHADER_MACRO macro;
		macro.Name       = macroStrings[i * 2].GetCStr();
		macro.Definition = macroStrings[i * 2 + 1].GetCStr();
		d3dMacros.push_back(macro);
	}
	d3dMacros.push_back(D3D_SHADER_MACRO{ nullptr, nullptr });

	ComPtr<ID3DBlob> errorData;
	HRESULT hr = D3DCompile2(
		sourceCode.GetCStr(),
		sourceCode.Length(),
		nullptr,
		d3dMacros.data(),
		nullptr,
		config.Entry.GetCStr(),
		config.Target.GetCStr(),
		0, 0, 0, nullptr, 0,
		blob.GetAddressOf(),
		errorData.GetAddressOf());

	if (FAILED(hr))
	{
		// 이전 코드는 error가 null이면 실패를 true로 반환했고, errorData가 null이면 역참조로 죽었다.
		if (error != nullptr)
		{
			if (errorData != nullptr)
			{
				*error += (const char*)errorData->GetBufferPointer();
			}
			else
			{
				*error += PString::Format("D3DCompile2 failed (hr = %d)", (int32)hr);
			}
			*error += "\n";
		}
		return false;
	}

	return true;
}

bool PDX12GraphicsShaderCompiler::Compile(const PString& code, EShaderCompileFlags flags, const HList<HPair<PName, PName>>& inMacros, PString* error)
{
	_flags = flags;
	
	if (EnumHasAnyFlags(_flags, EShaderCompileFlags::Allow_VertexShader))
	{
		if (compile(_VSData, code, HCompileConfig(HHLSL::VSEntry, HHLSL::VSTarget), inMacros, error) == false)
		{
			return false;
		}
	}
	if (EnumHasAnyFlags(_flags, EShaderCompileFlags::Allow_DomainShader))
	{
		if (compile(_DSData, code, HCompileConfig(HHLSL::DSEntry, HHLSL::DSTarget), inMacros, error) == false)
		{
			return false;
		}
	}
	if (EnumHasAnyFlags(_flags, EShaderCompileFlags::Allow_HullShader))
	{
		if (compile(_HSData, code, HCompileConfig(HHLSL::HSEntry, HHLSL::HSTarget), inMacros, error) == false)
		{
			return false;
		}
	}
	if (EnumHasAnyFlags(_flags, EShaderCompileFlags::Allow_GeometryShader))
	{
		if (compile(_GSData, code, HCompileConfig(HHLSL::GSEntry, HHLSL::GSTarget), inMacros, error) == false)
		{
			return false;
		}
	}
	if (EnumHasAnyFlags(_flags, EShaderCompileFlags::Allow_PixelShader))
	{
		if (compile(_PSData, code, HCompileConfig(HHLSL::PSEntry, HHLSL::PSTarget), inMacros, error) == false)
		{
			return false;
		}
	}

	_bSuccess = true;
	return true;
}



bool PDX12ComputeShaderCompiler::Compile(const PString& code, PString* error)
{
	if (compile(_CSData, code, HCompileConfig(HHLSL::CSEntry, HHLSL::CSTarget), HList<HPair<PName, PName>>(), error) == false)
	{
		_bSuccess = false;
	}
	else
	{
		_bSuccess = true;
	}

	return _bSuccess;
}

//HDXC12Blob* PDX12ComputeShader::compile(const PString& filePath, const PString& sourceCode, const HCompileConfig& config, PString* outErrorMsg)
//{
//	static IDxcCompiler* pCompiler = nullptr;
//	static IDxcLibrary* pLibrary = nullptr;
//	static IDxcIncludeHandler* dxcIncludeHandler;
//
//	HRESULT hr;
//
//	if (!pCompiler)
//	{
//		(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)&pCompiler));
//		(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary));
//		(pLibrary->CreateIncludeHandler(&dxcIncludeHandler));
//	}
//
//	IDxcBlobEncoding* pTextBlob;
//	(pLibrary->CreateBlobWithEncodingFromPinned(
//		(LPBYTE)sourceCode.GetCStr(), (uint32_t)sourceCode.Length(), 0, &pTextBlob));
//
//	// Compile
//	IDxcOperationResult* pResult;
//	HRESULT hResult = pCompiler->Compile(pTextBlob, filePath.GetRawWString().c_str(), config.Entry.GetRawWString().c_str(), config.Target.GetRawWString().c_str(), nullptr, 0, nullptr, 0,
//		dxcIncludeHandler, &pResult);
//	if (pResult == nullptr)
//	{
//		JG_LOG(Graphics, ELogLevel::Error, "{0} : Failed to get shader compiler error", filePath);
//		return nullptr;
//	}
//	else
//	{
//		// Verify the result
//		HRESULT resultCode;
//		(pResult->GetStatus(&resultCode));
//		if (FAILED(resultCode))
//		{
//			IDxcBlobEncoding* pError;
//			hr = pResult->GetErrorBuffer(&pError);
//			if (FAILED(hr))
//			{
//				JG_LOG(Graphics, ELogLevel::Error, "{0} : Failed to get shader compiler error", filePath);
//			}
//
//			// Convert error blob to a string
//			HList<char> infoLog(pError->GetBufferSize() + 1);
//			memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
//			infoLog[pError->GetBufferSize()] = 0;
//
//			PString errorMsg = "Shader Compiler Error:\n";
//			errorMsg.Append(infoLog.data());
//
//			JG_LOG(Graphics, ELogLevel::Error, "{0} : Failed compile shader \n Error : {1}", filePath, errorMsg);
//			if (outErrorMsg != nullptr)
//			{
//				*outErrorMsg = errorMsg;
//			}
//		}
//
//		HDXC12Blob* pBlob;
//		(pResult->GetResult(&pBlob));
//		return pBlob;
//	}
//}

bool PDX12GraphicsShader::IsValid() const
{
	// 그래픽스 파이프라인 최소 구성: 정점 + 픽셀
	return HasByteCode(EShaderDomain::Vertex) && HasByteCode(EShaderDomain::Pixel);
}

bool PDX12GraphicsShader::Compile(const HGraphicsShaderCompileArguments& inArgs, PString* outError)
{
	// 컴파일러는 이 함수 안에서만 산다. 결과 blob은 _byteCodes로 복사하고 컴파일러는 버린다.
	PDX12GraphicsShaderCompiler compiler;
	if (compiler.Compile(inArgs.SourceCode, inArgs.Flags, inArgs.Macros, outError) == false)
	{
		return false;
	}

	_byteCodes.clear();

	storeByteCode(EShaderDomain::Vertex,   compiler.GetVSData());
	storeByteCode(EShaderDomain::Domain,   compiler.GetDSData());
	storeByteCode(EShaderDomain::Hull,     compiler.GetHSData());
	storeByteCode(EShaderDomain::Geometry, compiler.GetGSData());
	storeByteCode(EShaderDomain::Pixel,    compiler.GetPSData());

	return IsValid();
}

void PDX12GraphicsShader::SetByteCode(EShaderDomain inDomain, const void* inData, uint64 inSize)
{
	HList<uint8>& byteCode = _byteCodes[inDomain];
	byteCode.resize(inSize);

	if (inSize > 0 && inData != nullptr)
	{
		memcpy(byteCode.data(), inData, inSize);
	}
}

const HHashMap<EShaderDomain, HList<uint8>>& PDX12GraphicsShader::GetByteCodes() const
{
	return _byteCodes;
}

const HList<uint8>* PDX12GraphicsShader::GetByteCode(EShaderDomain inDomain) const
{
	auto iter = _byteCodes.find(inDomain);
	if (iter == _byteCodes.end())
	{
		return nullptr;
	}

	return &(iter->second);
}

bool PDX12GraphicsShader::HasByteCode(EShaderDomain inDomain) const
{
	const HList<uint8>* byteCode = GetByteCode(inDomain);
	return byteCode != nullptr && byteCode->empty() == false;
}

void PDX12GraphicsShader::Reset()
{
	_byteCodes.clear();
}

void PDX12GraphicsShader::storeByteCode(EShaderDomain inDomain, HDX12Blob* inBlob)
{
	// 컴파일하지 않은 스테이지(DS/HS/GS 등)는 blob이 없으므로 건너뛴다.
	if (inBlob == nullptr || inBlob->GetBufferSize() == 0)
	{
		return;
	}

	SetByteCode(inDomain, inBlob->GetBufferPointer(), inBlob->GetBufferSize());
}
