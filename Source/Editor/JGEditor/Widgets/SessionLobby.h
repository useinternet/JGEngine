#pragma once
#include "JGEditorDefine.h"
#include "Widget.h"
#include "SessionLobby.generation.h"

class PGameplaySession;
class PGameplayHostSession;

// 세션 로비 개발용 창 (Server 5-5, 대기방 기능 묶음 D-38). 게임 인스턴스의 세션을 보여 주고 바꾼다.
//   맨 위 고정 줄: Host(포트, 이 기계에서만) · Join(주소 · 포트) · Leave(싱글로 돌아가 지금 게임을 이어받는다) ·
//     Reload World(권한 쪽만: 같은 월드를 다시 로드해 지금 참가자로 새 판 — 엔트리 액터가 권한 쪽에서 시작하는 규약을 다시 태운다) ·
//     Ready / Not Ready(네트워크 세션의 판 시작 전에만: 로컬 슬롯의 준비 표시)
//     버튼은 상태와 상관없이 같은 자리에 그린다. 지금 쓸 수 없는 버튼은 흐리게 그리고 누르면 이유를 보인다.
//   세션 공유 데이터 줄(권한 쪽만): 키 · 값 · Set Data · Remove Data
//   상태: 모드 · 세션 상태 · 로컬 슬롯 · 월드 · 붙은 GameMaster(순번 · 체크섬) · 닫힌 이유
//   슬롯 표: 번호 · 이름 · 접속 · 준비 · 조작 주체 / 세션 공유 데이터 표: 키 · 값
// 판을 만드는 일은 게임의 엔트리 액터 몫이고 이 창은 게임 인스턴스 · 세션 API 만 부른다.
// GameFrameWork 가 돌지 않으면 안내 문구만 그린다.
JGCLASS()
class JGEDITOR_API JGSessionLobby : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

private:
	PString _playerName = "Player";
	PString _portText   = "47771";
	PString _address    = "127.0.0.1";
	bool    _bLocalOnly = false;
	PString _dataKey    = "room";
	PString _dataValue;
	PString _lastResult;

protected:
	virtual PString GetTitleName() const override;
	virtual void OnGenerateGUI() override;

private:
	void drawActions(PGameplaySession& session);
	void drawDataActions(PGameplaySession& session);
	void drawStatus(PGameplaySession& session);
	void drawSlots(PGameplaySession& session);
	void drawSessionData(PGameplaySession& session);
	void host();
	void join();
	void leave();
	void reloadWorld();
	void toggleReady(PGameplaySession& session);
	void setData(PGameplayHostSession& host);
	void removeData(PGameplayHostSession& host);
	bool readPort(uint16* outPort);
};
