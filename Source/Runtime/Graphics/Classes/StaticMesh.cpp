#include "PCH/PCH.h"
#include "StaticMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "JGGraphics.h"
#include "Material.h"

bool HStaticSubMesh::IsValid() const
{
	if (VertexBuffer == nullptr || IndexBuffer == nullptr)
	{
		return false;
	}

	return VertexBuffer->IsValid() && IndexBuffer->IsValid();
}

void HStaticSubMesh::SetName(const PName& inName)
{
	if (IsValid() == false)
	{
		return;
	}

	VertexBuffer->SetName(inName);
	IndexBuffer->SetName(inName);
}

void HStaticSubMesh::WriteJson(PJsonData& json) const
{
	json.AddMember("SubMeshName", Name);

	HList<HVertex> vertexes;
	if (VertexBuffer != nullptr && VertexBuffer->IsValid())
	{
		uint64 vertexCount = VertexBuffer->GetVertexCount();
		uint64 vertexesTotalDataSize = VertexBuffer->GetVertexSize() * vertexCount;
		vertexes.resize(vertexCount);

		memcpy(vertexes.data(), VertexBuffer->GetDatas(), vertexesTotalDataSize);
	}

	json.AddMember("Vertexes", vertexes);

	HList<uint32> indexes;
	if (IndexBuffer != nullptr && IndexBuffer->IsValid())
	{
		uint64 indexCount = IndexBuffer->GetIndexCount();
		uint64 indexesTotalDataSize = sizeof(uint32) * indexCount;
		indexes.resize(indexCount);

		memcpy(indexes.data(), IndexBuffer->GetDatas(), indexesTotalDataSize);
	}

	json.AddMember("Indexes", indexes);
}

void HStaticSubMesh::ReadJson(const PJsonData& json)
{
	json.GetData("SubMeshName", &Name);
	
	HList<HVertex> vertexes;
	HList<uint32> indexes;

	json.GetData("Vertexes", &vertexes);
	json.GetData("Indexes", &indexes);

	HVertexBufferConstructArguments vertexBufferArgs;
	HIndexBufferConstructArguments indexBufferArgs;

	vertexBufferArgs.Name = Name;
	indexBufferArgs.Name  = Name;
	VertexBuffer = GetGraphicsAPI().CreateVertexBuffer(vertexBufferArgs);
	IndexBuffer = GetGraphicsAPI().CreateIndexBuffer(indexBufferArgs);

	VertexBuffer->SetDatas(vertexes.data(), sizeof(HVertex), vertexes.size());
	IndexBuffer->SetDatas(indexes.data(), indexes.size());
}

JGStaticMesh::JGStaticMesh()
{

}

const uint64 JGStaticMesh::GetTotalVertexCount() const
{
	uint64 result = 0;
	uint32 subMeshCount = (uint32)GetSubMeshCount();
	for (uint32 i = 0; i < subMeshCount; ++i)
	{
		result += GetVertexCount(i);
	}

	return result;
}

const uint64 JGStaticMesh::GetTotalIndexCount() const
{
	uint64 result = 0;
	uint32 subMeshCount = (uint32)GetSubMeshCount();
	for (uint32 i = 0; i < subMeshCount; ++i)
	{
		result += GetIndexCount(i);
	}

	return result;
}

const uint64 JGStaticMesh::GetVertexCount(uint32 inSubMeshIndex) const
{
	JG_CHECK(inSubMeshIndex < GetSubMeshCount());

	if (_subMeshes[inSubMeshIndex].IsValid() == false)
	{
		return 0;
	}

	return _subMeshes[inSubMeshIndex].VertexBuffer->GetVertexCount();
}

const uint64 JGStaticMesh::GetIndexCount(uint32 inSubMeshIndex) const
{
	JG_CHECK(inSubMeshIndex < GetSubMeshCount());

	if (_subMeshes[inSubMeshIndex].IsValid() == false)
	{
		return 0;
	}

	return _subMeshes[inSubMeshIndex].IndexBuffer->GetIndexCount();
}

