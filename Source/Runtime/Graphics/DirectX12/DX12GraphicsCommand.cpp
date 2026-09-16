#include "PCH/PCH.h"
#include "DX12GraphicsCommand.h"
#include "DirectX12API.h"
#include "DX12Texture.h"
#include "DX12VertexBuffer.h"
#include "DX12IndexBuffer.h"
#include "DX12Material.h"
#include "Classes/DX12Shader.h"
#include "Classes/CommandList.h"
#include "Classes/ConstantBuffer.h"
#include "Classes/StructuredBuffer.h"
#include "Classes/VertexBuffer.h"
#include "Classes/IndexBuffer.h"
#include "Classes/RootSignature.h"
#include "Classes/PipelineState.h"

void PDX12GraphicsCommand::BeginDraw()
{
	// 루트 서명 정의
	createRootSignature();

	if (_graphicsPSO == nullptr)
	{
		_graphicsPSO = Allocate<PGraphicsPipelineState>();
	}
	

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
	JG_CHECK(_graphicsPSO != nullptr);
	
	HList<DXGI_FORMAT> rtFormats;
	DXGI_FORMAT dvFormat = DXGI_FORMAT_UNKNOWN;
	HList<HDX12Resource*> rtTextures;
	HDX12Resource* depthTexture = nullptr;
	HList<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
	
	for (int32 i = 0; i < MAX_RENDERTARGET; ++i)
	{
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		if (inRenderTarget.RenderTextures[i].IsValid() && inRenderTarget.RenderTextures[i]->IsValid())
		{
			const HTextureInfo& texInfo = inRenderTarget.RenderTextures[i]->GetTextureInfo();
			format = HDirectX12Helper::ConvertDXGIFormat(texInfo.Format);

			
			PSharedPtr<PDX12Texture> dx12Texture = Cast<PDX12Texture>(inRenderTarget.RenderTextures[i]);
			JG_CHECK(dx12Texture != nullptr);
			rtTextures.push_back(dx12Texture->Get());

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx12Texture->GetRTV();
			JG_CHECK(rtvHandle.ptr != 0);
			rtvHandles.push_back(rtvHandle);
		}

		rtFormats.push_back(format);
	}

	if (inRenderTarget.DepthTexture.IsValid() && inRenderTarget.DepthTexture->IsValid())
	{
		const HTextureInfo& texInfo = inRenderTarget.DepthTexture->GetTextureInfo();
		dvFormat = HDirectX12Helper::ConvertDXGIFormat(texInfo.Format);
		
		PSharedPtr<PDX12Texture> dx12Texture = Cast<PDX12Texture>(inRenderTarget.DepthTexture);
		JG_CHECK(dx12Texture != nullptr);
		depthTexture = dx12Texture->Get();

		dsvHandle = dx12Texture->GetDSV();
		JG_CHECK(dsvHandle.ptr != 0);
	}
	
	_graphicsPSO->BindRenderTarget(rtFormats, dvFormat);

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	cmdList->SetViewports(inRenderTarget.Viewports);
	cmdList->SetScissorRects(inRenderTarget.ScissorRects);
	cmdList->SetRenderTarget(rtTextures.data(), rtvHandles.data(), rtvHandles.size(), depthTexture, &dsvHandle);
}

void PDX12GraphicsCommand::SetRenderPassData(const HRenderPassCBData& inData)
{
	JG_CHECK(_graphicsPSO != nullptr);

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	cmdList->BindConstantBuffer(RootParam_RenderPassCB, &inData, sizeof(HRenderPassCBData));
}

void PDX12GraphicsCommand::Draw(const HDrawArguments& inArgs)
{
	// PSO 셋팅
	// 리소스 바인딩

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	cmdList->BindConstantBuffer(RootParam_ObjectCB, &inArgs.ObjectCBData, sizeof(HObjectCBData));
}

void PDX12GraphicsCommand::Draw(const HSceneDrawArguments& inArgs)
{
	JG_CHECK(_graphicsPSO != nullptr);

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();

	PSharedPtr<PDX12Material> dx12Material = Cast<PDX12Material>(inArgs.Material);
	JG_CHECK(dx12Material != nullptr);

	HList<PSharedPtr<IRawTexture>> materialTextures = dx12Material->GetTextures();
	BindTextures(RootParam_Texture, materialTextures);

	// BindConstantBuffer가 인터페이스를 받으므로 구현체로 내려받을 필요가 없다.
	PSharedPtr<IConstantBuffer> materialCB = dx12Material->GetConstantBuffer().Pin();
	JG_CHECK(materialCB.IsValid());
	BindConstantBuffer(RootParam_MaterialCB, materialCB);

	// 셰이더 바이트코드는 PSO 설명에 들어가므로 Finalize보다 먼저 바인드해야 한다.
	PSharedPtr<IRawGraphicsShader> graphicsShader = dx12Material->GetShader().Pin();
	BindShader(graphicsShader);

	// assert 안에서 호출하면 NDEBUG 빌드에서 Finalize 호출 자체가 사라진다.
	const bool bFinalized = _graphicsPSO->Finalize();
	JG_CHECK(bFinalized);

	cmdList->BindPipelineState(_graphicsPSO);

	// TODO(Phase 2-7): 템플릿의 풀스크린 정점은 6개이고, 토폴로지 설정도 아직 없다.
	cmdList->Draw(4);
}

