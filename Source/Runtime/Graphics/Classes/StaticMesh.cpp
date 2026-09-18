#include "PCH/PCH.h"
#include "StaticMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "JGGraphics.h"
#include "Material.h"

// ---------------------------------------------------------------- HStaticSubMesh

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
	Name = inName;

	if (IsValid() == false)
	{
		return;
	}

	VertexBuffer->SetName(inName);
	IndexBuffer->SetName(inName);
}

void HStaticSubMesh::SetData(PName inName, const HList<HVertex>& inVertices, const HList<uint32>& inIndices)
{
	Name = inName;

	if (VertexBuffer == nullptr)
	{
		HVertexBufferConstructArguments vertexArgs;
		vertexArgs.Name = inName;
		VertexBuffer = GetGraphicsAPI().CreateVertexBuffer(vertexArgs);
	}
	if (IndexBuffer == nullptr)
	{
		HIndexBufferConstructArguments indexArgs;
		indexArgs.Name = inName;
		IndexBuffer = GetGraphicsAPI().CreateIndexBuffer(indexArgs);
	}

	// 0바이트 버퍼는 만들 수 없다. (CreateCommittedResource 실패 뒤 Map에서 죽는다)
	if (inVertices.empty() || inIndices.empty())
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : SubMesh has no vertices(%d) or indices(%d)", inName.ToString(), (int32)inVertices.size(), (int32)inIndices.size());
		return;
	}

	// HVertex -> HVertexData 슬라이스 복사로 vptr을 떼어낸다. GPU 스트라이드 = sizeof(HVertexData).
	HList<HVertexData> packedVertices;
	packedVertices.reserve(inVertices.size());
	for (const HVertex& vertex : inVertices)
	{
		packedVertices.push_back(static_cast<const HVertexData&>(vertex));
	}

	VertexBuffer->SetDatas(packedVertices.data(), sizeof(HVertexData), packedVertices.size());
	IndexBuffer->SetDatas(inIndices.data(), inIndices.size());

	SetName(inName);
}

void HStaticSubMesh::GetVertices(HList<HVertex>& outVertices) const
{
	outVertices.clear();
	if (VertexBuffer == nullptr || VertexBuffer->IsValid() == false)
	{
		return;
	}

	if (VertexBuffer->GetVertexSize() != sizeof(HVertexData))
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : VertexBuffer stride(%d) != sizeof(HVertexData)(%d)", Name.ToString(), (int32)VertexBuffer->GetVertexSize(), (int32)sizeof(HVertexData));
		return;
	}

	const uint64 vertexCount = VertexBuffer->GetVertexCount();
	const HVertexData* packedVertices = static_cast<const HVertexData*>(VertexBuffer->GetDatas());
	if (packedVertices == nullptr)
	{
		return;
	}

	outVertices.reserve(vertexCount);
	for (uint64 i = 0; i < vertexCount; ++i)
	{
		outVertices.push_back(HVertex(packedVertices[i]));
	}
}

void HStaticSubMesh::GetIndices(HList<uint32>& outIndices) const
{
	outIndices.clear();
	if (IndexBuffer == nullptr || IndexBuffer->IsValid() == false)
	{
		return;
	}

	const uint64 indexCount = IndexBuffer->GetIndexCount();
	const uint32* indices = IndexBuffer->GetDatas();
	if (indices == nullptr)
	{
		return;
	}

	outIndices.assign(indices, indices + indexCount);
}

void HStaticSubMesh::WriteJson(PJsonData& json) const
{
	json.AddMember("SubMeshName", Name);

	HList<HVertex> vertexes;
	GetVertices(vertexes);
	json.AddMember("Vertexes", vertexes);

	HList<uint32> indexes;
	GetIndices(indexes);
	json.AddMember("Indexes", indexes);
}

void HStaticSubMesh::ReadJson(const PJsonData& json)
{
	json.GetData("SubMeshName", &Name);

	HList<HVertex> vertexes;
	HList<uint32> indexes;

	json.GetData("Vertexes", &vertexes);
	json.GetData("Indexes", &indexes);

	SetData(Name, vertexes, indexes);
}

// ---------------------------------------------------------------- PStaticMesh

PStaticMesh::PStaticMesh()
{
	_inputLayout = HVertexData::GetInputLayout();
}

const PName& PStaticMesh::GetName() const
{
	return _name;
}

void PStaticMesh::SetName(const PName& inName)
{
	_name = inName;
}

uint32 PStaticMesh::GetSubMeshCount() const
{
	return (uint32)_subMeshes.size();
}

PSharedPtr<IVertexBuffer> PStaticMesh::GetVertexBuffer(uint32 inSubMeshIndex) const
{
	if (inSubMeshIndex >= GetSubMeshCount())
	{
		return nullptr;
	}

	return _subMeshes[inSubMeshIndex].VertexBuffer;
}

PSharedPtr<IIndexBuffer> PStaticMesh::GetIndexBuffer(uint32 inSubMeshIndex) const
{
	if (inSubMeshIndex >= GetSubMeshCount())
	{
		return nullptr;
	}

	return _subMeshes[inSubMeshIndex].IndexBuffer;
}

