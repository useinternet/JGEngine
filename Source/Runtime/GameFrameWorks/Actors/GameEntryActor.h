#pragma once
#include "Actors/Actor.h"
#include "GameEntryActor.generation.h"

class JGGameInstance;

// 월드 진입점. JGGameInstance 가 월드를 만들 때 등록된 파생 클래스를 스폰하고 OnEnterWorld 를 부른다.
// 게임은 여기서 GameMasterActor · Controller 를 스폰하고 GameMaster 구성을 호출한다.
// GameMaster 구성 자체는 여기에 두지 말고 규칙 쪽 함수로 분리한다 (콘솔 검증이 같은 함수를 부른다).
JGCLASS()
class GAMEFRAMEWORKS_API JGGameEntryActor : public JGActor
{
	JG_GENERATED_CLASS_BODY

	friend class JGGameInstance;

public:
	JGGameEntryActor() = default;
	virtual ~JGGameEntryActor() = default;

	JGGameInstance& GetGameInstance() const;

protected:
	// 월드 생성 직후 · BeginPlay 전.
	virtual void OnEnterWorld() {}
	// 월드 언로드 직전.
	virtual void OnExitWorld() {}
};
