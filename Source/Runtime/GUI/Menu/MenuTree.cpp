#include "PCH/PCH.h"
#include "MenuTree.h"
#include "Imgui/imgui.h"

void HMainMenuTree::AddMainMenuItem(const HMainMenuItem& InItem)
{
	if (MenuNodes.empty())
	{
		HMainMenuNode RootNode;
		RootNode.Index = 0;
		RootNode.Name = "Root";
		RootNode.ParentIndex = -1;
		MenuNodes.push_back(RootNode);
	}

	HList<PString> MenuTokens = InItem.MenuPath.Split('/');
	uint64 NumMenuTokens = MenuTokens.size();
	uint64 ParentMenuNodeIndex = 0;
	for (uint64 i = 0; i < NumMenuTokens; ++i)
	{
		if (MenuIndexesByName.find(MenuTokens[i]) == MenuIndexesByName.end())
		{
			HMainMenuNode Node;
			Node.Index = MenuNodes.size();
			Node.Name  = MenuTokens[i];
			Node.ParentIndex = ParentMenuNodeIndex;
			if (i == NumMenuTokens - 1)
			{
				Node.Item = InItem;
			}

			HMainMenuNode& ParentNode = MenuNodes[ParentMenuNodeIndex];
			ParentNode.ChildIndexes.push_back(Node.Index);
			
			MenuNodes.push_back(Node);
			MenuIndexesByName.emplace(MenuTokens[i], Node.Index);
		}

		ParentMenuNodeIndex = MenuIndexesByName[MenuTokens[i]];
	}
}

void HMainMenuTree::GenerateMainMenuGUI()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (MenuNodes.empty() == false)
		{
			const HMainMenuNode& RootNode = MenuNodes[0];
			for (uint64 ChildMenuIndex : RootNode.ChildIndexes)
			{
				GenerateMainMenuGUIInternal(MenuNodes[ChildMenuIndex]);
			}
		}



		ImGui::EndMainMenuBar();
	}
	
}

void HMainMenuTree::GenerateMainMenuGUIInternal(const HMainMenuNode& InNode)
{
	bool bLeafNode = InNode.ChildIndexes.empty();

	if (bLeafNode)
	{
		bool bVisibility = true;
		if (InNode.Item.Visibility.IsBound())
		{
			bVisibility = InNode.Item.Visibility.Execute();
		}

		if (bVisibility)
		{
			bool bEnable = true;
			if (InNode.Item.CanAction.IsBound())
			{
				bEnable = InNode.Item.CanAction.Execute();
			}

			if (ImGui::MenuItem(InNode.Name.GetCStr(), nullptr, nullptr, bEnable))
			{
				InNode.Item.Action.ExecuteIfBound();
			}
		}
	}
	else
	{
		if (ImGui::BeginMenu(InNode.Name.GetCStr()))
		{
			for (uint64 ChildMenuIndex : InNode.ChildIndexes)
			{
				const HMainMenuNode& ChildNode = MenuNodes[ChildMenuIndex];
				GenerateMainMenuGUIInternal(ChildNode);
			}
			ImGui::EndMenu();
		}
	}

}
