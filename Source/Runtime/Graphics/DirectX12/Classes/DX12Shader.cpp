#include "PCH/PCH.h"
#include "DX12Shader.h"
#include "DirectX12/DirectX12API.h"

bool PDX12ShaderCompiler::compile(HDX12ComPtr<HDX12Blob>& blob, const PString& sourceCode, const HCompileConfig& config, const HList<HPair<PName, PName>>& inMacros, PString* error)
{
	HList<D3D_SHADER_MACRO> d3dMacros;
	for (const HPair<PName, PName>& _pair : inMacros)
	{
		D3D_SHADER_MACRO macro;
		macro.Name = _pair.first.ToString().GetCStr();
		macro.Definition = _pair.second.ToString().GetCStr();

		d3dMacros.push_back(macro);
	}

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

	if (FAILED(hr) && error != nullptr)
	{
		*error += (char*)errorData->GetBufferPointer();
		*error += "\n";
		return false;
	}
	
	return true;
}

bool PDX12GraphicsShaderCompiler::Compile(const PString& code, EShaderCompileFlags flags, const HList<HPair<PName, PName>>& inMacros, PString* error)
{
	_flags = flags;
	
	if (_flags & EShaderCompileFlags::Allow_VertexShader)
	{
		if (compile(_VSData, code, HCompileConfig(HHLSL::VSEntry, HHLSL::VSTarget), inMacros, error) == false)
		{
			return false;
		}
	}
	if (_flags & EShaderCompileFlags::Allow_DomainShader)
	{
		if (compile(_DSData, code, HCompileConfig(HHLSL::DSEntry, HHLSL::DSTarget), inMacros, error) == false)
		{
			return false;
		}
	}
	if (_flags & EShaderCompileFlags::Allow_HullShader)
	{
		if (compile(_HSData, code, HCompileConfig(HHLSL::HSEntry, HHLSL::HSTarget), inMacros, error) == false)
		{
			return false;
		}
	}
	if (_flags & EShaderCompileFlags::Allow_GeometryShader)
	{
		if (compile(_GSData, code, HCompileConfig(HHLSL::GSEntry, HHLSL::GSTarget), inMacros, error) == false)
		{
			return false;
		}
	}
	if (_flags & EShaderCompileFlags::Allow_PixelShader)
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
