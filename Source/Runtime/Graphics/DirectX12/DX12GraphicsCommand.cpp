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

		// 서브메시 머터리얼. 덮어쓰기(장면의 메시 배치)가 있으면 그것, 없으면 메시의 것.
		// IMesh::GetMaterial은 없으면 기본 머터리얼을 돌려주므로 null은 오지 않는다.
		PSharedPtr<IRawMaterial> material = nullptr;
		if (inArgs.MaterialOverrides != nullptr && i < inArgs.MaterialOverrides->size())
		{
			material = (*inArgs.MaterialOverrides)[i];
		}
		if (material == nullptr)
		{
			material = inArgs.Mesh->GetMaterial(i);
		}
		PSharedPtr<PDX12Material> dx12Material = Cast<PDX12Material>(material);
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

void PDX12GraphicsCommand::Draw(const HScreenDrawArguments& inArgs)
{
	JG_CHECK(_graphicsPSO != nullptr);

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();

	PSharedPtr<PDX12Material> dx12Material = Cast<PDX12Material>(inArgs.Material);
	JG_CHECK(dx12Material != nullptr);
	if (dx12Material->GetDomain() != EMaterialDomain::Screen)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Draw(HScreenDrawArguments) requires a Screen domain material", dx12Material->GetName().ToString());
		return;
	}

	if (bindMaterial(dx12Material) == false)
	{
		return;
	}

	// Screen 도메인은 SV_VertexID로 풀스크린을 그리므로 입력 레이아웃이 없다.
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

