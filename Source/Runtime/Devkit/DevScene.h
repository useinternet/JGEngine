#pragma once
#include "DevkitDefines.h"
#include "WidgetComponent.h"
#include "JGGraphicsDefine.h"
#include "DevScene.generation.h"

/*
DevFeature

DevFeature은 기본적으로 아래 기능을 가지고 있음.

Scene => DataClass와 Widget 나누기
Setting => DataClass와 Widget 나누기
Log 출력 (DevConsole 기능도) => LogData와 Widget 나누기
*/
class IRawTexture;
class IRawMaterial;
class IJGGraphicsCommand;
class JGStaticMesh;
class JGAsset;

// 그래픽 테스트 표면. 매 프레임 두 패스를 기록한다.
//  1. 지오메트리 패스 : Surface 도메인 머터리얼로 메시를 G버퍼 4장(Albedo / Normal_Metallic / Specular_Roughness / Depth) + 깊이 텍스처에 그린다.
//  2. 합성 패스       : Scene 도메인 머터리얼로 G버퍼를 읽어 SceneTexture에 풀스크린으로 합성한다. (디퍼드 초석)
// SceneTexture를 ImGui Image로 표시한다.
JGCLASS()
class DEVKIT_API JGDevScene : public JGWidgetComponent
{
	JG_GENERATED_WIDGETCOMPONENT_BODY

private:
	PSharedPtr<IRawTexture>        SceneTexture;         // 최종 출력(GUI 표시)
	PSharedPtr<IRawTexture>        GBufferTextures[4];   // 템플릿 Surface 경로의 SV_TARGET0~3 순서
	PSharedPtr<IRawTexture>        DepthStencilTexture;  // D24_Unorm_S8_Uint
	PSharedPtr<IRawMaterial>       CompositeMaterial;    // Scene 도메인. G버퍼 -> SceneTexture
	PSharedPtr<JGStaticMesh>       Mesh;                 // 로드된 테스트 메시 (Content/TempAsset/Sample.jgasset, X Bot)
	PSharedPtr<IJGGraphicsCommand> GraphicsCommand;      // 프레임마다 재사용 (GetGraphicsCommand는 호출마다 새 객체를 만든다)
	HVector2 SceneSize;

	// 임시 카메라. PCamera가 빈 클래스라 여기서 직접 View/Proj를 계산한다. 메시가 로드되면 경계 상자에 맞춘다.
	HVector3 CameraEye;
	HVector3 CameraTarget;
	float32  CameraFovY;
	float32  CameraNearZ;
	float32  CameraFarZ;

	// 리드백 검증(Phase 5-1). 메시가 로드된 뒤 일정 프레임이 지나면 한 번만 알베도 G버퍼(비동기)와 씬 텍스처(동기)를 읽어 PNG로 저장한다.
	int32   FramesSinceMeshLoaded;
	bool    bReadbackDumped;
	PString ReadbackStatus;

public:
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnLayout(const HWidgetComponentLayout& InLayout) override;
	virtual void OnGenerateGUI() override;

private:
	void CreateRenderTargets();
	void CreateCompositeMaterial();
	void RequestMeshLoad();
	void OnMeshLoaded(PWeakPtr<JGAsset> InAsset);
	void FitCameraToMesh();
	void FillRenderPassData(HRenderPassCBData& OutData) const;

	// 매 프레임 OnGenerateGUI에서 호출. 지오메트리 패스 + 합성 패스.
	void RenderScene();
	void RenderGeometryPass();
	void RenderCompositePass();

	// 리드백 검증. 실행 폴더에 DevScene_Readback_*.png 를 쓴다.
	void DumpReadbackOnce();
	void OnAlbedoReadback(const HTexturePixels& InPixels);
};
