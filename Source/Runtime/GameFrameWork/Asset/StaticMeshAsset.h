#pragma once
#include "Core.h"
#include "Math/BBox.h"
#include "JGGraphicsDefine.h"
#include "Classes/StaticMesh.h"
#include "Asset/Asset.h"
#include "StaticMeshAsset.generation.h"

struct HStaticMeshConstructArguments
{
	HAssetPath Name;
	HList<PName> SubMeshNames;
	HList<HList<HVertex>> Verties;
	HList<HList<uint32>>  Indeies;
};

// 에셋(JGStaticMesh)의 서브메시 하나 = 정점/인덱스 CPU 사본 + GPU 버퍼 + 머터리얼(약참조).
// CPU 사본은 에셋이 든다. 저장(WriteJson), 경계 상자, GetVertex/GetIndex가 쓴다. GPU 버퍼는 GPU에 올리는 일만 한다. (5-26)
// GPU 버퍼에는 HVertex가 아니라 HVertexData(POD)만 올린다. HVertex는 vptr이 있어 그대로 올리면 입력 레이아웃과 어긋난다.
struct GAMEFRAMEWORK_API HStaticSubMesh : public IJsonable
{
	PName Name;
	HList<HVertexData> Vertices;   // CPU 사본
	HList<uint32>      Indices;    // CPU 사본
	PSharedPtr<IVertexBuffer> VertexBuffer;
	PSharedPtr<IIndexBuffer>  IndexBuffer;
	PWeakPtr<IRawMaterial> Material;

	bool IsValid() const;
	void SetName(const PName& inName);

	// CPU 사본을 채우고 GPU 버퍼로 올린다. 버퍼가 없으면 만든다.
	// inName은 값으로 받는다. ReadJson처럼 멤버 Name 자신을 넘기는 호출이 있어 참조로 받으면 자기 대입이 된다.
	void SetData(PName inName, const HList<HVertex>& inVertices, const HList<uint32>& inIndices);
	// CPU 사본을 HVertex/uint32 목록으로 복사한다. (직렬화용)
	void GetVertices(HList<HVertex>& outVertices) const;
	void GetIndices(HList<uint32>& outIndices) const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

// 스태틱 메시 에셋. 렌더러에는 GetMesh() 의 렌더 메시(PStaticMesh, Graphics)를 넘긴다.
JGCLASS()
class GAMEFRAMEWORK_API JGStaticMesh : public JGAsset
{
	JG_GENERATED_CLASS_BODY

	JGPROPERTY()
	HList<HStaticSubMesh> _subMeshes;

	// 모든 서브메시 정점의 경계 상자. 임포트(SetData) 때 구해 에셋에 저장한다. 정점이 없으면 min > max. (5-26)
	JGPROPERTY()
	HBBox _bounds;

	// 렌더 메시. _subMeshes와 같은 버퍼를 참조하며 GetMesh()에서 지연 생성한다. 직렬화 대상이 아니다.
	PSharedPtr<PStaticMesh> _mesh;
	bool _bMeshDirty;

public:
	JGStaticMesh();
	virtual ~JGStaticMesh() override = default;

	// 정점 · 인덱스로 메시 에셋을 만든다(GPU 버퍼까지). FBX 가져오기가 쓴다(옛 PJGGraphicsAPI::CreateStaticMesh).
	static PSharedPtr<JGStaticMesh> Create(const HStaticMeshConstructArguments& inArgs);

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
	// 에셋에 저장된 경계 상자. 정점이 하나도 없으면 false.
	bool GetBounds(HBBox& outBounds) const;

	// JGObject
	virtual void SetName(const PName& inName) override;
	// ~JGObject

	// JGAsset
	virtual bool IsValid() const override;
protected:
	virtual void OnLoadAsset_Thread() override;
	// ~JGAsset

private:
	// _subMeshes의 CPU 사본으로 _bounds를 다시 구한다.
	void updateBounds();
};
