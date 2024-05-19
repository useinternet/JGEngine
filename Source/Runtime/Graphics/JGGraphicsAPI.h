#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

class HJGGraphicsArguments
{
public:
	uint64 Handle;
	int32  BufferCount;
	int32  Width;
	int32  Height;
	HLinearColor ClearColor;

	HJGGraphicsArguments()
		: BufferCount(2)
		, Width(0)
		, Height(0) {}

};

class IRawTexture;
class IIndexBuffer;
class IVertexBuffer;
class IJGGui;
class IJGGraphicsCommand;
class IRawMaterial;
class JGStaticMesh;
class JGTexture;
struct HRawMaterialConstructArguments;
struct HVertexBufferConstructArguments;
struct HIndexBufferConstructArguments;
struct HStaticMeshConstructArguments;
struct HTextureConstructArguments;

class GRAPHICS_API PJGGraphicsAPI : public IMemoryObject
{
	friend class HJGGraphicsModule;

	PSharedPtr<IRawTexture>  _defaultTexture = nullptr;
	PSharedPtr<IRawMaterial> _defaultMaterial = nullptr;
public:
	virtual ~PJGGraphicsAPI() = default;

protected:
	virtual void Initialize(const HJGGraphicsArguments& args) = 0;
	virtual void Destroy() = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
public:

	virtual void SubmitFinalTexture(PSharedPtr<IRawTexture> inTexture) = 0;
public:
	virtual IJGGui* GetGui() const = 0;
	virtual PSharedPtr<IJGGraphicsCommand> GetGraphicsCommand() const = 0;
	virtual PSharedPtr<IRawTexture> GetDefaultTexture() const { return _defaultTexture; }
	virtual PSharedPtr<IRawMaterial> GetDefaultMaterial() const { return _defaultMaterial; }


	// -- Raw Data --
	virtual PSharedPtr<IRawTexture> CreateRawTexture(const HTextureInfo& textureInfo) = 0;
	virtual PSharedPtr<IRawTexture> CreateRawTexture(const uint8* pixels, const HTextureInfo& textureInfo) = 0;
	virtual PSharedPtr<IVertexBuffer> CreateVertexBuffer(const HVertexBufferConstructArguments& inArgs) = 0;
	virtual PSharedPtr<IIndexBuffer>  CreateIndexBuffer(const HIndexBufferConstructArguments& inArgs)  = 0;
	virtual PSharedPtr<IRawMaterial> CreateRawMaterial(const HRawMaterialConstructArguments& inArgs) = 0;

	// -- Asset Data --

	virtual PSharedPtr<JGStaticMesh> CreateStaticMesh(const HStaticMeshConstructArguments& inArgs);
	virtual PSharedPtr<JGTexture> CreateTexture(const HTextureConstructArguments& inArgs);
};