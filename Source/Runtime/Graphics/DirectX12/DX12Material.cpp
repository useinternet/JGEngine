#include "PCH/PCH.h"
#include "DX12Material.h"
#include "DirectX12API.h"
#include "Classes/DX12Shader.h"
#include "Classes/ShaderLibrary.h"
#include "Classes/Texture.h"
#include "Classes/PipelineState.h"

PDX12Material::PDX12Material()
{
	_graphicsPSO = Allocate<PGraphicsPipelineState>();
	_materialConstantBuffer = Allocate<PDX12ConstantBuffer>();
	_graphicsShader = Allocate<PDX12GraphicsShaderCompiler>();

	_shaderCode.Reset();
	_fullShaderCode.Reset();
	_propertyDefinitionist.Reset();

	_shaderBtDatas.clear();
	_bNeedCompile = true;
}

void PDX12Material::Initialize(const HRawMaterialConstructArguments& inArgs)
{
	_propertyDefinitionist = inArgs.PropertyDefinitionist;

	uint64 totalPropertyByteSize = _propertyDefinitionist.GetTotalDataSize();
	_materialConstantBuffer->SetData(nullptr, totalPropertyByteSize);

	SetName(inArgs.Name);

	_bNeedCompile = true;
}

const PName& PDX12Material::GetName() const
{
	return _name;
}

void PDX12Material::SetName(const PName& inName)
{
	_name = inName;
	if (IsValid() && _materialConstantBuffer->IsValid())
	{
		_materialConstantBuffer->SetName(inName);
	}
}

const PString& PDX12Material::GetShaderCode() const
{
	return _shaderCode;
}

const PString& PDX12Material::GetFullShaderCode() const
{
	return _fullShaderCode;
}

const HMaterialPropertyDefinitionist& PDX12Material::GetPropertyDefinitionist() const
{
	return _propertyDefinitionist;
}

bool PDX12Material::SetBool(const PName& inName, const bool& inValue) 
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Bool", GetName(), inName);
		return false;
	}

	return setData(inName, inValue);
}

bool PDX12Material::SetInt(const PName& inName, const int32& inValue) 
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Int", GetName(), inName);
		return false;
	}

	return setData(inName, inValue);
}

bool PDX12Material::SetFloat(const PName& inName, const float32& inValue) 
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Float", GetName(), inName);
		return false;
	}

	return setData(inName, inValue);
}

bool PDX12Material::SetFloat2(const PName& inName, const HVector2& inValue) 
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Float2", GetName(), inName);
		return false;
	}

	return setData(inName, inValue);
}

bool PDX12Material::SetFloat3(const PName& inName, const HVector3& inValue) 
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Float3", GetName(), inName);
		return false;
	}

	return setData(inName, inValue);
}

bool PDX12Material::SetFloat4(const PName& inName, const HVector4& inValue) 
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Float4", GetName(), inName);
		return false;
	}

	return setData(inName, inValue);
}

bool PDX12Material::SetMatrix(const PName& inName, const HMatrix& inValue)
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Matrix", GetName(), inName);
		return false;
	}

	return setData(inName, inValue);
}

bool PDX12Material::SetTexture(const PName& inName, PSharedPtr<IRawTexture> inValue) 
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Texture", GetName(), inName);
		return false;
	}

	_bNeedCompile = true;

	JG_LOG(Graphics, ELogLevel::Error, "%s : %s Not yet implemented.", GetName(), inName);
	return true;
}

bool PDX12Material::GetBool(const PName& inName, bool& outValue) const
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Bool", GetName(), inName);
		return false;
	}

	return getData(inName, outValue);
}

bool PDX12Material::GetInt(const PName& inName, int32& outValue) const
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Int", GetName(), inName);
		return false;
	}

	return getData(inName, outValue);
}

bool PDX12Material::GetFloat(const PName& inName, float32& outValue) const
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Float", GetName(), inName);
		return false;
	}

	return getData(inName, outValue);
}

bool PDX12Material::GetFloat2(const PName& inName, HVector2& outValue) const
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Float2", GetName(), inName);
		return false;
	}

	return getData(inName, outValue);
}

