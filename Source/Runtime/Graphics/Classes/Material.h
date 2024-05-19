#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"


enum class GRAPHICS_API EMaterialDomain
{
	Surface,
	Scene,
};

struct GRAPHICS_API HMaterialCompileArguments
{
	PString ShaderCode;
};

enum class GRAPHICS_API EMaterialPropertyType
{
	Unknown,
	Bool,
	Int,
	Float,
	Float2,
	Float3,
	Float4,
	Matrix,
	Texture,
	TextureCube,
};

struct GRAPHICS_API HMaterialDefineInfo
{
	PName Name = NAME_NONE;
	EMaterialPropertyType Type = EMaterialPropertyType::Unknown;
};

namespace HMaterialProperty
{
	inline uint64 GetPropertySize(EMaterialPropertyType inPropertyType)
	{
		switch (inPropertyType)
		{
		case EMaterialPropertyType::Bool: return sizeof(bool);
		case EMaterialPropertyType::Int: return sizeof(int);
		case EMaterialPropertyType::Float: return sizeof(float);
		case EMaterialPropertyType::Float2: return sizeof(HVector2);
		case EMaterialPropertyType::Float3: return sizeof(HVector3);
		case EMaterialPropertyType::Float4: return sizeof(HVector4);
		case EMaterialPropertyType::Matrix: return sizeof(HMatrix);
		case EMaterialPropertyType::Texture:
		case EMaterialPropertyType::TextureCube:
			return sizeof(int);
		default: return 0;
		}
	}

	inline uint64 IsResourceProperty(EMaterialPropertyType inPropertyType)
	{
		return inPropertyType == EMaterialPropertyType::Texture || inPropertyType == EMaterialPropertyType::TextureCube;
	}
}

class IRawTexture;
class GRAPHICS_API HMaterialPropertyDefinitionist : public IMemoryObject
{
	HList<HMaterialDefineInfo> _defineInfos;
	uint64 _dataSize = 0;
public:
	void DefineBool(const PName& inName)   { Define(inName, EMaterialPropertyType::Bool); }
	void DefineInt(const PName& inName)    { Define(inName, EMaterialPropertyType::Int); }
	void DefineFloat(const PName& inName)  { Define(inName, EMaterialPropertyType::Float); }
	void DefineFloat2(const PName& inName) { Define(inName, EMaterialPropertyType::Float2); }
	void DefineFloat3(const PName& inName) { Define(inName, EMaterialPropertyType::Float3); }
	void DefineFloat4(const PName& inName) { Define(inName, EMaterialPropertyType::Float4); }
	void DefineMatrix(const PName& inName) { Define(inName, EMaterialPropertyType::Matrix); }
	void DefineTexture(const PName& inName) { Define(inName, EMaterialPropertyType::Texture); }
	void DefineTextureCube(const PName& inName) { Define(inName, EMaterialPropertyType::TextureCube); }
	void Reset() { _defineInfos.clear(); _dataSize = 0; }

	const HList<HMaterialDefineInfo>& GetDefineInfos() const { return _defineInfos; }
	const uint64 GetTotalDataSize() const { return _dataSize; }
private:
	void Define(const PName& inName, EMaterialPropertyType inType)
	{
		HMaterialDefineInfo defineInfo;
		defineInfo.Name = inName;
		defineInfo.Type = inType;

		const uint32 alignByteSize = 16;
		uint64 propertySize = HMaterialProperty::GetPropertySize(inType);
		uint64 restSize   = _dataSize % alignByteSize;

		if ((restSize + propertySize) > alignByteSize)
		{
			const uint64 floatPropertySize = HMaterialProperty::GetPropertySize(EMaterialPropertyType::Float);
	
			uint64 tempPropertyCount = 0;
			uint64 tempPropertyDataOffset = 0;
			uint64 tempPropertyDataTotalSize = alignByteSize - restSize;
			PName  tempPropertyName;

			while (tempPropertyDataOffset < tempPropertyDataTotalSize)
			{

				tempPropertyName = PString::Format("_tempProperty_%d_%d", _dataSize, tempPropertyCount++);

				HMaterialDefineInfo tempPropertyDefineInfo;
				tempPropertyDefineInfo.Name = tempPropertyName;

				if (tempPropertyDataOffset + floatPropertySize > tempPropertyDataTotalSize)
				{
					tempPropertyDefineInfo.Type = EMaterialPropertyType::Float;
					tempPropertyDataOffset += floatPropertySize;
				}
				else
				{
					tempPropertyDefineInfo.Type = EMaterialPropertyType::Bool;
					tempPropertyDataOffset += HMaterialProperty::GetPropertySize(EMaterialPropertyType::Bool);
				}

				_defineInfos.push_back(tempPropertyDefineInfo);
			}

			_dataSize += tempPropertyDataTotalSize;
		}

		_dataSize += HMaterialProperty::GetPropertySize(inType);
		_defineInfos.push_back(defineInfo);
	}
};

struct GRAPHICS_API HRawMaterialConstructArguments
{
	PName Name;
	EMaterialDomain Domain;
	HMaterialPropertyDefinitionist PropertyDefinitionist;
};

class GRAPHICS_API IRawMaterial : public IJGGraphicsObject
{
public:
	virtual const PString& GetShaderCode() const = 0;
	virtual const PString& GetFullShaderCode() const = 0;

	virtual const HMaterialPropertyDefinitionist& GetPropertyDefinitionist() const = 0;

	virtual bool SetBool(const PName& inName, const bool& inValue) = 0;
	virtual bool SetInt(const PName& inName, const int32& inValue) = 0;
	virtual bool SetFloat(const PName& inName, const float32& inValue)   = 0;
	virtual bool SetFloat2(const PName& inName, const HVector2& inValue) = 0;
	virtual bool SetFloat3(const PName& inName, const HVector3& inValue) = 0;
	virtual bool SetFloat4(const PName& inName, const HVector4& inValue) = 0;
	virtual bool SetMatrix(const PName& inName, const HMatrix& inMatrix) = 0;
	virtual bool SetTexture(const PName& inName, PSharedPtr<IRawTexture> inValue) = 0;

	virtual bool GetBool(const PName& inName, bool& outValue) const = 0;
	virtual bool GetInt(const PName& inName, int32& outValue) const = 0;
	virtual bool GetFloat(const PName& inName, float32& outValue)   const = 0;
	virtual bool GetFloat2(const PName& inName, HVector2& outValue) const = 0;
	virtual bool GetFloat3(const PName& inName, HVector3& outValue) const = 0;
	virtual bool GetFloat4(const PName& inName, HVector4& outValue) const = 0;
	virtual bool GetMatrix(const PName& inName, HMatrix& outValue) const = 0;
	virtual bool GetTexture(const PName& inName, PSharedPtr<IRawTexture>& outValue) const = 0;
	virtual bool Compile(const HMaterialCompileArguments& inArgs) = 0;

	virtual bool IsValid() const = 0;
};