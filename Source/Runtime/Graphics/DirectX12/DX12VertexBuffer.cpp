#include "PCH/PCH.h"
#include "DX12VertexBuffer.h"
#include "DirectX12API.h"

PDX12VertexBuffer::~PDX12VertexBuffer()
{
	Reset();
}

const PName& PDX12VertexBuffer::GetName() const
{
	return _name;
}

void PDX12VertexBuffer::SetName(const PName& inName)
{
	_name = inName;
	if (IsValid())
	{
		_dx12Resource->SetName(inName.ToString().GetRawWString().c_str());
	}
}

void PDX12VertexBuffer::SetDatas(const void* inDatas, uint64 inElementSize, uint64 inElementCount)
{
	uint64 originBtSize = _elementSize * _elementCount;
	_elementSize  = inElementSize; 
	_elementCount = inElementCount;
	uint64 btSize = inElementSize * _elementCount;

	if (IsValid() && ((originBtSize != btSize) || _cpuData == nullptr))
	{
		Reset();
	}

	if (IsValid() == false)
	{
		CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(btSize);

		_dx12Resource = HDirectXAPI::CreateCommittedResource(
			GetName().ToString().GetCStr(),
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr
		);

		{
			std::lock_guard<std::mutex> lock(_mutex);
			_srv.Reset();
			_uav.Reset();
		}

		_dx12Resource->Map(0, nullptr, &_cpuData);
	}

	if (_cpuData != nullptr && inDatas != nullptr)
	{
		memcpy(_cpuData, inDatas, btSize);
	}
}

void PDX12VertexBuffer::SetData(const void* inData, uint64 inIndex)
{
	memcpy(GetData(inIndex), inData, _elementSize);
}

void* PDX12VertexBuffer::GetDatas() const
{
	return _cpuData;
}

void* PDX12VertexBuffer::GetData(uint64 inIndex) const
{
	JG_CHECK(inIndex < _elementCount && IsValid() && _cpuData != nullptr);

	uint64 dataOffset = HMath::AlignUp(inIndex * _elementSize, _elementSize);

	void* dataPos = (void*)((uint64)_cpuData + dataOffset);
	return dataPos;
}

uint64 PDX12VertexBuffer::GetVertexCount() const
{
	return _elementCount;
}

uint64 PDX12VertexBuffer::GetVertexSize() const
{
	return _elementSize;
}

void PDX12VertexBuffer::Reset()
{
	if (_dx12Resource == nullptr)
	{
		return;
	}

	if (HDirectXAPI::IsValid())
	{
		HDirectXAPI::DestroyCommittedResource(_dx12Resource);
	}

	_dx12Resource.Reset();
	_dx12Resource = nullptr;
	_cpuData = nullptr;
	_elementCount = 0;
	_elementSize = 0;
}

bool PDX12VertexBuffer::IsValid() const
{
	return _dx12Resource != nullptr;
}

HDX12Resource* PDX12VertexBuffer::Get() const
{
	return _dx12Resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12VertexBuffer::GetSRV() const
{
	if (IsValid() == false) return { 0 };
	if (_srv.IsValid()) return { _srv.CPU() };

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	desc.Buffer.NumElements = (uint32)GetVertexCount();
	desc.Buffer.StructureByteStride = (uint32)GetVertexSize();

	std::lock_guard<std::mutex> lock(_mutex);
	HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
	HDirectXAPI::GetDevice()->CreateShaderResourceView(Get(), &desc, alloc.CPU());

	_srv = std::move(alloc);
	return _srv.CPU();
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12VertexBuffer::GetUAV() const
{
	if (IsValid() == false) return { 0 };
	if (_uav.IsValid()) return { _uav.CPU() };

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	desc.Buffer.NumElements = (uint32)GetVertexCount();
	desc.Buffer.StructureByteStride = (uint32)GetVertexSize();

	std::lock_guard<std::mutex> lock(_mutex);
	HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
	HDirectXAPI::GetDevice()->CreateUnorderedAccessView(Get(), nullptr, &desc, alloc.CPU());

	_uav = std::move(alloc);
	return _uav.CPU();
}
