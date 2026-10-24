#include "PCH/PCH.h"
#include "SelfTest/RenderSelfTest.h"
#include "JGGraphics.h"
#include "Classes/SceneRenderer.h"
#include "Classes/Mesh.h"
#include "Classes/StaticMesh.h"
#include "Classes/Material.h"
#include "Classes/Texture.h"
#include "Classes/VertexBuffer.h"
#include "Classes/IndexBuffer.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"

// 리드백 결과를 PNG 증적으로 남기는 데만 쓴다.
#pragma warning(push)
#pragma warning(disable : 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#pragma warning(pop)

namespace
{
	constexpr uint32 SelfTestWidth  = 1920;
	constexpr uint32 SelfTestHeight = 1080;

	// 업로드가 반영되고 프레임 파이프라인이 한 바퀴 돈 뒤 읽는다.
	constexpr int32 RenderFrameCount   = 10;
	// 비동기 리드백 콜백은 GPU 복사가 끝난 뒤의 BeginFrame(1~2 프레임 뒤)에 온다.
	constexpr int32 ReadbackWaitFrames = 3;

	// 앞 상자 텍스처 색(1x1, R8G8B8A8_Unorm).
	const uint8 TextureColor[4] = { 200, 100, 50, 255 };

	// 덮인 픽셀 수(알베도 알파 > 0). 투영(FovY 45°, 1920x1080)으로 계산한 값:
	// 앞 상자 앞면 289.7 x 289.7 + 뒤 상자 앞면 372.5 x 372.5 - 겹침 144.9 x 289.7 = 180,705. 가장자리 래스터 차이로 1% 를 둔다.
	constexpr uint64  ExpectedCoverage       = 180705;
	constexpr float32 CoverageTolerance      = 0.01f;
	constexpr int32   AlbedoTolerance        = 1;
	constexpr float32 OutputTolerance        = 0.005f;
	constexpr int32   DisplayTolerance       = 2;

	// 찍어 볼 픽셀. 상자 경계에서 60 픽셀 넘게 떨어져 있다.
	struct HProbe
	{
		const char* Name;
		uint32 X;
		uint32 Y;
	};
	const HProbe FrontProbe      = { "P1 front",      900, 540 };
	const HProbe OverlapProbe    = { "P2 overlap",   1030, 540 };
	const HProbe BackProbe       = { "P3 back",      1200, 540 };
	const HProbe BackgroundProbe = { "P4 background", 100, 100 };

	// 합성 셰이더(SceneRenderer.cpp CompositeShaderCode)와 같은 식을 C++ 로 따로 계산한다. 식을 바꾸면 이 시험도 함께 고친다.
	const HVector3 CompositeBackground(0.10f, 0.12f, 0.16f);

	float32 litScale()
	{
		// 앞면 법선 (0, 0, -1) · 빛 방향 normalize(-0.4, 0.8, -0.6)
		const float32 lightLength = sqrtf(0.4f * 0.4f + 0.8f * 0.8f + 0.6f * 0.6f);
		const float32 ndl = 0.6f / lightLength;
		return 0.15f + 0.85f * ndl;
	}

	// 디스플레이 셰이더(DisplayShaderCode)와 같은 sRGB 전달 함수.
	float32 encodeSRGB(float32 inLinear)
	{
		const float32 value = HMath::Clamp(inLinear, 0.0f, 1.0f);
		if (value <= 0.0031308f)
		{
			return value * 12.92f;
		}
		return 1.055f * powf(value, 1.0f / 2.4f) - 0.055f;
	}

	int32 toByte(float32 inValue)
	{
		return (int32)(HMath::Clamp(inValue, 0.0f, 1.0f) * 255.0f + 0.5f);
	}

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

	HVector3 readOutput(const HTexturePixels& inPixels, const HProbe& inProbe)
	{
		const uint16* pixel = (const uint16*)inPixels.GetPixel(inProbe.X, inProbe.Y);
		return HVector3(halfToFloat(pixel[0]), halfToFloat(pixel[1]), halfToFloat(pixel[2]));
	}

	bool isNear(float32 inValue, float32 inExpected, float32 inTolerance)
	{
		return fabsf(inValue - inExpected) <= inTolerance;
	}

