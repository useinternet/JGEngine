#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "JGGraphicsObject.h"

class IVertexBuffer;
class IIndexBuffer;
class IRawMaterial;

// 렌더러가 소비하는 메시 인터페이스. 서브메시 단위로 정점/인덱스 버퍼와 머터리얼을 노출한다.
// 메모리 규칙: IMemoryObject는 IJGGraphicsObject 사슬을 통해 한 번만 상속하고, 구현 클래스(PStaticMesh)는 이 사슬만 탄다.
// 에셋(JGStaticMesh)은 JGObject 뿌리를 따로 가지므로 이 인터페이스를 직접 구현하지 않고 IMesh 구현체를 소유한다.
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
