#include "PCH/PCH.h"
#include "SceneRenderer.h"
#include "JGGraphics.h"
#include "JGGraphicsCommand.h"
#include "Classes/Texture.h"
#include "Classes/Material.h"
#include "Classes/Mesh.h"

namespace
{
	// 합성 셰이더. 텍스처 프로퍼티 토큰(GAlbedo 등)은 컴파일 시 _globalTexture[슬롯].Sample(샘플러, _input.tex)로 치환된다.
	// 배경(깊이 1)은 단색, 메시는 알베도 x 단순 램버트(고정 방향 조명). 형태가 보여야 깊이 테스트 결과를 눈으로 확인할 수 있다.
	constexpr const char* CompositeShaderCode =
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

	// 디스플레이 셰이더. 선형 출력 텍스처를 sRGB 전달 함수(IEC 61966-2-1)로 인코딩해 8비트 디스플레이 텍스처에 쓴다. (5-14)
	// 지금 장면은 1.0을 넘는 밝기가 없어 0~1로 자르기만 한다. 밝은 빛 · 발광이 생기면 인코딩 앞에 노출 · 톤매핑 곡선을 넣는다.
	// pow에 abs를 쓰는 건 FXC 경고(X3571, 음수 밑) 때문이다. saturate 뒤라 값은 그대로다.
	constexpr const char* DisplayShaderCode =
		"float3 linearColor = saturate(SceneColor.rgb);\n"
		"float3 low         = linearColor * 12.92f;\n"
		"float3 high        = 1.055f * pow(abs(linearColor), 1.0f / 2.4f) - 0.055f;\n"
		"_output.final      = float4(lerp(high, low, step(linearColor, 0.0031308f)), 1.0f);\n";
}

bool PSceneRenderer::Initialize(const PString& inName, uint32 inWidth, uint32 inHeight)
{
	if (inWidth == 0 || inHeight == 0)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : SceneRenderer size must not be zero (%dx%d)", inName, (int32)inWidth, (int32)inHeight);
		return false;
	}

	_width  = inWidth;
	_height = inHeight;

	if (_graphicsCommand == nullptr)
	{
		_graphicsCommand = GetGraphicsAPI().GetGraphicsCommand();
	}

	createTextures(inName);
	_graphicsCommand->ClearTexture(_outputTexture);
	_graphicsCommand->ClearTexture(_displayTexture);

	// 합성 · 디스플레이 머터리얼은 읽을 텍스처를 컴파일 전에 받아야 해서(샘플러 이름이 코드에 박힌다) 텍스처를 만들 때마다 새로 만든다.
	const bool bComposite = createCompositeMaterial(inName);
	const bool bDisplay   = createDisplayMaterial(inName);
	return bComposite && bDisplay;
}

void PSceneRenderer::Render(const PScene& inScene, HSceneCameraID inCamera)
{
	if (_graphicsCommand == nullptr || _outputTexture == nullptr)
	{
		return;
	}

	const HSceneCamera* camera = inScene.FindCamera(inCamera);
	if (camera == nullptr)
	{
		_graphicsCommand->ClearTexture(_outputTexture);
		_graphicsCommand->ClearTexture(_displayTexture);
		return;
	}

	HRenderPassCBData passData;
	fillRenderPassData(*camera, passData);

	renderGeometryPass(inScene, passData);
	renderCompositePass(passData);
	renderDisplayPass(passData);
}

PSharedPtr<IRawTexture> PSceneRenderer::GetOutputTexture() const
{
	return _outputTexture;
}

PSharedPtr<IRawTexture> PSceneRenderer::GetDisplayTexture() const
{
	return _displayTexture;
}

PSharedPtr<IRawTexture> PSceneRenderer::GetGBufferTexture(ESceneGBuffer inGBuffer) const
{
	if (inGBuffer >= ESceneGBuffer::Count)
	{
		return nullptr;
	}
	return _gbufferTextures[(uint32)inGBuffer];
}

uint32 PSceneRenderer::GetWidth() const
{
	return _width;
}

uint32 PSceneRenderer::GetHeight() const
{
	return _height;
}

