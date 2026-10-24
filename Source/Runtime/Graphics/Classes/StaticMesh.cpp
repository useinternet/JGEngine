#include "PCH/PCH.h"
#include "StaticMesh.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "JGGraphics.h"
#include "Material.h"

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

void PStaticMesh::SetSubMeshes(const HList<HRenderSubMesh>& inSubMeshes)
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

const HList<HRenderSubMesh>& PStaticMesh::GetSubMeshes() const
{
	return _subMeshes;
}
