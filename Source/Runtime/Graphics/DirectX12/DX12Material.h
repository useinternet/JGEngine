#pragma once
#include "Classes/Material.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "DX12ConstantBuffer.h"
class PDX12ConstantBuffer;
class PGraphicsPipelineState;
class PDX12GraphicsShader;

// 머터리얼 = 프로퍼티 정의(상수 버퍼 레이아웃 + 텍스처 슬롯) + 머터리얼 코드가 삽입된 템플릿 셰이더.
//
// 수명
//  Initialize : 프로퍼티 레이아웃을 한 번 만든다. (상수 버퍼 오프셋, 텍스처 슬롯, 슬롯 인덱스를 CB에 기록)
//               이 뒤로 Set*/SetTexture는 컴파일 전에도 동작한다.
//  Compile    : 템플릿에 상수 버퍼 선언과 머터리얼 코드를 삽입해 VS/PS를 컴파일한다.
//               텍스처 프로퍼티 토큰은 _globalTexture[슬롯].Sample(샘플러, uv)로 치환되는데 샘플러 이름이
//               텍스처의 필터/랩 모드에서 정해지므로, 컴파일 뒤 다른 샘플러 상태의 텍스처를 넣으면 재컴파일(_bNeedCompile)이 필요하다.
//
// 렌더링 단계(PDX12GraphicsCommand::bindMaterial)
//  GetTextures()/GetTextureCubes() -> 디스크립터 테이블, GetConstantBuffer() -> 루트 CBV, GetShader() -> PSO
class PDX12Material
	: public IRawMaterial
{
	PName _name;
	mutable bool _bNeedCompile;
	EMaterialDomain _domain = EMaterialDomain::Surface;

	// 컴파일된 셰이더. 바이트코드는 이 객체가 소유하고 머터리얼은 참조만 한다.
	PSharedPtr<PDX12GraphicsShader> _graphicsShader;

	PString _shaderCode;
	PString _fullShaderCode;

	HMaterialPropertyDefinitionist        _propertyDefinitionist;
	PSharedPtr<PGraphicsPipelineState>    _graphicsPSO;
	PSharedPtr<PDX12ConstantBuffer>       _materialConstantBuffer;
	HHashMap<PName, uint64>               _materialConstantDataOffsetMap;
	HList<PName>					      _materialConstantPropertyList;

	// 텍스처 슬롯. 인덱스 = 셰이더 _globalTexture[] / _globalTextureCube[] 의 인덱스 = CB에 기록된 int 값.
	PSharedPtr<IRawTexture>               _defaultTexture;
	HList<PSharedPtr<IRawTexture>>        _materialTextures;
	HList<PSharedPtr<IRawTexture>>        _materialTextureCubes;
	HHashMap<PName, uint64>               _materialTextureNameMap;
	HHashMap<PName, uint64>               _materialTextureCubeNameMap;

public:
	PDX12Material();
	virtual ~PDX12Material() = default;
public:
	// inDefaultTexture: 비어 있는 Texture 슬롯을 채우는 대체 텍스처 (PJGGraphicsAPI::GetDefaultTexture)
	void Initialize(const HRawMaterialConstructArguments& inArgs, PSharedPtr<IRawTexture> inDefaultTexture);

	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;

	virtual const PString& GetShaderCode() const override;
	virtual const PString& GetFullShaderCode() const override;
	virtual EMaterialDomain GetDomain() const override;

	virtual const HMaterialPropertyDefinitionist& GetPropertyDefinitionist() const override;

	virtual bool SetBool(const PName& inName, const bool& inValue) override;
	virtual bool SetInt(const PName& inName, const int32& inValue) override;
	virtual bool SetFloat(const PName& inName, const float32& inValue)  override;
	virtual bool SetFloat2(const PName& inName, const HVector2& inValue) override;
	virtual bool SetFloat3(const PName& inName, const HVector3& inValue) override;
	virtual bool SetFloat4(const PName& inName, const HVector4& inValue) override;
	virtual bool SetMatrix(const PName& inName, const HMatrix& inMatrix) override;
	virtual bool SetTexture(const PName& inName, PSharedPtr<IRawTexture> inValue) override;

	virtual bool GetBool(const PName& inName, bool& outValue) const override;
	virtual bool GetInt(const PName& inName, int32& outValue) const override;
	virtual bool GetFloat(const PName& inName, float32& outValue)   const override;
	virtual bool GetFloat2(const PName& inName, HVector2& outValue) const override;
	virtual bool GetFloat3(const PName& inName, HVector3& outValue) const override;
	virtual bool GetFloat4(const PName& inName, HVector4& outValue) const override;
	virtual bool GetMatrix(const PName& inName, HMatrix& outValue) const override;
	virtual bool GetTexture(const PName& inName, PSharedPtr<IRawTexture>& outValue) const override;

	virtual bool Compile(const HMaterialCompileArguments& inArgs) override;

	virtual HList<PSharedPtr<IRawTexture>> GetTextures() const override;
	virtual HList<PSharedPtr<IRawTexture>> GetTextureCubes() const override;

	virtual bool IsValid() const override;
	virtual PWeakPtr<IConstantBuffer> GetConstantBuffer() const override;
	virtual PWeakPtr<IRawGraphicsShader> GetShader() const override;
private:
	// 프로퍼티 정의 -> 상수 버퍼 오프셋 맵 + 텍스처 슬롯. Initialize에서 한 번.
	bool buildPropertyLayout();
	bool generateShaderCode();
	bool compileShader(const HMaterialCompileArguments& inArgs);
	// 프로퍼티 총 크기를 256바이트 단위로 올린 상수 버퍼 크기 (최소 256). 루트 CBV와 디스크립터 CBV 모두 만족.
	uint64 getConstantBufferSize() const;

	// 머터리얼 코드에서 식별자 토큰(앞뒤가 식별자 문자가 아닌 경우)만 치환한다. 부분 일치("Albedo" in "AlbedoTex")는 건드리지 않는다.
	static PString replaceIdentifier(const PString& inSource, const PString& inIdentifier, const PString& inReplacement);

	template<class T>
	bool setData(const PName& inName, const T& inData)
	{
		if (_materialConstantDataOffsetMap.contains(inName) == false)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Set Data", GetName(), inName);
			return false;
		}

		uint64 dataOffset = _materialConstantDataOffsetMap.at(inName);
		_materialConstantBuffer->SetData(&inData, dataOffset, sizeof(T));

		return true;
	}

	template<class T>
	bool getData(const PName& inName, T& outData) const
	{
		if (_materialConstantDataOffsetMap.contains(inName) == false)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : %s Fail Get Data", GetName(), inName);
			return false;
		}

		uint64 dataOffset = _materialConstantDataOffsetMap.at(inName);
		void* data = _materialConstantBuffer->GetData(dataOffset);

		memcpy(&outData, data, sizeof(T));

		return true;
	}

	void getSampleStateName(PSharedPtr<IRawTexture> inTexture, PString& outSampleStateName) const;
};
