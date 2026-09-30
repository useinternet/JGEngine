#pragma once
#include "MyGameDefines.h"
#include "Actors/GameEntryActor.h"
#include "MyGameEntryActor.generation.h"

// 월드 진입점. 엔진이 월드를 만든 직후 스폰하고 OnEnterWorld 를 부른다.
JGCLASS()
class MYGAME_API JGMyGameEntryActor : public JGGameEntryActor
{
	JG_GENERATED_CLASS_BODY

public:
	JGMyGameEntryActor() = default;
	virtual ~JGMyGameEntryActor() = default;

	JGPROPERTY()
	int32 StartTurn = 0;

protected:
	virtual void OnEnterWorld() override;
};