	bool isNear(int32 inValue, int32 inExpected, int32 inTolerance)
	{
		return abs(inValue - inExpected) <= inTolerance;
	}

	// 읽어 온 픽셀을 RGBA8로 바꾼다. R8G8B8A8_Unorm은 그대로, R16G16B16A16_Float은 0~1로 잘라 8비트로.
	bool convertToRGBA8(const HTexturePixels& inPixels, std::vector<uint8>& outRGBA)
	{
		if (inPixels.IsValid() == false)
		{
			return false;
		}

		const uint64 pixelCount = (uint64)inPixels.Width * inPixels.Height;
		outRGBA.resize(pixelCount * 4);

		switch (inPixels.Format)
		{
		case ETextureFormat::R8G8B8A8_Unorm:
			memcpy(outRGBA.data(), inPixels.Data.data(), pixelCount * 4);
			return true;
		case ETextureFormat::R16G16B16A16_Float:
		{
			const uint16* src = (const uint16*)inPixels.Data.data();
			for (uint64 i = 0; i < pixelCount; ++i)
			{
				for (int32 c = 0; c < 4; ++c)
				{
					outRGBA[i * 4 + c] = (uint8)toByte(halfToFloat(src[i * 4 + c]));
				}
			}
			return true;
		}
		default:
			return false;
		}
	}

	bool writePNG(const PString& inPath, const HTexturePixels& inPixels, bool bForceOpaque)
	{
		std::vector<uint8> rgba;   // 8MB. 엔진 풀 블록 한도를 넘으므로 std 할당자
		if (convertToRGBA8(inPixels, rgba) == false)
		{
			return false;
		}
		// 알베도는 배경 알파가 0이라 뷰어가 흰색으로 보여 준다. 눈으로 볼 증적이라 불투명하게 저장한다.
		if (bForceOpaque)
		{
			for (uint64 i = 3; i < rgba.size(); i += 4)
			{
				rgba[i] = 255;
			}
		}
		return stbi_write_png(inPath.GetRawString().c_str(),(int32)inPixels.Width, (int32)inPixels.Height, 4, rgba.data(), (int32)inPixels.Width * 4) != 0;
	}

