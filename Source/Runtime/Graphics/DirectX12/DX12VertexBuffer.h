#pragma once
#include "Classes/VertexBuffer.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"
#include "DirectX12/Classes/DirectX12Helper.h"
#include "Classes/DescriptionAllocator.h"

class PDX12VertexBuffer 
	: public IVertexBuffer
{

	PName _name;
	EBufferLoadMethod _loadMethod = EBufferLoadMethod::CPULoad;

	uint64 _elementSize  = 0;
	uint64 _elementCount = 0;
	void* _cpuData = nullptr;          // CPULoad: UPLOAD 힙 매핑 포인터
	HList<uint8> _shadowData;          // GPULoad: CPU 사본. GetDatas/GetData가 이걸 돌려준다.

	HDX12ComPtr<HDX12Resource> _dx12Resource;

	mutable std::mutex _mutex;

	mutable HDescriptionAllocation _srv;
	mutable HDescriptionAllocation _uav;
public:
	virtual ~PDX12VertexBuffer();

public:
	// IJGGraphicsObject
	virtual const PName& GetName() const override;
	virtual void SetName(const PName& inName) override;
	// ~IJGGraphicsObject

	// IVertexBuffer
	virtual void SetDatas(const void* inDatas, uint64 inElementSize, uint64 inElementCount) override;
	virtual void SetData(const void* inData, uint64 inIndex) override;
	virtual void* GetDatas() const override;
	virtual void* GetData(uint64 inIndex) const override;

	virtual uint64 GetVertexCount() const override;
	virtual uint64 GetVertexSize() const override;
	virtual EBufferLoadMethod GetLoadMethod() const override;

	virtual void Reset() override;
	virtual bool IsValid() const override;
	// ~IVertexBuffer

	// 리소스를 만들기 전(CreateVertexBuffer 직후)에 정한다. 이미 만들어진 뒤 바꾸면 버퍼를 비우고 다음 SetDatas에서 다시 만든다.
	void SetLoadMethod(EBufferLoadMethod inLoadMethod);

	HDX12Resource* Get() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const;

private:
	// GPULoad: CPU 사본 전체를 스테이징 관리자에 업로드 요청한다.
	void requestUpload();
};