void PSceneRenderer::createTextures(const PString& inName)
{
	// 합성 결과(선형 HDR). 클리어 컬러(빨강)는 "아무것도 그려지지 않았다"는 신호로 남겨 둔다.
	// 디스플레이 패스가 Point/Clamp로 1:1 읽는다. (샘플러 이름이 머터리얼 코드에 박힌다)
	{
		HTextureInfo texInfo;
		texInfo.Name   = PString::Format("%s_Output", inName);
		texInfo.Width  = _width;
		texInfo.Height = _height;
		texInfo.Format = ETextureFormat::R16G16B16A16_Float;
		texInfo.Flags  = ETextureFlags::Allow_RenderTarget;
		texInfo.MipLevel  = 1;
		texInfo.ArraySize = 1;
		texInfo.ClearColor = HLinearColor(1.0F, 0.0F, 0.0F, 1.0F);
		texInfo.FilterMode = ETextureFilterMode::Point;
		texInfo.WrapMode   = ETextureWrapMode::Clamp;

		_outputTexture = GetGraphicsAPI().CreateRawTexture(texInfo);
	}

	// 화면용 결과(sRGB 인코딩된 8비트). GUI 이미지 · 게임 UI가 쓴다. 클리어 컬러는 출력과 같은 빨강.
	{
		HTextureInfo texInfo;
		texInfo.Name   = PString::Format("%s_Display", inName);
		texInfo.Width  = _width;
		texInfo.Height = _height;
		texInfo.Format = ETextureFormat::R8G8B8A8_Unorm;
		texInfo.Flags  = ETextureFlags::Allow_RenderTarget;
		texInfo.MipLevel  = 1;
		texInfo.ArraySize = 1;
		texInfo.ClearColor = HLinearColor(1.0F, 0.0F, 0.0F, 1.0F);

		_displayTexture = GetGraphicsAPI().CreateRawTexture(texInfo);
	}

	// G버퍼. 합성 패스가 Point/Clamp 샘플러로 1:1 읽으므로 필터/랩 모드를 그렇게 둔다. (샘플러 이름이 머터리얼 코드에 박힌다)
	{
		struct HGBufferDesc
		{
			const char*    Suffix;
			ETextureFormat Format;
			HLinearColor   ClearColor;
		};
		const HGBufferDesc gbufferDescs[(uint32)ESceneGBuffer::Count] =
		{
			{ "GBuffer_Albedo",            ETextureFormat::R8G8B8A8_Unorm,     HLinearColor(0.0f, 0.0f, 0.0f, 0.0f) },
			{ "GBuffer_NormalMetallic",    ETextureFormat::R16G16B16A16_Float, HLinearColor(0.0f, 0.0f, 0.0f, 0.0f) },
			{ "GBuffer_SpecularRoughness", ETextureFormat::R8G8B8A8_Unorm,     HLinearColor(0.0f, 0.0f, 0.0f, 0.0f) },
			// 배경 깊이 = 1. 합성 패스가 배경 판정에 쓴다.
			{ "GBuffer_Depth",             ETextureFormat::R32_Float,          HLinearColor(1.0f, 1.0f, 1.0f, 1.0f) },
		};

		for (uint32 i = 0; i < (uint32)ESceneGBuffer::Count; ++i)
		{
			HTextureInfo texInfo;
			texInfo.Name   = PString::Format("%s_%s", inName, gbufferDescs[i].Suffix);
			texInfo.Width  = _width;
			texInfo.Height = _height;
			texInfo.Format = gbufferDescs[i].Format;
			texInfo.Flags  = ETextureFlags::Allow_RenderTarget;
			texInfo.MipLevel  = 1;
			texInfo.ArraySize = 1;
			texInfo.ClearColor = gbufferDescs[i].ClearColor;
			texInfo.FilterMode = ETextureFilterMode::Point;
			texInfo.WrapMode   = ETextureWrapMode::Clamp;

			_gbufferTextures[i] = GetGraphicsAPI().CreateRawTexture(texInfo);
		}
	}

	// 깊이 스텐실
	{
		HTextureInfo texInfo;
		texInfo.Name   = PString::Format("%s_DepthStencil", inName);
		texInfo.Width  = _width;
		texInfo.Height = _height;
		texInfo.Format = ETextureFormat::D24_Unorm_S8_Uint;
		texInfo.Flags  = ETextureFlags::Allow_DepthStencil;
		texInfo.MipLevel  = 1;
		texInfo.ArraySize = 1;
		texInfo.ClearDepth   = 1.0f;
		texInfo.ClearStencil = 0;

		_depthStencilTexture = GetGraphicsAPI().CreateRawTexture(texInfo);
	}
}

