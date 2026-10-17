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
class IJGGraphicsCommand;
class IRawMaterial;
class JGStaticMesh;
class JGTexture;
struct HRawMaterialConstructArguments;
struct HVertexBufferConstructArguments;
struct HIndexBufferConstructArguments;
struct HStaticMeshConstructArguments;
struct HTextureConstructArguments;

// 리드백 완료 콜백. 실패해도 한 번은 불리며 그때는 HTexturePixels::IsValid()가 false다.
JG_DECLARE_DELEGATE(HOnTextureReadbackComplete, const HTexturePixels&)

class GRAPHICS_API PJGGraphicsAPI : public IMemoryObject
{
	friend class HJGGraphicsModule;

protected:
	// 백엔드 구현이 Initialize()에서 채운다. 머터리얼이 비어 있는 서브메시 등이 이걸 참조한다.
	PSharedPtr<IRawTexture>  _defaultTexture = nullptr;
	PSharedPtr<IRawMaterial> _defaultMaterial = nullptr;
public:
	virtual ~PJGGraphicsAPI() = default;

protected:
	virtual void Initialize(const HJGGraphicsArguments& args) = 0;
	virtual void Destroy() = 0;
	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	// 메인 창 클라이언트 크기가 바뀌면 Graphics 모듈이 부른다(창 OnResize). 0 x 0 = 최소화. GUI 가 없어도 스왑체인이 따라간다. (GP-G1)
	virtual void OnWindowResized(uint32 inWidth, uint32 inHeight) = 0;
public:

	// 이 텍스처를 매 프레임 백 버퍼에 복사한다(GUI 보다 먼저). 크기 · 포맷이 백 버퍼와 다르면 그 프레임은 복사하지 않는다(로그 한 번, GP-G3).
	virtual void SubmitFinalTexture(PSharedPtr<IRawTexture> inTexture) = 0;
	// 수직 동기. 켜면 Present 가 모니터 주사율에 맞춰 기다린다. 기본은 꺼짐(에디터). 다음 Present 부터 적용된다. (GP-G3)
	virtual void SetVSyncEnabled(bool inEnabled) = 0;
	virtual bool IsVSyncEnabled() const = 0;
public:
	virtual PSharedPtr<IJGGraphicsCommand> GetGraphicsCommand() const = 0;
	virtual PSharedPtr<IRawTexture> GetDefaultTexture() const { return _defaultTexture; }
	virtual PSharedPtr<IRawMaterial> GetDefaultMaterial() const { return _defaultMaterial; }


	// -- Raw Data --
	virtual PSharedPtr<IRawTexture> CreateRawTexture(const HTextureInfo& textureInfo) = 0;
	virtual PSharedPtr<IRawTexture> CreateRawTexture(const uint8* pixels, const HTextureInfo& textureInfo) = 0;
	virtual PSharedPtr<IVertexBuffer> CreateVertexBuffer(const HVertexBufferConstructArguments& inArgs) = 0;
	virtual PSharedPtr<IIndexBuffer>  CreateIndexBuffer(const HIndexBufferConstructArguments& inArgs)  = 0;
	virtual PSharedPtr<IRawMaterial> CreateRawMaterial(const HRawMaterialConstructArguments& inArgs) = 0;

	// -- Transfer (GPU <-> CPU) --
	// 비동기 리드백. 어디서 불러도 된다. 복사는 이번 프레임 제출의 마지막(드로우 뒤)에 기록되고,
	// 완료 콜백은 다음 BeginFrame에 메인 스레드에서 불린다. 깊이 포맷은 지원하지 않는다.
	virtual bool RequestTextureReadback(PSharedPtr<IRawTexture> inTexture, const HOnTextureReadbackComplete& inOnComplete) = 0;
	// 동기 리드백(도구 전용). 전용 커맨드 리스트를 따로 제출하고 GPU 완료까지 기다린다. 프레임 리스트는 건드리지 않는다.
	// 결과는 GPU가 마지막으로 완료한 내용(렌더 타깃이면 지난 프레임)이고, 같은 텍스처의 대기 업로드는 먼저 반영된다. 메인 스레드 전용.
	virtual bool ReadbackTextureImmediate(PSharedPtr<IRawTexture> inTexture, HTexturePixels& outPixels) = 0;

	// -- Asset Data --

	virtual PSharedPtr<JGStaticMesh> CreateStaticMesh(const HStaticMeshConstructArguments& inArgs);
	virtual PSharedPtr<JGTexture> CreateTexture(const HTextureConstructArguments& inArgs);
};