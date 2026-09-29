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
	_graphicsShader = Allocate<PDX12GraphicsShader>();

	_shaderCode.Reset();
	_fullShaderCode.Reset();
	_propertyDefinitionist.Reset();

	_bNeedCompile = true;
}

void PDX12Material::Initialize(const HRawMaterialConstructArguments& inArgs)
{
	_domain = inArgs.Domain;
	_propertyDefinitionist = inArgs.PropertyDefinitionist;

	// 상수 버퍼를 만들기 전에 이름을 준다. (D3D 리소스 이름이 이 이름으로 붙는다)
	_name = inArgs.Name;
	_materialConstantBuffer->SetName(inArgs.Name);

	if (buildPropertyLayout() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Build Property Layout", inArgs.Name);
	}

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

EMaterialDomain PDX12Material::GetDomain() const
{
	return _domain;
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

	HList<PSharedPtr<IRawTexture>>* textures = nullptr;
	PSharedPtr<IRawTexture> fallbackTexture;
	uint64 slot = 0;
	if (_materialTextureNameMap.contains(inName))
	{
		textures = &_materialTextures;
		fallbackTexture = HDirectXAPI::GetDefaultTexture();
		slot = _materialTextureNameMap.at(inName);
	}
	else if (_materialTextureCubeNameMap.contains(inName))
	{
		textures = &_materialTextureCubes;
		slot = _materialTextureCubeNameMap.at(inName);
	}
	else
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : %s is not a Texture/TextureCube property", GetName(), inName);
		return false;
	}

	PSharedPtr<IRawTexture> newTexture = (inValue.IsValid() && inValue->IsValid()) ? inValue : fallbackTexture;

	// 샘플러 이름(필터/랩 모드)은 컴파일 시점에 셰이더 코드에 박힌다. 달라지면 재컴파일이 필요하다.
	PString oldSampleStateName;
	PString newSampleStateName;
	getSampleStateName((*textures)[slot], oldSampleStateName);
	getSampleStateName(newTexture, newSampleStateName);

	(*textures)[slot] = newTexture;

	if (oldSampleStateName.Equal(newSampleStateName) == false)
	{
		_bNeedCompile = true;
	}

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

	if (_materialTextureNameMap.contains(inName))
	{
		outValue = _materialTextures[_materialTextureNameMap.at(inName)];
		return true;
	}

	if (_materialTextureCubeNameMap.contains(inName))
	{
		outValue = _materialTextureCubes[_materialTextureCubeNameMap.at(inName)];
		return true;
	}

	JG_LOG(Graphics, ELogLevel::Error, "%s : %s is not a Texture/TextureCube property", GetName(), inName);
	return false;
}

bool PDX12Material::Compile(const HMaterialCompileArguments& inArgs)
{
	// 같은 코드로 이미 컴파일했고 재컴파일 사유(_bNeedCompile)도 없으면 그대로 쓴다.
	if (_bNeedCompile == false && _shaderCode.Equal(inArgs.ShaderCode))
	{
		return true;
	}

	if (IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, Invalid Material", GetName());
		return false;
	}

	_shaderCode = inArgs.ShaderCode;

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
}

HList<PSharedPtr<IRawTexture>> PDX12Material::GetTextures() const
{
	return _materialTextures;
}

HList<PSharedPtr<IRawTexture>> PDX12Material::GetTextureCubes() const
{
	return _materialTextureCubes;
}

bool PDX12Material::IsValid() const
{
	return _materialConstantBuffer.IsValid() && _materialConstantBuffer->IsValid();
}

PWeakPtr<IConstantBuffer> PDX12Material::GetConstantBuffer() const
{
	return _materialConstantBuffer;
}

PWeakPtr<IRawGraphicsShader> PDX12Material::GetShader() const
{
	return _graphicsShader;
}

