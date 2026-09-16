#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"


// 그래픽 리소스 인터페이스의 공통 뿌리. 메모리 시스템 규칙에 따라 IMemoryObject를 여기서 한 번만 상속하고,
// 구현 클래스(PDX12Texture 등)는 이 사슬만 타야 인터페이스 포인터가 할당 주소와 같아진다.
class GRAPHICS_API IJGGraphicsObject : public IMemoryObject
{
public:
	virtual ~IJGGraphicsObject() = default;

	virtual const PName& GetName() const = 0;
	virtual void SetName(const PName& inName) = 0;
};