const uint64 JGStaticMesh::GetSubMeshCount() const
{
	return (uint64)(_subMeshes.size());
}

const HVertex& JGStaticMesh::GetVertex(uint32 inSubMeshIndex, uint32 inIndex) const
{
	JG_CHECK(inSubMeshIndex < GetSubMeshCount());

	if (_subMeshes[inSubMeshIndex].IsValid() == false)
	{
		static HVertex nullVertex;
		return nullVertex;
	}

	return _subMeshes[inSubMeshIndex].VertexBuffer->GetData<HVertex>(inIndex);
}

uint32 JGStaticMesh::GetIndex(uint32 inSubMeshIndex, uint32 inIndex) const
{
	JG_CHECK(inSubMeshIndex < GetSubMeshCount());

	if (_subMeshes[inSubMeshIndex].IsValid() == false)
	{
		static uint32 nullVertex = INDEX_NONE;
		return nullVertex;
	}

	return _subMeshes[inSubMeshIndex].IndexBuffer->GetData(inIndex);
}

HList<PSharedPtr<IRawMaterial>> JGStaticMesh::GetMaterials() const
{
	if (IsValid() == false)
	{
		return { GetGraphicsAPI().GetDefaultMaterial() };
	}

	HList<PSharedPtr<IRawMaterial>> result;
	uint32 subMeshCount = (uint32)GetSubMeshCount();
	for (uint32 i = 0; i < subMeshCount; ++i)
	{
		if (_subMeshes[i].Material.IsValid() == false)
		{
			result.push_back(GetGraphicsAPI().GetDefaultMaterial());
		}
		else
		{
			result.push_back(_subMeshes[i].Material.Pin());
		}
	}
	return result;
}

PSharedPtr<IRawMaterial> JGStaticMesh::GetMaterial(uint32 inSlot) const
{
	if (inSlot >= GetSubMeshCount())
	{
		return GetGraphicsAPI().GetDefaultMaterial();
	}

	if (_subMeshes[inSlot].Material.IsValid() == false)
	{
		return GetGraphicsAPI().GetDefaultMaterial();
	}
	else
	{
		return _subMeshes[inSlot].Material.Pin();
	}
}

void JGStaticMesh::SetMaterial(uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial)
{
	JG_CHECK(inSlot < GetSubMeshCount());
	if (_subMeshes[inSlot].IsValid() == false)
	{
		return;
	}

	_subMeshes[inSlot].Material = inMaterial;
}

void JGStaticMesh::SetData(const HList<PName>& subMeshNames, const HList<HList<HVertex>>& inVerties, const HList<HList<uint32>>& inIndeies)
{
	JG_CHECK(subMeshNames.size() == inVerties.size() == inIndeies.size());

	_subMeshes.clear();

	uint64 subMeshCount = subMeshNames.size();

	for (uint64 i = 0; i < subMeshCount; ++i)
	{
		HStaticSubMesh subMesh;
		subMesh.Name = subMeshNames[i];

		HVertexBufferConstructArguments vertexArgs;
		HIndexBufferConstructArguments  indexArgs;

		vertexArgs.Name = subMesh.Name;
		indexArgs.Name  = subMesh.Name;

		subMesh.VertexBuffer = GetGraphicsAPI().CreateVertexBuffer(vertexArgs);
		subMesh.IndexBuffer  = GetGraphicsAPI().CreateIndexBuffer(indexArgs);
		subMesh.Material = GetGraphicsAPI().GetDefaultMaterial();

		subMesh.VertexBuffer->SetDatas(inVerties[i].data(), sizeof(HVertex), inVerties[i].size());
		subMesh.IndexBuffer->SetDatas(inIndeies[i].data(), inIndeies[i].size());
	}
}

void JGStaticMesh::SetName(const PName& inName)
{
	JG_SUPER::SetName(inName);
}

bool JGStaticMesh::IsValid() const
{
	if (JG_SUPER::IsValid() == false)
	{
		return false;
	}

	for (const HStaticSubMesh& subMesh : _subMeshes)
	{
		if (subMesh.IsValid() == false)
		{
			return false;
		}
	}

	return true;
}

