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
#include "Classes/Mesh.h"
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
	// 그린 렌더 타깃을 다음 소비자(GUI 표시, 다른 패스의 샘플링)가 읽을 수 있게 셰이더 리소스 상태로 넘긴다.
	if (_boundRenderTextures.empty())
	{
		return;
	}

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);

	for (const PSharedPtr<PDX12Texture>& renderTexture : _boundRenderTextures)
	{
		if (renderTexture.IsValid() && renderTexture->IsValid())
		{
			cmdList->TransitionBarrier(renderTexture->Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}
	cmdList->FlushResourceBarrier();

	_boundRenderTextures.clear();
}

void PDX12GraphicsCommand::SetRenderTarget(const HRenderTarget& inRenderTarget)
{
	JG_CHECK(_graphicsPSO != nullptr);

	// 렌더 타깃은 슬롯 0부터 연속으로 채워져야 한다.
	// OMSetRenderTargets의 핸들 배열과 PSO의 RTVFormats 인덱스가 같이 움직이므로 빈 슬롯을 만나면 거기서 끝낸다.
	HList<DXGI_FORMAT> rtFormats;
	HList<HDX12Resource*> rtTextures;
	HList<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
	_boundRenderTextures.clear();

	for (int32 i = 0; i < MAX_RENDERTARGET; ++i)
	{
		const PSharedPtr<IRawTexture>& renderTexture = inRenderTarget.RenderTextures[i];
		if (renderTexture.IsValid() == false || renderTexture->IsValid() == false)
		{
			break;
		}

		PSharedPtr<PDX12Texture> dx12Texture = Cast<PDX12Texture>(renderTexture);
		JG_CHECK(dx12Texture != nullptr);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dx12Texture->GetRTV();
		JG_CHECK(rtvHandle.ptr != 0);

		rtFormats.push_back(HDirectX12Helper::ConvertDXGIFormat(renderTexture->GetTextureInfo().Format));
		rtTextures.push_back(dx12Texture->Get());
		rtvHandles.push_back(rtvHandle);
		_boundRenderTextures.push_back(dx12Texture);
	}

	for (int32 i = (int32)rtFormats.size() + 1; i < MAX_RENDERTARGET; ++i)
	{
		if (inRenderTarget.RenderTextures[i].IsValid())
		{
			JG_LOG(Graphics, ELogLevel::Error, "SetRenderTarget : RenderTextures must be contiguous from slot 0. slot %d is ignored", i);
		}
	}

	DXGI_FORMAT dvFormat = DXGI_FORMAT_UNKNOWN;
	HDX12Resource* depthTexture = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
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

	// 깊이 텍스처가 없는데 DepthEnable이 켜져 있으면(기본값) PSO 생성이 실패한다.
	CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
	if (dvFormat == DXGI_FORMAT_UNKNOWN)
	{
		depthStencilDesc.DepthEnable   = FALSE;
		depthStencilDesc.StencilEnable = FALSE;
	}
	_graphicsPSO->SetDepthStencilState(depthStencilDesc);

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	cmdList->SetViewports(inRenderTarget.Viewports);
	cmdList->SetScissorRects(inRenderTarget.ScissorRects);
	cmdList->SetRenderTarget(rtTextures.data(), rtvHandles.data(), rtvHandles.size(), depthTexture, depthTexture != nullptr ? &dsvHandle : nullptr);
}

void PDX12GraphicsCommand::SetRenderPassData(const HRenderPassCBData& inData)
{
	JG_CHECK(_graphicsPSO != nullptr);

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	cmdList->BindConstantBuffer(RootParam_RenderPassCB, &inData, sizeof(HRenderPassCBData));
}

void PDX12GraphicsCommand::Draw(const HDrawArguments& inArgs)
{
	JG_CHECK(_graphicsPSO != nullptr);

	if (inArgs.Mesh.IsValid() == false || inArgs.Mesh->IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Draw(HDrawArguments) : Mesh is invalid");
		return;
	}

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);

	// 오브젝트 상수(월드 행렬)는 모든 서브메시가 공유한다.
	cmdList->BindConstantBuffer(RootParam_ObjectCB, &inArgs.ObjectCBData, sizeof(HObjectCBData));

	const PString meshName = inArgs.Mesh->GetName().ToString();
	const uint32 subMeshCount = inArgs.Mesh->GetSubMeshCount();
	for (uint32 i = 0; i < subMeshCount; ++i)
	{
		PSharedPtr<IVertexBuffer> vertexBuffer = inArgs.Mesh->GetVertexBuffer(i);
		PSharedPtr<IIndexBuffer>  indexBuffer  = inArgs.Mesh->GetIndexBuffer(i);
		if (vertexBuffer.IsValid() == false || vertexBuffer->IsValid() == false ||
			indexBuffer.IsValid()  == false || indexBuffer->IsValid()  == false || indexBuffer->GetIndexCount() == 0)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : SubMesh(%d) has no vertex/index buffer", meshName, (int32)i);
			continue;
		}

		// 서브메시 머터리얼. IMesh::GetMaterial이 없으면 기본 머터리얼을 돌려주므로 null은 오지 않는다.
		PSharedPtr<PDX12Material> dx12Material = Cast<PDX12Material>(inArgs.Mesh->GetMaterial(i));
		if (dx12Material == nullptr || dx12Material->GetDomain() != EMaterialDomain::Surface)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : SubMesh(%d) requires a Surface domain material", meshName, (int32)i);
			continue;
		}

		if (bindMaterial(dx12Material) == false)
		{
			continue;
		}

		// Surface 도메인: 메시의 정점 입력 레이아웃 + 삼각형 리스트
		_graphicsPSO->BindInputLayout(inArgs.Mesh->GetInputLayout());
		_graphicsPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		if (_graphicsPSO->Finalize() == false)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Finalize PSO for SubMesh(%d), Material(%s)", meshName, (int32)i, dx12Material->GetName().ToString());
			continue;
		}

		cmdList->BindPipelineState(_graphicsPSO);
		cmdList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		BindVertexBuffer(vertexBuffer);
		BindIndexBuffer(indexBuffer);

		cmdList->DrawIndexed((uint32)indexBuffer->GetIndexCount());
	}
}

