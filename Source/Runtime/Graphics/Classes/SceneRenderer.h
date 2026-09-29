#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "Classes/Scene.h"

class IRawTexture;
class IRawMaterial;
class IJGGraphicsCommand;

// G버퍼 슬롯. 템플릿 Surface 경로의 SV_TARGET0~3 순서와 같다.
enum class ESceneGBuffer : uint32
{
	Albedo = 0,
	NormalMetallic,
	SpecularRoughness,
	Depth,
	Count,
};

// PScene을 읽어 그린다. (디퍼드 초석)
//  1. 지오메트리 패스 : 장면의 메시 배치를 Surface 도메인 머터리얼로 G버퍼 4장 + 깊이 텍스처에 그린다.
//  2. 합성 패스       : Screen 도메인 머터리얼로 G버퍼를 읽어 출력 텍스처에 풀스크린으로 합성한다.
// 출력 텍스처는 GUI(HGUI::Image)나 프레임버퍼로 보여 준다. 메인 스레드 전용.
class GRAPHICS_API PSceneRenderer : public IMemoryObject
{
	PSharedPtr<IJGGraphicsCommand> _graphicsCommand;   // 계속 쓴다 (GetGraphicsCommand는 호출마다 새 객체를 만든다)
	PSharedPtr<IRawTexture>  _outputTexture;
	PSharedPtr<IRawTexture>  _gbufferTextures[(uint32)ESceneGBuffer::Count];
	PSharedPtr<IRawTexture>  _depthStencilTexture;
	PSharedPtr<IRawMaterial> _compositeMaterial;
	uint32 _width  = 0;
	uint32 _height = 0;

public:
	virtual ~PSceneRenderer() = default;

	// 출력 · G버퍼 · 깊이 텍스처와 합성 머터리얼을 만든다. inName은 텍스처 이름의 앞부분(디버그용). 다시 부르면 새 크기로 다시 만든다.
	bool Initialize(const PString& inName, uint32 inWidth, uint32 inHeight);
	// 카메라를 찾지 못하면 출력은 클리어 색(빨강)으로 남는다.
	void Render(const PScene& inScene, HSceneCameraID inCamera);

	PSharedPtr<IRawTexture> GetOutputTexture() const;
	PSharedPtr<IRawTexture> GetGBufferTexture(ESceneGBuffer inGBuffer) const;
	uint32 GetWidth() const;
	uint32 GetHeight() const;

private:
	void createTextures(const PString& inName);
	bool createCompositeMaterial(const PString& inName);
	void fillRenderPassData(const HSceneCamera& inCamera, HRenderPassCBData& outData) const;
	void renderGeometryPass(const PScene& inScene, const HRenderPassCBData& inPassData);
	void renderCompositePass(const HRenderPassCBData& inPassData);
};
