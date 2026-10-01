#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class IRawTexture;
class IConstantBuffer;
class IVertexBuffer;
class IIndexBuffer;
class IStructuredBuffer;
class IMesh;
class IRawGraphicsShader;

class IJGGraphicsCommand : public IMemoryObject
{
public:
	virtual ~IJGGraphicsCommand() = default;
public:
	//
	virtual void BeginDraw() = 0;
	virtual void EndDraw() = 0;

	// Render Setting
	virtual void SetRenderTarget(const HRenderTarget& inRenderTarget) = 0;
	virtual void SetRenderPassData(const HRenderPassCBData& inData) = 0;
	// Surface 도메인: 메시의 서브메시를 순회해 각 머터리얼(덮어쓰기가 있으면 그것)로 그린다. SetRenderTarget에 깊이 텍스처가 있으면 깊이 테스트한다.
	virtual void Draw(const HDrawArguments& inArgs) = 0;
	// Screen 도메인: 풀스크린 삼각형 2개.
	virtual void Draw(const HScreenDrawArguments& inArgs) = 0;
	// 2D: 렌더 타깃 픽셀 좌표의 삼각형 배치(게임 UI 등). SetRenderTarget의 렌더 타깃에 알파 블렌드로 그린다. 깊이 텍스처는 두지 않는다.
	virtual void Draw(const H2DDrawArguments& inArgs) = 0;

	// Util
	virtual void ClearTexture(PSharedPtr<IRawTexture> InTexture) const = 0;
	virtual void ClearTexture(PSharedPtr<IRawTexture> InTexture, const HLinearColor& InClearColor) const = 0;
	// 깊이 텍스처(Allow_DepthStencil)를 HTextureInfo의 ClearDepth/ClearStencil 또는 지정한 값으로 지운다.
	virtual void ClearDepthTexture(PSharedPtr<IRawTexture> InTexture) const = 0;
	virtual void ClearDepthTexture(PSharedPtr<IRawTexture> InTexture, float32 InClearDepth, uint8 InClearStencil) const = 0;

	// Bind
	virtual void BindTextures(uint32 rootParam, HList<PSharedPtr<IRawTexture>> inTextures) = 0;
	virtual void BindConstantBuffer(uint32 rootParam, PSharedPtr<IConstantBuffer> inConstantBuffer) = 0;
	virtual void BindStructuredBuffer(uint32 rootParam, PSharedPtr<IStructuredBuffer> inStructuredBuffer) = 0;
	virtual void BindVertexBuffer(PSharedPtr<IVertexBuffer> inVertexBuffer) = 0;
	virtual void BindIndexBuffer(PSharedPtr<IIndexBuffer> inIndexBuffer) = 0;
	virtual void BindShader(PSharedPtr<IRawGraphicsShader> inGraphicsShader) = 0;
};
