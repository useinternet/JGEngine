#include "PCH/PCH.h"
#include "JGGraphicsAPI.h"
#include "Classes/StaticMesh.h"
#include "Classes/Texture.h"

PSharedPtr<JGStaticMesh> PJGGraphicsAPI::CreateStaticMesh(const HStaticMeshConstructArguments& inArgs)
{
    PSharedPtr<JGStaticMesh> result = Allocate<JGStaticMesh>();
    result->SetName(inArgs.Name.GetAssetPath());
    result->SetData(inArgs.SubMeshNames, inArgs.Verties, inArgs.Indeies);
    return result;
}

PSharedPtr<JGTexture> PJGGraphicsAPI::CreateTexture(const HTextureConstructArguments& inArgs)
{
    PSharedPtr<JGTexture> result = Allocate<JGTexture>();
    result->SetName(inArgs.TextureInfo.Name);
    result->_texture = CreateRawTexture(inArgs.Pixels.data(), inArgs.TextureInfo);
    
    return result;
}
