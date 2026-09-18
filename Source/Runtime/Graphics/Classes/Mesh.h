#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"

class IVertexBuffer;
class IIndexBuffer;
class IRawMaterial;

class GRAPHICS_API IMesh : public IJGGraphicsObject
{
public:
	virtual ~IMesh() = default;

	virtual uint32 GetSubMeshCount() const = 0;
	virtual PSharedPtr<IVertexBuffer> GetVertexBuffer(uint32 inSubMeshIndex) const = 0;
	virtual PSharedPtr<IIndexBuffer>  GetIndexBuffer(uint32 inSubMeshIndex) const = 0;
	// 서브메시에 머터리얼이 없으면 기본 머터리얼을 돌려준다. Draw 쪽은 null을 따로 처리하지 않는다.
	virtual PSharedPtr<IRawMaterial>  GetMaterial(uint32 inSubMeshIndex) const = 0;
	// 정점 버퍼의 입력 레이아웃. PSO 생성에 쓴다.
	virtual const HInputLayout& GetInputLayout() const = 0;

	virtual bool IsValid() const = 0;
	virtual void Reset() = 0;
};
