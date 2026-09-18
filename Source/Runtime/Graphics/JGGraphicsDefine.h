#pragma once
#include "FileIO/Json.h"
#include "FileIO/Jsonable.h"

#ifdef _GRAPHICS
#define GRAPHICS_API __declspec(dllexport)
#define GRAPHICS_C_API extern "C" __declspec(dllexport)
#else
#define GRAPHICS_API __declspec(dllimport)
#define GRAPHICS_C_API extern "C" __declspec(dllimport)
#endif

#define MAX_RENDERTARGET 8
#define TEXTURE_ID_NULL  0

using TextureID					= uint64;
using PrimitiveResourcePtr		= void*;
using ResourceGPUVirtualAddress = uint64;
using ResourceViewPtr			= uint64;

JGENUM()
enum class EShaderDomain
{
	Vertex,
	Domain,
	Hull,
	Geometry,
	Pixel
};

JGENUM()
enum class ETextureFormat
{
	None,					JGENUMMETA(Channels = 0)
	R8_Unorm,				JGENUMMETA(Channels = 1)
	R16_Float,				JGENUMMETA(Channels = 1)
	R32_Float,				JGENUMMETA(Channels = 1)
	R16G16_Float,			JGENUMMETA(Channels = 2)
	R8G8B8A8_Unorm,			JGENUMMETA(Channels = 4)
	R16G16B16A16_Unorm,		JGENUMMETA(Channels = 4)
	R11G11B10_Float,		JGENUMMETA(Channels = 3)
	R16G16B16A16_Float,		JGENUMMETA(Channels = 4)
	R32G32B32A32_Float,		JGENUMMETA(Channels = 4)
	R16G16B16A16_Uint,		JGENUMMETA(Channels = 4)
	R8_Uint,				JGENUMMETA(Channels = 1)
	R16_Uint,				JGENUMMETA(Channels = 1)
	R32_Uint,				JGENUMMETA(Channels = 1)
	R24G8_TYPELESS,			JGENUMMETA(Channels = 2)
	D24_Unorm_S8_Uint,		JGENUMMETA(Channels = 2)
};

// JGENUMMETA()가 없는 항목은 헤더 툴이 이름 목록에 넣지 않아 StaticEnum<>()->GetEnumNameByValue()가 NAME_NONE을 돌려준다.
// 머터리얼 샘플러 이름(_PointClamp_ 등)이 이 이름으로 만들어지므로 모든 항목에 붙인다.
JGENUM()
enum class ETextureFilterMode
{
	Point,			JGENUMMETA()
	Linear,			JGENUMMETA()
	Anisotropic,	JGENUMMETA()
};

JGENUM()
enum class ETextureWrapMode
{
	Wrap,			JGENUMMETA()
	Clamp,			JGENUMMETA()
	//Border
};

JGENUM()
enum class EResourceState
{
	Common,								JGENUMMETA()
	VertexAndConstantBuffer,			JGENUMMETA()
	IndexBuffer,						JGENUMMETA()
	RenderTarget,						JGENUMMETA()
	UnorderedAccess,					JGENUMMETA()
	DepthWrite,							JGENUMMETA()
	DepthRead,							JGENUMMETA()
	NonePixelShaderResource,			JGENUMMETA()
	PixelShaderResource,				JGENUMMETA()
	CopyDest,							JGENUMMETA()
	CopySource,							JGENUMMETA()
	RayTracingAccelerationStructure,	JGENUMMETA()
	GenericRead,						JGENUMMETA()
};

