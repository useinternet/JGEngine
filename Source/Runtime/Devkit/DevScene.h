#pragma once
#include "DevkitDefines.h"
#include "WidgetComponent.h"
#include "JGGraphicsDefine.h"
#include "Classes/Scene.h"
#include "DevScene.generation.h"

/*
DevFeature

DevFeature은 기본적으로 아래 기능을 가지고 있음.

Scene => DataClass와 Widget 나누기
Setting => DataClass와 Widget 나누기
Log 출력 (DevConsole 기능도) => LogData와 Widget 나누기
*/
class PSceneRenderer;
class JGStaticMesh;
class JGAsset;

// 그래픽 테스트 표면. 장면(PScene) 하나에 테스트 메시 배치 하나와 카메라 하나를 두고,
// PSceneRenderer(지오메트리 패스 → 합성 패스)로 그린 출력 텍스처를 ImGui Image로 표시한다.
JGCLASS()
class DEVKIT_API JGDevScene : public JGWidgetComponent
{
	JG_GENERATED_WIDGETCOMPONENT_BODY

private:
	PSharedPtr<PScene>         Scene;          // 이 위젯이 소유하는 장면
	PSharedPtr<PSceneRenderer> Renderer;       // 장면을 G버퍼 → 합성으로 그린다
	HSceneCameraID             Camera;         // 메시가 로드되면 경계 상자에 맞춘다
	HSceneMeshID               MeshInstance;   // 테스트 메시 배치
	PSharedPtr<JGStaticMesh>   Mesh;           // 로드된 테스트 메시 에셋 (Content/TempAsset/Sample.jgasset, X Bot). 경계 상자 계산용
	HVector2 SceneSize;

	// 리드백 검증(Phase 5-1). 메시가 로드된 뒤 일정 프레임이 지나면 한 번만 알베도 G버퍼(비동기)와 출력 텍스처(동기)를 읽어 PNG로 저장한다.
	int32   FramesSinceMeshLoaded;
	bool    bReadbackDumped;
	PString ReadbackStatus;

public:
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnLayout(const HWidgetComponentLayout& InLayout) override;
	virtual void OnUpdate() override;
	virtual void OnGenerateGUI() override;

private:
	void RequestMeshLoad();
	void OnMeshLoaded(PWeakPtr<JGAsset> InAsset);
	void FitCameraToMesh();

	// 리드백 검증. 실행 폴더에 DevScene_Readback_*.png 를 쓴다.
	void DumpReadbackOnce();
	void OnAlbedoReadback(const HTexturePixels& InPixels);
};
