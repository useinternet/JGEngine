#include "PCH/PCH.h"
#include "JGDevGraphicsTest.h"
#include "JGGraphicsDefine.h"
#include "JGGraphics.h"
#include "AssetPath.h"
#include "AssetDatabase.h"
#include "Asset.h"

void HJGDevGraphicsTest::Init()
{
	HTextureInfo TexInfo;
	TexInfo.Width = 1920;
	TexInfo.Height = 1080;
	TexInfo.Format = ETextureFormat::R16G16B16A16_Float;
	TexInfo.Flags = ETextureFlags::Allow_RenderTarget;
	TexInfo.MipLevel = 1;
	TexInfo.ArraySize = 1;

	TexInfo.ClearColor = HLinearColor(1.0F, 0.0F, 0.0F, 1.0F);

	auto tex = GetGraphicsAPI().CreateRawTexture(TexInfo);
	GetGraphicsAPI().GetGraphicsCommand()->ClearTexture(tex);
	GetGraphicsAPI().SubmitFinalTexture(tex);


	HAssetPath AssetPath = "/JGEngine/TempAsset/Sample";
	GAssetDatabase::GetInstance().LoadAssetAsync(AssetPath, POnLoadCompelete::CreateLambda([](PWeakPtr<JGAsset> InAsset)
		{
			JG_LOG(DevGraphicsTest, ELogLevel::Info, "Load Complete Asset : %s", InAsset.Pin()->GetName().ToString());

		}));
	// Mesh 뷰어
	// 머터리얼까지
}

void HJGDevGraphicsTest::Shutdown()
{

}

void HJGDevGraphicsTest::Update()
{

}
