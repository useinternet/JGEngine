#include "PCH/PCH.h"
#include "DevScene.h"
#include "JGGraphicsDefine.h"
#include "JGGraphics.h"
#include "JGGraphicsCommand.h"
#include "Classes/Texture.h"
#include "Classes/Material.h"
#include "Classes/Mesh.h"
#include "Classes/StaticMesh.h"
#include "AssetDatabase.h"
#include "GUI.h"

namespace
{
	constexpr uint32 DevSceneTextureWidth  = 1920;
	constexpr uint32 DevSceneTextureHeight = 1080;

	// 첫 메시 드로우 검증용 에셋 (FBX X Bot을 임포트한 JGStaticMesh)
	constexpr const char* DevSceneMeshAssetPath = "/JGEngine/TempAsset/Sample.jgasset";

	// G버퍼 슬롯. 템플릿 Surface 경로의 SV_TARGET0~3 순서와 같다.
	enum EGBuffer : int32
	{
		GBuffer_Albedo = 0,
		GBuffer_NormalMetallic,
		GBuffer_SpecularRoughness,
		GBuffer_Depth,
		GBuffer_Count
	};
}

void JGDevScene::OnInitialize()
{
	// 기본 카메라. 메시가 로드되면 FitCameraToMesh가 경계 상자에 맞춰 다시 잡는다.
	CameraEye    = HVector3(0.0f, 100.0f, -300.0f);
	CameraTarget = HVector3(0.0f, 100.0f, 0.0f);
	CameraFovY   = HMath::ConvertToRadians(45.0f);
	CameraNearZ  = 0.1f;
	CameraFarZ   = 5000.0f;

	CreateRenderTargets();

	// GetGraphicsCommand()는 호출마다 새 커맨드 객체(루트 시그니처·PSO 포함)를 만들므로 한 번 받아 보관한다.
	GraphicsCommand = GetGraphicsAPI().GetGraphicsCommand();
	GraphicsCommand->ClearTexture(SceneTexture);

	CreateCompositeMaterial();
	RequestMeshLoad();
}