bool PDX12Material::buildPropertyLayout()
{
	// 상수 버퍼 확보(256바이트 정렬). 이후 Set*/SetTexture가 컴파일 전에도 동작해야 하므로 여기서 한 번만 만든다.
	_materialConstantBuffer->SetData(nullptr, getConstantBufferSize());
	if (_materialConstantBuffer->IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Create Material Constant Buffer", GetName());
		return false;
	}

	// 새 업로드 버퍼의 내용은 정해져 있지 않으므로 0으로 채운다.
	memset(_materialConstantBuffer->GetData(), 0, _materialConstantBuffer->GetDataSize());

	_materialConstantDataOffsetMap.clear();
	_materialConstantPropertyList.clear();
	_materialTextures.clear();
	_materialTextureCubes.clear();
	_materialTextureNameMap.clear();
	_materialTextureCubeNameMap.clear();

	// 빈 Texture 슬롯의 대체 텍스처. API 소유 리소스라 멤버로 들지 않고 필요할 때 접근자로 얻는다.
	// (기본 머터리얼을 만드는 시점에도 기본 텍스처는 먼저 만들어져 있다. createDefaultResources 순서)
	PSharedPtr<IRawTexture> defaultTexture = HDirectXAPI::GetDefaultTexture();
	if (defaultTexture.IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Warning, "%s : Default texture is not available. Empty texture slots stay null until SetTexture", GetName());
	}

	// 오프셋은 정의 순서대로 쌓는다. 16바이트 경계 패딩은 정의기(HMaterialPropertyDefinitionist::Define)가 넣어 둔 상태.
	uint64 dataOffset = 0;

	const HList<HMaterialDefineInfo>& defineInfos = _propertyDefinitionist.GetDefineInfos();
	for (const HMaterialDefineInfo& info : defineInfos)
	{
		if (info.Type == EMaterialPropertyType::Unknown)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : Invalid Property(%s)", GetName(), info.Name);
			return false;
		}

		_materialConstantDataOffsetMap[info.Name] = dataOffset;
		_materialConstantPropertyList.push_back(info.Name);
		dataOffset += HMaterialProperty::GetPropertySize(info.Type);

		// 텍스처 프로퍼티는 슬롯을 할당하고 슬롯 인덱스를 CB의 int 필드에 기록한다. 셰이더는 _globalTexture[슬롯]으로 읽는다.
		if (info.Type == EMaterialPropertyType::Texture)
		{
			const int32 slot = (int32)_materialTextures.size();
			_materialTextureNameMap[info.Name] = (uint64)slot;
			_materialTextures.push_back(defaultTexture);
			setData(info.Name, slot);
		}
		else if (info.Type == EMaterialPropertyType::TextureCube)
		{
			const int32 slot = (int32)_materialTextureCubes.size();
			_materialTextureCubeNameMap[info.Name] = (uint64)slot;
			_materialTextureCubes.push_back(nullptr); // 기본 큐브 텍스처는 아직 없다.
			setData(info.Name, slot);
		}
	}

	return true;
}

bool PDX12Material::generateShaderCode()
{
	PString fullShaderCode = GShaderLibrary::GetInstance().GetGraphicsShaderTemplateCode();
	if (fullShaderCode.Empty())
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, Shader Template(%s) Not Found", GetName(), GShaderLibrary::GraphicsShaderTemplate);
		return false;
	}

	PString constantBufferShaderCode;
	PString materialShaderCode = _shaderCode;

	const HList<HMaterialDefineInfo>& defineInfos = _propertyDefinitionist.GetDefineInfos();
	for (const HMaterialDefineInfo& info : defineInfos)
	{
		// 상수 버퍼 선언. 텍스처 프로퍼티는 슬롯 인덱스(int)로 들어간다.
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

		// 머터리얼 코드의 텍스처 토큰을 샘플 식으로 치환한다. (예: "Albedo" -> "_globalTexture[Albedo].Sample(_LinearWrap_, _input.tex)")
		if (HMaterialProperty::IsResourceProperty(info.Type))
		{
			const PString propertyName = info.Name.ToString();
			PString sampleStateName;

			if (info.Type == EMaterialPropertyType::Texture)
			{
				const uint64 slot = _materialTextureNameMap.at(info.Name);
				getSampleStateName(_materialTextures[slot], sampleStateName);
				materialShaderCode = replaceIdentifier(materialShaderCode, propertyName,
					PString::Format("_globalTexture[%s].Sample(%s, _input.tex)", propertyName, sampleStateName));
			}
			else
			{
				const uint64 slot = _materialTextureCubeNameMap.at(info.Name);
				getSampleStateName(_materialTextureCubes[slot], sampleStateName);
				materialShaderCode = replaceIdentifier(materialShaderCode, propertyName,
					PString::Format("_globalTextureCube[%s].Sample(%s, _input.local_position)", propertyName, sampleStateName));
			}
		}
	}

	// 프로퍼티가 없으면 cbuffer가 비게 되므로 패딩 하나를 둔다. (상수 버퍼 최소 크기 256바이트와도 맞는다)
	if (constantBufferShaderCode.Empty())
	{
		constantBufferShaderCode.AppendLine("\tfloat4 _MaterialPadding;");
	}

	// 활성 도메인 자리에 머터리얼 코드를 넣고, 비활성 도메인 자리는 비운다.
	const bool bScreenDomain = (_domain == EMaterialDomain::Screen);
	fullShaderCode.ReplaceAll(GShaderLibrary::MaterialConstantBufferContentsScript, constantBufferShaderCode);
	fullShaderCode.ReplaceAll(GShaderLibrary::MaterialSurfaceContentsScript, bScreenDomain ? PString() : materialShaderCode);
	fullShaderCode.ReplaceAll(GShaderLibrary::MaterialScreenContentsScript,  bScreenDomain ? materialShaderCode : PString());

	_fullShaderCode = std::move(fullShaderCode);

	return true;
}