	// 한 변 1, 원점 중심 상자. 면마다 정점 4개(법선 · UV 따로). 바깥에서 볼 때 시계 방향이 앞면이다(뒷면 버림).
	// right = cross(up, -normal) 이라 BL → TL → TR 이 바깥에서 시계 방향이다.
	void buildCube(HList<HVertexData>& outVertices, HList<uint32>& outIndices)
	{
		struct HFace
		{
			HVector3 Normal;
			HVector3 Right;
			HVector3 Up;
		};
		const HFace faces[6] =
		{
			{ HVector3( 0.0f,  0.0f, -1.0f), HVector3( 1.0f, 0.0f,  0.0f), HVector3(0.0f, 1.0f, 0.0f) },
			{ HVector3( 0.0f,  0.0f,  1.0f), HVector3(-1.0f, 0.0f,  0.0f), HVector3(0.0f, 1.0f, 0.0f) },
			{ HVector3( 1.0f,  0.0f,  0.0f), HVector3( 0.0f, 0.0f,  1.0f), HVector3(0.0f, 1.0f, 0.0f) },
			{ HVector3(-1.0f,  0.0f,  0.0f), HVector3( 0.0f, 0.0f, -1.0f), HVector3(0.0f, 1.0f, 0.0f) },
			{ HVector3( 0.0f,  1.0f,  0.0f), HVector3( 1.0f, 0.0f,  0.0f), HVector3(0.0f, 0.0f, 1.0f) },
			{ HVector3( 0.0f, -1.0f,  0.0f), HVector3(-1.0f, 0.0f,  0.0f), HVector3(0.0f, 0.0f, 1.0f) },
		};
		const HVector2 uvs[4] = { HVector2(0.0f, 1.0f), HVector2(0.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(1.0f, 1.0f) };
		const float32  signs[4][2] = { { -1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f } };   // BL, TL, TR, BR

		outVertices.clear();
		outIndices.clear();
		for (const HFace& face : faces)
		{
			const uint32 base = (uint32)outVertices.size();
			for (int32 i = 0; i < 4; ++i)
			{
				HVertexData vertex;
				vertex.Position  = face.Normal * 0.5f + face.Right * (0.5f * signs[i][0]) + face.Up * (0.5f * signs[i][1]);
				vertex.Texcoord  = uvs[i];
				vertex.Normal    = face.Normal;
				vertex.Tangent   = face.Right;
				vertex.Bitangent = face.Up;
				outVertices.push_back(vertex);
			}
			outIndices.push_back(base + 0);
			outIndices.push_back(base + 1);
			outIndices.push_back(base + 2);
			outIndices.push_back(base + 0);
			outIndices.push_back(base + 2);
			outIndices.push_back(base + 3);
		}
	}

	int32 countErrorLogsSince(uint64 inSerial)
	{
		const GLogGlobalSystem& log = GLogGlobalSystem::GetInstance();
		const uint64 newLineCount = log.GetLogSerial() - inSerial;

		HList<HLogLine> lines;
		log.GetRecentLogs(lines);

		const uint64 lineCount = lines.size();
		const uint64 firstNewLine = (newLineCount < lineCount) ? lineCount - newLineCount : 0;

		int32 errorCount = 0;
		for (uint64 i = firstNewLine; i < lineCount; ++i)
		{
			if (lines[i].Level == ELogLevel::Error || lines[i].Level == ELogLevel::Critical)
			{
				++errorCount;
			}
		}
		return errorCount;
	}

	// graphics.selftest [-exit] — 에디터 DevConsole, JGConsole -graphics 에서 쓴다. 결과는 끝날 때 로그 한 줄(OK/FAILED)로 나온다.
	bool executeRenderSelfTest(const HConsoleCommandArgs& inArgs)
	{
		HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>();
		if (graphicsModule == nullptr)
		{
			JG_LOG(GraphicsSelfTest, ELogLevel::Error, "graphics.selftest: Graphics module is not connected");
			return false;
		}
		return graphicsModule->StartRenderSelfTest(inArgs.Has("exit"));
	}

	HAutoConsoleCommand RenderSelfTestCommand(
		"graphics.selftest",
		"graphics.selftest [-exit]",
		"Render two boxes with PScene + PSceneRenderer, read back and compare coverage, depth, material, lighting and sRGB with precomputed values",
		&executeRenderSelfTest);
}

bool PRenderSelfTest::Start(bool bExitWhenDone)
{
	_bExitWhenDone    = bExitWhenDone;
	_logSerialAtStart = GLogGlobalSystem::GetInstance().GetLogSerial();
	_state = EState::Rendering;

	JG_LOG(GraphicsSelfTest, ELogLevel::Info, "graphics.selftest: start (%dx%d, %d frames)", (int32)SelfTestWidth, (int32)SelfTestHeight, RenderFrameCount);

	if (createResources() == false)
	{
		finish();
		return true;
	}

	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::Update, PTaskDelegate::CreateSP(SharedWrap(this), &PRenderSelfTest::tick));
	return true;
}

bool PRenderSelfTest::IsRunning() const
{
	return _state == EState::Rendering || _state == EState::WaitReadback;
}

