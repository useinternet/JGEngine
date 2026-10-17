#include "PCH/PCH.h"
#include "GameMaster/State/GameplayState.h"

namespace
{
	// FNV-1a 64. 상태 체크섬 · 되감기 지점 해시가 같은 함수를 쓴다.
	uint64 hashGameplayText(const PString& text)
	{
		uint64 hash = 14695981039346656037ULL;
		for (char c : text.GetRawString())
		{
			hash ^= (uint64)(uint8)c;
			hash *= 1099511628211ULL;
		}
		return hash;
	}
}

// writeState 를 "State" 멤버로 바로 쓰기 위한 감싸개. 읽기는 HGameplayState::ReadJson 이 한다.
struct HGameplayState::HStateJsonWriter : public IJsonable
{
	const HGameplayState&   State;
	const ERewindPointJson  Points;

	HStateJsonWriter(const HGameplayState& state, ERewindPointJson points)
		: State(state)
		, Points(points)
	{
	}

protected:
	virtual void WriteJson(PJsonData& json) const override
	{
		State.writeState(json, Points);
	}
	virtual void ReadJson(const PJsonData& json) override
	{
	}
};

void HGameplayRewindPoint::WriteJson(PJsonData& json) const
{
	json.AddMember("Name", Name);
	json.AddMember("State", StateText);
}

void HGameplayRewindPoint::ReadJson(const PJsonData& json)
{
	Name      = PName();
	StateText = PString();
	json.GetData("Name", &Name);
	json.GetData("State", &StateText);
	Hash = hashGameplayText(StateText);
}

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
	Sequence     = rhs.Sequence;
	Seed         = rhs.Seed;
	RewindPoints = rhs.RewindPoints;

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
	Sequence     = rhs.Sequence;
	Seed         = rhs.Seed;
	RewindPoints = std::move(rhs.RewindPoints);
	Tables       = std::move(rhs.Tables);
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

const HGameplayRewindPoint* HGameplayState::FindRewindPoint(const PName& name) const
{
	for (const HGameplayRewindPoint& point : RewindPoints)
	{
		if (point.Name == name)
		{
			return &point;
		}
	}
	return nullptr;
}

void HGameplayState::MarkRewindPoint(const PName& name)
{
	HGameplayRewindPoint point;
	point.Name      = name;
	point.StateText = toJsonText(ERewindPointJson::None);
	point.Hash      = hashGameplayText(point.StateText);

	for (HGameplayRewindPoint& existing : RewindPoints)
	{
		if (existing.Name == name)
		{
			existing = std::move(point);
			return;
		}
	}
	RewindPoints.push_back(std::move(point));
}

bool HGameplayState::ClearRewindPoint(const PName& name)
{
	for (auto iter = RewindPoints.begin(); iter != RewindPoints.end(); ++iter)
	{
		if (iter->Name == name)
		{
			RewindPoints.erase(iter);
			return true;
		}
	}
	return false;
}

bool HGameplayState::RewindTo(const PName& name, EGameplayRewindRandom random)
{
	const HGameplayRewindPoint* point = FindRewindPoint(name);
	if (point == nullptr)
	{
		return false;
	}

	// 먼저 읽어 본다. 실패하면 아무것도 바꾸지 않는다.
	PJson json;
	if (PJson::ToObject(point->StateText, &json) == false)
	{
		JG_LOG(GameMaster, ELogLevel::Error, "HGameplayState: rewind point %s has invalid state text", name.ToString());
		return false;
	}
	PJsonData stateJson;
	if (json.FindMember("State", &stateJson) == false)
	{
		JG_LOG(GameMaster, ELogLevel::Error, "HGameplayState: rewind point %s has no state", name.ToString());
		return false;
	}

	// 되돌리지 않는 것: 명령 순번(참가자가 seq 로 명령을 맞춘다) · 시드 · 지점 목록 · 지금까지 낸 엔티티 세대 · (KeepCurrent 면) 난수
	const uint32 sequence = Sequence;
	const uint64 seed     = Seed;
	HList<HGameplayRewindPoint> points = std::move(RewindPoints);
	HGameplayEntityRegistry     before = Entities;
	HGameplayRandomStream       current[RandomStreamCount];
	for (int32 i = 0; i < RandomStreamCount; ++i)
	{
		current[i] = Random[i];
	}

	stateJson.GetData(this);

	Sequence     = sequence;
	Seed         = seed;
	RewindPoints = std::move(points);
	if (random == EGameplayRewindRandom::KeepCurrent)
	{
		for (int32 i = 0; i < RandomStreamCount; ++i)
		{
			Random[i] = current[i];
		}
	}
	Entities.KeepIssuedGenerations(before);
	return true;
}

PString HGameplayState::ToJsonString() const
{
	return toJsonText(ERewindPointJson::Full);
}

PString HGameplayState::toJsonText(ERewindPointJson points) const
{
	PJson json;
	json.AddMember("SchemaVersion", SchemaVersion);
	json.AddMember("State", HStateJsonWriter(*this, points));

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
	if (version < MinReadableSchemaVersion || version > SchemaVersion)
	{
		JG_LOG(GameMaster, ELogLevel::Warning, "HGameplayState: schema version %u is not in %u..%u", version, MinReadableSchemaVersion, SchemaVersion);
	}

	return json.GetData("State", this);
}

uint64 HGameplayState::Checksum() const
{
	// 되감기 지점은 표시한 뒤 바뀌지 않는다. 사본 전체 대신 지점 해시만 섞어 지점이 있어도 체크섬 비용이 그대로다.
	return hashGameplayText(toJsonText(ERewindPointJson::HashOnly));
}

void HGameplayState::WriteJson(PJsonData& json) const
{
	writeState(json, ERewindPointJson::Full);
}

void HGameplayState::writeState(PJsonData& json, ERewindPointJson points) const
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

	// 지점이 없으면 쓰지 않는다 — 되감기를 쓰지 않는 게임의 상태 JSON 은 그대로다.
	if (RewindPoints.empty() == true || points == ERewindPointJson::None)
	{
		return;
	}
	if (points == ERewindPointJson::Full)
	{
		json.AddMember("RewindPoints", RewindPoints);
		return;
	}
	HList<PName>  names;
	HList<uint64> hashes;
	for (const HGameplayRewindPoint& point : RewindPoints)
	{
		names.push_back(point.Name);
		hashes.push_back(point.Hash);
	}
	json.AddMember("RewindPointNames", names);
	json.AddMember("RewindPointHashes", hashes);
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

	RewindPoints.clear();
	json.GetData("RewindPoints", &RewindPoints);

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
