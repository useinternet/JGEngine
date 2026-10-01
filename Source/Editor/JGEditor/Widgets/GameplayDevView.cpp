#include "PCH/PCH.h"
#include "Widgets/GameplayDevView.h"
#include "Core/GameInstance.h"
#include "Core/World.h"
#include "Actors/GameMasterActor.h"
#include "Actors/GameplayControllerActor.h"
#include "GameMaster/GameMaster.h"
#include "GUI.h"

namespace
{
	constexpr int32 DevViewMaxLogLines = 300;
}

// DevView 가 GameMaster 에 붙이는 관찰자. 위젯과 따로 둔다 (한 객체에 IMemoryObject 뿌리를 둘 두지 않는다).
class PGameplayDevViewObserver : public IGameplayObserver
{
	JGGameplayDevView* _devView = nullptr;

public:
	PGameplayDevViewObserver() = default;
	explicit PGameplayDevViewObserver(JGGameplayDevView* devView)
		: _devView(devView)
	{
	}
	virtual ~PGameplayDevViewObserver() = default;

	void Clear()
	{
		_devView = nullptr;
	}

	virtual void OnGameplayEvents(const HGameplayState& state, const HList<HGameplayEvent>& events) override
	{
		if (_devView != nullptr)
		{
			_devView->onEvents(events);
		}
	}

	virtual void OnGameplayStateReplaced(const HGameplayState& state) override
	{
		if (_devView != nullptr)
		{
			_devView->onStateReplaced();
		}
	}
};

PString JGGameplayDevView::GetTitleName() const
{
	return "Gameplay DevView";
}

void JGGameplayDevView::OnInitialize()
{
	_observer = Allocate<PGameplayDevViewObserver>(this);
}

void JGGameplayDevView::OnShutdown()
{
	unwatch();
	if (_observer != nullptr)
	{
		_observer->Clear();
		_observer = nullptr;
	}
}

void JGGameplayDevView::OnLayout(const HWidgetLayout& InLayout)
{
	_contentSize = InLayout.ContentSize;
}

void JGGameplayDevView::OnGenerateGUI()
{
	if (JGGameInstance::HasInstance() == false)
	{
		unwatch();
		HGUI::Text("GameFrameWorks is not running");
		return;
	}

	PSharedPtr<PWorld> world = JGGameInstance::Get().GetWorld();
	if (world == nullptr)
	{
		unwatch();
		HGUI::Text("No world is loaded");
		return;
	}

	PSharedPtr<JGGameMasterActor> gameMasterActor = findGameMasterActor(world);
	if (gameMasterActor == nullptr)
	{
		unwatch();
		HGUI::Text(PString::Format("World %s has no JGGameMasterActor", world->GetName().ToString()));
		drawPicks(world);
		return;
	}

	PSharedPtr<PGameMaster> gameMaster = gameMasterActor->GetGameMaster();
	if (gameMaster == nullptr)
	{
		unwatch();
		HGUI::Text(PString::Format("%s has no GameMaster", gameMasterActor->GetName().ToString()));
		drawPicks(world);
		return;
	}

	// 대상이 바뀌었으면(월드 재로드 · GameMaster 교체) 관찰자를 옮긴다.
	if (_gameMasterActor.Pin() != gameMasterActor || _gameMaster.Pin() != gameMaster)
	{
		watch(gameMasterActor);
	}

	if (_cachedRevision != _revision || _cachedEntity != _selectedEntity)
	{
		refreshCache(*gameMaster);
	}

	drawSummary(*gameMaster);
	drawEntities();
	drawEventLog();
	drawPicks(world);
}

PSharedPtr<JGGameMasterActor> JGGameplayDevView::findGameMasterActor(PSharedPtr<PWorld> world) const
{
	HList<PSharedPtr<JGGameMasterActor>> gameMasterActors;
	world->FindActors<JGGameMasterActor>(gameMasterActors);
	if (gameMasterActors.empty() == true)
	{
		return nullptr;
	}
	return gameMasterActors[0];
}

