#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"


class IJGGraphicsObject;
class IRawMaterial;
class PStaticMesh;
class PSkeletalMesh;

class PScene : public IMemoryObject
{
public:
	virtual ~PScene() = default;

	//PSharedPtr<IMaterial> AddMaterial();
	//PSh

	// Scene�� �ܺο��� ���͸��� �߰�
	// Scene�� �ܺο��� �޽� �߰�

	void AddMaterial(PSharedPtr<IRawMaterial> inMaterial);
	void RemoveMaterial(PSharedPtr<IRawMaterial> inMaterial);

	void AddStaticMesh(PSharedPtr)
public:
	HList<PSharedPtr<IJGGraphicsObject>> _graphicsObjectPool;


	// scene�� vertex ����


};