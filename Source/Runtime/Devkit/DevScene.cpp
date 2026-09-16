#include "PCH/PCH.h"
#include "DevScene.h"
#include "JGGraphicsDefine.h"
#include "JGGraphics.h"
#include "JGGraphicsCommand.h"
#include "Classes/Texture.h"
#include "Classes/Material.h"
#include "GUI.h"

namespace
{
	constexpr uint32 DevSceneTextureWidth  = 1920;
	constexpr uint32 DevSceneTextureHeight = 1080;
}

void JGDevScene::OnInitialize()
{
	HTextureInfo TexInfo;
	TexInfo.Name   = "DevSceneTexture";
	TexInfo.Width  = DevSceneTextureWidth;
	TexInfo.Height = DevSceneTextureHeight;
	TexInfo.Format = ETextureFormat::R16G16B16A16_Float;
	TexInfo.Flags  = ETextureFlags::Allow_RenderTarget;
	TexInfo.MipLevel  = 1;
	TexInfo.ArraySize = 1;
	TexInfo.ClearColor = HLinearColor(1.0F, 0.0F, 0.0F, 1.0F);

	SceneTexture = GetGraphicsAPI().CreateRawTexture(TexInfo);

	// GetGraphicsCommand()는 호출마다 새 커맨드 객체(루트 시그니처·PSO 포함)를 만들므로 한 번 받아 보관한다.
	GraphicsCommand = GetGraphicsAPI().GetGraphicsCommand();
	GraphicsCommand->ClearTexture(SceneTexture);

	// 첫 드로우 검증용 Scene 도메인 머터리얼. 프로퍼티 없이 단색(주황)만 출력한다.
	// 클리어 컬러(빨강)와 다른 색이어야 드로우가 실제로 일어났는지 구분할 수 있다.
	HRawMaterialConstructArguments MaterialArgs;
	MaterialArgs.Name   = PName("DevSceneTestMaterial");
	MaterialArgs.Domain = EMaterialDomain::Scene;
	TestMaterial = GetGraphicsAPI().CreateRawMaterial(MaterialArgs);

	HMaterialCompileArguments CompileArgs;
	CompileArgs.ShaderCode = "_output.final = float4(1.0f, 0.5f, 0.0f, 1.0f);";
	if (TestMaterial.IsValid() == false || TestMaterial->Compile(CompileArgs) == false)
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : Fail Compile Test Material");
		TestMaterial = nullptr;
	}
}

void JGDevScene::OnShutdown()
{
	TestMaterial    = nullptr;
	GraphicsCommand = nullptr;
	SceneTexture.Reset();
}

void JGDevScene::OnLayout(const HWidgetComponentLayout& InLayout)
{
	SceneSize = InLayout.ContentSize;
}

void JGDevScene::OnGenerateGUI()
{
	if (!SceneTexture.IsValid())
	{
		return;
	}

	// GenerateGUI는 GraphicsBegin(펜스 대기) 뒤에 매 프레임 불리고, GUI가 이 텍스처를 실제로 그리는 시점은
	// GraphicsEnd의 프레임버퍼 갱신이므로, 여기서 기록한 드로우 커맨드가 먼저 실행된다.
	RenderScene();

	HGUI::Image(SceneTexture->GetTextureID(), SceneSize);
}

void JGDevScene::RenderScene()
{
	if (GraphicsCommand.IsValid() == false || TestMaterial.IsValid() == false)
	{
		return;
	}

	GraphicsCommand->ClearTexture(SceneTexture);
	GraphicsCommand->BeginDraw();

	HRenderTarget RenderTarget;
	RenderTarget.RenderTextures[0] = SceneTexture;
	RenderTarget.Viewports.push_back(HViewport((float32)DevSceneTextureWidth, (float32)DevSceneTextureHeight));
	RenderTarget.ScissorRects.push_back(HScissorRect(0, 0, (int32)DevSceneTextureWidth, (int32)DevSceneTextureHeight));
	GraphicsCommand->SetRenderTarget(RenderTarget);

	// Scene 도메인은 SV_VertexID로 풀스크린을 그리므로 카메라 행렬은 아직 쓰지 않는다. 항등원으로 채운다.
	HRenderPassCBData PassData;
	PassData.ProjMatrix        = HMatrix::Identity();
	PassData.ViewMatrix        = HMatrix::Identity();
	PassData.ViewProjMatrix    = HMatrix::Identity();
	PassData.InvViewMatrix     = HMatrix::Identity();
	PassData.InvProjMatrix     = HMatrix::Identity();
	PassData.InvViewProjMatrix = HMatrix::Identity();
	PassData.Resolution  = HVector2((float32)DevSceneTextureWidth, (float32)DevSceneTextureHeight);
	PassData.NearZ       = 0.1f;
	PassData.FarZ        = 1000.0f;
	PassData.EyePosition = HVector3(0.0f, 0.0f, 0.0f);
	GraphicsCommand->SetRenderPassData(PassData);

	HSceneDrawArguments DrawArgs;
	DrawArgs.Material = TestMaterial;
	GraphicsCommand->Draw(DrawArgs);

	GraphicsCommand->EndDraw();
}
