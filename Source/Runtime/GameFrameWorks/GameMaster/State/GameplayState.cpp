#include "PCH/PCH.h"
#include "GameMaster/State/GameplayState.h"

HGameplayState::HGameplayState()
{
	SeedAll(0);
}

HGameplayState::HGameplayState(const HGameplayState& rhs)
{
	copyFrom(rhs);
}

HGameplayState::HGameplayState(HGameplayState&& rhs) noexcept
{
	moveFrom(rhs);
}

HGameplayState& HGameplayState::operator=(const HGameplayState& rhs)
{
	if (this != &rhs)
	{
		copyFrom(rhs);
	}
	return *this;
}

HGameplayState& HGameplayState::operator=(HGameplayState&& rhs) noexcept
{
	if (this != &rhs)
	{
		moveFrom(rhs);
	}
	return *this;
}

void HGameplayState::copyFrom(const HGameplayState& rhs)
{
	Entities = rhs.Entities;
	Zones    = rhs.Zones;
	Board    = rhs.Board;
	Turn     = rhs.Turn;
	Choice   = rhs.Choice;
	for (int32 i = 0; i < RandomStreamCount; ++i)
	{
		Random[i] = rhs.Random[i];
	}
	Sequence = rhs.Sequence;
	Seed     = rhs.Seed;

	Tables.clear();
	for (const HSTLUniquePtr<IGameplayComponentTable>& table : rhs.Tables)
	{
		Tables.push_back(table->Clone());
	}
}

void HGameplayState::moveFrom(HGameplayState& rhs) noexcept
{
	Entities = std::move(rhs.Entities);
	Zones    = std::move(rhs.Zones);
	Board    = std::move(rhs.Board);
	Turn     = std::move(rhs.Turn);
	Choice   = std::move(rhs.Choice);
	for (int32 i = 0; i < RandomStreamCount; ++i)
	{
		Random[i] = rhs.Random[i];
	}
	Sequence = rhs.Sequence;
	Seed     = rhs.Seed;
	Tables   = std::move(rhs.Tables);
}

void HGameplayState::SeedAll(uint64 seed)
{
	Seed = seed;
	for (int32 i = 0; i < RandomStreamCount; ++i)
	{
		Random[i].Seed(seed, (uint64)i + 1);
	}
}

HGameplayRandomStream& HGameplayState::Rng(EGameplayRandomStream stream)
{
	int32 index = (int32)stream;
	if (index < 0 || index >= RandomStreamCount)
	{
		index = (int32)EGameplayRandomStream::Misc0;
	}
	return Random[index];
}

HGameplayEntityId HGameplayState::CreateEntity()
{
	return Entities.Create();
}

bool HGameplayState::DestroyEntity(const HGameplayEntityId& id)
{
	if (Entities.IsAlive(id) == false)
	{
		return false;
	}

	for (HSTLUniquePtr<IGameplayComponentTable>& table : Tables)
	{
		table->RemoveEntity(id);
	}
	Zones.RemoveEverywhere(id);
	Board.ClearPosition(id);

	return Entities.Destroy(id);
}

bool HGameplayState::IsAlive(const HGameplayEntityId& id) const
{
	return Entities.IsAlive(id);
}

bool HGameplayState::SetBoardPosition(const HGameplayEntityId& id, const HGameplayCoord& coord)
{
	if (IsAlive(id) == false)
	{
		return false;
	}
	return Board.SetPosition(id, coord);
}

bool HGameplayState::MoveToZone(const HGameplayEntityId& id, const PName& zoneName)
{
	if (IsAlive(id) == false)
	{
		return false;
	}
	return Zones.MoveTo(id, zoneName);
}

void HGameplayState::CollectInputActors(HList<HGameplayEntityId>& outActors) const
{
	if (Turn.IsStarted() == false || Turn.IsFinished() == true)
	{
		return;
	}
	if (Choice.bPending == true)
	{
		outActors.push_back(Choice.Chooser);
		return;
	}
	// 죽은 행동자를 거르지 않는다. 기본 흐름에서 차례 중에 죽은 행동자도 EndTurn 으로 차례를 넘길 수 있어야 한다.
	for (const HGameplayEntityId& actor : Turn.Actors)
	{
		outActors.push_back(actor);
	}
}