void JGDevScene::OnShutdown()
{
	Mesh              = nullptr;
	CompositeMaterial = nullptr;
	GraphicsCommand   = nullptr;
	DepthStencilTexture = nullptr;
	for (int32 i = 0; i < GBuffer_Count; ++i)
	{
		GBufferTextures[i] = nullptr;
	}
	SceneTexture = nullptr;
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

void JGDevScene::CreateRenderTargets()
{
	// 최종 출력. 클리어 컬러(빨강)는 "아무것도 그려지지 않았다"는 신호로 남겨 둔다.
	{
		HTextureInfo texInfo;
		texInfo.Name   = "DevSceneTexture";
		texInfo.Width  = DevSceneTextureWidth;
		texInfo.Height = DevSceneTextureHeight;
		texInfo.Format = ETextureFormat::R16G16B16A16_Float;
		texInfo.Flags  = ETextureFlags::Allow_RenderTarget;
		texInfo.MipLevel  = 1;
		texInfo.ArraySize = 1;
		texInfo.ClearColor = HLinearColor(1.0F, 0.0F, 0.0F, 1.0F);

		SceneTexture = GetGraphicsAPI().CreateRawTexture(texInfo);
	}

	// G버퍼. 합성 패스가 Point/Clamp 샘플러로 1:1 읽으므로 필터/랩 모드를 그렇게 둔다. (샘플러 이름이 머터리얼 코드에 박힌다)
	{
		struct HGBufferDesc
		{
			const char*    Name;
			ETextureFormat Format;
			HLinearColor   ClearColor;
		};
		const HGBufferDesc gbufferDescs[GBuffer_Count] =
		{
			{ "DevScene_GBuffer_Albedo",            ETextureFormat::R8G8B8A8_Unorm,     HLinearColor(0.0f, 0.0f, 0.0f, 0.0f) },
			{ "DevScene_GBuffer_NormalMetallic",    ETextureFormat::R16G16B16A16_Float, HLinearColor(0.0f, 0.0f, 0.0f, 0.0f) },
			{ "DevScene_GBuffer_SpecularRoughness", ETextureFormat::R8G8B8A8_Unorm,     HLinearColor(0.0f, 0.0f, 0.0f, 0.0f) },
			// 배경 깊이 = 1. 합성 패스가 배경 판정에 쓴다.
			{ "DevScene_GBuffer_Depth",             ETextureFormat::R32_Float,          HLinearColor(1.0f, 1.0f, 1.0f, 1.0f) },
		};

		for (int32 i = 0; i < GBuffer_Count; ++i)
		{
			HTextureInfo texInfo;
			texInfo.Name   = gbufferDescs[i].Name;
			texInfo.Width  = DevSceneTextureWidth;
			texInfo.Height = DevSceneTextureHeight;
			texInfo.Format = gbufferDescs[i].Format;
			texInfo.Flags  = ETextureFlags::Allow_RenderTarget;
			texInfo.MipLevel  = 1;
			texInfo.ArraySize = 1;
			texInfo.ClearColor = gbufferDescs[i].ClearColor;
			texInfo.FilterMode = ETextureFilterMode::Point;
			texInfo.WrapMode   = ETextureWrapMode::Clamp;

			GBufferTextures[i] = GetGraphicsAPI().CreateRawTexture(texInfo);
		}
	}

	// 깊이 스텐실
	{
		HTextureInfo texInfo;
		texInfo.Name   = "DevScene_DepthStencil";
		texInfo.Width  = DevSceneTextureWidth;
		texInfo.Height = DevSceneTextureHeight;
		texInfo.Format = ETextureFormat::D24_Unorm_S8_Uint;
		texInfo.Flags  = ETextureFlags::Allow_DepthStencil;
		texInfo.MipLevel  = 1;
		texInfo.ArraySize = 1;
		texInfo.ClearDepth   = 1.0f;
		texInfo.ClearStencil = 0;

		DepthStencilTexture = GetGraphicsAPI().CreateRawTexture(texInfo);
	}
}

void JGDevScene::CreateCompositeMaterial()
{
	// Scene 도메인 풀스크린 머터리얼. G버퍼 3장을 텍스처 프로퍼티로 받아 SceneTexture에 합성한다.
	HRawMaterialConstructArguments materialArgs;
	materialArgs.Name   = PName("DevSceneCompositeMaterial");
	materialArgs.Domain = EMaterialDomain::Scene;
	materialArgs.PropertyDefinitionist.DefineTexture(PName("GAlbedo"));
	materialArgs.PropertyDefinitionist.DefineTexture(PName("GNormal"));
	materialArgs.PropertyDefinitionist.DefineTexture(PName("GDepth"));

	CompositeMaterial = GetGraphicsAPI().CreateRawMaterial(materialArgs);
	if (CompositeMaterial.IsValid() == false || CompositeMaterial->IsValid() == false)
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : Fail Create Composite Material");
		CompositeMaterial = nullptr;
		return;
	}

	// 텍스처는 컴파일 전에 넣는다. 샘플러 이름(Point/Clamp)이 컴파일 시점에 코드에 박히기 때문.
	CompositeMaterial->SetTexture(PName("GAlbedo"), GBufferTextures[GBuffer_Albedo]);
	CompositeMaterial->SetTexture(PName("GNormal"), GBufferTextures[GBuffer_NormalMetallic]);
	CompositeMaterial->SetTexture(PName("GDepth"),  GBufferTextures[GBuffer_Depth]);

	// 텍스처 프로퍼티 토큰(GAlbedo 등)은 컴파일 시 _globalTexture[슬롯].Sample(샘플러, _input.tex)로 치환된다.
	// 배경(깊이 1)은 단색, 메시는 알베도 x 단순 램버트. 형태가 보여야 깊이 테스트 결과를 눈으로 확인할 수 있다.
	HMaterialCompileArguments compileArgs;
	compileArgs.ShaderCode =
		"float  sceneDepth = GDepth.r;\n"
		"float4 albedo     = GAlbedo;\n"
		"float3 normalW    = GNormal.xyz;\n"
		"if (sceneDepth >= 1.0f)\n"
		"{\n"
		"    _output.final = float4(0.10f, 0.12f, 0.16f, 1.0f);\n"
		"}\n"
		"else\n"
		"{\n"
		"    float3 lightDir = normalize(float3(-0.4f, 0.8f, -0.6f));\n"
		"    float  ndl      = saturate(dot(normalize(normalW), lightDir));\n"
		"    _output.final   = float4(albedo.rgb * (0.15f + 0.85f * ndl), 1.0f);\n"
		"}\n";

	if (CompositeMaterial->Compile(compileArgs) == false)
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : Fail Compile Composite Material");
		CompositeMaterial = nullptr;
	}
}