void PDX12GraphicsCommand::Draw(const HSceneDrawArguments& inArgs)
{
	JG_CHECK(_graphicsPSO != nullptr);

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();

	PSharedPtr<PDX12Material> dx12Material = Cast<PDX12Material>(inArgs.Material);
	JG_CHECK(dx12Material != nullptr);
	if (dx12Material->GetDomain() != EMaterialDomain::Scene)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Draw(HSceneDrawArguments) requires a Scene domain material", dx12Material->GetName().ToString());
		return;
	}

	if (bindMaterial(dx12Material) == false)
	{
		return;
	}

	// Scene 도메인은 SV_VertexID로 풀스크린을 그리므로 입력 레이아웃이 없다.
	_graphicsPSO->BindInputLayout(HInputLayout());
	_graphicsPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	if (_graphicsPSO->Finalize() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Finalize PSO", dx12Material->GetName().ToString());
		return;
	}

	cmdList->BindPipelineState(_graphicsPSO);
	cmdList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 템플릿 gTexCoords 6개 = 삼각형 2개
	cmdList->Draw(6);
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

void PDX12GraphicsCommand::ClearDepthTexture(PSharedPtr<IRawTexture> inTexture) const
{
	JG_CHECK(inTexture.IsValid() && inTexture->IsValid());

	const HTextureInfo& texInfo = inTexture->GetTextureInfo();
	ClearDepthTexture(inTexture, texInfo.ClearDepth, texInfo.ClearStencil);
}

