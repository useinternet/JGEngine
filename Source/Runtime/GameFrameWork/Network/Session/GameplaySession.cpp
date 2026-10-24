#include "PCH/PCH.h"
#include "Network/Session/GameplaySession.h"

EGameplayNetMode PGameplaySession::GetMode() const
{
	return _mode;
}

EGameplaySessionState PGameplaySession::GetState() const
{
	return _state;
}

bool PGameplaySession::IsAuthority() const
{
	return _mode != EGameplayNetMode::Client;
}

const PString& PGameplaySession::GetCloseReason() const
{
	return _closeReason;
}

void PGameplaySession::BindGameMaster(PSharedPtr<PGameMaster> gameMaster)
{
	if (_gameMaster != nullptr)
	{
		UnbindGameMaster();
	}

	_gameMaster = gameMaster;
	if (_gameMaster == nullptr)
	{
		return;
	}

	if (_mode != EGameplayNetMode::Standalone)
	{
		_gameMaster->SetUndoEnabled(false);
	}
	onBound();
}

void PGameplaySession::UnbindGameMaster()
{
	if (_gameMaster == nullptr)
	{
		return;
	}

	_gameMaster->SetUndoEnabled(true);
	_gameMaster.Reset();
	onUnbound();
}

PSharedPtr<PGameMaster> PGameplaySession::GetGameMaster() const
{
	return _gameMaster;
}

bool PGameplaySession::Undo()
{
	if (_mode != EGameplayNetMode::Standalone)
	{
		JG_LOG(Network, ELogLevel::Warning, "PGameplaySession::Undo: not available in a network session");
		return false;
	}
	if (_gameMaster == nullptr)
	{
		return false;
	}
	return _gameMaster->Undo();
}

const HList<HGameplayPlayerSlot>& PGameplaySession::GetSlots() const
{
	return _slots;
}

const HGameplayPlayerSlot* PGameplaySession::FindSlot(int32 slot) const
{
	for (const HGameplayPlayerSlot& entry : _slots)
	{
		if (entry.Slot == slot)
		{
			return &entry;
		}
	}
	return nullptr;
}

int32 PGameplaySession::GetLocalSlot() const
{
	return _localSlot;
}

bool PGameplaySession::IsLocalReady() const
{
	const HGameplayPlayerSlot* local = FindSlot(_localSlot);
	return local != nullptr && local->bReady == true;
}

const PString* PGameplaySession::FindSessionData(const PName& key) const
{
	for (const HGameplaySessionDataEntry& entry : _sessionData)
	{
		if (entry.Key == key)
		{
			return &entry.Value;
		}
	}
	return nullptr;
}

const HList<HGameplaySessionDataEntry>& PGameplaySession::GetSessionData() const
{
	return _sessionData;
}

bool PGameplaySession::IsLocallyControlled(const HGameplayEntityId& actor) const
{
	const HGameplayPlayerSlot* local = FindSlot(_localSlot);
	if (local == nullptr)
	{
		return false;
	}
	return slotControls(*local, actor);
}

HGameplayEntityId PGameplaySession::GetInputActor() const
{
	if (_gameMaster == nullptr)
	{
		return HGameplayEntityId::None();
	}
	return _gameMaster->GetState().FirstInputActor();
}

bool PGameplaySession::slotControls(const HGameplayPlayerSlot& slot, const HGameplayEntityId& actor) const
{
	if (_gameMaster == nullptr)
	{
		return false;
	}

	int32 controller = _gameMaster->TeamOfActor(actor);
	if (_gameMaster->FindAgent(controller) != nullptr)
	{
		return false;
	}
	return slot.Controls(controller);
}

HGameplayPlayerSlot* PGameplaySession::findSlotMutable(int32 slot)
{
	for (HGameplayPlayerSlot& entry : _slots)
	{
		if (entry.Slot == slot)
		{
			return &entry;
		}
	}
	return nullptr;
}

void PGameplaySession::setClosed(const PString& reason)
{
	if (_state == EGameplaySessionState::Closed)
	{
		return;
	}
	_state       = EGameplaySessionState::Closed;
	_closeReason = reason;
	JG_LOG(Network, ELogLevel::Info, "PGameplaySession: closed (%s)", reason);
	OnClosed.BroadCast(reason);
}

bool PGameplaySession::putSessionData(const PName& key, const PString& value)
{
	for (HGameplaySessionDataEntry& entry : _sessionData)
	{
		if (entry.Key != key)
		{
			continue;
		}
		if (entry.Value == value)
		{
			return false;
		}
		entry.Value = value;
		return true;
	}

	HGameplaySessionDataEntry entry;
	entry.Key   = key;
	entry.Value = value;
	_sessionData.push_back(entry);
	return true;
}

bool PGameplaySession::dropSessionData(const PName& key)
{
	for (auto iter = _sessionData.begin(); iter != _sessionData.end(); ++iter)
	{
		if ((*iter).Key == key)
		{
			_sessionData.erase(iter);
			return true;
		}
	}
	return false;
}

HList<PName> PGameplaySession::replaceSessionData(const HList<HGameplaySessionDataEntry>& entries)
{
	HList<PName> changed;
	for (const HGameplaySessionDataEntry& entry : entries)
	{
		if (entry.Key == NAME_NONE)
		{
			continue;
		}
		const PString* before = FindSessionData(entry.Key);
		if (before == nullptr || *before != entry.Value)
		{
			changed.push_back(entry.Key);
		}
	}
	for (const HGameplaySessionDataEntry& entry : _sessionData)
	{
		bool bKept = false;
		for (const HGameplaySessionDataEntry& next : entries)
		{
			if (next.Key == entry.Key)
			{
				bKept = true;
				break;
			}
		}
		if (bKept == false)
		{
			changed.push_back(entry.Key);
		}
	}

	_sessionData.clear();
	for (const HGameplaySessionDataEntry& entry : entries)
	{
		if (entry.Key != NAME_NONE)
		{
			_sessionData.push_back(entry);
		}
	}
	return changed;
}

void PGameplaySession::notifySessionData(const HList<PName>& keys)
{
	for (const PName& key : keys)
	{
		OnSessionDataChanged.BroadCast(key);
	}
}
