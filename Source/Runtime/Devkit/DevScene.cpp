#include "PCH/PCH.h"
#include "DevScene.h"
#include "JGGraphicsDefine.h"
#include "JGGraphics.h"
#include "Classes/Texture.h"
#include "GUI.h"

void JGDevScene::OnInitialize()
{
	HTextureInfo TexInfo;
	TexInfo.Width  = 1920;
	TexInfo.Height = 1080;
	TexInfo.Format = ETextureFormat::R16G16B16A16_Float;
	TexInfo.Flags  = ETextureFlags::Allow_RenderTarget;
	TexInfo.MipLevel  = 1;
	TexInfo.ArraySize = 1;
	TexInfo.ClearColor = HLinearColor(1.0F, 0.0F, 0.0F, 1.0F);

	SceneTexture = GetGraphicsAPI().CreateRawTexture(TexInfo);
	GetGraphicsAPI().GetGraphicsCommand()->ClearTexture(SceneTexture);
}

void JGDevScene::OnShutdown()
{
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
	
	
	HGUI::Image(SceneTexture->GetTextureID(), SceneSize);
}
