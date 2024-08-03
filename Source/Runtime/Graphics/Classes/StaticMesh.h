#pragma once
#include "Core.h"
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

struct HStaticSubMesh : public IJsonable
{
	PName Name;
	PSharedPtr<IVertexBuffer> VertexBuffer;
	PSharedPtr<IIndexBuffer>  IndexBuffer;
	PWeakPtr<IRawMaterial> Material;

	bool IsValid() const;
	void SetName(const PName& inName);

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

JGCLASS()
class GRAPHICS_API JGStaticMesh : public JGAsset
{
	JG_GENERATED_CLASS_BODY

	JGPROPERTY()
	HList<HStaticSubMesh> _subMeshes;

public:
	JGStaticMesh();
	virtual ~JGStaticMesh() override = default;

	const uint64 GetTotalVertexCount() const;
	const uint64 GetTotalIndexCount() const;

	const uint64 GetVertexCount(uint32 inSubMeshIndex) const;
	const uint64 GetIndexCount(uint32 inSubMeshIndex) const;

	const uint64 GetSubMeshCount() const;

	const HVertex& GetVertex(uint32 inSubMeshIndex, uint32 inIndex) const;
	uint32 GetIndex(uint32 inSubMeshIndex, uint32 inIndex) const;

	HList<PSharedPtr<IRawMaterial>> GetMaterials() const;
	PSharedPtr<IRawMaterial> GetMaterial(uint32 inSlot) const;

	void SetMaterial(uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial);
	void SetData(const HList<PName>& subMeshNames, const HList<HList<HVertex>>& inVerties, const HList<HList<uint32>>& inIndeies);

	// JGObject 
	virtual void SetName(const PName& inName) override;
	// ~JGObject
	
	// JGAsset
	virtual bool IsValid() const override;
	// ~JGAsset
};