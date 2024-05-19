#include "PCH/PCH.h"
#include "DX12IndexBuffer.h"
#include "DirectX12API.h"

PDX12IndexBuffer::~PDX12IndexBuffer()
{
	Reset();
}

const PName& PDX12IndexBuffer::GetName() const
{
	return _name;
}

void PDX12IndexBuffer::SetName(const PName& inName)
{
	_name = inName;
	if (IsValid())
	{
		_dx12Resource->SetName(inName.ToString().GetRawWString().c_str());
	}
}

void PDX12IndexBuffer::SetDatas(const uint32* inDatas, uint64 inCount)
{
	uint64 originBtSize = sizeof(uint32) * _indexCount;
	_indexCount = inCount;
	uint64 btSize = sizeof(uint32) * _indexCount;

	// Create
	if (IsValid() && ((originBtSize != btSize) || _cpuData == nullptr))
	{
		Reset();
	}

	if (IsValid() == false)
	{
		CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(btSize);

		_dx12Resource = HDirectXAPI::CreateCommittedResource(
			GetName().ToString(),
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

		_dx12Resource->Map(0, nullptr, (void**)&_cpuData);
	}

	if (_cpuData != nullptr && inDatas != nullptr)
	{
		memcpy(_cpuData, inDatas, btSize);
	}
}

void PDX12IndexBuffer::SetData(uint32 inData, uint64 inIndex)
{
	JG_CHECK(inIndex < _indexCount && IsValid() && _cpuData != nullptr);

	uint64  dataOffset = HMath::AlignUp(inIndex * sizeof(uint32), sizeof(uint32));
	uint32* dataPos = (uint32*)((uint64)_cpuData + dataOffset);

	*dataPos = inData;
}

uint32* PDX12IndexBuffer::GetDatas() const
{
	return _cpuData;
}

uint32 PDX12IndexBuffer::GetData(uint64 inIndex) const
{
	JG_CHECK(inIndex < _indexCount && IsValid() && _cpuData != nullptr);

	uint64 dataOffset = HMath::AlignUp(inIndex * sizeof(uint32), sizeof(uint32));
	uint32 data = *((uint32*)((uint64)_cpuData + dataOffset));

	return data;
}

uint64 PDX12IndexBuffer::GetIndexCount() const
{
	return _indexCount;
}


void PDX12IndexBuffer::Reset()
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
	_indexCount = 0;
}

bool PDX12IndexBuffer::IsValid() const
{
	return _dx12Resource != nullptr;
}

HDX12Resource* PDX12IndexBuffer::Get() const
{
	return _dx12Resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12IndexBuffer::GetSRV() const
{
	if (IsValid() == false) return { 0 };
	if (_srv.IsValid()) return { _srv.CPU().ptr };

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Buffer.NumElements = (uint32)_indexCount;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	desc.Buffer.StructureByteStride = 0;

	std::lock_guard<std::mutex> lock(_mutex);
	HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
	HDirectXAPI::GetDevice()->CreateShaderResourceView(Get(), &desc, alloc.CPU());

	_srv = std::move(alloc);
	return _srv.CPU();
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12IndexBuffer::GetUAV() const
{
	if (IsValid() == false) return { 0 };
	if (_uav.IsValid()) return { _uav.CPU() };

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Format = DXGI_FORMAT_R32_TYPELESS;
	desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	desc.Buffer.NumElements = (uint32)_indexCount;
	desc.Buffer.StructureByteStride = 0;

	std::lock_guard<std::mutex> lock(_mutex);
	HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
	HDirectXAPI::GetDevice()->CreateUnorderedAccessView(Get(), nullptr, &desc, alloc.CPU());

	_uav = std::move(alloc);
	return _uav.CPU();
}
