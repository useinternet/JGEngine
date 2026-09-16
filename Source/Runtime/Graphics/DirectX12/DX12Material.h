#pragma once
#include "Classes/Material.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "DX12ConstantBuffer.h"
class PDX12ConstantBuffer;
class PGraphicsPipelineState;
class PDX12GraphicsShader;

class PDX12Material
	: public IMemoryObject
	, public IRawMaterial
{
	// 구현 할거
	// 데이터 셋팅
	// PSO 에 Shader 바인딩
	// 하고 임포트할수있는 간단한 거 준비해서 렌더링해보기

	// 미리 Shader 컴파일

	PName _name;
	mutable bool _bNeedCompile;
	EMaterialDomain _domain = EMaterialDomain::Surface;


	// 컴파일된 셰이더. 바이트코드는 이 객체가 소유하고 머터리얼은 참조만 한다.
	PSharedPtr<PDX12GraphicsShader> _graphicsShader;

	PString _shaderCode;
	PString _fullShaderCode;


	// 아래 데이터만 있으면 동작하도록 구성
	HMaterialPropertyDefinitionist        _propertyDefinitionist;
	PSharedPtr<PGraphicsPipelineState>    _graphicsPSO;
	PSharedPtr<PDX12ConstantBuffer>       _materialConstantBuffer;
	HHashMap<PName, uint64>               _materialConstantDataOffsetMap;
	HList<PSharedPtr<IRawTexture>>        _materialTextures;
	HList<PSharedPtr<IRawTexture>>        _materialTextureCubes;
	HHashMap<PName, uint64> _materialTextureNameMap;
	HHashMap<PName, uint64> _materialTextureCubeNameMap;
	
	
	HList<PName>					      _materialConstantPropertyList;

	// 머터리얼 컴파일 단계
	// 템플릿 셰이더 코드 가져옴
	// 소스코드 삽입
	// 매크로 삽입
	// 셰이더 컴파일


	// 렌더링 단계
	// Scene에서 통합 렌더링 전용 RootSignature 바인드
	// Scene에서 텍스쳐 인덱스 할당
	// 리소스 및 공용 버퍼 바인드
	// --------------------------- 반복
	// 파이프라인 바인드
	// 머터리얼 상수버퍼 바인드
	// Mesh 바인드
	// 렌더링
	//---------------------------

public:
	PDX12Material();
	virtual ~PDX12Material() = default;
public:
	void Initialize(const HRawMaterialConstructArguments& inArgs);

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
	
	virtual bool IsValid() const override;
	virtual PWeakPtr<IConstantBuffer> GetConstantBuffer() const override;
	virtual PWeakPtr<IRawGraphicsShader> GetShader() const override;
private:
	bool updateMaterialConstantData();
	bool generateShaderCode();
	bool compileShader(const HMaterialCompileArguments& inArgs);
	// 프로퍼티 총 크기를 256바이트 단위로 올린 상수 버퍼 크기 (최소 256). 루트 CBV와 디스크립터 CBV 모두 만족.
	uint64 getConstantBufferSize() const;

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
