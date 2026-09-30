#include "PCH/PCH.h"
#include "DevScene.h"
#include "JGGraphicsDefine.h"
#include "JGGraphics.h"
#include "Classes/Texture.h"
#include "Classes/StaticMesh.h"
#include "Classes/Scene.h"
#include "Classes/SceneRenderer.h"
#include "AssetDatabase.h"
#include "GUI.h"

// 리드백 결과를 PNG로 저장하는 데만 쓴다. (검증용)
#pragma warning(push)
#pragma warning(disable : 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#pragma warning(pop)

namespace
{
	constexpr uint32 DevSceneTextureWidth  = 1920;
	constexpr uint32 DevSceneTextureHeight = 1080;

	// 첫 메시 드로우 검증용 에셋 (FBX X Bot을 임포트한 JGStaticMesh)
	constexpr const char* DevSceneMeshAssetPath = "/JGEngine/TempAsset/Sample.jgasset";

	// 리드백 검증. 메시 로드 뒤 이 프레임 수가 지나면 한 번 덤프한다. (그동안 G버퍼와 출력 텍스처가 몇 번 그려진다)
	constexpr int32       DevSceneReadbackDumpFrame  = 30;
	constexpr const char* DevSceneReadbackAlbedoPath = "DevScene_Readback_Albedo_Async.png";
	constexpr const char* DevSceneReadbackScenePath  = "DevScene_Readback_Scene_Immediate.png";

	// IEEE 754 half -> float
	float32 halfToFloat(uint16 h)
	{
		const uint32 sign     = (uint32)(h & 0x8000) << 16;
		const uint32 exponent = (h >> 10) & 0x1F;
		const uint32 mantissa = h & 0x03FF;

		uint32 bits = 0;
		if (exponent == 0)
		{
			if (mantissa != 0)
			{
				// 비정규화 값
				uint32 e = 127 - 15 + 1;
				uint32 m = mantissa;
				while ((m & 0x0400) == 0)
				{
					m <<= 1;
					--e;
				}
				m &= 0x03FF;
				bits = sign | (e << 23) | (m << 13);
			}
			else
			{
				bits = sign;
			}
		}
		else if (exponent == 0x1F)
		{
			bits = sign | 0x7F800000 | (mantissa << 13);
		}
		else
		{
			bits = sign | ((exponent + (127 - 15)) << 23) | (mantissa << 13);
		}

		float32 result = 0.0f;
		memcpy(&result, &bits, sizeof(result));
		return result;
	}

