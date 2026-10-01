// 2D 그리기 내장 셰이더 (IJGGraphicsCommand::Draw(const H2DDrawArguments&)). 게임 UI(GameGUI) 같은 화면 공간 사각형용.
// 루트 서명은 엔진 표준(PDX12GraphicsCommand::createRootSignature)을 그대로 쓴다: b0 패스 상수, t0(space0) 텍스처 표, s4 LinearClamp.
// 정점 좌표는 렌더 타깃 픽셀(왼쪽 위 원점). 배치 텍스처는 표의 0번 슬롯에 묶인다. 머터리얼 템플릿과 무관하다.

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

Texture2D    _Draw2DTexture : register(t0, space0);
SamplerState _LinearClamp_  : register(s4);

struct VS_IN
{
    float2 pos   : POSITION;
    float2 tex   : TEXCOORD;
    float4 color : COLOR;
};

struct VS_OUT
{
    float4 posH  : SV_POSITION;
    float2 tex   : TEXCOORD;
    float4 color : COLOR;
};

VS_OUT vs_main(VS_IN vin)
{
    VS_OUT vout;
    // 픽셀 → NDC. y는 아래로 +이므로 뒤집는다.
    const float2 ndc = float2(vin.pos.x / _Resolution.x * 2.0f - 1.0f, 1.0f - vin.pos.y / _Resolution.y * 2.0f);
    vout.posH  = float4(ndc, 0.0f, 1.0f);
    vout.tex   = vin.tex;
    vout.color = vin.color;
    return vout;
}

float4 ps_main(VS_OUT pin) : SV_TARGET
{
    return _Draw2DTexture.Sample(_LinearClamp_, pin.tex) * pin.color;
}