bool PSceneRenderer::createCompositeMaterial(const PString& inName)
{
	// Screen 도메인 풀스크린 머터리얼. G버퍼 3장을 텍스처 프로퍼티로 받아 출력 텍스처에 합성한다.
	HRawMaterialConstructArguments materialArgs;
	materialArgs.Name   = PName(PString::Format("%s_CompositeMaterial", inName));
	materialArgs.Domain = EMaterialDomain::Screen;
	materialArgs.PropertyDefinitionist.DefineTexture(PName("GAlbedo"));
	materialArgs.PropertyDefinitionist.DefineTexture(PName("GNormal"));
	materialArgs.PropertyDefinitionist.DefineTexture(PName("GDepth"));

	_compositeMaterial = GetGraphicsAPI().CreateRawMaterial(materialArgs);
	if (_compositeMaterial.IsValid() == false || _compositeMaterial->IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Create Composite Material", inName);
		_compositeMaterial = nullptr;
		return false;
	}

	// 텍스처는 컴파일 전에 넣는다. 샘플러 이름(Point/Clamp)이 컴파일 시점에 코드에 박히기 때문. (5-21)
	_compositeMaterial->SetTexture(PName("GAlbedo"), _gbufferTextures[(uint32)ESceneGBuffer::Albedo]);
	_compositeMaterial->SetTexture(PName("GNormal"), _gbufferTextures[(uint32)ESceneGBuffer::NormalMetallic]);
	_compositeMaterial->SetTexture(PName("GDepth"),  _gbufferTextures[(uint32)ESceneGBuffer::Depth]);

	HMaterialCompileArguments compileArgs;
	compileArgs.ShaderCode = CompositeShaderCode;
	if (_compositeMaterial->Compile(compileArgs) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile Composite Material", inName);
		_compositeMaterial = nullptr;
		return false;
	}
	return true;
}

bool PSceneRenderer::createDisplayMaterial(const PString& inName)
{
	// Screen 도메인 풀스크린 머터리얼. 출력 텍스처(선형)를 읽어 sRGB로 인코딩한다.
	HRawMaterialConstructArguments materialArgs;
	materialArgs.Name   = PName(PString::Format("%s_DisplayMaterial", inName));
	materialArgs.Domain = EMaterialDomain::Screen;
	materialArgs.PropertyDefinitionist.DefineTexture(PName("SceneColor"));

	_displayMaterial = GetGraphicsAPI().CreateRawMaterial(materialArgs);
	if (_displayMaterial.IsValid() == false || _displayMaterial->IsValid() == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Create Display Material", inName);
		_displayMaterial = nullptr;
		return false;
	}

	// 텍스처는 컴파일 전에 넣는다. (5-21)
	_displayMaterial->SetTexture(PName("SceneColor"), _outputTexture);

	HMaterialCompileArguments compileArgs;
	compileArgs.ShaderCode = DisplayShaderCode;
	if (_displayMaterial->Compile(compileArgs) == false)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Compile Display Material", inName);
		_displayMaterial = nullptr;
		return false;
	}
	return true;
}

void PSceneRenderer::fillRenderPassData(const HSceneCamera& inCamera, HRenderPassCBData& outData) const
{
	const float32 aspectRatio = (float32)_width / (float32)_height;

	// HMatrix는 행 우선(DirectXMath)이고 셰이더는 행 우선 패킹으로 컴파일되므로 그대로 올린다. (mul(v, M) 규약)
	const HMatrix view     = inCamera.ViewMatrix;
	const HMatrix proj     = inCamera.GetProjMatrix(aspectRatio);
	const HMatrix viewProj = view * proj;

	outData.ProjMatrix        = proj;
	outData.ViewMatrix        = view;
	outData.ViewProjMatrix    = viewProj;
	outData.InvViewMatrix     = HMatrix::Inverse(view);
	outData.InvProjMatrix     = HMatrix::Inverse(proj);
	outData.InvViewProjMatrix = HMatrix::Inverse(viewProj);
	outData.Resolution  = HVector2((float32)_width, (float32)_height);
	outData.NearZ       = inCamera.NearZ;
	outData.FarZ        = inCamera.FarZ;
	outData.EyePosition = inCamera.Position;
}