PSharedPtr<IRawMaterial> PStaticMesh::GetMaterial(uint32 inSubMeshIndex) const
{
	if (inSubMeshIndex < GetSubMeshCount() && _subMeshes[inSubMeshIndex].Material.IsValid())
	{
		return _subMeshes[inSubMeshIndex].Material.Pin();
	}

	return GetGraphicsAPI().GetDefaultMaterial();
}

const HInputLayout& PStaticMesh::GetInputLayout() const
{
	return _inputLayout;
}

bool PStaticMesh::IsValid() const
{
	if (_subMeshes.empty())
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

void PStaticMesh::Reset()
{
	_subMeshes.clear();
}

void PStaticMesh::SetSubMeshes(const HList<HStaticSubMesh>& inSubMeshes)
{
	_subMeshes = inSubMeshes;
}

void PStaticMesh::SetMaterial(uint32 inSlot, PSharedPtr<IRawMaterial> inMaterial)
{
	if (inSlot >= GetSubMeshCount())
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : SetMaterial slot(%d) out of range(%d)", _name.ToString(), (int32)inSlot, (int32)GetSubMeshCount());
		return;
	}

	_subMeshes[inSlot].Material = inMaterial;
}

const HList<HStaticSubMesh>& PStaticMesh::GetSubMeshes() const
{
	return _subMeshes;
}

// ---------------------------------------------------------------- JGStaticMesh

JGStaticMesh::JGStaticMesh()
{
	_bMeshDirty = true;
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

const HVertexData& JGStaticMesh::GetVertex(uint32 inSubMeshIndex, uint32 inIndex) const
{
	JG_CHECK(inSubMeshIndex < GetSubMeshCount());

	if (_subMeshes[inSubMeshIndex].IsValid() == false)
	{
		static HVertexData nullVertex;
		return nullVertex;
	}

	return _subMeshes[inSubMeshIndex].VertexBuffer->GetData<HVertexData>(inIndex);
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

	// 렌더 메시는 서브메시 목록의 복사본을 들고 있으므로 같이 갱신한다.
	if (_mesh.IsValid())
	{
		_mesh->SetMaterial(inSlot, inMaterial);
	}
}

void JGStaticMesh::SetData(const HList<PName>& subMeshNames, const HList<HList<HVertex>>& inVerties, const HList<HList<uint32>>& inIndeies)
{
	_subMeshes.clear();

	uint64 subMeshCount = subMeshNames.size();
	JG_CHECK(inVerties.size() == subMeshCount && inIndeies.size() == subMeshCount);

	for (uint64 i = 0; i < subMeshCount; ++i)
	{
		// 머터리얼은 비워 둔다. 비어 있으면 GetMaterial()이 그릴 때 기본 머터리얼로 대체한다.
		// (API가 소유한 기본 머터리얼을 약참조로 들고 있으면 종료 순서에 따라 해제된 카운터를 건드릴 수 있다)
		HStaticSubMesh subMesh;
		subMesh.SetData(subMeshNames[i], inVerties[i], inIndeies[i]);

		_subMeshes.push_back(subMesh);
	}

	_bMeshDirty = true;
}

PSharedPtr<IMesh> JGStaticMesh::GetMesh()
{
	if (_mesh.IsValid() == false)
	{
		_mesh = Allocate<PStaticMesh>();
		_bMeshDirty = true;
	}

	if (_bMeshDirty)
	{
		_mesh->SetName(GetName());
		_mesh->SetSubMeshes(_subMeshes);
		_bMeshDirty = false;
	}

	return _mesh;
}

bool JGStaticMesh::CalculateBounds(HBBox& outBounds) const
{
	bool bHasVertex = false;
	HVector3 minPos(FLT_MAX);
	HVector3 maxPos(-FLT_MAX);

	for (const HStaticSubMesh& subMesh : _subMeshes)
	{
		if (subMesh.IsValid() == false || subMesh.VertexBuffer->GetVertexSize() != sizeof(HVertexData))
		{
			continue;
		}

		const uint64 vertexCount = subMesh.VertexBuffer->GetVertexCount();
		const HVertexData* vertices = static_cast<const HVertexData*>(subMesh.VertexBuffer->GetDatas());
		if (vertices == nullptr)
		{
			continue;
		}

		for (uint64 i = 0; i < vertexCount; ++i)
		{
			minPos = HVector3::Min(minPos, vertices[i].Position);
			maxPos = HVector3::Max(maxPos, vertices[i].Position);
			bHasVertex = true;
		}
	}

	if (bHasVertex == false)
	{
		return false;
	}

	outBounds.min = minPos;
	outBounds.max = maxPos;
	return true;
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

	if (_subMeshes.empty())
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

void JGStaticMesh::OnLoadAsset_Thread()
{
	// 로드 스레드에서 ReadJson으로 _subMeshes가 채워진 뒤 불린다. 다음 GetMesh()에서 렌더 메시를 다시 만든다.
	_bMeshDirty = true;
}
