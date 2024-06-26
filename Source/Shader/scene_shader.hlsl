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