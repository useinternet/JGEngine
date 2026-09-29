#pragma once
#include "GameMaster/State/GameplayState.h"
#include "GameMaster/Messages/GameplayEvent.h"

// 실행 결과(이벤트 목록)와 상태 교체(되돌리기 · 로드 · 리플레이) 구독. 프레젠테이션 · 로그 · 테스트가 구현한다.
class GAMEFRAMEWORKS_API IGameplayObserver : public IMemoryObject
{
public:
	virtual ~IGameplayObserver() = default;

	virtual void OnGameplayEvents(const HGameplayState& state, const HList<HGameplayEvent>& events) {}
	virtual void OnGameplayStateReplaced(const HGameplayState& state) {}
};

// 저장 스키마 버전 간 변환. 로드 시 문서 버전이 현재와 다르면 호출된다.
class GAMEFRAMEWORKS_API IGameplayMigrator : public IMemoryObject
{
public:
	virtual ~IGameplayMigrator() = default;

	// document 는 파일 전체 (Initial · Current · Commands). 성공하면 true.
	virtual bool Migrate(uint32 fromVersion, uint32 toVersion, PJson& document) const = 0;
};