void PDX12GraphicsCommand::Draw(const H2DDrawArguments& inArgs)
{
	JG_CHECK(_graphicsPSO != nullptr);

	if (inArgs.Vertices == nullptr || inArgs.VertexCount == 0 || inArgs.Indices == nullptr || inArgs.IndexCount == 0 ||
		inArgs.Commands == nullptr || inArgs.CommandCount == 0)
	{
		return;
	}
	if (inArgs.TargetSize.x <= 0.0f || inArgs.TargetSize.y <= 0.0f)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Draw(H2DDrawArguments) : TargetSize is invalid");
		return;
	}

	// 내장 셰이더. 컴파일 실패는 API가 처음 한 번만 로그로 남긴다.
	PSharedPtr<IRawGraphicsShader> shader = HDirectXAPI::GetDraw2DShader();
	if (shader.IsValid() == false || shader->IsValid() == false)
	{
		return;
	}

	PSharedPtr<PGraphicsCommandList> cmdList = HDirectXAPI::RequestGraphicsCommandList();
	JG_CHECK(cmdList != nullptr);

	// 정점 · 인덱스는 커맨드 리스트의 업로드 할당자(프레임마다 재사용하는 페이지)로 올린다.
	if (cmdList->BindDynamicVertexBuffer(inArgs.Vertices, inArgs.VertexCount, sizeof(H2DVertex)) == false ||
		cmdList->BindDynamicIndexBuffer(inArgs.Indices, inArgs.IndexCount) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Draw(H2DDrawArguments) : %d vertices / %d indices do not fit in one upload page", (int32)inArgs.VertexCount, (int32)inArgs.IndexCount);
		return;
	}

	BindShader(shader);
	_graphicsPSO->BindInputLayout(H2DVertex::GetInputLayout());
	_graphicsPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	// 알파 블렌드 · 컬링 없음은 이 호출에서만 쓴다. 같은 명령 객체의 다음 Surface/Screen 드로우가 기본 상태를 쓰도록 끝에서 되돌린다.
	const D3D12_BLEND_DESC prevBlendDesc = _graphicsPSO->GetBlendDesc();
	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_RENDER_TARGET_BLEND_DESC& targetBlend = blendDesc.RenderTarget[0];
	targetBlend.BlendEnable    = TRUE;
	targetBlend.SrcBlend       = D3D12_BLEND_SRC_ALPHA;
	targetBlend.DestBlend      = D3D12_BLEND_INV_SRC_ALPHA;
	targetBlend.BlendOp        = D3D12_BLEND_OP_ADD;
	targetBlend.SrcBlendAlpha  = D3D12_BLEND_ONE;
	targetBlend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	targetBlend.BlendOpAlpha   = D3D12_BLEND_OP_ADD;
	_graphicsPSO->SetBlendState(blendDesc);

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	_graphicsPSO->SetRasterizerState(rasterizerDesc);

	if (_graphicsPSO->Finalize())
	{
		cmdList->BindPipelineState(_graphicsPSO);
		cmdList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 셰이더는 패스 상수의 _Resolution만 읽는다(픽셀 → NDC).
		HRenderPassCBData passData = {};
		passData.Resolution = inArgs.TargetSize;
		cmdList->BindConstantBuffer(RootParam_RenderPassCB, &passData, sizeof(HRenderPassCBData));

		PSharedPtr<IRawTexture> defaultTexture = HDirectXAPI::GetDefaultTexture();
		for (uint64 i = 0; i < inArgs.CommandCount; ++i)
		{
			const H2DDrawCommand& command = inArgs.Commands[i];
			if (command.IndexCount == 0 || (uint64)command.IndexOffset + command.IndexCount > inArgs.IndexCount)
			{
				continue;
			}
			if (command.ClipRight <= command.ClipLeft || command.ClipBottom <= command.ClipTop)
			{
				continue;
			}

			// 텍스처가 없으면 기본 텍스처(1x1 흰색) = 단색 사각형.
			PSharedPtr<IRawTexture> texture = command.Texture;
			if (texture.IsValid() == false || texture->IsValid() == false)
			{
				texture = defaultTexture;
			}
			if (texture.IsValid() == false || texture->IsValid() == false)
			{
				continue;
			}

			// 드로우마다 텍스처 표를 다시 올리므로(DrawIndexed가 디스크립터 표를 밀어 넣는다) 배치별로 텍스처를 바꿀 수 있다.
			BindTextures(RootParam_Texture, { texture });
			cmdList->SetScissorRect(HScissorRect(command.ClipLeft, command.ClipTop, command.ClipRight, command.ClipBottom));
			cmdList->DrawIndexed(command.IndexCount, 1, command.IndexOffset, 0, 0);
		}
	}
	else
	{
		JG_LOG(Graphics, ELogLevel::Error, "Draw(H2DDrawArguments) : Fail Finalize PSO");
	}

	_graphicsPSO->SetBlendState(prevBlendDesc);
	_graphicsPSO->SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
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

	// 슬롯 목록에 빈 항목(null)이 있으면 테이블을 올리지 않는다. BindTextures는 무효 텍스처에 assert를 걸기 때문이다.
	// 2D 슬롯은 기본 텍스처로 채워지므로 보통 비지 않고(기본 텍스처 생성 실패 시에만), 큐브 슬롯은 기본값이 없어 비어 있을 수 있다.
	auto isAllValid = [](const HList<PSharedPtr<IRawTexture>>& inTextures)
	{
		for (const PSharedPtr<IRawTexture>& texture : inTextures)
		{
			if (texture.IsValid() == false || texture->IsValid() == false)
			{
				return false;
			}
		}
		return true;
	};

	// 텍스처 슬롯 -> 디스크립터 테이블. 셰이더의 _globalTexture[슬롯]은 이 테이블을 가리킨다.
	HList<PSharedPtr<IRawTexture>> textures = inMaterial->GetTextures();
	if (isAllValid(textures))
	{
		BindTextures(RootParam_Texture, textures);
	}
	else
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Texture slot is empty. Skip binding textures", inMaterial->GetName().ToString());
	}

	HList<PSharedPtr<IRawTexture>> textureCubes = inMaterial->GetTextureCubes();
	if (textureCubes.empty() == false)
	{
		if (isAllValid(textureCubes))
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
