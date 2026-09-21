#include "PCH/PCH.h"
#include "DX12VertexBuffer.h"
#include "DirectX12API.h"
#include "Classes/ResourceStagingManager.h"

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
	const uint64 btSize = inElementSize * inElementCount;
	if (btSize == 0)
	{
		// 0바이트 리소스는 만들 수 없다. 빈 버퍼로 둔다.
		Reset();
		return;
	}

	const uint64 originBtSize = _elementSize * _elementCount;
	const bool   bGPULoad     = (_loadMethod == EBufferLoadMethod::GPULoad);
	if (IsValid() && (originBtSize != btSize || (bGPULoad == false && _cpuData == nullptr)))
	{
		Reset();
	}

	// Reset()이 개수를 0으로 지우므로 그 뒤에 넣는다. (이전에는 크기가 바뀌면 GetVertexCount()가 0을 돌려줬다. 5-12)
	_elementSize  = inElementSize;
	_elementCount = inElementCount;

	if (IsValid() == false)
	{
		// GPULoad: DEFAULT 힙, COMMON. 스테이징 관리자가 스테이징으로 올리고 VERTEX_AND_CONSTANT_BUFFER로 전이한다.
		// CPULoad: UPLOAD 힙, GENERIC_READ, 상시 매핑.
		CD3DX12_HEAP_PROPERTIES heapProperties(bGPULoad ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(btSize);

		_dx12Resource = HDirectXAPI::CreateCommittedResource(
			GetName().ToString(),
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			bGPULoad ? D3D12_RESOURCE_STATE_COMMON : D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr
		);

		if (_dx12Resource == nullptr)
		{
			JG_LOG(Graphics, ELogLevel::Error, "%s : Fail Create VertexBuffer (%d bytes)", GetName(), (int32)btSize);
			_elementSize  = 0;
			_elementCount = 0;
			return;
		}

		{
			std::lock_guard<std::mutex> lock(_mutex);
			_srv.Reset();
			_uav.Reset();
		}

		if (bGPULoad == false)
		{
			_dx12Resource->Map(0, nullptr, &_cpuData);
		}
	}

	if (inDatas == nullptr)
	{
		return;
	}

	if (bGPULoad)
	{
		_shadowData.assign((const uint8*)inDatas, (const uint8*)inDatas + btSize);
		requestUpload();
	}
	else if (_cpuData != nullptr)
	{
		memcpy(_cpuData, inDatas, btSize);
	}
}

void PDX12VertexBuffer::SetData(const void* inData, uint64 inIndex)
{
	void* dataPos = GetData(inIndex);
	if (dataPos == nullptr || inData == nullptr)
	{
		return;
	}

	memcpy(dataPos, inData, _elementSize);

	if (_loadMethod == EBufferLoadMethod::GPULoad)
	{
		// CPU 사본만 고쳤으므로 GPU에 다시 올린다. 같은 대상의 대기 요청은 하나로 합쳐진다.
		requestUpload();
	}
}

void* PDX12VertexBuffer::GetDatas() const
{
	if (_loadMethod == EBufferLoadMethod::GPULoad)
	{
		return _shadowData.empty() ? nullptr : (void*)_shadowData.data();
	}
	return _cpuData;
}

void* PDX12VertexBuffer::GetData(uint64 inIndex) const
{
	JG_CHECK(inIndex < _elementCount && IsValid());

	uint8* datas = (uint8*)GetDatas();
	if (datas == nullptr || inIndex >= _elementCount)
	{
		return nullptr;
	}

	return datas + inIndex * _elementSize;
}

uint64 PDX12VertexBuffer::GetVertexCount() const
{
	return _elementCount;
}

uint64 PDX12VertexBuffer::GetVertexSize() const
{
	return _elementSize;
}

EBufferLoadMethod PDX12VertexBuffer::GetLoadMethod() const
{
	return _loadMethod;
}

void PDX12VertexBuffer::SetLoadMethod(EBufferLoadMethod inLoadMethod)
{
	if (_loadMethod == inLoadMethod)
	{
		return;
	}

	_loadMethod = inLoadMethod;
	if (IsValid())
	{
		Reset();
	}
}

void PDX12VertexBuffer::Reset()
{
	_shadowData.clear();

	if (_dx12Resource == nullptr)
	{
		return;
	}

	// 대기 중인 업로드 요청이 있어도 된다. 등록이 풀린 대상은 기록 시점에 버려진다. (PResourceStagingManager::recordUpload)
	HDirectXAPI::DestroyCommittedResource(_dx12Resource);

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

void PDX12VertexBuffer::requestUpload()
{
	if (IsValid() == false || _shadowData.empty())
	{
		return;
	}

	PSharedPtr<PResourceStagingManager> stagingManager = HDirectXAPI::GetResourceStagingManager();
	if (stagingManager == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "%s : ResourceStagingManager is not available. vertex data is not uploaded", GetName());
		return;
	}

	stagingManager->RequestBufferUpload(_dx12Resource, _shadowData.data(), _shadowData.size(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, _name);
}