bool PDX12Material::compileShader(const HMaterialCompileArguments& inArgs)
{
	HGraphicsShaderCompileArguments compileArgs;
	compileArgs.SourceCode = GetFullShaderCode();
	compileArgs.Flags      = EShaderCompileFlags::Allow_VertexShader | EShaderCompileFlags::Allow_PixelShader;
	// 템플릿의 #if MATERIAL_DOMAIN_SCREEN 분기 선택. Surface는 0으로 명시해 미정의 매크로에 의존하지 않는다.
	compileArgs.Macros.push_back(HPair<PName, PName>(PName("MATERIAL_DOMAIN_SCREEN"), PName(_domain == EMaterialDomain::Screen ? "1" : "0")));

	PString errorCode;
	if (_graphicsShader->Compile(compileArgs, &errorCode) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile, %s", GetName(), errorCode);
		return false;
	}

	return true;
}

uint64 PDX12Material::getConstantBufferSize() const
{
	const uint64 alignByteSize = 256;
	uint64 totalPropertyByteSize = _propertyDefinitionist.GetTotalDataSize();
	if (totalPropertyByteSize == 0)
	{
		// 프로퍼티가 없어도 0바이트 버퍼는 만들 수 없다. 셰이더 쪽 _MaterialPadding(float4)과 짝.
		totalPropertyByteSize = alignByteSize;
	}
	return HMath::AlignUp(totalPropertyByteSize, alignByteSize);
}

PString PDX12Material::replaceIdentifier(const PString& inSource, const PString& inIdentifier, const PString& inReplacement)
{
	const uint64 sourceLength     = inSource.Length();
	const uint64 identifierLength = inIdentifier.Length();
	if (identifierLength == 0 || sourceLength == 0)
	{
		return inSource;
	}

	auto isIdentifierChar = [](char c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
	};

	PString result;
	uint64 cursor = 0;
	while (cursor < sourceLength)
	{
		const uint64 pos = inSource.Find(inIdentifier, cursor);
		if (pos == PString::NPOS)
		{
			PString rest;
			inSource.SubString(&rest, cursor);
			result.Append(rest);
			break;
		}

		// 토큰 앞뒤가 식별자 문자가 아니어야 온전한 토큰이다.
		const bool bLeftBoundary  = (pos == 0) || (isIdentifierChar(inSource[pos - 1]) == false);
		const bool bRightBoundary = (pos + identifierLength >= sourceLength) || (isIdentifierChar(inSource[pos + identifierLength]) == false);

		PString chunk;
		inSource.SubString(&chunk, cursor, pos - cursor);
		result.Append(chunk);

		if (bLeftBoundary && bRightBoundary)
		{
			result.Append(inReplacement);
		}
		else
		{
			result.Append(inIdentifier);
		}

		cursor = pos + identifierLength;
	}

	return result;
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
