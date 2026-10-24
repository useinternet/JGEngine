#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"
#include "Classes/Mesh.h"

class IVertexBuffer;
class IIndexBuffer;
class IRawMaterial;

// 렌더 메시(PStaticMesh)의 서브메시. GPU 버퍼와 머터리얼만 든다.
// 정점 · 인덱스 CPU 사본은 메시 에셋(JGStaticMesh)에만 있다.
struct GRAPHICS_API HRenderSubMesh
{
	PName Name;
	PSharedPtr<IVertexBuffer> VertexBuffer;
	PSharedPtr<IIndexBuffer>  IndexBuffer;
	PWeakPtr<IRawMaterial> Material;

	bool IsValid() const;
};


class GRAPHICS_API PStaticMesh : public IMesh
{
	PName _name;
	HList<HRenderSubMesh> _subMeshes;
	// 정점 입력 레이아웃(HVertexData). GC 객체의 멤버로 보관해 메모리 시스템과 수명을 맞춘다. (static 금지, JGGraphicsDefine.h 참고)
	HInputLayout _inputLayout;

public:
	PStaticMesh();
	virtual ~PStaticMesh() = default;

	// IJGGraphicsObject
	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;
	// ~IJGGraphicsObject

	// IMesh
	virtual uint32 GetSubMeshCount() const override;
	virtual PSharedPtr<IVertexBuffer> GetVertexBuffer(uint32 inSubMeshIndex) const override;
	virtual PSharedPtr<IIndexBuffer>  GetIndexBuffer(uint32 inSubMeshIndex) const override;
	virtual PSharedPtr<IRawMaterial>  GetMaterial(uint32 inSubMeshIndex) const override;
	virtual const HInputLayout& GetInputLayout() const override;
	virtual bool IsValid() const override;
	virtual void Reset() override;
	// ~IMesh

	// 서브메시의 GPU 버퍼와 머터리얼을 받는다. 메시 에셋(JGStaticMesh::GetMesh)이 서브메시마다 하나씩 만들어 넘긴다.
	void SetSubMeshes(const HList<HRenderSubMesh>& inSubMeshes);
	void SetMaterial(uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial);
	const HList<HRenderSubMesh>& GetSubMeshes() const;
};
