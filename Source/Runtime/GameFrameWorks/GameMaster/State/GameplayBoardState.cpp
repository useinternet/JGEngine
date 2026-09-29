#include "PCH/PCH.h"
#include "GameMaster/State/GameplayBoardState.h"

void HGameplayBoardState::Reset(EGameplayBoardKind kind, int32 width, int32 height)
{
	Kind   = kind;
	Width  = width;
	Height = height;

	Occupants.clear();
	Blocked.clear();
	Positions.clear();
	PositionGenerations.clear();

	if (IsGrid() == true && width > 0 && height > 0)
	{
		Occupants.resize((uint64)width * (uint64)height);
		Blocked.resize((uint64)width * (uint64)height, 0);
	}
	else
	{
		Width  = 0;
		Height = 0;
	}
}

bool HGameplayBoardState::IsGrid() const
{
	return Kind == EGameplayBoardKind::Square || Kind == EGameplayBoardKind::Hex;
}

bool HGameplayBoardState::InBounds(const HGameplayCoord& coord) const
{
	if (Kind == EGameplayBoardKind::None)
	{
		return false;
	}
	if (Kind == EGameplayBoardKind::Free)
	{
		return true;
	}
	return coord.X >= 0 && coord.Y >= 0 && coord.X < Width && coord.Y < Height;
}

int32 HGameplayBoardState::CellIndex(const HGameplayCoord& coord) const
{
	if (IsGrid() == false || InBounds(coord) == false)
	{
		return INDEX_NONE;
	}
	return coord.Y * Width + coord.X;
}

bool HGameplayBoardState::HasPosition(const HGameplayEntityId& id) const
{
	return FindPosition(id) != nullptr;
}

const HGameplayCoord* HGameplayBoardState::FindPosition(const HGameplayEntityId& id) const
{
	if (id.IsValid() == false)
	{
		return nullptr;
	}
	if (id.Index >= (uint32)PositionGenerations.size())
	{
		return nullptr;
	}
	if (PositionGenerations[id.Index] != id.Generation)
	{
		return nullptr;
	}
	return &Positions[id.Index];
}

bool HGameplayBoardState::SetPosition(const HGameplayEntityId& id, const HGameplayCoord& coord)
{
	if (id.IsValid() == false || Kind == EGameplayBoardKind::None)
	{
		return false;
	}
	if (InBounds(coord) == false)
	{
		return false;
	}

	// 격자: 목적지가 차 있거나 막혀 있으면 실패
	if (IsGrid() == true)
	{
		int32 cell = CellIndex(coord);
		if (Blocked[cell] != 0)
		{
			return false;
		}
		if (Occupants[cell].IsValid() == true && Occupants[cell] != id)
		{
			return false;
		}
	}

	ClearPosition(id);
	ensurePositionCapacity(id.Index + 1);
	Positions[id.Index]           = coord;
	PositionGenerations[id.Index] = id.Generation;

	if (IsGrid() == true)
	{
		Occupants[CellIndex(coord)] = id;
	}
	return true;
}

bool HGameplayBoardState::ClearPosition(const HGameplayEntityId& id)
{
	const HGameplayCoord* current = FindPosition(id);
	if (current == nullptr)
	{
		return false;
	}

	if (IsGrid() == true)
	{
		int32 cell = CellIndex(*current);
		if (cell != INDEX_NONE && Occupants[cell] == id)
		{
			Occupants[cell] = HGameplayEntityId::None();
		}
	}

	Positions[id.Index]           = HGameplayCoord();
	PositionGenerations[id.Index] = 0;
	return true;
}

HGameplayEntityId HGameplayBoardState::OccupantAt(const HGameplayCoord& coord) const
{
	if (IsGrid() == true)
	{
		int32 cell = CellIndex(coord);
		if (cell == INDEX_NONE)
		{
			return HGameplayEntityId::None();
		}
		return Occupants[cell];
	}

	// 자유 공간: 선형 탐색
	HGameplayEntityId found = HGameplayEntityId::None();
	EachPositioned([&](const HGameplayEntityId& id, const HGameplayCoord& position)
	{
		if (found.IsValid() == false && position == coord)
		{
			found = id;
		}
	});
	return found;
}

bool HGameplayBoardState::IsBlocked(const HGameplayCoord& coord) const
{
	int32 cell = CellIndex(coord);
	if (cell == INDEX_NONE)
	{
		return false;
	}
	return Blocked[cell] != 0;
}

void HGameplayBoardState::SetBlocked(const HGameplayCoord& coord, bool bBlocked)
{
	int32 cell = CellIndex(coord);
	if (cell == INDEX_NONE)
	{
		return;
	}
	Blocked[cell] = bBlocked ? 1 : 0;
}

void HGameplayBoardState::ensurePositionCapacity(uint32 capacity)
{
	if ((uint32)PositionGenerations.size() >= capacity)
	{
		return;
	}
	Positions.resize(capacity);
	PositionGenerations.resize(capacity, 0);
}

void HGameplayBoardState::WriteJson(PJsonData& json) const
{
	json.AddMember("Kind", (int32)Kind);
	json.AddMember("Width", Width);
	json.AddMember("Height", Height);
	json.AddMember("Occupants", Occupants);
	json.AddMember("Blocked", Blocked);
	json.AddMember("Positions", Positions);
	json.AddMember("PositionGenerations", PositionGenerations);
}

void HGameplayBoardState::ReadJson(const PJsonData& json)
{
	Occupants.clear();
	Blocked.clear();
	Positions.clear();
	PositionGenerations.clear();

	int32 kind = (int32)EGameplayBoardKind::None;
	json.GetData("Kind", &kind);
	json.GetData("Width", &Width);
	json.GetData("Height", &Height);
	json.GetData("Occupants", &Occupants);
	json.GetData("Blocked", &Blocked);
	json.GetData("Positions", &Positions);
	json.GetData("PositionGenerations", &PositionGenerations);

	Kind = (EGameplayBoardKind)kind;
}