void PDX12GraphicsCommand::ClearTexture(PSharedPtr<IRawTexture> inTexture) const
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);
	JG_CHECK(inTexture.IsValid() && inTexture->IsValid());

	PSharedPtr<PDX12Texture> dx12Texture = Cast<PDX12Texture>(inTexture);
	const HTextureInfo& texInfo = inTexture->GetTextureInfo();
	cmdList->ClearRenderTargetTexture(dx12Texture->Get(), dx12Texture->GetRTV(), texInfo.ClearColor);
}

void PDX12GraphicsCommand::ClearTexture(PSharedPtr<IRawTexture> inTexture, const HLinearColor& inClearColor) const
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);
	JG_CHECK(inTexture.IsValid() && inTexture->IsValid());

	PSharedPtr<PDX12Texture> dx12Texture = Cast<PDX12Texture>(inTexture);
	cmdList->ClearRenderTargetTexture(dx12Texture->Get(), dx12Texture->GetRTV(), inClearColor);
}

void PDX12GraphicsCommand::BindTextures(uint32 rootParam, HList<PSharedPtr<IRawTexture>> inTextures)
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);

	HList<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
	for (PSharedPtr<IRawTexture> rawTexture : inTextures)
	{
		JG_CHECK(rawTexture.IsValid() && rawTexture->IsValid());

		handles.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ rawTexture->GetTextureID() });
	}

	cmdList->BindTextures(rootParam, handles);
}

void PDX12GraphicsCommand::BindConstantBuffer(uint32 rootParam, PSharedPtr<IConstantBuffer> inConstantBuffer)
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);
	JG_CHECK(inConstantBuffer.IsValid() && inConstantBuffer->IsValid());
	
	cmdList->BindConstantBuffer(rootParam, inConstantBuffer->GetData(), inConstantBuffer->GetDataSize());
}

void PDX12GraphicsCommand::BindStructuredBuffer(uint32 rootParam, PSharedPtr<IStructuredBuffer> inStructuredBuffer)
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);
	JG_CHECK(inStructuredBuffer.IsValid() && inStructuredBuffer->IsValid());

	const void* pData = inStructuredBuffer->GetDatas();
	const uint64 elementCount = inStructuredBuffer->GetElementCount();
	const uint64 elementSize  = inStructuredBuffer->GetElementSize();

	cmdList->BindStructuredBuffer(rootParam, pData, elementCount, elementSize);
}

void PDX12GraphicsCommand::BindVertexBuffer(PSharedPtr<IVertexBuffer> inVertexBuffer)
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);
	JG_CHECK(inVertexBuffer.IsValid() && inVertexBuffer->IsValid());

	PSharedPtr<PDX12VertexBuffer> dxVertexBuffer = Cast<PDX12VertexBuffer>(inVertexBuffer);

	uint64 vertexCount = inVertexBuffer->GetVertexCount();
	uint64 vertexSize  = inVertexBuffer->GetVertexSize();

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = dxVertexBuffer->Get()->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vertexCount * vertexSize;
	vertexBufferView.StrideInBytes = 0;

	cmdList->BindVertexBuffer(vertexBufferView);
}

void PDX12GraphicsCommand::BindIndexBuffer(PSharedPtr<IIndexBuffer> inIndexBuffer)
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);
	JG_CHECK(inIndexBuffer.IsValid() && inIndexBuffer->IsValid());

	PSharedPtr<PDX12IndexBuffer> dx12IndexBuffer = Cast<PDX12IndexBuffer>(inIndexBuffer);

	uint64 indexCount = inIndexBuffer->GetIndexCount();

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = dx12IndexBuffer->Get()->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = indexCount * sizeof(uint32);

	cmdList->BindIndexBuffer(indexBufferView);
}

void PDX12GraphicsCommand::BindShader(PSharedPtr<IRawGraphicsShader> inGraphicsShader)
{
	JG_CHECK(_graphicsPSO != nullptr);
	JG_CHECK(inGraphicsShader.IsValid() && inGraphicsShader->IsValid());

	PSharedPtr<PDX12GraphicsShader> dx12Shader = Cast<PDX12GraphicsShader>(inGraphicsShader);
	JG_CHECK(dx12Shader != nullptr);

	// 셰이더 객체가 소유한 도메인별 바이트코드를 PSO 설명에 연결한다.
	// 입력 레이아웃과 토폴로지는 도메인(Scene/Surface)에 따라 다르므로 Draw 쪽에서 설정한다(Phase 2-7, 4-2).
	_graphicsPSO->BindShader(dx12Shader->GetByteCodes());
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
