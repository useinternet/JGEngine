#pragma once
#include "Core/GameFrameWorksDefines.h"
#include "GameMaster/Messages/GameplayEvent.h"
#include "GameplayCue.generation.h"

class JGGameMasterActor;

// 연출 단위 하나. 이벤트 종류마다 게임이 파생한다 (연극의 큐: 배우에게 주는 신호).
// GameMasterActor 가 프로토타입에 Accepts 를 묻고, 받아들이면 새 인스턴스를 만들어 Begin → Tick → IsDone 으로 재생한다.
// 상태는 읽기만 한다. 명령을 직접 내지 않는다.
JGCLASS()
class GAMEFRAMEWORKS_API JGGameplayCue : public JGObject
{
	JG_GENERATED_CLASS_BODY

public:
	JGGameplayCue() = default;
	virtual ~JGGameplayCue() = default;

	// 이 큐가 다루는 이벤트인가. 프로토타입에서 불린다.
	virtual bool Accepts(const HGameplayEvent& event) const;
	// 재생 시작. gameMasterActor 로 액터를 찾는다.
	virtual void Begin(const HGameplayEvent& event, JGGameMasterActor& gameMasterActor);
	virtual void Tick(float32 deltaSeconds);
	// 기본은 즉시 완료 (연출 없는 이벤트).
	virtual bool IsDone() const;
	// 연타 · 스킵 시 최종 상태로 즉시.
	virtual void Skip();

	// 같은 클래스의 새 인스턴스 (리플렉션).
	PSharedPtr<JGGameplayCue> CreateInstance() const;
};