void JGGameplayDevView::watch(PSharedPtr<JGGameMasterActor> gameMasterActor)
{
	unwatch();

	_gameMasterActor = gameMasterActor;
	PSharedPtr<PGameMaster> gameMaster = gameMasterActor->GetGameMaster();
	_gameMaster = gameMaster;
	if (gameMaster != nullptr && _observer != nullptr)
	{
		gameMaster->AddObserver(_observer);
	}

	_selectedEntity = HGameplayEntityId::None();
	_eventLog.clear();
	appendLog(PString::Format("-- watching %s", gameMasterActor->GetName().ToString()));
	++_revision;
}

void JGGameplayDevView::unwatch()
{
	PSharedPtr<PGameMaster> gameMaster = _gameMaster.Pin();
	if (gameMaster != nullptr && _observer != nullptr)
	{
		gameMaster->RemoveObserver(_observer);
	}
	_gameMaster.Reset();
	_gameMasterActor.Reset();
}

void JGGameplayDevView::refreshCache(const PGameMaster& gameMaster)
{
	const HGameplayState& state = gameMaster.GetState();

	_cachedRevision = _revision;
	_cachedEntity   = _selectedEntity;
	_cachedChecksum = gameMaster.Checksum();

	// 엔티티 목록: "E3:1  Health, Position" (가진 컴포넌트 테이블 이름)
	_cachedEntityIds.clear();
	_cachedEntityLabels.clear();
	state.Entities.Each([&](const HGameplayEntityId& id)
	{
		PString tables;
		for (const HSTLUniquePtr<IGameplayComponentTable>& table : state.Tables)
		{
			if (table->HasEntity(id) == false)
			{
				continue;
			}
			if (tables.Empty() == false)
			{
				tables += ", ";
			}
			tables += table->GetTypeName().ToString();
		}
		_cachedEntityIds.push_back(id);
		_cachedEntityLabels.push_back(PString::Format("%s  %s", id.ToString(), tables));
	});

	// 고른 엔티티의 영역 · 보드 위치 · 컴포넌트 값
	_cachedComponents = PString();
	if (_selectedEntity.IsValid() == false || state.IsAlive(_selectedEntity) == false)
	{
		return;
	}

	PName zoneName;
	if (state.Zones.FindZoneOf(_selectedEntity, &zoneName) == true)
	{
		_cachedComponents += PString::Format("Zone: %s\n", zoneName.ToString());
	}
	const HGameplayCoord* position = state.Board.FindPosition(_selectedEntity);
	if (position != nullptr)
	{
		_cachedComponents += PString::Format("Board: %s\n", position->ToString());
	}

	for (const HSTLUniquePtr<IGameplayComponentTable>& table : state.Tables)
	{
		PJson json;
		if (table->WriteEntity(_selectedEntity, json) == false)
		{
			continue;
		}
		PString text;
		PJson::ToString(json, &text);
		_cachedComponents += PString::Format("[%s]\n%s\n", table->GetTypeName().ToString(), text);
	}
}

void JGGameplayDevView::appendLog(const PString& line)
{
	_eventLog.push_back(line);
	while ((int32)_eventLog.size() > DevViewMaxLogLines)
	{
		_eventLog.pop_front();
	}
}

void JGGameplayDevView::onEvents(const HList<HGameplayEvent>& events)
{
	for (const HGameplayEvent& event : events)
	{
		appendLog(event.ToString());
	}
	++_revision;
}

void JGGameplayDevView::onStateReplaced()
{
	appendLog("-- state replaced (undo / load / replay)");
	++_revision;
}