void JGDevScene::RequestMeshLoad()
{
	if (GAssetDatabase::IsValid() == false)
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : AssetDatabase is not available. Mesh will not be loaded");
		return;
	}

	// 콜백은 에셋 로드가 끝난 뒤 메인 스레드에서 불린다. 그 사이 위젯이 닫힐 수 있으므로 약참조로 잡는다.
	PWeakPtr<JGDevScene> weakThis = SharedWrap(this);
	GAssetDatabase::GetInstance().LoadAssetAsync(HAssetPath(DevSceneMeshAssetPath),
		POnLoadCompelete::CreateLambda([weakThis](PWeakPtr<JGAsset> InAsset)
		{
			PSharedPtr<JGDevScene> self = weakThis.Pin();
			if (self.IsValid())
			{
				self->OnMeshLoaded(InAsset);
			}
		}));
}

void JGDevScene::OnMeshLoaded(PWeakPtr<JGAsset> InAsset)
{
	PSharedPtr<JGAsset> asset = InAsset.Pin();
	if (asset.IsValid() == false)
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : Fail Load Mesh Asset (%s)", DevSceneMeshAssetPath);
		return;
	}

	// Cast<>는 정적 캐스트라 타입을 먼저 확인한다.
	if ((asset->GetType() == JGType::GenerateType<JGStaticMesh>()) == false)
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : %s is not a JGStaticMesh (%s)", DevSceneMeshAssetPath, asset->GetType().GetName().ToString());
		return;
	}

	PSharedPtr<JGStaticMesh> mesh = Cast<JGStaticMesh>(asset);
	if (mesh.IsValid() == false || mesh->IsValid() == false)
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : Loaded mesh is invalid (%s)", DevSceneMeshAssetPath);
		return;
	}

	Mesh = mesh;
	FitCameraToMesh();

	JG_LOG(Devkit, ELogLevel::Info, "DevScene : Mesh Loaded (%s) SubMesh %d, Vertex %d, Index %d",
		DevSceneMeshAssetPath, (int32)Mesh->GetSubMeshCount(), (int32)Mesh->GetTotalVertexCount(), (int32)Mesh->GetTotalIndexCount());
}

void JGDevScene::FitCameraToMesh()
{
	if (Mesh.IsValid() == false)
	{
		return;
	}

	HBBox bounds;
	if (Mesh->CalculateBounds(bounds) == false)
	{
		return;
	}

	// 경계 구가 시야에 꽉 차는 거리에서 -Z 쪽(LH 정면)에서 바라본다.
	const HVector3 center = (bounds.min + bounds.max) * 0.5f;
	const HVector3 extent = (bounds.max - bounds.min) * 0.5f;
	const float32  radius = HMath::Max(HVector3::Length(extent), 1.0f);
	const float32  distance = radius / sinf(CameraFovY * 0.5f) * 1.1f;

	CameraTarget = center;
	CameraEye    = center + HVector3(0.0f, radius * 0.1f, -distance);
	CameraNearZ  = HMath::Max(distance - radius * 2.0f, 0.1f);
	CameraFarZ   = distance + radius * 2.0f;

	JG_LOG(Devkit, ELogLevel::Info, "DevScene : Camera fitted. Bounds min(%.1f, %.1f, %.1f) max(%.1f, %.1f, %.1f) distance %.1f",
		bounds.min.x, bounds.min.y, bounds.min.z, bounds.max.x, bounds.max.y, bounds.max.z, distance);
}

