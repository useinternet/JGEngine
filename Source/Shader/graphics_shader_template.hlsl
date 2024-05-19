
#define JG_PI 3.141592
#define FLT_EPSILON     1.192092896e-07 
#define FLT_MIN         1.175494351e-38 
#define FLT_MAX         3.402823466e+38 
#define FLT_10BIT_MIN   6.1e-5
#define FLT_10BIT_MAX   6.5e4

#define SHADING_MODEL_DEFAULT_LIT 0
#define SHADING_MODEL_SKYBOX      1

#define MAX_TEXTURE_COUNT 10240
#define MAX_TEXTURECUBE_COUNT 10

cbuffer __RenderPassDataCB__ : register(b0)
{
    float4x4 _ProjMatrix;
    float4x4 _ViewMatrix;
    float4x4 _ViewProjMatrix;
    float4x4 _InvViewMatrix;
    float4x4 _InvProjMatrix;
    float4x4 _InvViewProjMatrix;
    float2   _Resolution;
    float    _NearZ;
    float    _FarZ;
    float3   _EyePosition;
};

cbuffer __ObjectDataCB__ : register(b1)
{
    float4x4 _WorldMatrix;
};

cbuffer __MaterialDataCB__ : register(b2)
{
	__PS_CONSTANT_BUFFER_CONTENTS_SCRIPT__
};

// Resource
Texture2D _globalTexture[MAX_TEXTURE_COUNT] : register(t0, space0);
TextureCube _globalTextureCube[MAX_TEXTURECUBE_COUNT] : register(t0, space1);

// Sampler
SamplerState _PointWrap_  : register(s0);
SamplerState _LinearWrap_ : register(s1);
SamplerState _AnisotropicWrap_ : register(s2);
SamplerState _PointClamp_  : register(s3);
SamplerState _LinearClamp_ : register(s4);
SamplerState _AnisotropicClamp_ : register(s5);

__PS_CONSTANT_BUFFER_CONTENTS_SCRIPT__
		
#if MATERIAL_DOMAIN_SCENE
static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};


struct VS_OUT
{
	float4 PosH : SV_POSITION;
	float2 TexC : TEXCOORD0;
};

struct PS_SCENE_OUTPUT
{
	float4 final;
};

struct PS_SCENE_INPUT
{
	float4 posH;
	float2 tex;
};

PS_SCENE_OUTPUT PS_SCENE_FUNCTION(PS_SCENE_INPUT _input)
{
	PS_SCENE_OUTPUT _output;
	_output.final    = float4(0.0f,0.0f,0.0f,0.0f);


	__PS_SCENE_CONTENTS_SCRIPT__

	return _output;
};

VS_OUT vs_main(uint vid : SV_VertexID)
{
	VS_OUT vout;

	vout.TexC = gTexCoords[vid];
	vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);
	return vout;
}


float4 ps_main(VS_OUT pin) : SV_TARGET
{
	PS_SCENE_INPUT input;
	input.tex = pin.TexC;
	input.posH = pin.PosH;

	PS_SCENE_OUTPUT output;
	output =  PS_SCENE_FUNCTION(input);

	return output.final;
}

#else // MATERIAL_DOMAIN_SURFACE

struct VS_IN
{
	float3 posL : POSITION;
	float2 tex : TEXCOORD;
	float3 normalL : NORMAL;
	float3 tanL : TANGENT;
	float3 bitL : BITANGENT;
};
struct VS_OUT
{
	float4 posH    : SV_POSITION;
	float3 posW    : POSITION;
	float3 posL     : LOCAL_POSITION;
	float3 normalW : NORMAL;
	float3 tanW	   : TANGENT;
	float3 bitW    : BITANGENT;
	float2 tex     : TEXCOORD;
	float2 depth	: DEPTH;
};

struct PS_SURFACE_OUTPUT
{
	float4 albedo;
	float3 normal;
	float3 specular;
	float roughness;
	float metallic;
	float3 emissive;
	int shadingmodel;
};


struct PS_SURFACE_INPUT
{
	float3 position;
	float3 local_position;
	float3 normal;
	float3 tangent;
	float3 bitangent;
	float2 tex;
};

struct PS_OUT
{
	float4 Albedo : SV_TARGET0;
	float4 Normal_Metallic    : SV_TARGET1;
	float4 Specular_Roughness : SV_TARGET2;
    float Depth = SV_TARGET3;
};

PS_SURFACE_OUTPUT PS_SURFACE_FUNCTION(PS_SURFACE_INPUT _input)
{
	PS_SURFACE_OUTPUT _output;
	_output.albedo    = float4(1.0f,1.0f,1.0f,1.0f);
	_output.specular  = float3(0.5f, 0.5f, 0.5f);
	_output.normal    = _input.normal;
    _output.roughness = 0.0f;
	_output.metallic  = 0.0f;
	_output.emissive = float3(0.0f,0.0f,0.0f);
	_output.shadingmodel = 0;

	__PS_SURFACE_CONTENTS_SCRIPT__
	return _output;
};

VS_OUT vs_main(VS_IN vin)
{
	VS_OUT vout;
	vout.posL = vin.posL;
	float3 posW = mul(float4(vin.posL, 1.0f), _WorldMatrix);
	float3 normalW = mul(float4(vin.normalL, 0.0f), _WorldMatrix);
	float3 tanW =  mul(float4(vin.tanL, 0.0f), _WorldMatrix);
	float3 bitW =  mul(float4(vin.bitL, 0.0f), _WorldMatrix);
	vout.posH = mul(float4(posW, 1.0f), _ViewProjMatrix);
	vout.posW = posW;
	vout.normalW = normalize(normalW);
	vout.tanW = normalize(tanW);
	vout.bitW = normalize(bitW);
	vout.tex   = vin.tex;
	vout.depth = float2(vout.posH.z, vout.posH.w);
	return vout;
}

PS_OUT ps_main(VS_OUT pin) : SV_TARGET
{
	PS_SURFACE_INPUT input;
	input.position = pin.posW;
	input.local_position = pin.posL;
	input.normal   = normalize(pin.normalW);
	input.tangent  = normalize(pin.tanW);
	input.bitangent = normalize(pin.bitW);
	input.tex       = pin.tex;
	PS_SURFACE_OUTPUT output = PS_SURFACE_FUNCTION(input);


	PS_OUT ps_out;
	ps_out.Albedo   = output.albedo;
	ps_out.Normal_Metallic   = float4(output.normal.rgb, output.metallic);
	ps_out.Specular_Roughness = float4(output.specular, output.roughness);

	// if(output.shadingmodel == SHADING_MODEL_SKYBOX)
	// {
	// 	ps_out.Depth = 1.0f;
	// }
	// else
	// {
		ps_out.Depth 	  = saturate(pin.depth.x / pin.depth.y);
	//}
	
	return ps_out;
}
#endif

