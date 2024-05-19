#include "PCH/PCH.h"
#include "DX12GraphicsCommand.h"
#include "DirectX12API.h"
#include "DX12Texture.h"
#include "Classes/CommandList.h"
#include "Classes/RootSignature.h"
#include "Classes/PipelineState.h"

void PDX12GraphicsCommand::BeginDraw()
{
	// 루트 서명 정의
	createRootSignature();

	_graphicsPSO = Allocate<PGraphicsPipelineState>();

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);

	cmdList->BindRootSignature(_rootSignature);
	_graphicsPSO->BindRootSignature(*_rootSignature);
}

void PDX12GraphicsCommand::EndDraw()
{
	/*
		void BindRootSignature(const PRootSignature& rootSig);
	void BindRenderTarget(const HList<DXGI_FORMAT>& rtFormats, DXGI_FORMAT dvFormat = DXGI_FORMAT_UNKNOWN);
	void BindInputLayout(const HInputLayout& inputLayout);
	void BindShader(const HHashMap<EShaderDomain, HList<uint8>>& inShaderBtDatas);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
	void SetSampleMask(uint32_t sampleMask);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& desc);
	void SetBlendState(const D3D12_BLEND_DESC& desc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc);
	*/
}

void PDX12GraphicsCommand::SetRenderTarget(const HRenderTarget& inRenderTarget)
{
	// PSO 셋팅
	// 렌더타켓 바인딩

	// _graphicsPSO
}

void PDX12GraphicsCommand::SetRenderPassData(const HRenderPassData& inData)
{
	// 상수 버퍼 바인딩
}

void PDX12GraphicsCommand::Draw(const HDrawArguments& inArgs)
{
	// PSO 셋팅
	// 리소스 바인딩
}

void PDX12GraphicsCommand::Draw(const HSceneDrawArguments& inArgs)
{
	// PSO 셋팅
    // 리소스 바인딩
}

void PDX12GraphicsCommand::ClearTexture(PSharedPtr<IRawTexture> inTexture) const
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);

	PSharedPtr<PDX12Texture> dx12Texture = Cast<PDX12Texture>(inTexture);
	const HTextureInfo& texInfo = inTexture->GetTextureInfo();
	cmdList->ClearRenderTargetTexture(dx12Texture->Get(), dx12Texture->GetRTV(), texInfo.ClearColor);
}

void PDX12GraphicsCommand::ClearTexture(PSharedPtr<IRawTexture> inTexture, const HLinearColor& inClearColor) const
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);

	PSharedPtr<PDX12Texture> dx12Texture = Cast<PDX12Texture>(inTexture);
	cmdList->ClearRenderTargetTexture(dx12Texture->Get(), dx12Texture->GetRTV(), inClearColor);
}

void PDX12GraphicsCommand::createRootSignature()
{
	if (_rootSignature != nullptr)
	{
		return;
	}

	_rootSignature = Allocate<PRootSignature>();
	_rootSignature->InitAsCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	_rootSignature->InitAsCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	_rootSignature->InitAsCBV(2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	_rootSignature->InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10240, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	_rootSignature->InitAsDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
	_rootSignature->AddStaticSamplerState(CD3DX12_STATIC_SAMPLER_DESC(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f, 16, D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK));

	_rootSignature->AddStaticSamplerState(CD3DX12_STATIC_SAMPLER_DESC(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f, 16, D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK));

	_rootSignature->AddStaticSamplerState(CD3DX12_STATIC_SAMPLER_DESC(
		2, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f, 16, D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK));

	_rootSignature->AddStaticSamplerState(CD3DX12_STATIC_SAMPLER_DESC(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f, 16, D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK));

	_rootSignature->AddStaticSamplerState(CD3DX12_STATIC_SAMPLER_DESC(
		4, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f, 16, D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK));

	_rootSignature->AddStaticSamplerState(CD3DX12_STATIC_SAMPLER_DESC(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f, 16, D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK));
	_rootSignature->Finalize();
}