JGENUM()
enum class EShaderDataType
{
	Unknown,			JGENUMMETA(HLSLName = Unknown,		DataSize = 0)
	Bool,				JGENUMMETA(HLSLName = bool,			DataSize = 1)
	Int,				JGENUMMETA(HLSLName = int,			DataSize = 4)
	Int2,				JGENUMMETA(HLSLName = int2,			DataSize = 8)
	Int3,				JGENUMMETA(HLSLName = int3,			DataSize = 12)
	Int4,				JGENUMMETA(HLSLName = int4,			DataSize = 16)
	Uint,				JGENUMMETA(HLSLName = uint,			DataSize = 4)
	Uint2,				JGENUMMETA(HLSLName = uint2,		DataSize = 8)
	Uint3,				JGENUMMETA(HLSLName = uint3,		DataSize = 12)
	Uint4,				JGENUMMETA(HLSLName = uint4,		DataSize = 16)
	Float,				JGENUMMETA(HLSLName = float,		DataSize = 4)
	Float2,				JGENUMMETA(HLSLName = float2,		DataSize = 8)
	Float3,				JGENUMMETA(HLSLName = float3,		DataSize = 12)
	Float4,				JGENUMMETA(HLSLName = float4,		DataSize = 16)
	Float3x3,			JGENUMMETA(HLSLName = float3x3,		DataSize = 36)
	Float4x4,			JGENUMMETA(HLSLName = float4x4,		DataSize = 64)
	Texture2D,			JGENUMMETA(HLSLName = Texture2D,	DataSize = 0)
	TextureCube,		JGENUMMETA(HLSLName = TextureCube,	DataSize = 0)
	RWTexture2D,		JGENUMMETA(HLSLName = RWTexture2D,	DataSize = 0)
};

enum class EDepthStencilStateTemplate
{
	Default = 0,
	NoDepth = 1,
	LessEqual = 2,
};

enum class EBlendStateTemplate
{
	Default = 0,
	Transparent_Default,
};

enum class ERasterizerStateTemplate
{
	Default = 0,
	Cull_None = 1,
};

JG_ENUM_FLAG(ETextureFlags)
enum class ETextureFlags
{
	None = 0x00001,
	Allow_RenderTarget = 0x00002,
	Allow_DepthStencil = 0x00004,
	Allow_UnorderedAccessView = 0x00008,
	SRV_TextureCube = 0x00010,
};

JG_ENUM_FLAG(EShaderCompileFlags)
enum class EShaderCompileFlags
{
	None = 0x000,
	Allow_VertexShader = 0x001,
	Allow_PixelShader = 0x002,
	Allow_GeometryShader = 0x004,
	Allow_HullShader = 0x008,
	Allow_DomainShader = 0x010,
	Allow_ComputeShader = 0x020,
};

class HInputElement
{
public:
	const char* SementicName = nullptr;
	uint32 SementicSlot = 0;
	EShaderDataType Type = EShaderDataType::Unknown;
public:
	HInputElement() = default;
	HInputElement(EShaderDataType type, const char* sementicName, uint32 sementicSlot)
		: Type(type), SementicName(sementicName), SementicSlot(sementicSlot) {}
};

class HInputLayout
{
private:
	HList<HInputElement> _inputElements;

public:
	void Add(EShaderDataType type, const char* sementicName, uint32 sementicSlot)
	{
		_inputElements.push_back(HInputElement(type, sementicName, sementicSlot));
	}

	void ForEach(const std::function<void(const HInputElement&)>& action) const
	{
		for (auto& element : _inputElements)
		{
			action(element);
		}
	}
};

// GPU 정점 버퍼에 올라가는 순수 데이터(POD). 가상 함수가 없어 입력 레이아웃 오프셋과 1:1로 맞는다.
// HVertex는 여기에 IJsonable(vptr 8바이트)을 얹은 직렬화용 타입이다. GPU에는 항상 이 타입만 올린다.
//
// GetInputLayout()은 값으로 돌려준다. 함수 지역 static으로 두면 안 된다:
// HList는 엔진 메모리 풀 할당자(HAllocator)를 쓰므로 static 컨테이너는 메모리 시스템이 내려간 뒤 DLL 언로드 시점에
// 소멸되면서 이미 free된 풀 메모리를 읽는다. (2026-09-17 종료 크래시 원인) 레이아웃이 필요한 객체(PStaticMesh 등)가 멤버로 보관한다.
struct HVertexData
{
	HVector3 Position;
	HVector2 Texcoord;
	HVector3 Normal;
	HVector3 Tangent;
	HVector3 Bitangent;

