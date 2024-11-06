#include "PCH/PCH.h"
#include "GUIBackend.h"

void PGUIBackend::Initialize()
{
	
}

void PGUIBackend::Shutdown()
{
	OnGUI.RemoveAll();
	OnMainMenuGUI.RemoveAll();
}

void PGUIBackend::GenerateGUI()
{
	OnGUI.BroadCast();
}

void PGUIBackend::GenerateMainMenuGUI()
{
	OnMainMenuGUI.BroadCast();
}