bool PDX12Material::GetFloat3(const PName& inName, HVector3& outValue) const
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Float3", GetName(), inName);
		return false;
	}

	return getData(inName, outValue);
}

bool PDX12Material::GetFloat4(const PName& inName, HVector4& outValue) const
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Float4", GetName(), inName);
		return false;
	}

	return getData(inName, outValue);
}

bool PDX12Material::GetMatrix(const PName& inName, HMatrix& outValue) const
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Matrix", GetName(), inName);
		return false;
	}

	return getData(inName, outValue);
}

bool PDX12Material::GetTexture(const PName& inName, PSharedPtr<IRawTexture>& outValue) const
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Texture", GetName(), inName);
		return false;
	}

	JG_LOG(Graphics, ELogLevel::Error, "%s : %s Not yet implemented.", GetName(), inName);
	return false;
}

bool PDX12Material::Compile(const HMaterialCompileArguments& inArgs)
{
	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, Invalid Material", GetName());
		return false;
	}

	_shaderCode = inArgs.ShaderCode;

	if (updateMaterialConstantData() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, Fail Update Material Constant Data", GetName());
		return false;
	}
	
	if (generateShaderCode() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, Fail Generate Shader Code", GetName());
		return false;
	}

	if (compileShader(inArgs) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile Shader", GetName());
		return false;
	}

	_bNeedCompile = false;
	return true;

	// 16에 맞춰야함
	// 1. Texture, TextureCube 슬롯 만들기
	// 2. 변수 슬롯 만들기
	// 3. 상수 버퍼 생성
	// 4. Template Code에 Source Code 붙히기
	// 5. Texture, 나 TextureCube 변수는 Global Texture 배열 접근으로 변경
	// 6. FullSourceCode 완성

	// 셰이더 컴파일
	// 바이트 형태로 저장
	// 컴파일은 수동이고, 매크로마다 바이트 형태로 저장하고있다.
	// 머터리얼은 바이트 형태로 저장

	return false;
}

bool PDX12Material::IsValid() const
{
	return _shaderBtDatas.empty();
}

bool PDX12Material::updateMaterialConstantData()
{
	uint64 totalPropertyByteSize = _propertyDefinitionist.GetTotalDataSize();

	_materialConstantBuffer->SetData(nullptr, totalPropertyByteSize);
	JG_CHECK(_materialConstantBuffer->IsValid());

	_materialConstantDataOffsetMap.clear();

	const uint32 alignByteSize = 16;
	uint64 dataOffset = 0;

	const HList<HMaterialDefineInfo>& defineInfos = _propertyDefinitionist.GetDefineInfos();
	uint64 propertyDefineDataSize = _propertyDefinitionist.GetTotalDataSize();
	JG_CHECK(HMath::IsAligned(propertyDefineDataSize, alignByteSize));

	for (const HMaterialDefineInfo& info : defineInfos)
	{
		if (info.Type == EMaterialPropertyType::Unknown)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, Invalid Property(%s)", GetName(), info.Name);
			return false;
		}

		_materialConstantDataOffsetMap[info.Name] = dataOffset;
		_materialConstantPropertyList.push_back(info.Name);
		dataOffset += HMaterialProperty::GetPropertySize(info.Type);
	}

	return true;
}

