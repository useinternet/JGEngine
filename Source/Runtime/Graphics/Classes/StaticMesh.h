#pragma once
#include "Core.h"
#include "Math/BBox.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"
#include "Asset.h"
#include "Classes/Mesh.h"
#include "StaticMesh.generation.h"

class IVertexBuffer;
class IIndexBuffer;
class IRawMaterial;

struct HStaticMeshConstructArguments
{
	HAssetPath Name;
	HList<PName> SubMeshNames;
	HList<HList<HVertex>> Verties;
	HList<HList<uint32>>  Indeies;
};

// 서브메시 하나 = 정점 버퍼 + 인덱스 버퍼 + 머터리얼(약참조).
// GPU 버퍼에는 HVertex가 아니라 HVertexData(POD)만 올린다. HVertex는 vptr이 있어 그대로 올리면 입력 레이아웃과 어긋난다.
struct GRAPHICS_API HStaticSubMesh : public IJsonable
{
	PName Name;
	PSharedPtr<IVertexBuffer> VertexBuffer;
	PSharedPtr<IIndexBuffer>  IndexBuffer;
	PWeakPtr<IRawMaterial> Material;

	bool IsValid() const;
	void SetName(const PName& inName);

	// 정점/인덱스를 GPU 버퍼로 올린다. 버퍼가 없으면 만든다.
	// inName은 값으로 받는다. ReadJson처럼 멤버 Name 자신을 넘기는 호출이 있어 참조로 받으면 자기 대입이 된다.
	void SetData(PName inName, const HList<HVertex>& inVertices, const HList<uint32>& inIndices);
	// GPU 버퍼 내용을 HVertex/uint32 목록으로 복사한다. (직렬화용)
	void GetVertices(HList<HVertex>& outVertices) const;
	void GetIndices(HList<uint32>& outIndices) const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};


class GRAPHICS_API PStaticMesh : public IMesh
{
	PName _name;
	HList<HStaticSubMesh> _subMeshes;
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

	void SetSubMeshes(const HList<HStaticSubMesh>& inSubMeshes);
	void SetMaterial(uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial);
	const HList<HStaticSubMesh>& GetSubMeshes() const;
};

JGCLASS()
class GRAPHICS_API JGStaticMesh : public JGAsset
{
	JG_GENERATED_CLASS_BODY

	JGPROPERTY()
	HList<HStaticSubMesh> _subMeshes;

	// 렌더 메시. _subMeshes와 같은 버퍼를 참조하며 GetMesh()에서 지연 생성한다. 직렬화 대상이 아니다.
	PSharedPtr<PStaticMesh> _mesh;
	bool _bMeshDirty;

public:
	JGStaticMesh();
	virtual ~JGStaticMesh() override = default;

	const uint64 GetTotalVertexCount() const;
	const uint64 GetTotalIndexCount() const;

	const uint64 GetVertexCount(uint32 inSubMeshIndex) const;
	const uint64 GetIndexCount(uint32 inSubMeshIndex) const;

	const uint64 GetSubMeshCount() const;

	const HVertexData& GetVertex(uint32 inSubMeshIndex, uint32 inIndex) const;
	uint32 GetIndex(uint32 inSubMeshIndex, uint32 inIndex) const;

	HList<PSharedPtr<IRawMaterial>> GetMaterials() const;
	PSharedPtr<IRawMaterial> GetMaterial(uint32 inSlot) const;

	void SetMaterial(uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial);
	void SetData(const HList<PName>& subMeshNames, const HList<HList<HVertex>>& inVerties, const HList<HList<uint32>>& inIndeies);

	// 렌더러에 넘길 IMesh. 서브메시가 바뀌었으면(SetData, 로드) 다시 만든다.
	PSharedPtr<IMesh> GetMesh();
	// 모든 서브메시 정점을 순회해 경계 상자를 구한다. 정점이 하나도 없으면 false.
	bool CalculateBounds(HBBox& outBounds) const;

	// JGObject 
	virtual void SetName(const PName& inName) override;
	// ~JGObject
	
	// JGAsset
	virtual bool IsValid() const override;
protected:
	virtual void OnLoadAsset_Thread() override;
	// ~JGAsset
};
