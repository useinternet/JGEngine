#pragma once
#include "Core.h"
#include "JGGraphicsObject.h"

struct GRAPHICS_API HVertexBufferConstructArguments
{
	PName Name;
	// GPULoad: DEFAULT 힙, 스테이징 관리자가 올린다(정적 메시). CPULoad: UPLOAD 힙 상시 매핑.
	// CPULoad를 매 프레임 다시 쓰면 앞 프레임이 GPU에서 읽는 중일 수 있다(프레임 파이프라이닝, 5-5). 매 프레임 바뀌는 데이터는 커맨드 리스트의 업로드 할당자로 올린다.
	EBufferLoadMethod LoadMethod = EBufferLoadMethod::CPULoad;
};

// GPU 정점 버퍼. GPU에 올리는 일만 약속한다. CPU에서 다시 읽는 길은 없다(정점 CPU 사본은 에셋이 든다, 5-26).
class GRAPHICS_API IVertexBuffer : public IJGGraphicsObject
{
public:
	// 버퍼 전체를 올린다. GPULoad는 스테이징 관리자가 요청 시점에 스테이징으로 복사하므로 호출이 끝나면 inDatas를 버려도 된다.
	virtual void SetDatas(const void* inDatas, uint64 inElementSize, uint64 inElementCount) = 0;

	virtual uint64 GetVertexCount() const = 0;
	virtual uint64 GetVertexSize() const = 0;
	virtual EBufferLoadMethod GetLoadMethod() const = 0;

	virtual void Reset() = 0;
	virtual bool IsValid() const = 0;
};
