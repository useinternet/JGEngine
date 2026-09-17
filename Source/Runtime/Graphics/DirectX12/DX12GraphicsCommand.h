#pragma once
#include "JGGraphicsCommand.h"

class PDX12ConstantBuffer;
class PDX12Material;
class PRootSignature;
class PGraphicsPipelineState;
class PDX12Texture;

// 한 프레임당 요놈 한개 만 사용
class PDX12GraphicsCommand : public IJGGraphicsCommand
{
	enum ERootParam : uint32
	{
		RootParam_RenderPassCB,
		RootParam_ObjectCB,
		RootParam_MaterialCB,
		RootParam_Texture,
		RootParam_TextureCube
	};
	
	PSharedPtr<PRootSignature>		   _rootSignature;
	PSharedPtr<PGraphicsPipelineState> _graphicsPSO;
	PSharedPtr<PDX12ConstantBuffer>    _renderPassConstantBuffer;
	// SetRenderTarget에서 바인드한 렌더 타깃. EndDraw에서 셰이더 리소스 상태로 넘긴다.
	HList<PSharedPtr<PDX12Texture>>    _boundRenderTextures;
public:
	virtual ~PDX12GraphicsCommand() = default;

public:
	virtual void BeginDraw() override;
	virtual void EndDraw() override;

	virtual void SetRenderTarget(const HRenderTarget& inRenderTarget) override;
	virtual void SetRenderPassData(const HRenderPassCBData& inData) override;
	virtual void Draw(const HDrawArguments& inArgs) override;
	virtual void Draw(const HSceneDrawArguments& inArgs) override;
	
	virtual void ClearTexture(PSharedPtr<IRawTexture> inTexture) const override;
	virtual void ClearTexture(PSharedPtr<IRawTexture> inTexture, const HLinearColor& inClearColor) const override;
	virtual void ClearDepthTexture(PSharedPtr<IRawTexture> inTexture) const override;
	virtual void ClearDepthTexture(PSharedPtr<IRawTexture> inTexture, float32 inClearDepth, uint8 inClearStencil) const override;

private:
	virtual void BindTextures(uint32 rootParam, HList<PSharedPtr<IRawTexture>> inTextures) override;
	virtual void BindConstantBuffer(uint32 rootParam, PSharedPtr<IConstantBuffer> inConstantBuffer) override;
	virtual void BindStructuredBuffer(uint32 rootParam, PSharedPtr<IStructuredBuffer> inStructuredBuffer) override;
	virtual void BindVertexBuffer(PSharedPtr<IVertexBuffer> inVertexBuffer) override;
	virtual void BindIndexBuffer(PSharedPtr<IIndexBuffer> inIndexBuffer) override;
	virtual void BindShader(PSharedPtr<IRawGraphicsShader> inGraphicsShader) override;

private:
	void createRootSignature();
	// 머터리얼의 텍스처 / 상수 버퍼 / 셰이더를 바인드한다. 셰이더가 컴파일되지 않았으면 false.
	bool bindMaterial(PSharedPtr<PDX12Material> inMaterial);
};
