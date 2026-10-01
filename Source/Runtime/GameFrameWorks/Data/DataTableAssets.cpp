#include "PCH/PCH.h"
#include "Data/DataTableAssets.h"
#include "AssetDatabase.h"
#include "AssetPath.h"
#include "Object/ObjectGlobalSystem.h"

namespace
{
	PSharedPtr<JGDataTable> findLoadedTable(const PString& inTablePath)
	{
		if (GAssetDatabase::IsValid() == false)
		{
			return nullptr;
		}

		const HAssetPath assetPath(inTablePath);
		if (assetPath.IsValid() == false)
		{
			return nullptr;
		}

		PSharedPtr<JGAsset> asset = GAssetDatabase::GetInstance().GetLoadedAsset(assetPath).Pin();
		if (asset == nullptr || asset->GetType() != JGTYPE(JGDataTable))
		{
			return nullptr;
		}
		return Cast<JGDataTable>(asset);
	}
}

void GetLoadedDataTables(HList<HDataTableAssetEntry>& outTables)
{
	outTables.clear();
	if (GAssetDatabase::IsValid() == false)
	{
		return;
	}

	for (const HPair<const PName, PWeakPtr<JGAsset>>& entry : GAssetDatabase::GetInstance()._assetsByAssetPath)
	{
		PSharedPtr<JGAsset> asset = entry.second.Pin();
		if (asset == nullptr || asset->GetType() != JGTYPE(JGDataTable))
		{
			continue;
		}

		HDataTableAssetEntry tableEntry;
		tableEntry.TokenPath = entry.first.ToString();
		tableEntry.Table     = Cast<JGDataTable>(asset);
		outTables.push_back(tableEntry);
	}

	std::sort(outTables.begin(), outTables.end(), [](const HDataTableAssetEntry& a, const HDataTableAssetEntry& b)
		{
			return a.TokenPath.GetRawString() < b.TokenPath.GetRawString();
		});
}

void GetLoadedAssetPaths(const PString& inClassName, HList<PString>& outPaths)
{
	outPaths.clear();
	if (GAssetDatabase::IsValid() == false)
	{
		return;
	}

	JGType filterType;
	const bool bFilter = inClassName.Empty() == false;
	if (bFilter)
	{
		filterType = GObjectGlobalSystem::GetInstance().GetType(PName(inClassName));
	}

	for (const HPair<const PName, PWeakPtr<JGAsset>>& entry : GAssetDatabase::GetInstance()._assetsByAssetPath)
	{
		PSharedPtr<JGAsset> asset = entry.second.Pin();
		if (asset == nullptr)
		{
			continue;
		}
		if (bFilter && asset->GetType() != filterType && GObjectGlobalSystem::GetInstance().CanCast(filterType, asset->GetType()) == false)
		{
			continue;
		}
		outPaths.push_back(entry.first.ToString());
	}

	std::sort(outPaths.begin(), outPaths.end(), [](const PString& a, const PString& b)
		{
			return a.GetRawString() < b.GetRawString();
		});
}

bool MakeDataTableTokenPath(const PString& inContentToken, const PString& inRelativePath, PString* outTokenPath, PString* outError)
{
	if (inContentToken != JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN && inContentToken != JG_ASSET_GAME_PATH_RECOGNITION_TOEKN)
	{
		if (outError != nullptr)
		{
			*outError = "content must be /JGGame/ or /JGEngine/";
		}
		return false;
	}

	HRawString relative = inRelativePath.GetRawString();
	while (relative.empty() == false && (relative.front() == '/' || relative.front() == '\\'))
	{
		relative.erase(relative.begin());
	}
	const HRawString extension = JG_ASSET_FORMAT;
	if (relative.size() > extension.size() && relative.compare(relative.size() - extension.size(), extension.size(), extension) == 0)
	{
		relative.erase(relative.size() - extension.size());
	}

	if (relative.empty() || relative.back() == '/')
	{
		if (outError != nullptr)
		{
			*outError = "name is empty";
		}
		return false;
	}
	for (char& c : relative)
	{
		if (c == '\\')
		{
			c = '/';
		}
		const bool bAllowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '/';
		if (bAllowed == false)
		{
			if (outError != nullptr)
			{
				*outError = "use only ASCII letters, digits, _ - and / (file paths are narrow-character)";
			}
			return false;
		}
	}
	if (relative.find("//") != HRawString::npos)
	{
		if (outError != nullptr)
		{
			*outError = "empty folder name";
		}
		return false;
	}

	if (outTokenPath != nullptr)
	{
		*outTokenPath = PString((inContentToken.GetRawString() + relative + extension).c_str());
	}
	return true;
}

bool HDataTableAssetResolver::GetTableKeys(const PString& inTablePath, HList<PString>* outKeys) const
{
	PSharedPtr<JGDataTable> table = findLoadedTable(inTablePath);
	if (table == nullptr)
	{
		return false;
	}

	if (outKeys != nullptr)
	{
		outKeys->clear();
		const int32 rowCount = table->GetRowCount();
		for (int32 i = 0; i < rowCount; ++i)
		{
			outKeys->push_back(table->GetRowKey(i));
		}
	}
	return true;
}

bool HDataTableAssetResolver::HasAsset(const PString& inAssetPath) const
{
	const HAssetPath assetPath(inAssetPath);
	if (assetPath.IsValid() == false)
	{
		return false;
	}

	if (GAssetDatabase::IsValid() && GAssetDatabase::GetInstance().GetLoadedAsset(assetPath).Pin() != nullptr)
	{
		return true;
	}
	return HFileHelper::Exists(assetPath.GetRawAssetPath().ToString());
}
