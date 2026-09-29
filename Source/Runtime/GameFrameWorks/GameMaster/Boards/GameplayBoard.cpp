#include "PCH/PCH.h"
#include "GameMaster/Boards/GameplayBoard.h"

namespace
{
	int32 absInt(int32 v)
	{
		return v < 0 ? -v : v;
	}

	int32 maxInt(int32 a, int32 b)
	{
		return a > b ? a : b;
	}
}

// IGameplayBoard --------------------------------------------------------------

bool IGameplayBoard::IsPassable(const HGameplayBoardState& board, const HGameplayCoord& coord) const
{
	if (board.InBounds(coord) == false)
	{
		return false;
	}
	if (board.IsBlocked(coord) == true)
	{
		return false;
	}
	return board.OccupantAt(coord).IsValid() == false;
}

bool IGameplayBoard::FindPath(const HGameplayBoardState& board, const HGameplayCoord& from, const HGameplayCoord& to, int32 maxSteps, HList<HGameplayCoord>& outPath) const
{
	outPath.clear();

	if (board.InBounds(from) == false || board.InBounds(to) == false)
	{
		return false;
	}
	if (from == to)
	{
		return true;
	}
	if (maxSteps <= 0)
	{
		return false;
	}

	// BFS. 방문 순서와 이웃 순서가 고정이라 결과가 결정론적이다.
	HList<HGameplayCoord> frontier;
	HList<HGameplayCoord> parents;
	HList<int32>            parentIndex;
	HList<int32>            depth;

	frontier.push_back(from);
	parentIndex.push_back(INDEX_NONE);
	depth.push_back(0);

	// 방문 집합. 격자는 셀 인덱스로, 자유 공간은 선형 비교로.
	HList<uint8> visitedCells;
	if (board.IsGrid() == true)
	{
		visitedCells.resize((uint64)board.Width * (uint64)board.Height, 0);
		visitedCells[board.CellIndex(from)] = 1;
	}

	auto isVisited = [&](const HGameplayCoord& c) -> bool
	{
		if (board.IsGrid() == true)
		{
			return visitedCells[board.CellIndex(c)] != 0;
		}
		for (const HGameplayCoord& v : frontier)
		{
			if (v == c)
			{
				return true;
			}
		}
		return false;
	};

	auto markVisited = [&](const HGameplayCoord& c)
	{
		if (board.IsGrid() == true)
		{
			visitedCells[board.CellIndex(c)] = 1;
		}
	};

	int32 foundIndex = INDEX_NONE;
	HList<HGameplayCoord> neighbors;

	for (uint64 head = 0; head < frontier.size(); ++head)
	{
		HGameplayCoord current = frontier[head];
		int32 currentDepth = depth[head];
		if (currentDepth >= maxSteps)
		{
			continue;
		}

		neighbors.clear();
		Neighbors(board, current, neighbors);

		for (const HGameplayCoord& next : neighbors)
		{
			if (isVisited(next) == true)
			{
				continue;
			}
			bool bGoal = (next == to);
			if (bGoal == false && IsPassable(board, next) == false)
			{
				continue;
			}

			markVisited(next);
			frontier.push_back(next);
			parentIndex.push_back((int32)head);
			depth.push_back(currentDepth + 1);

			if (bGoal == true)
			{
				foundIndex = (int32)frontier.size() - 1;
				break;
			}
		}

		if (foundIndex != INDEX_NONE)
		{
			break;
		}
	}

	if (foundIndex == INDEX_NONE)
	{
		return false;
	}

	HList<HGameplayCoord> reversed;
	int32 cursor = foundIndex;
	while (cursor != INDEX_NONE && cursor != 0)
	{
		reversed.push_back(frontier[cursor]);
		cursor = parentIndex[cursor];
	}

	for (int32 i = (int32)reversed.size() - 1; i >= 0; --i)
	{
		outPath.push_back(reversed[i]);
	}
	return true;
}

bool IGameplayBoard::HasLineOfSight(const HGameplayBoardState& board, const HGameplayCoord& from, const HGameplayCoord& to) const
{
	return board.InBounds(from) == true && board.InBounds(to) == true;
}