	static HInputLayout GetInputLayout() {
		HInputLayout layout;
		layout.Add(EShaderDataType::Float3, "POSITION", 0);
		layout.Add(EShaderDataType::Float2, "TEXCOORD", 0);
		layout.Add(EShaderDataType::Float3, "NORMAL", 0);
		layout.Add(EShaderDataType::Float3, "TANGENT", 0);
		layout.Add(EShaderDataType::Float3, "BITANGENT", 0);
		return layout;
	}
};
static_assert(sizeof(HVertexData) == 56, "HVertexData must match the input layout (float3 + float2 + float3 x3 = 56 bytes)");

class HVertex : public HVertexData, public IJsonable
{
public:
	HVertex() = default;
	HVertex(const HVertexData& inData) : HVertexData(inData) {}
	virtual ~HVertex() = default;

public:
	static HInputLayout GetInputLayout() {
		return HVertexData::GetInputLayout();
	}
protected:
	// IJsonable
	virtual void WriteJson(PJsonData& json) const
	{
		json.AddMember(Position);
		json.AddMember(Texcoord);
		json.AddMember(Normal);
		json.AddMember(Tangent);
		json.AddMember(Bitangent);
	}
	virtual void ReadJson(const PJsonData& json)
	{
		PJsonData element;
		json.FindMemberFromIndex(0, &element); element.GetData(&Position);
		json.FindMemberFromIndex(1, &element); element.GetData(&Texcoord);
		json.FindMemberFromIndex(2, &element); element.GetData(&Normal);
		json.FindMemberFromIndex(3, &element); element.GetData(&Tangent);
		json.FindMemberFromIndex(4, &element); element.GetData(&Bitangent);
	}
	// ~ IJsonable
};

class HQuadVertex : public IJsonable
{
public:
	HVector3 Position;
	HVector2 Texcoord;

public:
	virtual ~HQuadVertex() = default;

public:
	static HInputLayout GetInputLayout() {
		HInputLayout layout;
		layout.Add(EShaderDataType::Float3, "POSITION", 0);
		layout.Add(EShaderDataType::Float2, "TEXCOORD", 0);
		return layout;
	}
protected:
	// IJsonable
	virtual void WriteJson(PJsonData& json) const
	{
		json.AddMember(Position);
		json.AddMember(Texcoord);
	}
	virtual void ReadJson(const PJsonData& json)
	{
		PJsonData element;
		json.FindMemberFromIndex(0, &element); element.GetData(&Position);
		json.FindMemberFromIndex(1, &element); element.GetData(&Texcoord);
	}
	// ~ IJsonable
};
struct HDebugVertex
{
	HVector3 Position;
	HDebugVertex() = default;
	HDebugVertex(float32 x, float32 y, float32 z) : Position(x, y, z) {}
	HDebugVertex(const HVector3& p) : Position(p) {}
public:
	static HInputLayout GetInputLayout() {
		HInputLayout layout;
		layout.Add(EShaderDataType::Float3, "POSITION", 0);
		return layout;
	}
};

struct HBoneVertex : public IJsonable
{
public:
	HVector4Int BoneIDs;
	HVector4	BoneWeights;
public:
	virtual ~HBoneVertex() = default;
protected:
	// IJsonable
	virtual void WriteJson(PJsonData& json) const
	{
		json.AddMember(BoneIDs);
		json.AddMember(BoneWeights);
	}
	virtual void ReadJson(const PJsonData& json)
	{
		PJsonData element;
		json.FindMemberFromIndex(0, &element); element.GetData(&BoneIDs);
		json.FindMemberFromIndex(1, &element); element.GetData(&BoneWeights);
	}
	// ~ IJsonable
};
struct HBoneOffsetData : public IJsonable
{
	uint32 ID;
	HMatrix Offset;

public:
	virtual ~HBoneOffsetData() = default;

protected:
	// IJsonable
	virtual void WriteJson(PJsonData& json) const
	{
		json.AddMember(ID);
		json.AddMember(Offset);
	}
	virtual void ReadJson(const PJsonData& json)
	{
		PJsonData element;
		json.FindMemberFromIndex(0, &element); element.GetData(&ID);
		json.FindMemberFromIndex(1, &element); element.GetData(&Offset);
	}
	// ~ IJsonable
};