bool PRenderSelfTest::createResources()
{
	// 1. 렌더러
	_renderer = Allocate<PSceneRenderer>();
	const bool bInitialized = _renderer->Initialize(PString("GraphicsSelfTest"), SelfTestWidth, SelfTestHeight);
	PSharedPtr<IRawTexture> output  = _renderer->GetOutputTexture();
	PSharedPtr<IRawTexture> display = _renderer->GetDisplayTexture();
	const bool bRendererReady = bInitialized && output != nullptr && display != nullptr
		&& output->GetTextureInfo().Format == ETextureFormat::R16G16B16A16_Float
		&& display->GetTextureInfo().Format == ETextureFormat::R8G8B8A8_Unorm
		&& output->GetTextureInfo().Width == SelfTestWidth && output->GetTextureInfo().Height == SelfTestHeight;
	check(bRendererReady, PString::Format("renderer: Initialize %dx%d, output R16G16B16A16_Float, display R8G8B8A8_Unorm", (int32)SelfTestWidth, (int32)SelfTestHeight));
	if (bRendererReady == false)
	{
		return false;
	}

	// 2. 상자 메시 · 텍스처 · 머터리얼 (Graphics API 로 직접)
	HList<HVertexData> vertices;
	HList<uint32>      indices;
	buildCube(vertices, indices);

	HVertexBufferConstructArguments vertexArgs;
	vertexArgs.Name       = PName("GraphicsSelfTest_CubeVB");
	vertexArgs.LoadMethod = EBufferLoadMethod::GPULoad;
	PSharedPtr<IVertexBuffer> vertexBuffer = GetGraphicsAPI().CreateVertexBuffer(vertexArgs);

	HIndexBufferConstructArguments indexArgs;
	indexArgs.Name       = PName("GraphicsSelfTest_CubeIB");
	indexArgs.LoadMethod = EBufferLoadMethod::GPULoad;
	PSharedPtr<IIndexBuffer> indexBuffer = GetGraphicsAPI().CreateIndexBuffer(indexArgs);

	if (vertexBuffer != nullptr && indexBuffer != nullptr)
	{
		vertexBuffer->SetDatas(vertices.data(), sizeof(HVertexData), vertices.size());
		indexBuffer->SetDatas(indices.data(), indices.size());
	}

	// 게임 메시와 같은 렌더 메시(PStaticMesh)를 같은 입구(JGStaticMesh::GetMesh 가 부르는 SetSubMeshes)로 만든다. (H-103)
	// 머터리얼은 비워 둔다. PStaticMesh::GetMaterial 이 기본 머터리얼(흰색)로 대신한다.
	HRenderSubMesh subMesh;
	subMesh.Name         = PName("Cube");
	subMesh.VertexBuffer = vertexBuffer;
	subMesh.IndexBuffer  = indexBuffer;

	HList<HRenderSubMesh> subMeshes;
	subMeshes.push_back(subMesh);

	PSharedPtr<PStaticMesh> cubeMesh = Allocate<PStaticMesh>();
	cubeMesh->SetName(PName("GraphicsSelfTest_Cube"));
	cubeMesh->SetSubMeshes(subMeshes);
	_cubeMesh = cubeMesh;

	HTextureInfo textureInfo;
	textureInfo.Name       = "GraphicsSelfTest_Albedo";
	textureInfo.Width      = 1;
	textureInfo.Height     = 1;
	textureInfo.Format     = ETextureFormat::R8G8B8A8_Unorm;
	textureInfo.Flags      = ETextureFlags::None;
	textureInfo.MipLevel   = 1;
	textureInfo.ArraySize  = 1;
	textureInfo.FilterMode = ETextureFilterMode::Point;
	textureInfo.WrapMode   = ETextureWrapMode::Clamp;
	_texture = GetGraphicsAPI().CreateRawTexture(TextureColor, textureInfo);

	HRawMaterialConstructArguments materialArgs;
	materialArgs.Name   = PName("GraphicsSelfTest_TexturedMaterial");
	materialArgs.Domain = EMaterialDomain::Surface;
	materialArgs.PropertyDefinitionist.DefineTexture(PName("AlbedoTex"));
	_texturedMaterial = GetGraphicsAPI().CreateRawMaterial(materialArgs);

	bool bMaterialCompiled = false;
	if (_texturedMaterial != nullptr && _texturedMaterial->IsValid() && _texture != nullptr && _texture->IsValid())
	{
		// 텍스처는 컴파일 전에 넣는다. 샘플러 이름이 컴파일 시점에 코드에 박힌다. (5-21)
		_texturedMaterial->SetTexture(PName("AlbedoTex"), _texture);

		HMaterialCompileArguments compileArgs;
		compileArgs.ShaderCode = "_output.albedo = AlbedoTex;";
		bMaterialCompiled = _texturedMaterial->Compile(compileArgs);
	}

	const bool bResourcesReady = _cubeMesh->IsValid() && vertexBuffer->GetVertexCount() == 24 && indexBuffer->GetIndexCount() == 36
		&& _texture != nullptr && _texture->IsValid() && bMaterialCompiled;
	check(bResourcesReady, PString("resources: cube vertex 24 / index 36 buffers, 1x1 texture, textured Surface material compiles"));
	if (bResourcesReady == false)
	{
		return false;
	}

	// 3. 장면: 카메라 (0, 0, -5) → +Z, 앞 상자(텍스처 머터리얼로 덮어쓰기), 뒤 상자(2배 · (1, 0, 3), 기본 머터리얼)
	_scene = Allocate<PScene>();

	HSceneCamera camera;
	camera.SetLookAt(HVector3(0.0f, 0.0f, -5.0f), HVector3(0.0f, 0.0f, 0.0f), HVector3(0.0f, 1.0f, 0.0f));
	_camera = _scene->CreateCamera(camera);

	HSceneMesh front;
	front.Mesh = _cubeMesh;
	front.Materials.push_back(_texturedMaterial);
	front.WorldMatrix = HMatrix::Identity();
	_scene->CreateMesh(front);

	HSceneMesh back;
	back.Mesh = _cubeMesh;
	back.WorldMatrix = HMatrix::Scaling(HVector3(2.0f, 2.0f, 2.0f)) * HMatrix::Translation(HVector3(1.0f, 0.0f, 3.0f));
	_scene->CreateMesh(back);

	return true;
}

