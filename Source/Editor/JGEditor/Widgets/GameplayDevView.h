#pragma once
#include "JGEditorDefine.h"
#include "Widget.h"
#include "GameMaster/State/GameplayEntityId.h"
#include "GameplayDevView.generation.h"

class PGameMaster;
class PWorld;
class JGGameMasterActor;
class PGameplayDevViewObserver;
struct HGameplayEvent;

// GameMaster 개발용 창. 활성 월드의 JGGameMasterActor 하나를 골라 규칙 쪽 상태를 보여 준다. 게임별 필드를 몰라도 되게 컴포넌트는 JSON 으로 보인다.
//   요약(명령 수 · 라운드 · 페이즈 · 행동자 · 엔티티 수 · 선택 대기) · 되돌리기 버튼 · 체크섬
//   엔티티 목록 → 고른 엔티티의 컴포넌트 값 (테이블마다 JSON)
//   이벤트 로그 (관찰자로 받은 이벤트와 상태 교체)
//   컨트롤러의 마지막 피킹 결과
// 무거운 텍스트(JSON · 체크섬)는 GameMaster 가 이벤트를 내거나 상태를 바꿀 때만 다시 만든다.
JGCLASS()
class JGEDITOR_API JGGameplayDevView : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

	friend class PGameplayDevViewObserver;

private:
	PWeakPtr<JGGameMasterActor>          _gameMasterActor;
	PWeakPtr<PGameMaster>                _gameMaster;
	PSharedPtr<PGameplayDevViewObserver> _observer;
	HDeque<PString>                      _eventLog;
	HVector2                             _contentSize;

	HGameplayEntityId _selectedEntity;
	uint64            _revision       = 1;   // 관찰자가 올린다
	uint64            _cachedRevision = 0;
	HGameplayEntityId _cachedEntity;
	uint64            _cachedChecksum = 0;
	HList<PString>    _cachedEntityLabels;
	HList<HGameplayEntityId> _cachedEntityIds;
	PString           _cachedComponents;

protected:
	virtual PString GetTitleName() const override;
	virtual void OnInitialize() override;
	virtual void OnShutdown() override;
	virtual void OnLayout(const HWidgetLayout& InLayout) override;
	virtual void OnGenerateGUI() override;

private:
	PSharedPtr<JGGameMasterActor> findGameMasterActor(PSharedPtr<PWorld> world) const;
	void watch(PSharedPtr<JGGameMasterActor> gameMasterActor);
	void unwatch();
	void refreshCache(const PGameMaster& gameMaster);
	void appendLog(const PString& line);
	void onEvents(const HList<HGameplayEvent>& events);
	void onStateReplaced();

	void drawSummary(PGameMaster& gameMaster);
	void drawEntities();
	void drawEventLog();
	void drawPicks(PSharedPtr<PWorld> world);
};
