#pragma once
#include "Core.h"
#include "GUIDefines.h"

JG_DECLARE_DELEGATE(HMainMenuAction);
JG_DECLARE_DELEGATE_RET(HMainMenuCanAction, bool);
JG_DECLARE_DELEGATE_RET(HMainMenuVisibility, bool);

struct GUI_API HMainMenuItem
{
	PString MenuPath;
	HMainMenuAction		Action;
	HMainMenuCanAction	CanAction;
	HMainMenuVisibility Visibility;

	HMainMenuItem() = default;
	HMainMenuItem(const PString& InMenuPath, HMainMenuAction InAction, HMainMenuCanAction InCanAction, HMainMenuVisibility InVisibility)
	{
		MenuPath = InMenuPath;
		Action = InAction;
		CanAction = InCanAction;
		Visibility = InVisibility;
	}

	HMainMenuItem(const PString& InMenuPath, HMainMenuAction InAction, HMainMenuCanAction InCanAction)
	{
		MenuPath = InMenuPath;
		Action = InAction;
		CanAction = InCanAction;
	}

	HMainMenuItem(const PString& InMenuPath, HMainMenuAction InAction)
	{
		MenuPath = InMenuPath;
		Action = InAction;
	}
};

struct HMainMenuNode
{
	uint64 Index = -1;
	PString Name;
	uint64 ParentIndex = -1;
	HList<uint64> ChildIndexes;

	HMainMenuItem Item;
};

struct HMainMenuTree
{
	HList<HMainMenuNode> MenuNodes;
	HHashMap<PName, uint64> MenuIndexesByName;

public:
	void AddMainMenuItem(const HMainMenuItem& InItem);
	void GenerateMainMenuGUI();

private:
	void GenerateMainMenuGUIInternal(const HMainMenuNode& InNode);
};