	uint8 floatToByte(float32 v)
	{
		return (uint8)(HMath::Clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
	}

	// 읽어 온 픽셀을 RGBA8로 바꾼다. R8G8B8A8_Unorm은 그대로, R16G16B16A16_Float은 0~1로 잘라 8비트로.
	bool convertToRGBA8(const HTexturePixels& InPixels, std::vector<uint8>& OutRGBA)
	{
		if (InPixels.IsValid() == false)
		{
			return false;
		}

		const uint64 pixelCount = (uint64)InPixels.Width * InPixels.Height;
		OutRGBA.resize(pixelCount * 4);

		switch (InPixels.Format)
		{
		case ETextureFormat::R8G8B8A8_Unorm:
			memcpy(OutRGBA.data(), InPixels.Data.data(), pixelCount * 4);
			return true;
		case ETextureFormat::R16G16B16A16_Float:
		{
			const uint16* src = (const uint16*)InPixels.Data.data();
			for (uint64 i = 0; i < pixelCount; ++i)
			{
				for (int32 c = 0; c < 4; ++c)
				{
					OutRGBA[i * 4 + c] = floatToByte(halfToFloat(src[i * 4 + c]));
				}
			}
			return true;
		}
		default:
			return false;
		}
	}

	bool writePNG(const char* InPath, const HTexturePixels& InPixels, bool bForceOpaque)
	{
		std::vector<uint8> rgba;   // 8MB. 엔진 풀 블록 한도(2MB)를 넘으므로 std 할당자
		if (convertToRGBA8(InPixels, rgba) == false)
		{
			return false;
		}
		// 알파를 보존하면 뷰어가 배경(알파 0)을 흰색으로 보여 실루엣이 안 보인다. 눈으로 확인할 용도면 불투명하게 저장한다.
		if (bForceOpaque)
		{
			for (uint64 i = 3; i < rgba.size(); i += 4)
			{
				rgba[i] = 255;
			}
		}
		return stbi_write_png(InPath, (int32)InPixels.Width, (int32)InPixels.Height, 4, rgba.data(), (int32)InPixels.Width * 4) != 0;
	}
}

void JGDevScene::OnInitialize()
{
	FramesSinceMeshLoaded = 0;
	bReadbackDumped       = false;

	Scene    = Allocate<PScene>();
	Renderer = Allocate<PSceneRenderer>();
	Renderer->Initialize(PString("DevScene"), DevSceneTextureWidth, DevSceneTextureHeight);

	// 기본 카메라. 메시가 로드되면 FitCameraToMesh가 경계 상자에 맞춰 다시 잡는다.
	HSceneCamera camera;
	camera.SetLookAt(HVector3(0.0f, 100.0f, -300.0f), HVector3(0.0f, 100.0f, 0.0f), HVector3(0.0f, 1.0f, 0.0f));
	Camera = Scene->CreateCamera(camera);

	RequestMeshLoad();
}

void JGDevScene::OnShutdown()
{
	Mesh     = nullptr;
	Renderer = nullptr;
	Scene    = nullptr;
	Camera       = HSceneCameraID();
	MeshInstance = HSceneMeshID();
}

void JGDevScene::OnLayout(const HWidgetComponentLayout& InLayout)
{
	SceneSize = InLayout.ContentSize;
}

void JGDevScene::OnGenerateGUI()
{
	if (Scene.IsValid() == false || Renderer.IsValid() == false || Renderer->GetOutputTexture() == nullptr)
	{
		return;
	}

	// GenerateGUI는 GraphicsBegin(펜스 대기) 뒤에 매 프레임 불리고, GUI가 이 텍스처를 실제로 그리는 시점은
	// GraphicsEnd의 프레임버퍼 갱신이므로, 여기서 기록한 드로우 커맨드가 먼저 실행된다.
	Renderer->Render(*Scene, Camera);

	// 메시가 그려지기 시작한 뒤 몇 프레임 지나면 리드백을 한 번 검증한다.
	if (bReadbackDumped == false && MeshInstance.IsValid())
	{
		++FramesSinceMeshLoaded;
		if (FramesSinceMeshLoaded >= DevSceneReadbackDumpFrame)
		{
			DumpReadbackOnce();
		}
	}

	HGUI::Image(Renderer->GetOutputTexture()->GetTextureID(), SceneSize);
	if (ReadbackStatus.Empty() == false)
	{
		HGUI::Text(ReadbackStatus);
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

	if (Scene.IsValid() == false)
	{
		return;
	}

	Mesh = mesh;

	// 장면에 메시 하나를 원점에 놓는다. 머터리얼은 메시의 것(비워 두면 서브메시 머터리얼 = 기본 머터리얼).
	HSceneMesh sceneMesh;
	sceneMesh.Mesh        = Mesh->GetMesh();
	sceneMesh.WorldMatrix = HMatrix::Identity();
	MeshInstance = Scene->CreateMesh(sceneMesh);

	FitCameraToMesh();

	JG_LOG(Devkit, ELogLevel::Info, "DevScene : Mesh Loaded (%s) SubMesh %d, Vertex %d, Index %d",
		DevSceneMeshAssetPath, (int32)Mesh->GetSubMeshCount(), (int32)Mesh->GetTotalVertexCount(), (int32)Mesh->GetTotalIndexCount());
}

void JGDevScene::FitCameraToMesh()
{
	if (Mesh.IsValid() == false || Scene.IsValid() == false)
	{
		return;
	}

	const HSceneCamera* currentCamera = Scene->FindCamera(Camera);
	if (currentCamera == nullptr)
	{
		return;
	}

	HBBox bounds;
	if (Mesh->GetBounds(bounds) == false)
	{
		return;
	}

	// 경계 구가 시야에 꽉 차는 거리에서 -Z 쪽(LH 정면)에서 바라본다.
	HSceneCamera camera = *currentCamera;
	const HVector3 center = (bounds.min + bounds.max) * 0.5f;
	const HVector3 extent = (bounds.max - bounds.min) * 0.5f;
	const float32  radius = HMath::Max(HVector3::Length(extent), 1.0f);
	const float32  distance = radius / sinf(camera.FovY * 0.5f) * 1.1f;

	camera.SetLookAt(center + HVector3(0.0f, radius * 0.1f, -distance), center, HVector3(0.0f, 1.0f, 0.0f));
	camera.NearZ = HMath::Max(distance - radius * 2.0f, 0.1f);
	camera.FarZ  = distance + radius * 2.0f;
	Scene->SetCamera(Camera, camera);

	JG_LOG(Devkit, ELogLevel::Info, "DevScene : Camera fitted. Bounds min(%.1f, %.1f, %.1f) max(%.1f, %.1f, %.1f) distance %.1f",
		bounds.min.x, bounds.min.y, bounds.min.z, bounds.max.x, bounds.max.y, bounds.max.z, distance);
}

void JGDevScene::DumpReadbackOnce()
{
	bReadbackDumped = true;

	// 1. 비동기: 알베도 G버퍼. 복사는 이번 프레임 제출의 마지막(드로우 뒤)에 기록되고 콜백은 다음 BeginFrame에 온다.
	PWeakPtr<JGDevScene> weakThis = SharedWrap(this);
	const bool bRequested = GetGraphicsAPI().RequestTextureReadback(Renderer->GetGBufferTexture(ESceneGBuffer::Albedo),
		HOnTextureReadbackComplete::CreateLambda([weakThis](const HTexturePixels& InPixels)
		{
			PSharedPtr<JGDevScene> self = weakThis.Pin();
			if (self.IsValid())
			{
				self->OnAlbedoReadback(InPixels);
			}
		}));

	// 2. 동기: 출력 텍스처(R16G16B16A16_Float). 프레임 중간 호출이라 결과는 지난 프레임의 최종 이미지다.
	HTexturePixels scenePixels;
	const bool bRead = GetGraphicsAPI().ReadbackTextureImmediate(Renderer->GetOutputTexture(), scenePixels);
	bool bSaved = false;
	if (bRead)
	{
		bSaved = writePNG(DevSceneReadbackScenePath, scenePixels, false);

		const uint16* center = (const uint16*)scenePixels.GetPixel(scenePixels.Width / 2, scenePixels.Height / 2);
		JG_LOG(Devkit, ELogLevel::Info, "DevScene : ReadbackTextureImmediate(Scene) %dx%d, %d bytes/pixel, %d bytes, center RGBA(%.3f, %.3f, %.3f, %.3f), saved %s -> %s",
			(int32)scenePixels.Width, (int32)scenePixels.Height, (int32)scenePixels.BytesPerPixel, (int32)scenePixels.Data.size(),
			halfToFloat(center[0]), halfToFloat(center[1]), halfToFloat(center[2]), halfToFloat(center[3]),
			PString(bSaved ? "ok" : "fail"), PString(DevSceneReadbackScenePath));
	}
	else
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : ReadbackTextureImmediate(Scene) failed");
	}

	ReadbackStatus = PString::Format("Readback : async albedo %s, immediate scene %s",
		PString(bRequested ? "requested" : "request failed"), PString((bRead && bSaved) ? "saved" : "failed"));
}

void JGDevScene::OnAlbedoReadback(const HTexturePixels& InPixels)
{
	if (InPixels.IsValid() == false)
	{
		JG_LOG(Devkit, ELogLevel::Error, "DevScene : RequestTextureReadback(Albedo) returned empty pixels");
		ReadbackStatus += PString(" / async albedo: empty");
		return;
	}

	// 알파 > 0 인 픽셀 = 메시가 덮은 픽셀. (클리어 값은 알파 0)
	uint64 coveredCount = 0;
	for (uint32 y = 0; y < InPixels.Height; ++y)
	{
		for (uint32 x = 0; x < InPixels.Width; ++x)
		{
			if (InPixels.GetPixel(x, y)[3] > 0)
			{
				++coveredCount;
			}
		}
	}

	const uint8* center = InPixels.GetPixel(InPixels.Width / 2, InPixels.Height / 2);
	const bool   bSaved = writePNG(DevSceneReadbackAlbedoPath, InPixels, true);   // 배경 알파 0을 불투명 검정으로
	const float32 coveredPercent = (float32)coveredCount * 100.0f / (float32)((uint64)InPixels.Width * InPixels.Height);

	JG_LOG(Devkit, ELogLevel::Info, "DevScene : RequestTextureReadback(Albedo) %dx%d, %d bytes/pixel, covered %d px (%.1f%%), center RGBA(%d, %d, %d, %d), saved %s -> %s",
		(int32)InPixels.Width, (int32)InPixels.Height, (int32)InPixels.BytesPerPixel, (int32)coveredCount, coveredPercent,
		(int32)center[0], (int32)center[1], (int32)center[2], (int32)center[3],
		PString(bSaved ? "ok" : "fail"), PString(DevSceneReadbackAlbedoPath));

	ReadbackStatus += PString::Format(" / async albedo: covered %d px, %s", (int32)coveredCount, PString(bSaved ? "saved" : "save failed"));
}