void PSceneRenderer::renderGeometryPass(const PScene& inScene, const HRenderPassCBData& inPassData)
{
	for (uint32 i = 0; i < (uint32)ESceneGBuffer::Count; ++i)
	{
		_graphicsCommand->ClearTexture(_gbufferTextures[i]);
	}
	_graphicsCommand->ClearDepthTexture(_depthStencilTexture);

	_graphicsCommand->BeginDraw();

	HRenderTarget renderTarget;
	for (uint32 i = 0; i < (uint32)ESceneGBuffer::Count; ++i)
	{
		renderTarget.RenderTextures[i] = _gbufferTextures[i];
	}
	renderTarget.DepthTexture = _depthStencilTexture;
	renderTarget.Viewports.push_back(HViewport((float32)_width, (float32)_height));
	renderTarget.ScissorRects.push_back(HScissorRect(0, 0, (int32)_width, (int32)_height));
	_graphicsCommand->SetRenderTarget(renderTarget);
	_graphicsCommand->SetRenderPassData(inPassData);

	// 메시는 비동기 로드라 아직 없을 수 있다. 배치가 없으면 G버퍼는 클리어 값(배경)만 남는다.
	for (const HPair<const uint64, HSceneMesh>& pair : inScene.GetMeshes())
	{
		const HSceneMesh& sceneMesh = pair.second;
		if (sceneMesh.Mesh == nullptr || sceneMesh.Mesh->IsValid() == false)
		{
			continue;
		}

		HDrawArguments drawArgs;
		drawArgs.Mesh = sceneMesh.Mesh;
		drawArgs.ObjectCBData.WorldMatrix = sceneMesh.WorldMatrix;
		drawArgs.MaterialOverrides = &sceneMesh.Materials;
		_graphicsCommand->Draw(drawArgs);
	}

	_graphicsCommand->EndDraw();
}

void PSceneRenderer::renderCompositePass(const HRenderPassCBData& inPassData)
{
	_graphicsCommand->ClearTexture(_outputTexture);

	// 합성 머터리얼이 없으면 출력은 클리어 컬러(빨강)로 남아 실패가 눈에 보인다.
	if (_compositeMaterial.IsValid() == false)
	{
		return;
	}

	_graphicsCommand->BeginDraw();

	HRenderTarget renderTarget;
	renderTarget.RenderTextures[0] = _outputTexture;
	renderTarget.Viewports.push_back(HViewport((float32)_width, (float32)_height));
	renderTarget.ScissorRects.push_back(HScissorRect(0, 0, (int32)_width, (int32)_height));
	_graphicsCommand->SetRenderTarget(renderTarget);
	_graphicsCommand->SetRenderPassData(inPassData);

	HScreenDrawArguments drawArgs;
	drawArgs.Material = _compositeMaterial;
	_graphicsCommand->Draw(drawArgs);

	_graphicsCommand->EndDraw();
}

void PSceneRenderer::renderDisplayPass(const HRenderPassCBData& inPassData)
{
	_graphicsCommand->ClearTexture(_displayTexture);

	// 디스플레이 머터리얼이 없으면 디스플레이 텍스처는 클리어 컬러(빨강)로 남는다.
	if (_displayMaterial.IsValid() == false)
	{
		return;
	}

	_graphicsCommand->BeginDraw();

	HRenderTarget renderTarget;
	renderTarget.RenderTextures[0] = _displayTexture;
	renderTarget.Viewports.push_back(HViewport((float32)_width, (float32)_height));
	renderTarget.ScissorRects.push_back(HScissorRect(0, 0, (int32)_width, (int32)_height));
	_graphicsCommand->SetRenderTarget(renderTarget);
	_graphicsCommand->SetRenderPassData(inPassData);

	HScreenDrawArguments drawArgs;
	drawArgs.Material = _displayMaterial;
	_graphicsCommand->Draw(drawArgs);

	_graphicsCommand->EndDraw();
}
