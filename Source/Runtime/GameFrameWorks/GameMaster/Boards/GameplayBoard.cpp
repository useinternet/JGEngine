#include "PCH/PCH.h"
#include "GameMaster/Boards/GameplayBoard.h"
#include <cmath>

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

	// 두 칸의 정해진 순서(Y 먼저, 같으면 X). 격자 선은 늘 앞 칸에서 긋고 반대 방향이면 뒤집는다 — 어느 쪽에서 물어도 같은 칸을 지난다(대칭).
	bool isCanonicalOrder(const HGameplayCoord& a, const HGameplayCoord& b)
	{
		if (a.Y != b.Y)
		{
			return a.Y < b.Y;
		}
		return a.X <= b.X;
	}

	void reverseCells(HList<HGameplayCoord>& cells)
	{
		for (uint64 i = 0, j = cells.size(); i + 1 < j; ++i, --j)
		{
			const HGameplayCoord temp = cells[i];
			cells[i]     = cells[j - 1];
			cells[j - 1] = temp;
		}
	}

	// 큐브 좌표 반올림: 세 값을 각각 반올림하고, 가장 많이 바뀐 값을 나머지 둘로 다시 맞춘다(q + r + s = 0). 축 좌표(q, r)로 돌려준다.
	HGameplayCoord roundCube(double q, double r, double s)
	{
		double roundedQ = std::round(q);
		double roundedR = std::round(r);
		const double roundedS = std::round(s);

		const double diffQ = std::fabs(roundedQ - q);
		const double diffR = std::fabs(roundedR - r);
		const double diffS = std::fabs(roundedS - s);
		if (diffQ > diffR && diffQ > diffS)
		{
			roundedQ = -roundedR - roundedS;
		}
		else if (diffR > diffS)
		{
			roundedR = -roundedQ - roundedS;
		}
		return HGameplayCoord((int32)roundedQ, (int32)roundedR);
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

void IGameplayBoard::TraceLine(const HGameplayCoord& from, const HGameplayCoord& to, HList<HGameplayCoord>& outCells) const
{
	// 격자가 아닌 보드에는 격자 선이 없다.
	outCells.clear();
}

bool IGameplayBoard::HasLineOfSight(const HGameplayBoardState& board, const HGameplayCoord& from, const HGameplayCoord& to) const
{
	if (board.InBounds(from) == false || board.InBounds(to) == false)
	{
		return false;
	}

	HList<HGameplayCoord> line;
	TraceLine(from, to, line);

	// 사이 칸(양 끝 제외)만 본다: 막힌 칸 위에 서 있거나 막힌 칸 자체를 보는 것은 된다. 보드 밖을 지나면 막힌 것으로 친다.
	for (uint64 i = 1; i + 1 < line.size(); ++i)
	{
		if (board.InBounds(line[i]) == false || board.IsBlocked(line[i]) == true)
		{
			return false;
		}
	}
	return true;
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

void PGameplayBoardSquare::TraceLine(const HGameplayCoord& from, const HGameplayCoord& to, HList<HGameplayCoord>& outCells) const
{
	outCells.clear();
	const bool           bForward = isCanonicalOrder(from, to);
	const HGameplayCoord start    = bForward ? from : to;
	const HGameplayCoord end      = bForward ? to : from;

	// 브레젠험(모든 방향, 정수 연산). 칸 수 = max(|dx|, |dy|) + 1.
	int32       x  = start.X;
	int32       y  = start.Y;
	const int32 dx = absInt(end.X - start.X);
	const int32 dy = -absInt(end.Y - start.Y);
	const int32 sx = (start.X < end.X) ? 1 : -1;
	const int32 sy = (start.Y < end.Y) ? 1 : -1;
	int32       error = dx + dy;
	for (;;)
	{
		outCells.push_back(HGameplayCoord(x, y));
		if (x == end.X && y == end.Y)
		{
			break;
		}
		const int32 doubled = 2 * error;
		if (doubled >= dy)
		{
			error += dy;
			x     += sx;
		}
		if (doubled <= dx)
		{
			error += dx;
			y     += sy;
		}
	}

	if (bForward == false)
	{
		reverseCells(outCells);
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

void PGameplayBoardHex::TraceLine(const HGameplayCoord& from, const HGameplayCoord& to, HList<HGameplayCoord>& outCells) const
{
	outCells.clear();
	const bool           bForward = isCanonicalOrder(from, to);
	const HGameplayCoord start    = bForward ? from : to;
	const HGameplayCoord end      = bForward ? to : from;

	const int32 steps = Distance(start, end);
	if (steps == 0)
	{
		outCells.push_back(start);
		return;
	}

	// 큐브 좌표(q, r, s = -q - r) 선형 보간 → 반올림. 선이 두 칸의 경계를 정확히 지나면(칸 모서리) 한쪽으로 정하도록 아주 조금 민다.
	// 미는 방향이 늘 같고 긋는 방향도 정해져 있어 결과가 결정론 · 대칭이다. 칸 수 = 거리 + 1, 이웃끼리 이어진다.
	const double nudge  = 1e-6;
	const double startQ = (double)start.X + nudge;
	const double startR = (double)start.Y + nudge;
	const double startS = (double)(-start.X - start.Y) - 2.0 * nudge;
	const double endQ   = (double)end.X + nudge;
	const double endR   = (double)end.Y + nudge;
	const double endS   = (double)(-end.X - end.Y) - 2.0 * nudge;
	for (int32 i = 0; i <= steps; ++i)
	{
		const double t = (double)i / (double)steps;
		outCells.push_back(roundCube(startQ + (endQ - startQ) * t, startR + (endR - startR) * t, startS + (endS - startS) * t));
	}

	if (bForward == false)
	{
		reverseCells(outCells);
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