void PRenderSelfTest::tick()
{
	if (_state == EState::Rendering)
	{
		if (_renderedFrames < RenderFrameCount)
		{
			_renderer->Render(*_scene, _camera);
			++_renderedFrames;
			return;
		}

		// 그리기를 멈춘 프레임에서 읽는다. 즉시 리드백은 지난 프레임까지 GPU 가 끝낸 내용, 비동기 리드백은 이 프레임 제출 끝에 복사한다.
		requestReadbacks();
		_state = EState::WaitReadback;
		_framesSinceReadback = 0;
		return;
	}

	if (_state == EState::WaitReadback)
	{
		++_framesSinceReadback;
		if (_bAlbedoArrived == false && _framesSinceReadback <= ReadbackWaitFrames)
		{
			return;
		}

		judge();
		finish();
	}
}

void PRenderSelfTest::requestReadbacks()
{
	_bImmediateRead = GetGraphicsAPI().ReadbackTextureImmediate(_renderer->GetOutputTexture(), _outputPixels)
		&& GetGraphicsAPI().ReadbackTextureImmediate(_renderer->GetDisplayTexture(), _displayPixels);

	PWeakPtr<PRenderSelfTest> weakThis = SharedWrap(this);
	const bool bRequested = GetGraphicsAPI().RequestTextureReadback(_renderer->GetGBufferTexture(ESceneGBuffer::Albedo),
		HOnTextureReadbackComplete::CreateLambda([weakThis](const HTexturePixels& inPixels)
		{
			PSharedPtr<PRenderSelfTest> self = weakThis.Pin();
			if (self.IsValid())
			{
				self->onAlbedoReadback(inPixels);
			}
		}));

	if (bRequested == false)
	{
		JG_LOG(GraphicsSelfTest, ELogLevel::Error, "graphics.selftest: RequestTextureReadback(albedo) failed");
	}
}

void PRenderSelfTest::onAlbedoReadback(const HTexturePixels& inPixels)
{
	_albedoPixels   = inPixels;
	_bAlbedoArrived = true;
}

