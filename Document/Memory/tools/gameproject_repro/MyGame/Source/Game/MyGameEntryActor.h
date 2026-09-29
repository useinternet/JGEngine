#pragma once
#include "GameDefines.h"
#include "Actors/GameEntryActor.h"
#include "MyGameEntryActor.generation.h"

JGCLASS()
class GAME_API JGMyGameEntryActor : public JGGameEntryActor
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
