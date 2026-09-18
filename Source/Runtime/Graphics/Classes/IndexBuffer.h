#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"

struct GRAPHICS_API HIndexBufferConstructArguments
{
	PName Name;
	// GPULoad: DEFAULT 힙, 전송 관리자가 올린다(정적 메시). CPULoad: UPLOAD 힙 상시 매핑(매 프레임 갱신).
	EBufferLoadMethod LoadMethod = EBufferLoadMethod::CPULoad;
};

class GRAPHICS_API IIndexBuffer : public IJGGraphicsObject
{
public:

	virtual void SetDatas(const uint32* inDatas, uint64 inCount) = 0;
	// GPULoad 버퍼는 CPU 사본을 고친 뒤 버퍼 전체를 다시 올린다. 한 프레임 안의 여러 호출은 스테이징 하나로 합쳐진다.
	virtual void SetData(uint32 inData, uint64 inIndex) = 0;
	// CPU에서 읽을 수 있는 데이터. CPULoad는 매핑된 UPLOAD 메모리, GPULoad는 버퍼가 들고 있는 CPU 사본.
	virtual uint32* GetDatas() const = 0;
	virtual uint32 GetData(uint64 inIndex) const = 0;

	virtual uint64 GetIndexCount() const = 0;
	virtual EBufferLoadMethod GetLoadMethod() const = 0;

	virtual void Reset() = 0;
	virtual bool IsValid() const = 0;
};