void PRenderSelfTest::judge()
{
	// 8. 오류 로그는 이 시험이 [FAIL] 을 남기기 전에 센다.
	const int32 errorLogCount = countErrorLogsSince(_logSerialAtStart);

	// 7. 비동기 리드백 도착
	const bool bAlbedoValid = _bAlbedoArrived && _albedoPixels.IsValid() && _albedoPixels.Width == SelfTestWidth && _albedoPixels.Height == SelfTestHeight
		&& _albedoPixels.Format == ETextureFormat::R8G8B8A8_Unorm;
	check(bAlbedoValid, PString::Format("async readback: albedo callback within %d frames (arrived after %d), %dx%d R8G8B8A8_Unorm",
		ReadbackWaitFrames, _framesSinceReadback, (int32)_albedoPixels.Width, (int32)_albedoPixels.Height));

	const bool bImmediateValid = _bImmediateRead && _outputPixels.IsValid() && _displayPixels.IsValid()
		&& _outputPixels.Format == ETextureFormat::R16G16B16A16_Float && _displayPixels.Format == ETextureFormat::R8G8B8A8_Unorm;
	check(bImmediateValid, PString("immediate readback: output R16G16B16A16_Float and display R8G8B8A8_Unorm read"));

	if (bAlbedoValid)
	{
		// 3. 덮인 픽셀 수
		uint64 coverage = 0;
		for (uint32 y = 0; y < _albedoPixels.Height; ++y)
		{
			for (uint32 x = 0; x < _albedoPixels.Width; ++x)
			{
				if (_albedoPixels.GetPixel(x, y)[3] > 0)
				{
					++coverage;
				}
			}
		}
		const float32 coverageError = fabsf((float32)coverage - (float32)ExpectedCoverage) / (float32)ExpectedCoverage;
		check(coverageError <= CoverageTolerance, PString::Format("coverage: %d px == %d px +-1%% (%.2f%%)", (int32)coverage, (int32)ExpectedCoverage, coverageError * 100.0f));

		// 4. 알베도: 앞 상자 = 텍스처 색, 겹친 곳도 앞 상자(깊이), 뒤 상자 = 흰색(기본 머터리얼), 배경 알파 0
		struct HAlbedoExpect
		{
			const HProbe* Probe;
			int32 Color[4];
		};
		const HAlbedoExpect albedoExpects[3] =
		{
			{ &FrontProbe,   { TextureColor[0], TextureColor[1], TextureColor[2], TextureColor[3] } },
			{ &OverlapProbe, { TextureColor[0], TextureColor[1], TextureColor[2], TextureColor[3] } },
			{ &BackProbe,    { 255, 255, 255, 255 } },
		};
		for (const HAlbedoExpect& expect : albedoExpects)
		{
			const uint8* pixel = _albedoPixels.GetPixel(expect.Probe->X, expect.Probe->Y);
			bool bMatch = true;
			for (int32 c = 0; c < 4; ++c)
			{
				bMatch = bMatch && isNear((int32)pixel[c], expect.Color[c], AlbedoTolerance);
			}
			check(bMatch, PString::Format("albedo %s (%d, %d): (%d, %d, %d, %d) == (%d, %d, %d, %d)", expect.Probe->Name, (int32)expect.Probe->X, (int32)expect.Probe->Y,
				(int32)pixel[0], (int32)pixel[1], (int32)pixel[2], (int32)pixel[3], expect.Color[0], expect.Color[1], expect.Color[2], expect.Color[3]));
		}
		const uint8* backgroundAlbedo = _albedoPixels.GetPixel(BackgroundProbe.X, BackgroundProbe.Y);
		check(backgroundAlbedo[3] == 0, PString::Format("albedo %s: alpha %d == 0", BackgroundProbe.Name, (int32)backgroundAlbedo[3]));
	}

	if (bImmediateValid)
	{
		// 5. 출력(선형) = 알베도 x 조명, 배경 = 합성 셰이더 단색
		const float32  lit = litScale();
		const HVector3 expectedFront((TextureColor[0] / 255.0f) * lit, (TextureColor[1] / 255.0f) * lit, (TextureColor[2] / 255.0f) * lit);
		const HVector3 expectedBack(lit, lit, lit);

		struct HOutputExpect
		{
			const HProbe* Probe;
			HVector3 Color;
		};
		const HOutputExpect outputExpects[3] =
		{
			{ &FrontProbe,      expectedFront },
			{ &BackProbe,       expectedBack },
			{ &BackgroundProbe, CompositeBackground },
		};
		for (const HOutputExpect& expect : outputExpects)
		{
			const HVector3 color = readOutput(_outputPixels, *expect.Probe);
			const bool bMatch = isNear(color.x, expect.Color.x, OutputTolerance) && isNear(color.y, expect.Color.y, OutputTolerance) && isNear(color.z, expect.Color.z, OutputTolerance);
			check(bMatch, PString::Format("output %s: (%.4f, %.4f, %.4f) == (%.4f, %.4f, %.4f) +-%.3f", expect.Probe->Name,
				color.x, color.y, color.z, expect.Color.x, expect.Color.y, expect.Color.z, OutputTolerance));

			// 6. 디스플레이 = sRGB(출력 기대값)
			const uint8* display = _displayPixels.GetPixel(expect.Probe->X, expect.Probe->Y);
			const int32 expected[3] = { toByte(encodeSRGB(expect.Color.x)), toByte(encodeSRGB(expect.Color.y)), toByte(encodeSRGB(expect.Color.z)) };
			const bool bDisplayMatch = isNear((int32)display[0], expected[0], DisplayTolerance) && isNear((int32)display[1], expected[1], DisplayTolerance)
				&& isNear((int32)display[2], expected[2], DisplayTolerance);
			check(bDisplayMatch, PString::Format("display %s: (%d, %d, %d) == sRGB (%d, %d, %d) +-%d", expect.Probe->Name,
				(int32)display[0], (int32)display[1], (int32)display[2], expected[0], expected[1], expected[2], DisplayTolerance));
		}
	}

	check(errorLogCount == 0, PString::Format("error logs during the test: %d == 0", errorLogCount));
}

