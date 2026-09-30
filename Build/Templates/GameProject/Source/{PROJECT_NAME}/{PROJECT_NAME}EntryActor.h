#pragma once
#include "{PROJECT_NAME}Defines.h"
#include "Actors/GameEntryActor.h"
#include "{PROJECT_NAME}EntryActor.generation.h"

// 월드 진입점. 엔진이 월드를 만든 직후 스폰하고 OnEnterWorld 를 부른다.
// 리플렉션 헤더 규칙: 헤더 하나에 JGCLASS 하나, 자기 .generation.h 를 직접 include,
// JGPROPERTY() 는 한 줄에, 파일 이름은 엔진의 리플렉션 헤더(예: Actor.h)와 겹치지 않게.
JGCLASS()
class {PROJECT_NAME_UPPER}_API JG{PROJECT_NAME}EntryActor : public JGGameEntryActor
{
	JG_GENERATED_CLASS_BODY

public:
	JG{PROJECT_NAME}EntryActor() = default;
	virtual ~JG{PROJECT_NAME}EntryActor() = default;

protected:
	virtual void OnEnterWorld() override;
};