bool PDX12Material::generateShaderCode()
{
	PString templateShaderCode = GShaderLibrary::GetInstance().GetGraphicsShaderTemplateCode();
	PString constantBufferShaderCode;
	PString materialShaderCode = _shaderCode;
	PString fullShaderCode = std::move(templateShaderCode);

	// 상수버퍼 코드 작성

	// 머터리얼 Shader Code 컨버팅
	const HList<HMaterialDefineInfo>& defineInfos = _propertyDefinitionist.GetDefineInfos();
	for (const HMaterialDefineInfo& info : defineInfos)
	{
		switch (info.Type)
		{
		case EMaterialPropertyType::Bool: constantBufferShaderCode.AppendLine(PString::Format("\tbool %s;", info.Name)); break;
		case EMaterialPropertyType::Texture:
		case EMaterialPropertyType::TextureCube:
		case EMaterialPropertyType::Int:    constantBufferShaderCode.AppendLine(PString::Format("\tint %s;", info.Name)); break;
		case EMaterialPropertyType::Float:  constantBufferShaderCode.AppendLine(PString::Format("\tfloat %s;", info.Name)); break;
		case EMaterialPropertyType::Float2: constantBufferShaderCode.AppendLine(PString::Format("\tfloat2 %s;", info.Name)); break;
		case EMaterialPropertyType::Float3: constantBufferShaderCode.AppendLine(PString::Format("\tfloat3 %s;", info.Name)); break;
		case EMaterialPropertyType::Float4: constantBufferShaderCode.AppendLine(PString::Format("\tfloat4 %s;", info.Name)); break;
		case EMaterialPropertyType::Matrix: constantBufferShaderCode.AppendLine(PString::Format("\tfloat4x4 %s;", info.Name)); break;
		default:
			JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, Invalid Property(%s)", GetName(), info.Name);
			return false;
		}

		if (HMaterialProperty::IsResourceProperty(info.Type))
		{
			PString sampleStateName;

			getSampleStateName(_materialTextures[info.Name], sampleStateName);

			switch(info.Type)
			{
			case EMaterialPropertyType::Texture:
				materialShaderCode.ReplaceAll(info.Name.ToString() + " ",
					PString::Format("_globalTexture[%s].Sample(%s, _input.tex)", info.Name, sampleStateName));
				break;
			case EMaterialPropertyType::TextureCube:
				materialShaderCode.ReplaceAll(info.Name.ToString() + " ",
					PString::Format("_globalTextureCube[%s].Sample(%s, _input.local_position)", info.Name, sampleStateName));
				break;
			}
		}
	}

	fullShaderCode.ReplaceAll(GShaderLibrary::MaterialConstantBufferContentScript, constantBufferShaderCode);
	fullShaderCode.ReplaceAll(GShaderLibrary::MaterialSurfaceContentScript, materialShaderCode);

	_fullShaderCode = std::move(fullShaderCode);

	return true;
}

bool PDX12Material::compileShader(const HMaterialCompileArguments& inArgs)
{
	PString errorCode;

	// 머터리얼 옵션에 따른 디파인 주기 ( 수동 )
	// 조합별 바이트 코드 가지고있기

	_graphicsShader->Compile(GetFullShaderCode(), EShaderCompileFlags::Allow_VertexShader | EShaderCompileFlags::Allow_PixelShader, HList<HPair<PName, PName>>(), &errorCode);
	if (_graphicsShader->IsSuccessed() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, %s", GetName(), errorCode);
		return false;
	}

	uint64 vsDataSize = _graphicsShader->GetVSData()->GetBufferSize();
	uint64 psDataSize = _graphicsShader->GetPSData()->GetBufferSize();

	_shaderBtDatas[EShaderDomain::Vertex].resize(vsDataSize);
	_shaderBtDatas[EShaderDomain::Pixel].resize(psDataSize);

	memcpy(_shaderBtDatas[EShaderDomain::Vertex].data(), _graphicsShader->GetVSData()->GetBufferPointer(), vsDataSize);
	memcpy(_shaderBtDatas[EShaderDomain::Pixel].data(), _graphicsShader->GetPSData()->GetBufferPointer(), psDataSize);

	return true;
}

void PDX12Material::getSampleStateName(PSharedPtr<IRawTexture> inTexture, PString& outSampleStateName) const
{
	PName wrapMode;
	PName filterMode;

	if (inTexture == nullptr || inTexture->IsValid() == false)
	{
		wrapMode = StaticEnum<ETextureWrapMode>()->GetEnumNameByValue((int32)ETextureWrapMode::Wrap);
		filterMode = StaticEnum<ETextureFilterMode>()->GetEnumNameByValue((int32)ETextureFilterMode::Anisotropic);
	}
	else
	{
		const HTextureInfo& textureInfo = inTexture->GetTextureInfo();
		wrapMode = StaticEnum<ETextureWrapMode>()->GetEnumNameByValue((int32)textureInfo.WrapMode);
		filterMode = StaticEnum<ETextureFilterMode>()->GetEnumNameByValue((int32)textureInfo.FilterMode);
	}

	outSampleStateName = PString::Format("_%s%s_", filterMode, wrapMode);
}