void JGGameplayDevView::drawSummary(PGameMaster& gameMaster)
{
	const HGameplayState& state = gameMaster.GetState();
	const HGameplayTurnState& turn = state.Turn;

	// 흐름 공용 칸(단계 이름 · 입력 행동자)을 보인다. 어떤 흐름이든 같은 줄이고, 기본 흐름이면 단계 이름이 페이즈 이름이다.
	HList<HGameplayEntityId> inputActors;
	state.CollectInputActors(inputActors);
	PString inputText;
	for (const HGameplayEntityId& actor : inputActors)
	{
		if (inputText.Empty() == false)
		{
			inputText += ", ";
		}
		inputText += actor.ToString();
	}
	if (inputText.Empty() == true)
	{
		inputText = "-";
	}
	const PString stepText = (turn.Step != NAME_NONE) ? turn.Step.ToString() : PString(GetGameplayPhaseName(turn.Phase));

	HGUI::Text(PString::Format("Sequence %u | Round %d | Step %s | Input %s | Turns %d | Entities %u",
		state.Sequence, turn.Round, stepText, inputText, turn.TurnCount, state.Entities.Count()));

	if (state.Choice.bPending == true)
	{
		HGUI::Text(PString::Format("Pending choice %s by %s (%d candidates, pick %d..%d)",
			state.Choice.Kind.ToString(), state.Choice.Chooser.ToString(), (int32)state.Choice.Candidates.size(), state.Choice.Min, state.Choice.Max));
	}

	// 버튼을 줄 맨 앞에 둔다 (뒤의 숫자 길이에 따라 자리가 흔들리지 않게).
	if (HGUI::Button("Undo") == true)
	{
		if (gameMaster.IsUndoEnabled() == true && gameMaster.UndoCount() > 0)
		{
			// 상태 교체 알림으로 GameMasterActor 가 바인딩을 다시 만들고, 이 창은 onStateReplaced 로 캐시를 다시 만든다.
			gameMaster.Undo();
		}
	}
	HGUI::SameLine();
	HGUI::Text(PString::Format("Undo %d%s | Checksum %llu", gameMaster.UndoCount(),
		gameMaster.IsUndoEnabled() ? PString("") : PString(" (disabled by session)"), _cachedChecksum));
	HGUI::Separator();
}

void JGGameplayDevView::drawEntities()
{
	if (HGUI::CollapsingHeader(PString::Format("Entities (%d)###Entities", (int32)_cachedEntityIds.size())) == false)
	{
		return;
	}

	const float32 listHeight = HMath::Max(_contentSize.y * 0.25f, 80.0f);
	HGUI::BeginChild("##DevViewEntityList", HVector2(0.0f, listHeight));
	for (uint64 i = 0; i < _cachedEntityIds.size(); ++i)
	{
		const bool bSelected = _cachedEntityIds[i] == _selectedEntity;
		if (HGUI::Selectable(_cachedEntityLabels[i], bSelected) == true)
		{
			_selectedEntity = bSelected ? HGameplayEntityId::None() : _cachedEntityIds[i];
		}
	}
	HGUI::EndChild();

	if (_cachedComponents.Empty() == false)
	{
		const float32 valueHeight = HMath::Max(_contentSize.y * 0.3f, 100.0f);
		HGUI::Text(PString::Format("Selected %s", _selectedEntity.ToString()));
		HGUI::BeginChild("##DevViewEntityValues", HVector2(0.0f, valueHeight));
		HGUI::Text(_cachedComponents);
		HGUI::EndChild();
	}
}

void JGGameplayDevView::drawEventLog()
{
	if (HGUI::CollapsingHeader(PString::Format("Event log (%d)###EventLog", (int32)_eventLog.size())) == false)
	{
		return;
	}

	const float32 logHeight = HMath::Max(_contentSize.y * 0.25f, 80.0f);
	HGUI::BeginChild("##DevViewEventLog", HVector2(0.0f, logHeight));
	for (const PString& line : _eventLog)
	{
		HGUI::Text(line);
	}
	HGUI::EndChild(true);
}

void JGGameplayDevView::drawPicks(PSharedPtr<PWorld> world)
{
	if (HGUI::CollapsingHeader("Last pick") == false)
	{
		return;
	}

	HList<PSharedPtr<JGGameplayControllerActor>> controllers;
	world->FindActors<JGGameplayControllerActor>(controllers);
	if (controllers.empty() == true)
	{
		HGUI::Text("No JGGameplayControllerActor in the world");
		return;
	}

	for (const PSharedPtr<JGGameplayControllerActor>& controller : controllers)
	{
		if (controller->HasLastPick() == false)
		{
			HGUI::Text(PString::Format("%s: no click yet", controller->GetName().ToString()));
			continue;
		}
		HGUI::Text(PString::Format("%s: %s", controller->GetName().ToString(), controller->GetLastPick().ToString()));
	}
}