struct HTextureInfo
{
	PString Name;
	uint32 Width = 0;
	uint32 Height = 0;
	uint32 PixelPerUnit = 1;
	ETextureFormat Format = ETextureFormat::None;
	ETextureFlags  Flags  = ETextureFlags::None;
	ETextureFilterMode FilterMode = ETextureFilterMode::Anisotropic;
	ETextureWrapMode WrapMode = ETextureWrapMode::Wrap;
	uint16 MipLevel = 0;
	uint16 ArraySize = 1;

	HLinearColor ClearColor;
	float32  ClearDepth = 1.0f;
	uint8    ClearStencil = 0;

	HTextureInfo()
	{
		Name = "Texture";
	}
};

// GPU 텍스처에서 CPU로 읽어 온 픽셀 (PJGGraphicsAPI::RequestReadPixels / ReadPixelsImmediate 결과).
// 서브리소스 0(밉 0, 슬라이스 0)만 담고, 행은 빈틈없이(RowPitch = Width * PixelSize) 채워진다.
struct HTexturePixels
{
	uint32 Width  = 0;
	uint32 Height = 0;
	uint32 PixelSize = 0;                          // 픽셀 하나의 바이트 수 (HJGGraphicsHelper::GetTextureFormatPixelSize)
	ETextureFormat Format = ETextureFormat::None;
	// 엔진 메모리 풀은 블록 하나가 최대 2MB라 HList로는 1080p 한 장(16MB)도 담을 수 없다. 큰 픽셀 버퍼는 std 할당자를 쓴다.
	std::vector<uint8> Data;

	uint64 GetRowPitch() const { return (uint64)Width * PixelSize; }
	bool IsValid() const { return Width > 0 && Height > 0 && PixelSize > 0 && Data.size() == GetRowPitch() * Height; }
	const uint8* GetPixel(uint32 x, uint32 y) const { return Data.data() + (uint64)y * GetRowPitch() + (uint64)x * PixelSize; }
};

struct MeshInfo
{
	PName Name;
	uint32 TotalSubMeshCount = 0;
	uint32 TotalVertexCount = 0;
	uint32 TotalIndexCount = 0;

	HList<PName> SubMeshNames;
	HList<uint32>    SubMeshVertexCounts;
	HList<uint32>    SubMeshIndexCounts;

	HInputLayout InputLayout;
};

// 버퍼 데이터를 GPU에 올리는 방식 (HVertexBufferConstructArguments / HIndexBufferConstructArguments)
enum class EBufferLoadMethod
{
	GPULoad,   // DEFAULT 힙. 전송 관리자가 스테이징으로 올린다. 정적 데이터(메시 VB/IB). CPU 사본은 버퍼가 따로 든다.
	CPULoad,   // UPLOAD 힙 + 상시 Map. 매 프레임 바뀌는 데이터.
};

enum class EGeometryType
{
	Box,
	Sphere,
	Capsule,
	Line,
};
enum class ECommandQueueType
{
	Unknown,
	Graphics,
	Compute
};



class IRawTexture;
class IMesh;
class IRawMaterial;
struct HRenderTarget
{
public:
	PSharedPtr<IRawTexture> RenderTextures[MAX_RENDERTARGET];
	PSharedPtr<IRawTexture> DepthTexture;
	HList<HViewport>    Viewports;
	HList<HScissorRect> ScissorRects;
};

struct HRenderPassCBData
{
	HMatrix ProjMatrix;
	HMatrix ViewMatrix;
	HMatrix ViewProjMatrix;
	HMatrix InvViewMatrix;
	HMatrix InvProjMatrix;
	HMatrix InvViewProjMatrix;
	HVector2   Resolution;
	float32    NearZ;
	float32    FarZ;
	HVector3   EyePosition;
};

struct HObjectCBData
{
	HMatrix WorldMatrix;	
};

struct HDrawArguments
{
	PSharedPtr<IMesh> Mesh;
	HObjectCBData ObjectCBData;
};

struct HSceneDrawArguments
{
	PSharedPtr<IRawMaterial> Material;
};