void JGDevScene::FillRenderPassData(HRenderPassCBData& OutData) const
{
	const float32 aspectRatio = (float32)DevSceneTextureWidth / (float32)DevSceneTextureHeight;

	// HMatrix는 행 우선(DirectXMath)이고 셰이더는 행 우선 패킹으로 컴파일되므로 그대로 올린다. (mul(v, M) 규약)
	const HMatrix view     = HMatrix::LookAtLH(CameraEye, CameraTarget, HVector3(0.0f, 1.0f, 0.0f));
	const HMatrix proj     = HMatrix::PerspectiveFovLH(CameraFovY, aspectRatio, CameraNearZ, CameraFarZ);
	const HMatrix viewProj = view * proj;

	OutData.ProjMatrix        = proj;
	OutData.ViewMatrix        = view;
	OutData.ViewProjMatrix    = viewProj;
	OutData.InvViewMatrix     = HMatrix::Inverse(view);
	OutData.InvProjMatrix     = HMatrix::Inverse(proj);
	OutData.InvViewProjMatrix = HMatrix::Inverse(viewProj);
	OutData.Resolution  = HVector2((float32)DevSceneTextureWidth, (float32)DevSceneTextureHeight);
	OutData.NearZ       = CameraNearZ;
	OutData.FarZ        = CameraFarZ;
	OutData.EyePosition = CameraEye;
}

void JGDevScene::RenderScene()
{
	if (GraphicsCommand.IsValid() == false)
	{
		return;
	}

	RenderGeometryPass();
	RenderCompositePass();
}

void JGDevScene::RenderGeometryPass()
{
	for (int32 i = 0; i < GBuffer_Count; ++i)
	{
		GraphicsCommand->ClearTexture(GBufferTextures[i]);
	}
	GraphicsCommand->ClearDepthTexture(DepthStencilTexture);

	GraphicsCommand->BeginDraw();

	HRenderTarget renderTarget;
	for (int32 i = 0; i < GBuffer_Count; ++i)
	{
		renderTarget.RenderTextures[i] = GBufferTextures[i];
	}
	renderTarget.DepthTexture = DepthStencilTexture;
	renderTarget.Viewports.push_back(HViewport((float32)DevSceneTextureWidth, (float32)DevSceneTextureHeight));
	renderTarget.ScissorRects.push_back(HScissorRect(0, 0, (int32)DevSceneTextureWidth, (int32)DevSceneTextureHeight));
	GraphicsCommand->SetRenderTarget(renderTarget);

	HRenderPassCBData passData;
	FillRenderPassData(passData);
	GraphicsCommand->SetRenderPassData(passData);

	// 메시는 비동기 로드라 아직 없을 수 있다. 없으면 G버퍼는 클리어 값(배경)만 남는다.
	if (Mesh.IsValid() && Mesh->IsValid())
	{
		HDrawArguments drawArgs;
		drawArgs.Mesh = Mesh->GetMesh();
		drawArgs.ObjectCBData.WorldMatrix = HMatrix::Identity();
		GraphicsCommand->Draw(drawArgs);
	}

	GraphicsCommand->EndDraw();
}

void JGDevScene::RenderCompositePass()
{
	GraphicsCommand->ClearTexture(SceneTexture);

	// 합성 머터리얼이 없으면 SceneTexture는 클리어 컬러(빨강)로 남아 실패가 눈에 보인다.
	if (CompositeMaterial.IsValid() == false)
	{
		return;
	}

	GraphicsCommand->BeginDraw();

	HRenderTarget renderTarget;
	renderTarget.RenderTextures[0] = SceneTexture;
	renderTarget.Viewports.push_back(HViewport((float32)DevSceneTextureWidth, (float32)DevSceneTextureHeight));
	renderTarget.ScissorRects.push_back(HScissorRect(0, 0, (int32)DevSceneTextureWidth, (int32)DevSceneTextureHeight));
	GraphicsCommand->SetRenderTarget(renderTarget);

	HRenderPassCBData passData;
	FillRenderPassData(passData);
	GraphicsCommand->SetRenderPassData(passData);

	HSceneDrawArguments drawArgs;
	drawArgs.Material = CompositeMaterial;
	GraphicsCommand->Draw(drawArgs);

	GraphicsCommand->EndDraw();
}
