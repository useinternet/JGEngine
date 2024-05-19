#include "PCH/PCH.h"
#include "JGDev_Graphics.h"
#include "Platform/Platform.h"
#include "Platform/JWindow.h"
#include "JGGraphics.h"
#ifdef _PLATFORM_WINDOWS
#include "Platform/Windows/WindowsJWindow.h"
#endif
#include "Importers/FBXAssetImporter.h"

 JG_MODULE_IMPL(HJGDev_GraphicsModule, JGDEV_GRAPHICS_C_API)

 HJGDev_GraphicsModule::~HJGDev_GraphicsModule()
 {
	
 }

 JGType HJGDev_GraphicsModule::GetModuleType() const
 {
 	return JGTYPE(HJGDev_GraphicsModule);
 }

 void HJGDev_GraphicsModule::StartupModule()
 {
 	HJWindowArguments winArgs;
 	winArgs.Title = "JGDev_Graphics";
 	winArgs.Size  = HVector2Int(800, 600);
 	_window = HPlatform::CreateJWindow(winArgs);

 	GCoreSystem::GetGlobalValues().MainWindow = _window.GetRawPointer();

 	if (GModuleGlobalSystem::GetInstance().ConnectModule("Graphics") == false)
 	{
 		JG_LOG(JGDev_GraphicsModule, ELogLevel::Critical, "Fail Connect Graphics Module...");
 	}

 	JG_LOG(JGDev_GraphicsModule, ELogLevel::Trace, "Startup JGDev_GraphicsModule Module...");


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

    PSharedPtr<JGFBXAssetImporter> importer = Allocate< JGFBXAssetImporter>();
    PSharedPtr<PFBXAssetImportArguments> args = Allocate<PFBXAssetImportArguments>();
    HFileHelper::CombinePath(HFileHelper::EngineContentsDirectory(), "RawResources", &(args->SrcPath));
    HFileHelper::CombinePath(HFileHelper::EngineContentsDirectory(), "TempAsset", &(args->DestPath));

    importer->Import(args);
 }

 void HJGDev_GraphicsModule::ShutdownModule()
 {
 	_window = nullptr;

 	GModuleGlobalSystem::GetInstance().DisconnectModule("Graphics");
 	JG_LOG(JGDev_GraphicsModule, ELogLevel::Trace, "Shutdown JGDev_GraphicsModule Module...");
 }

