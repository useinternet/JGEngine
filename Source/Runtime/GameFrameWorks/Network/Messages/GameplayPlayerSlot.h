#pragma once
#include "GameMaster/GameMasterDefines.h"
#include "Network/Transport/NetTransport.h"

// 참가자 자리. 조작 주체(팀) 번호 목록으로 이 자리의 플레이어가 어떤 행동자를 조작하는지 정한다.
// 공개 정보(번호 · 조작 주체 · 이름 · 접속 여부)만 직렬화한다. 피어 · 토큰 · 마지막 수신 시각은 호스트 내부용이다.
struct GAMEFRAMEWORKS_API HGameplayPlayerSlot : public IJsonable
{
	int32        Slot            = INDEX_NONE;
	HList<int32> Controllers;
	bool         bAllControllers = false;   // 모든 조작 주체 (Standalone 의 로컬 플레이어)
	bool         bConnected      = false;
	PString      Name;

	// 호스트 내부용. 직렬화하지 않는다.
	HNetPeerId   Peer      = NetPeerNone;
	PString      Token;                     // 재접속 확인. 한 번이라도 쓰인 자리는 토큰이 있다
	float32      LastHeard = 0.0f;

	bool Controls(int32 controller) const;
	bool IsFree() const;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