void PRenderSelfTest::finish()
{
	writeEvidence();

	if (_failures == 0)
	{
		JG_LOG(GraphicsSelfTest, ELogLevel::Info, "graphics.selftest: OK (%d/%d)", _passed, _passed);
	}
	else
	{
		JG_LOG(GraphicsSelfTest, ELogLevel::Error, "graphics.selftest: FAILED (%d failures, %d passed)", _failures, _passed);
	}

	// 자원을 놓는다. GPU 리소스는 그 프레임이 끝난 뒤 미룬 해제로 놓인다. tick 은 Done 이라 더 하지 않는다.
	_scene            = nullptr;
	_renderer         = nullptr;
	_cubeMesh         = nullptr;
	_texturedMaterial = nullptr;
	_texture          = nullptr;
	_state = EState::Done;

	if (_bExitWhenDone)
	{
		GCoreSystem::RequestExit(_failures == 0 ? 0 : 1);
	}
}

void PRenderSelfTest::check(bool bCondition, const PString& inWhat)
{
	if (bCondition)
	{
		++_passed;
		JG_LOG(GraphicsSelfTest, ELogLevel::Info, "[PASS] %s", inWhat);
		return;
	}
	++_failures;
	JG_LOG(GraphicsSelfTest, ELogLevel::Error, "[FAIL] %s", inWhat);
}

void PRenderSelfTest::writeEvidence() const
{
	// 판정은 수치로 한다. PNG 는 사람이 볼 증적이다. <엔진>/Temp/GraphicsSelfTest/ 에 매번 덮어쓴다.
	PString directory;
	HFileHelper::CombinePath(HFileHelper::EngineTempDirectory(), "GraphicsSelfTest", &directory);
	if (HFileHelper::Exists(directory) == false)
	{
		HFileHelper::CreateDirectory(directory);
	}

	struct HEvidence
	{
		const char* FileName;
		const HTexturePixels* Pixels;
		bool bForceOpaque;
	};
	const HEvidence evidences[3] =
	{
		{ "albedo.png",  &_albedoPixels,  true },
		{ "output.png",  &_outputPixels,  false },
		{ "display.png", &_displayPixels, false },
	};
	for (const HEvidence& evidence : evidences)
	{
		if (evidence.Pixels->IsValid() == false)
		{
			continue;
		}
		PString path;
		HFileHelper::CombinePath(directory, evidence.FileName, &path);
		if (writePNG(path, *evidence.Pixels, evidence.bForceOpaque) == false)
		{
			JG_LOG(GraphicsSelfTest, ELogLevel::Warning, "graphics.selftest: fail to write %s", path);
		}
	}
	JG_LOG(GraphicsSelfTest, ELogLevel::Info, "graphics.selftest: evidence PNGs in %s", directory);
}
