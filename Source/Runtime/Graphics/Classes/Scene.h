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

	// Scene에 외부에서 머터리얼 추가
	// Scene에 외부에서 메시 추가

	void AddMaterial(PSharedPtr<IRawMaterial> inMaterial);
	void RemoveMaterial(PSharedPtr<IRawMaterial> inMaterial);

	void AddStaticMesh(PSharedPtr)
public:
	HList<PSharedPtr<IJGGraphicsObject>> _graphicsObjectPool;


	// scene에 vertex 통합


};