bool HGameplayState::IsInputActor(const HGameplayEntityId& id) const
{
	if (id.IsValid() == false || Turn.IsStarted() == false || Turn.IsFinished() == true)
	{
		return false;
	}
	if (Choice.bPending == true)
	{
		return Choice.Chooser == id;
	}
	return Turn.IsActor(id);
}

HGameplayEntityId HGameplayState::FirstInputActor() const
{
	HList<HGameplayEntityId> actors;
	CollectInputActors(actors);
	if (actors.empty() == true)
	{
		return HGameplayEntityId::None();
	}
	return actors[0];
}

HGameplayZone& HGameplayState::Zone(const PName& name)
{
	return Zones.FindOrAdd(name);
}

const HGameplayZone* HGameplayState::FindZone(const PName& name) const
{
	return Zones.Find(name);
}

IGameplayComponentTable* HGameplayState::findTable(uint64 typeId) const
{
	for (const HSTLUniquePtr<IGameplayComponentTable>& table : Tables)
	{
		if (table->GetTypeId() == typeId)
		{
			return table.get();
		}
	}
	return nullptr;
}

PString HGameplayState::ToJsonString() const
{
	PJson json;
	json.AddMember("SchemaVersion", SchemaVersion);
	json.AddMember("State", *this);

	PString text;
	if (PJson::ToString(json, &text) == false)
	{
		return PString();
	}
	return text;
}

bool HGameplayState::FromJsonString(const PString& text)
{
	PJson json;
	if (PJson::ToObject(text, &json) == false)
	{
		return false;
	}

	uint32 version = 0;
	json.GetData("SchemaVersion", &version);
	if (version != SchemaVersion)
	{
		JG_LOG(GameMaster, ELogLevel::Warning, "HGameplayState: schema version %u != %u", version, SchemaVersion);
	}

	return json.GetData("State", this);
}

uint64 HGameplayState::Checksum() const
{
	PString text = ToJsonString();
	const HRawString& raw = text.GetRawString();

	// FNV-1a 64
	uint64 hash = 14695981039346656037ULL;
	for (char c : raw)
	{
		hash ^= (uint64)(uint8)c;
		hash *= 1099511628211ULL;
	}
	return hash;
}

void HGameplayState::WriteJson(PJsonData& json) const
{
	json.AddMember("Entities", Entities);
	json.AddMember("Zones", Zones);
	json.AddMember("Board", Board);
	json.AddMember("Turn", Turn);
	json.AddMember("Choice", Choice);

	HList<HGameplayRandomStream> random;
	for (int32 i = 0; i < RandomStreamCount; ++i)
	{
		random.push_back(Random[i]);
	}
	json.AddMember("Random", random);
	json.AddMember("Sequence", Sequence);
	json.AddMember("Seed", Seed);

	PJsonData tablesJson = json.CreateJsonData();
	for (const HSTLUniquePtr<IGameplayComponentTable>& table : Tables)
	{
		PJsonData tableJson = json.CreateJsonData();
		table->Write(tableJson);
		tablesJson.AddMember(table->GetTypeName().ToString(), tableJson);
	}
	json.AddMember("Tables", tablesJson);
}

void HGameplayState::ReadJson(const PJsonData& json)
{
	json.GetData("Entities", &Entities);
	json.GetData("Zones", &Zones);
	json.GetData("Board", &Board);
	json.GetData("Turn", &Turn);
	json.GetData("Choice", &Choice);

	HList<HGameplayRandomStream> random;
	if (json.GetData("Random", &random) == true)
	{
		int32 count = (int32)random.size();
		for (int32 i = 0; i < RandomStreamCount && i < count; ++i)
		{
			Random[i] = random[i];
		}
	}
	json.GetData("Sequence", &Sequence);
	json.GetData("Seed", &Seed);

	// 테이블은 등록된 것만 읽는다. 저장에는 있는데 등록이 없는 타입은 경고 후 무시.
	PJsonData tablesJson;
	if (json.FindMember("Tables", &tablesJson) == true)
	{
		for (HSTLUniquePtr<IGameplayComponentTable>& table : Tables)
		{
			PJsonData tableJson;
			if (tablesJson.FindMember(table->GetTypeName().ToString(), &tableJson) == true)
			{
				if (table->Read(tableJson) == false)
				{
					JG_LOG(GameMaster, ELogLevel::Warning, "HGameplayState: fail to read table %s", table->GetTypeName().ToString());
				}
			}
			else
			{
				table->Clear();
			}
		}
	}
}
