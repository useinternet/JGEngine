#pragma once
#include "Classes/Material.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "DX12ConstantBuffer.h"
class PDX12ConstantBuffer;
class PGraphicsPipelineState;
class PDX12GraphicsShaderCompiler;

struct HDX12CompileConfig
{
	PString Entry;
	PString Target;
	HDX12CompileConfig(const PString& entry, const PString& target) : Entry(entry), Target(target) {}
};

class PDX12Material
	: public IMemoryObject
	, public IRawMaterial
{
	// ���� �Ұ�
	// ������ ����
	// PSO �� Shader ���ε�
	// �ϰ� ����Ʈ�Ҽ��ִ� ������ �� �غ��ؼ� �������غ���

	// �̸� Shader ������

	PName _name;
	mutable bool _bNeedCompile;


	// �ӽ� ������ ( ������ �� )
	PSharedPtr<PDX12GraphicsShaderCompiler> _graphicsShader;

	PString _shaderCode;
	PString _fullShaderCode;


	// �Ʒ� �����͸� ������ �����ϵ��� ����
	HMaterialPropertyDefinitionist  _propertyDefinitionist;
	PSharedPtr<PGraphicsPipelineState>    _graphicsPSO;
	PSharedPtr<PDX12ConstantBuffer>       _materialConstantBuffer;
	HHashMap<PName, uint64>               _materialConstantDataOffsetMap;
	HHashMap<PName, PSharedPtr<IRawTexture>> _materialTextures;
	HList<PName>					      _materialConstantPropertyList;

	// ���߿� �����ϵ� ����Ʈ ������ �����ͼ� �ٷ� �����Ҽ��ֵ��� ���� �����Դϴ�.
	HHashMap<EShaderDomain, HList<uint8>> _shaderBtDatas;

	// ���͸��� ������ �ܰ�
	// ���ø� ���̴� �ڵ� ������
	// �ҽ��ڵ� ����
	// ��ũ�� ����
	// ���̴� ������


	// ������ �ܰ�
	// Scene���� ���� ������ ���� RootSignature ���ε�
	// Scene���� �ؽ��� �ε��� �Ҵ�
	// ���ҽ� �� ���� ���� ���ε�
	// --------------------------- �ݺ�
	// ���������� ���ε�
	// ���͸��� ������� ���ε�
	// Mesh ���ε�
	// ������
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

	virtual bool IsValid() const override;
private:
	bool updateMaterialConstantData();
	bool generateShaderCode();
	bool compileShader(const HMaterialCompileArguments& inArgs);

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