void PDX12GraphicsCommand::ClearDepthTexture(PSharedPtr<IRawTexture> inTexture, float32 inClearDepth, uint8 inClearStencil) const
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);
	JG_CHECK(inTexture.IsValid() && inTexture->IsValid());

	PSharedPtr<PDX12Texture> dx12Texture = Cast<PDX12Texture>(inTexture);
	JG_CHECK(dx12Texture != nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dx12Texture->GetDSV();
	if (dsvHandle.ptr == 0)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : ClearDepthTexture requires ETextureFlags::Allow_DepthStencil", inTexture->GetName().ToString());
		return;
	}

	// 스텐실이 없는 깊이 포맷에 STENCIL 플래그를 주면 디버그 레이어 오류다. 지원 포맷 중 스텐실이 있는 것은 D24S8 하나.
	D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH;
	if (inTexture->GetTextureInfo().Format == ETextureFormat::D24_Unorm_S8_Uint)
	{
		clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
	}

	cmdList->ClearDepthTexture(dx12Texture->Get(), dsvHandle, inClearDepth, inClearStencil, clearFlags);
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

	const uint64 vertexCount = inVertexBuffer->GetVertexCount();
	const uint64 vertexSize  = inVertexBuffer->GetVertexSize();

	// 스트라이드는 정점 하나의 크기. (이전 코드는 0이라 모든 정점이 첫 정점을 읽었다)
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = dxVertexBuffer->Get()->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes    = (UINT)(vertexCount * vertexSize);
	vertexBufferView.StrideInBytes  = (UINT)vertexSize;

	cmdList->BindVertexBuffer(vertexBufferView);
}

void PDX12GraphicsCommand::BindIndexBuffer(PSharedPtr<IIndexBuffer> inIndexBuffer)
{
	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);
	JG_CHECK(inIndexBuffer.IsValid() && inIndexBuffer->IsValid());

	PSharedPtr<PDX12IndexBuffer> dx12IndexBuffer = Cast<PDX12IndexBuffer>(inIndexBuffer);

	const uint64 indexCount = inIndexBuffer->GetIndexCount();

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = dx12IndexBuffer->Get()->GetGPUVirtualAddress();
	indexBufferView.Format         = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes    = (UINT)(indexCount * sizeof(uint32));

	cmdList->BindIndexBuffer(indexBufferView);
}

void PDX12GraphicsCommand::BindShader(PSharedPtr<IRawGraphicsShader> inGraphicsShader)
{
	JG_CHECK(_graphicsPSO != nullptr);
	JG_CHECK(inGraphicsShader.IsValid() && inGraphicsShader->IsValid());

	PSharedPtr<PDX12GraphicsShader> dx12Shader = Cast<PDX12GraphicsShader>(inGraphicsShader);
	JG_CHECK(dx12Shader != nullptr);

	// 셰이더 객체가 소유한 도메인별 바이트코드를 PSO 설명에 연결한다.
	// 입력 레이아웃과 토폴로지는 도메인(Scene/Surface)에 따라 다르므로 Draw 쪽에서 설정한다.
	_graphicsPSO->BindShader(dx12Shader->GetByteCodes());
}

bool PDX12GraphicsCommand::bindMaterial(PSharedPtr<PDX12Material> inMaterial)
{
	JG_CHECK(inMaterial != nullptr);

	PSharedPtr<IRawGraphicsShader> graphicsShader = inMaterial->GetShader().Pin();
	if (graphicsShader.IsValid() == false || graphicsShader->IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Material shader is not compiled", inMaterial->GetName().ToString());
		return false;
	}

	PSharedPtr<IConstantBuffer> materialCB = inMaterial->GetConstantBuffer().Pin();
	if (materialCB.IsValid() == false || materialCB->IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Material constant buffer is invalid", inMaterial->GetName().ToString());
		return false;
	}

	// 텍스처 슬롯 -> 디스크립터 테이블. 셰이더의 _globalTexture[슬롯]은 이 테이블을 가리킨다.
	BindTextures(RootParam_Texture, inMaterial->GetTextures());

	// 큐브 텍스처는 기본값이 없어 빈 슬롯(null)이 있을 수 있다. 하나라도 비어 있으면 테이블을 올리지 않는다.
	HList<PSharedPtr<IRawTexture>> textureCubes = inMaterial->GetTextureCubes();
	if (textureCubes.empty() == false)
	{
		bool bAllValid = true;
		for (const PSharedPtr<IRawTexture>& textureCube : textureCubes)
		{
			if (textureCube.IsValid() == false || textureCube->IsValid() == false)
			{
				bAllValid = false;
				break;
			}
		}

		if (bAllValid)
		{
			BindTextures(RootParam_TextureCube, textureCubes);
		}
		else
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : TextureCube slot is empty. Skip binding cube textures", inMaterial->GetName().ToString());
		}
	}

	BindConstantBuffer(RootParam_MaterialCB, materialCB);
	BindShader(graphicsShader);

	return true;
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
