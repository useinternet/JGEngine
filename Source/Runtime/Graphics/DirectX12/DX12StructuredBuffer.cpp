#include "PCH/PCH.h"
#include "DX12StructuredBuffer.h"
#include "DirectX12API.h"

PDX12StructuredBuffer::~PDX12StructuredBuffer()
{
	Reset();
}

const PName& PDX12StructuredBuffer::GetName() const
{
	return _name;
}

void PDX12StructuredBuffer::SetName(const PName& inName)
{
	_name = inName;
	if (IsValid())
	{
		_dx12Resource->SetName(inName.ToString().GetRawWString().c_str());
	}
}

void PDX12StructuredBuffer::SetDatas(const void* inDatas, uint64 inElementSize, uint64 inElementCount)
{
	uint64 originBtSize = _elementSize * _elementCount;
	_elementCount = inElementCount;
	uint64 btSize = _elementSize * _elementCount;


	if (IsValid() && (originBtSize != btSize))
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

		_dx12Resource->Map(0, nullptr, &_cpuData);
	}

	if (_cpuData != nullptr && inDatas != nullptr)
	{
		memcpy(_cpuData, inDatas, btSize);
	}
}

void PDX12StructuredBuffer::SetData(const void* inData, uint64 inIndex) const
{
	memcpy(GetData(inIndex), inData, _elementSize);
}

void* PDX12StructuredBuffer::GetDatas() const
{
	return _cpuData;
}

void* PDX12StructuredBuffer::GetData(uint64 inIndex) const
{
	JG_CHECK(inIndex < _elementCount && IsValid() && _cpuData != nullptr);

	uint64 dataOffset = HMath::AlignUp(inIndex * _elementSize, _elementSize);

	void* dataPos = (void*)((uint64)_cpuData + dataOffset);
	return dataPos;
}

uint64 PDX12StructuredBuffer::GetDataSize() const
{
	return _elementSize * _elementCount;
}

uint64 PDX12StructuredBuffer::GetElementCount() const
{
	return _elementCount;
}

uint64 PDX12StructuredBuffer::GetElementSize() const
{
	return _elementSize;
}

void PDX12StructuredBuffer::Reset()
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

bool PDX12StructuredBuffer::IsValid() const
{
	return _dx12Resource != nullptr;
}

HDX12Resource* PDX12StructuredBuffer::Get() const
{
	return _dx12Resource.Get();
}