void IGameplayBoard::CoordsInRange(const HGameplayBoardState& board, const HGameplayCoord& center, int32 range, HList<HGameplayCoord>& outCoords) const
{
	if (board.IsGrid() == false || range < 0)
	{
		return;
	}

	for (int32 y = center.Y - range; y <= center.Y + range; ++y)
	{
		for (int32 x = center.X - range; x <= center.X + range; ++x)
		{
			HGameplayCoord coord(x, y);
			if (board.InBounds(coord) == false)
			{
				continue;
			}
			if (Distance(center, coord) <= range)
			{
				outCoords.push_back(coord);
			}
		}
	}
}

PSharedPtr<IGameplayBoard> IGameplayBoard::Create(EGameplayBoardKind kind)
{
	switch (kind)
	{
	case EGameplayBoardKind::Square:
		return Allocate<PGameplayBoardSquare>();
	case EGameplayBoardKind::Hex:
		return Allocate<PGameplayBoardHex>();
	case EGameplayBoardKind::Free:
		return Allocate<PGameplayBoardFree>();
	case EGameplayBoardKind::None:
	default:
		return Allocate<PGameplayBoardNone>();
	}
}

// None ------------------------------------------------------------------------

EGameplayBoardKind PGameplayBoardNone::GetKind() const
{
	return EGameplayBoardKind::None;
}

int32 PGameplayBoardNone::Distance(const HGameplayCoord& a, const HGameplayCoord& b) const
{
	return 0;
}

void PGameplayBoardNone::Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const
{
}

bool PGameplayBoardNone::IsPassable(const HGameplayBoardState& board, const HGameplayCoord& coord) const
{
	return false;
}

// Square ----------------------------------------------------------------------

EGameplayBoardKind PGameplayBoardSquare::GetKind() const
{
	return EGameplayBoardKind::Square;
}

int32 PGameplayBoardSquare::Distance(const HGameplayCoord& a, const HGameplayCoord& b) const
{
	return absInt(a.X - b.X) + absInt(a.Y - b.Y);
}

void PGameplayBoardSquare::Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const
{
	const int32 dx[4] = { 1, 0, -1, 0 };
	const int32 dy[4] = { 0, 1, 0, -1 };
	for (int32 i = 0; i < 4; ++i)
	{
		HGameplayCoord next(coord.X + dx[i], coord.Y + dy[i]);
		if (board.InBounds(next) == true)
		{
			outNeighbors.push_back(next);
		}
	}
}

// Hex -------------------------------------------------------------------------

EGameplayBoardKind PGameplayBoardHex::GetKind() const
{
	return EGameplayBoardKind::Hex;
}

int32 PGameplayBoardHex::Distance(const HGameplayCoord& a, const HGameplayCoord& b) const
{
	// 축 좌표 (q, r). s = -q - r.
	int32 dq = a.X - b.X;
	int32 dr = a.Y - b.Y;
	int32 ds = (-a.X - a.Y) - (-b.X - b.Y);
	return maxInt(absInt(dq), maxInt(absInt(dr), absInt(ds)));
}

void PGameplayBoardHex::Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const
{
	const int32 dq[6] = { 1, 1, 0, -1, -1, 0 };
	const int32 dr[6] = { 0, -1, -1, 0, 1, 1 };
	for (int32 i = 0; i < 6; ++i)
	{
		HGameplayCoord next(coord.X + dq[i], coord.Y + dr[i]);
		if (board.InBounds(next) == true)
		{
			outNeighbors.push_back(next);
		}
	}
}

// Free ------------------------------------------------------------------------

EGameplayBoardKind PGameplayBoardFree::GetKind() const
{
	return EGameplayBoardKind::Free;
}

int32 PGameplayBoardFree::Distance(const HGameplayCoord& a, const HGameplayCoord& b) const
{
	return maxInt(absInt(a.X - b.X), absInt(a.Y - b.Y));
}

void PGameplayBoardFree::Neighbors(const HGameplayBoardState& board, const HGameplayCoord& coord, HList<HGameplayCoord>& outNeighbors) const
{
	const int32 dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
	const int32 dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	for (int32 i = 0; i < 8; ++i)
	{
		outNeighbors.push_back(HGameplayCoord(coord.X + dx[i], coord.Y + dy[i]));
	}
}

bool PGameplayBoardFree::IsPassable(const HGameplayBoardState& board, const HGameplayCoord& coord) const
{
	return board.OccupantAt(coord).IsValid() == false;
}
