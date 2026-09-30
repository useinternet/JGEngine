#include "PCH/PCH.h"
#include "Network/Messages/GameplayPlayerSlot.h"

bool HGameplayPlayerSlot::Controls(int32 controller) const
{
	if (bAllControllers == true)
	{
		return true;
	}
	for (int32 owned : Controllers)
	{
		if (owned == controller)
		{
			return true;
		}
	}
	return false;
}

bool HGameplayPlayerSlot::IsFree() const
{
	return Token.Empty() == true;
}

void HGameplayPlayerSlot::WriteJson(PJsonData& json) const
{
	json.AddMember("Slot", Slot);
	json.AddMember("Controllers", Controllers);
	json.AddMember("AllControllers", bAllControllers);
	json.AddMember("Connected", bConnected);
	json.AddMember("Name", Name);
}

void HGameplayPlayerSlot::ReadJson(const PJsonData& json)
{
	Controllers.clear();

	json.GetData("Slot", &Slot);
	json.GetData("Controllers", &Controllers);
	json.GetData("AllControllers", &bAllControllers);
	json.GetData("Connected", &bConnected);
	json.GetData("Name", &Name);
}
