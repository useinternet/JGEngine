#pragma once
#include "GameMaster/State/GameplayBoardState.h"

// 보드 질의. 상태(HGameplayBoardState)는 값 타입으로 HGameplayState 안에 있고, 규칙(거리 · 이웃 · 경로 · 시야)은 이 객체가 준다.
// 구현은 상태를 갖지 않는다. 같은 보드 객체를 어떤 상태 복사본에도 쓸 수 있다.
class GAMEFRAMEWORKS_API IGameplayBoard : public IMemoryObject
{
public:
	virtual ~IGameplayBoard() = default;

	virtual EGameplayBoardKind GetKind() const = 0;
	virtual int32 Distance(const HGameplayCoord& a, const HGameplayCoord& b) const = 0;
	// 경계 안의 이웃만. 순서는 고정 (결정론).
	virtual void  Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const = 0;

	// 경계 안 · 막히지 않음 · 비어 있음.
	virtual bool IsPassable(const HGameplayBoardState& board, const HGameplayCoord& coord) const;
	// 너비 우선 탐색. 출발은 제외하고 도착을 포함한 경로. 도착 칸은 점유돼 있어도 허용한다 (공격 이동 등은 게임이 자른다).
	virtual bool FindPath(const HGameplayBoardState& board, const HGameplayCoord& from, const HGameplayCoord& to, int32 maxSteps, HList<HGameplayCoord>& outPath) const;
	// 격자 선: from 에서 to 까지 지나는 칸(양 끝 포함, from 부터 차례로). 같은 두 칸이면 어느 쪽에서 그어도 같은 칸들을 지난다(대칭 · 결정론).
	// 사각 = 브레젠험(대각 걸음 허용), 육각 = 큐브 좌표 선형 보간 + 반올림. 격자가 아닌 보드는 비운다. 보드 경계는 보지 않는다.
	virtual void TraceLine(const HGameplayCoord& from, const HGameplayCoord& to, HList<HGameplayCoord>& outCells) const;
	// 시야. 기본 규칙: 양 끝이 경계 안이고, 격자 선의 사이 칸(양 끝 제외)이 막히지 않았으면(IsBlocked) 보인다. 점유(엔티티)는 시야를 막지 않는다.
	// 격자 선이 없는 보드(자유 공간)는 경계만 본다. 다른 규칙(유닛이 가림 · 높이 등)은 게임이 파생해 정의한다 — TraceLine 을 그대로 써도 된다.
	virtual bool HasLineOfSight(const HGameplayBoardState& board, const HGameplayCoord& from, const HGameplayCoord& to) const;
	// 거리 range 이하의 경계 안 좌표 (중심 포함). 격자만 지원. 자유 공간은 비어 있다.
	virtual void CoordsInRange(const HGameplayBoardState& board, const HGameplayCoord& center, int32 range, HList<HGameplayCoord>& outCoords) const;

	static PSharedPtr<IGameplayBoard> Create(EGameplayBoardKind kind);
};

class GAMEFRAMEWORKS_API PGameplayBoardNone : public IGameplayBoard
{
public:
	virtual ~PGameplayBoardNone() = default;
	virtual EGameplayBoardKind GetKind() const override;
	virtual int32 Distance(const HGameplayCoord& a, const HGameplayCoord& b) const override;
	virtual void  Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const override;
	virtual bool  IsPassable(const HGameplayBoardState& board, const HGameplayCoord& coord) const override;
};

// 사각 격자. 4방 이웃, 맨해튼 거리. 격자 선은 브레젠험(8방 걸음 — 대각선 끝이 맞닿은 두 막힌 칸 사이로도 보인다).
class GAMEFRAMEWORKS_API PGameplayBoardSquare : public IGameplayBoard
{
public:
	virtual ~PGameplayBoardSquare() = default;
	virtual EGameplayBoardKind GetKind() const override;
	virtual int32 Distance(const HGameplayCoord& a, const HGameplayCoord& b) const override;
	virtual void  Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const override;
	virtual void  TraceLine(const HGameplayCoord& from, const HGameplayCoord& to, HList<HGameplayCoord>& outCells) const override;
};

// 육각 격자. 축 좌표 (X = q, Y = r). 6방 이웃, 육각 거리. 격자 선은 거리 + 1 칸이고 이웃끼리 이어진다.
class GAMEFRAMEWORKS_API PGameplayBoardHex : public IGameplayBoard
{
public:
	virtual ~PGameplayBoardHex() = default;
	virtual EGameplayBoardKind GetKind() const override;
	virtual int32 Distance(const HGameplayCoord& a, const HGameplayCoord& b) const override;
	virtual void  Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const override;
	virtual void  TraceLine(const HGameplayCoord& from, const HGameplayCoord& to, HList<HGameplayCoord>& outCells) const override;
};

// 자유 공간. 정수 좌표, 체비쇼프 거리, 8방 이웃 (경계 없음).
class GAMEFRAMEWORKS_API PGameplayBoardFree : public IGameplayBoard
{
public:
	virtual ~PGameplayBoardFree() = default;
	virtual EGameplayBoardKind GetKind() const override;
	virtual int32 Distance(const HGameplayCoord& a, const HGameplayCoord& b) const override;
	virtual void  Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const override;
	virtual bool  IsPassable(const HGameplayBoardState& board, const HGameplayCoord& coord) const override;
};
