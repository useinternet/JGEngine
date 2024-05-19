#include "PCH/PCH.h"
#include "DX12ConstantBuffer.h"
#include "DirectX12API.h"

PDX12ConstantBuffer::~PDX12ConstantBuffer()
{
	Reset();
}

const PName& PDX12ConstantBuffer::GetName() const
{
	return _name;
}

void PDX12ConstantBuffer::SetName(const PName& inName)
{
	_name = inName;
	if (IsValid())
	{
		_dx12Resource->SetName(inName.ToString().GetRawWString().c_str());
	}
}

void PDX12ConstantBuffer::SetData(const void* inData, uint64 elementSize)
{
	uint64 originBtSize = _elementSize;
	_elementSize = elementSize;

	if (IsValid() && (originBtSize != _elementSize))
	{
		Reset();
	}

	if (IsValid() == false)
	{
		CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(_elementSize);

		_dx12Resource = HDirectXAPI::CreateCommittedResource(
			GetName().ToString(),
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr
		);

		_dx12Resource->Map(0, nullptr, &_cpuData);

		{
			std::lock_guard<std::mutex> lock(_mutex);
			_csv.Reset();
		}
	}

	if (_cpuData != nullptr && inData != nullptr)
	{
		memcpy(_cpuData, inData, _elementSize);
	}
}

void PDX12ConstantBuffer::SetData(const void* inData, uint64 inOffset, uint64 inElementSize)
{
	JG_CHECK(IsValid());
	memcpy(GetData(inOffset), inData, inElementSize);
}

void* PDX12ConstantBuffer::GetData(uint64 inOffset) const
{
	return (void*)((uint64)_cpuData + inOffset);
}

uint64 PDX12ConstantBuffer::GetDataSize() const
{
	return _elementSize;
}

void PDX12ConstantBuffer::Reset()
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
	_elementSize = 0;
}

bool PDX12ConstantBuffer::IsValid() const
{
	return _dx12Resource != nullptr;
}

HDX12Resource* PDX12ConstantBuffer::Get() const
{
	return _dx12Resource.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE PDX12ConstantBuffer::GetCSV() const
{
	if (IsValid() == false) return { 0 };
	if (_csv.IsValid()) return { _csv.CPU() };

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = _dx12Resource->GetGPUVirtualAddress();
	desc.SizeInBytes    = (uint32)_elementSize;

	std::lock_guard<std::mutex> lock(_mutex);
	HDescriptionAllocation alloc = HDirectXAPI::CSUAllocate();
	HDirectXAPI::GetDevice()->CreateConstantBufferView(&desc, alloc.CPU());

	_csv = std::move(alloc);
	return _csv.CPU();
}