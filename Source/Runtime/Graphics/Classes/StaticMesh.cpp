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
		vertexArgs.LoadMethod = EBufferLoadMethod::GPULoad;   // 정적 메시는 DEFAULT 힙. 스테이징 관리자가 올린다. CPU 사본은 에셋(Vertices/Indices)이 든다. (5-6, 5-26)
		VertexBuffer = GetGraphicsAPI().CreateVertexBuffer(vertexArgs);
	}
	if (IndexBuffer == nullptr)
	{
		HIndexBufferConstructArguments indexArgs;
		indexArgs.Name = inName;
		indexArgs.LoadMethod = EBufferLoadMethod::GPULoad;
		IndexBuffer = GetGraphicsAPI().CreateIndexBuffer(indexArgs);
	}

	// 0바이트 버퍼는 만들 수 없다. (CreateCommittedResource 실패 뒤 Map에서 죽는다)
	if (inVertices.empty() || inIndices.empty())
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : SubMesh has no vertices(%d) or indices(%d)", inName.ToString(), (int32)inVertices.size(), (int32)inIndices.size());
		Vertices.clear();
		Indices.clear();
		return;
	}

	// HVertex -> HVertexData 슬라이스 복사로 vptr을 떼어낸다. 이 사본을 에셋이 든다(저장 · 경계 상자). GPU 스트라이드 = sizeof(HVertexData).
	Vertices.clear();
	Vertices.reserve(inVertices.size());
	for (const HVertex& vertex : inVertices)
	{
		Vertices.push_back(static_cast<const HVertexData&>(vertex));
	}
	Indices = inIndices;

	// 스테이징 관리자가 요청 시점에 스테이징으로 복사하므로 버퍼는 사본을 들지 않는다.
	VertexBuffer->SetDatas(Vertices.data(), sizeof(HVertexData), Vertices.size());
	IndexBuffer->SetDatas(Indices.data(), Indices.size());

	SetName(inName);
}

void HStaticSubMesh::GetVertices(HList<HVertex>& outVertices) const
{
	outVertices.clear();
	outVertices.reserve(Vertices.size());
	for (const HVertexData& vertex : Vertices)
	{
		outVertices.push_back(HVertex(vertex));
	}
}

void HStaticSubMesh::GetIndices(HList<uint32>& outIndices) const
{
	outIndices = Indices;
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

// ---------------------------------------------------------------- HRenderSubMesh

bool HRenderSubMesh::IsValid() const
{
	if (VertexBuffer == nullptr || IndexBuffer == nullptr)
	{
		return false;
	}

	return VertexBuffer->IsValid() && IndexBuffer->IsValid();
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

	for (const HRenderSubMesh& subMesh : _subMeshes)
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
	_subMeshes.clear();
	_subMeshes.reserve(inSubMeshes.size());
	for (const HStaticSubMesh& subMesh : inSubMeshes)
	{
		HRenderSubMesh renderSubMesh;
		renderSubMesh.Name         = subMesh.Name;
		renderSubMesh.VertexBuffer = subMesh.VertexBuffer;
		renderSubMesh.IndexBuffer  = subMesh.IndexBuffer;
		renderSubMesh.Material     = subMesh.Material;
		_subMeshes.push_back(renderSubMesh);
	}
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

const HList<HRenderSubMesh>& PStaticMesh::GetSubMeshes() const
{
	return _subMeshes;
}

// ---------------------------------------------------------------- JGStaticMesh

namespace
{
	// 정점이 없을 때의 경계 상자. min > max라 GetBounds가 false를 돌려준다.
	HBBox makeEmptyBounds()
	{
		HBBox bounds;
		bounds.min = HVector3(FLT_MAX);
		bounds.max = HVector3(-FLT_MAX);
		return bounds;
	}

	bool isValidBounds(const HBBox& inBounds)
	{
		return inBounds.min.x <= inBounds.max.x && inBounds.min.y <= inBounds.max.y && inBounds.min.z <= inBounds.max.z;
	}
}

JGStaticMesh::JGStaticMesh()
{
	_bMeshDirty = true;
	_bounds = makeEmptyBounds();
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

	return (uint64)_subMeshes[inSubMeshIndex].Vertices.size();
}

const uint64 JGStaticMesh::GetIndexCount(uint32 inSubMeshIndex) const
{
	JG_CHECK(inSubMeshIndex < GetSubMeshCount());

	return (uint64)_subMeshes[inSubMeshIndex].Indices.size();
}

const uint64 JGStaticMesh::GetSubMeshCount() const
{
	return (uint64)(_subMeshes.size());
}

const HVertexData& JGStaticMesh::GetVertex(uint32 inSubMeshIndex, uint32 inIndex) const
{
	JG_CHECK(inSubMeshIndex < GetSubMeshCount());

	const HList<HVertexData>& vertices = _subMeshes[inSubMeshIndex].Vertices;
	if (inIndex >= vertices.size())
	{
		static HVertexData nullVertex;
		return nullVertex;
	}

	return vertices[inIndex];
}

uint32 JGStaticMesh::GetIndex(uint32 inSubMeshIndex, uint32 inIndex) const
{
	JG_CHECK(inSubMeshIndex < GetSubMeshCount());

	const HList<uint32>& indices = _subMeshes[inSubMeshIndex].Indices;
	if (inIndex >= indices.size())
	{
		return (uint32)INDEX_NONE;
	}

	return indices[inIndex];
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

		// 업로드 데이터는 SetData 안에서 스테이징으로 복사됐다. 사본을 옮겨 담아 두 벌이 되지 않게 한다.
		_subMeshes.push_back(std::move(subMesh));
	}

	updateBounds();
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

bool JGStaticMesh::GetBounds(HBBox& outBounds) const
{
	if (isValidBounds(_bounds) == false)
	{
		return false;
	}

	outBounds = _bounds;
	return true;
}

void JGStaticMesh::updateBounds()
{
	_bounds = makeEmptyBounds();
	for (const HStaticSubMesh& subMesh : _subMeshes)
	{
		for (const HVertexData& vertex : subMesh.Vertices)
		{
			_bounds.min = HVector3::Min(_bounds.min, vertex.Position);
			_bounds.max = HVector3::Max(_bounds.max, vertex.Position);
		}
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
	// 로드 스레드에서 ReadJson으로 _subMeshes · _bounds가 채워진 뒤 불린다. 다음 GetMesh()에서 렌더 메시를 다시 만든다.
	// _bounds 없이 저장된 에셋(5-26 이전)은 정점으로 구한다. 다시 저장하면 이 경로를 타지 않는다.
	if (isValidBounds(_bounds) == false)
	{
		updateBounds();
		JG_LOG(Graphics, ELogLevel::Trace, "%s : no saved bounds (saved before 5-26), computed from vertices", GetName());
	}
	_bMeshDirty = true;
}
