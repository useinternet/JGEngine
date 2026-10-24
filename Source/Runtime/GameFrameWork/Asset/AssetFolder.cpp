#include "PCH/PCH.h"
#include "Asset/AssetFolder.h"
#include "Asset/Asset.h"
#include "Asset/AssetDatabase.h"
#include "Asset/Importers/FBXAssetImporter.h"
#include "FileIO/FileHelper.h"
#include "Misc/Module.h"
#include <algorithm>
#include <filesystem>

namespace
{
	namespace fs = std::filesystem;

	// 토큰 경로(/JGEngine/… · /JGGame/…)를 디스크 경로로. Content 밖으로 나가는 ".." 는 받지 않는다.
	bool tokenToDisk(const PString& inTokenPath, PString* outDiskPath)
	{
		PString contentDirectory;
		PString relativePath = inTokenPath;
		if (inTokenPath.StartWidth(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN) || inTokenPath == PString("/JGEngine"))
		{
			contentDirectory = HFileHelper::EngineContentDirectory();
			relativePath.Remove(0, PString("/JGEngine").Length());
		}
		else if (inTokenPath.StartWidth(JG_ASSET_GAME_PATH_RECOGNITION_TOEKN) || inTokenPath == PString("/JGGame"))
		{
			contentDirectory = HFileHelper::GameContentDirectory();
			relativePath.Remove(0, PString("/JGGame").Length());
		}
		if (contentDirectory.Empty() || relativePath.Contains(".."))
		{
			return false;
		}

		relativePath.ReplaceAll("\\", "/");
		while (relativePath.Empty() == false && relativePath[0] == '/')
		{
			relativePath.Remove(0, 1);
		}
		if (relativePath.Empty())
		{
			*outDiskPath = contentDirectory;
			return true;
		}
		HFileHelper::CombinePath(contentDirectory, relativePath, outDiskPath);
		return true;
	}

	PString joinToken(const PString& inTokenFolder, const PString& inName)
	{
		PString result = inTokenFolder;
		if (result.EndWidth("/") == false)
		{
			result += "/";
		}
		return result + inName;
	}

	PString lowerExtension(const PString& inName)
	{
		PString extension;
		HFileHelper::FileExtension(inName, &extension);
		return PString::ToLower(extension);
	}

	// 확장자를 받는 가져오기 클래스가 있는가. 첫 판은 FBX 하나(PNG · TGA 는 Engine-28).
	bool isImportable(const PString& inExtension)
	{
		return Allocate<JGFBXAssetImporter>()->IsSupportedExtension(inExtension);
	}

	PString loadedClassName(const PString& inTokenPath)
	{
		if (GAssetDatabase::IsValid() == false)
		{
			return PString();
		}
		PSharedPtr<JGAsset> asset = GAssetDatabase::GetInstance().GetLoadedAsset(HAssetPath(inTokenPath)).Pin();
		return asset != nullptr ? asset->GetType().GetName().ToString() : PString();
	}

	// 폴더 안 .jgasset 이름 → 마지막으로 쓴 시각. 가져오기 전 · 뒤를 견줘 새로 쓴 에셋을 찾는다.
	HHashMap<PString, fs::file_time_type> snapshotAssets(const PString& inDiskFolder)
	{
		HHashMap<PString, fs::file_time_type> result;
		std::error_code error;
		for (const fs::directory_entry& item : fs::directory_iterator(inDiskFolder.GetRawString(), error))
		{
			const PString name = item.path().filename().string().c_str();
			if (item.is_regular_file(error) && lowerExtension(name) == PString(JG_ASSET_FORMAT))
			{
				result[name] = item.last_write_time(error);
			}
		}
		return result;
	}

	bool importFailed(HAssetImportResult* outResult, const PString& inSourcePath, const PString& inError)
	{
		outResult->Error = inError;
		JG_LOG(Asset, ELogLevel::Warning, "ImportAssetFile: %s : %s", inSourcePath, inError);
		return false;
	}
}

bool ListAssetFolder(const PString& inTokenFolder, HList<HAssetFolderEntry>& outEntries)
{
	outEntries.clear();

	PString folder;
	if (tokenToDisk(inTokenFolder, &folder) == false || HFileHelper::IsDirectory(folder) == false)
	{
		return false;
	}

	std::error_code error;
	for (const fs::directory_entry& item : fs::directory_iterator(folder.GetRawString(), error))
	{
		// 파일 입출력이 좁은 문자 경로라 그 코드 페이지로 바꿀 수 없는 이름(한글 등)은 건너뛴다
		std::string narrowName;
		try
		{
			narrowName = item.path().filename().string();
		}
		catch (const std::exception&)
		{
			JG_LOG(Asset, ELogLevel::Warning, "ListAssetFolder: %s : a file name that is not ASCII is skipped", inTokenFolder);
			continue;
		}
		const PString name = narrowName.c_str();

		HAssetFolderEntry entry;
		entry.TokenPath = joinToken(inTokenFolder, name);
		if (item.is_directory(error))
		{
			entry.Kind = EAssetFolderEntryKind::Folder;
		}
		else
		{
			const PString extension = lowerExtension(name);
			if (extension == PString(JG_ASSET_FORMAT))
			{
				entry.Kind      = EAssetFolderEntryKind::Asset;
				entry.ClassName = loadedClassName(entry.TokenPath);
			}
			else if (isImportable(extension))
			{
				entry.Kind = EAssetFolderEntryKind::Source;
			}
			else
			{
				continue;
			}
		}
		outEntries.push_back(entry);
	}

	std::sort(outEntries.begin(), outEntries.end(), [](const HAssetFolderEntry& a, const HAssetFolderEntry& b)
		{
			if (a.Kind != b.Kind)
			{
				return a.Kind < b.Kind;
			}
			return PString::ToLower(a.TokenPath).GetRawString() < PString::ToLower(b.TokenPath).GetRawString();
		});
	return true;
}

bool ImportAssetFile(const PString& inSourcePath, const PString& inTokenFolder, HAssetImportResult* outResult)
{
	HAssetImportResult localResult;
	HAssetImportResult* result = outResult != nullptr ? outResult : &localResult;
	result->CreatedAssets.clear();
	result->Error = PString();

	PString source;
	if (inSourcePath.StartWidth(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN) || inSourcePath.StartWidth(JG_ASSET_GAME_PATH_RECOGNITION_TOEKN))
	{
		if (tokenToDisk(inSourcePath, &source) == false)
		{
			return importFailed(result, inSourcePath, "the source path is not in a content folder");
		}
	}
	else
	{
		HFileHelper::AbsolutePath(inSourcePath, &source);
	}
	if (HFileHelper::Exists(source) == false || HFileHelper::IsDirectory(source))
	{
		return importFailed(result, inSourcePath, "the source file does not exist");
	}

	const PString extension = lowerExtension(source);
	if (isImportable(extension) == false)
	{
		return importFailed(result, inSourcePath, PString::Format("no importer for %s files", extension));
	}

	PString destination;
	if (tokenToDisk(inTokenFolder, &destination) == false || HFileHelper::IsDirectory(destination) == false)
	{
		return importFailed(result, inSourcePath, PString::Format("%s is not a content folder", inTokenFolder));
	}

	// 메시 · 텍스처 에셋을 만들면 GPU 리소스도 만든다(JGStaticMesh::Create · JGTexture::Create)
	if (GModuleGlobalSystem::GetInstance().FindModule(PName("Graphics")) == nullptr)
	{
		return importFailed(result, inSourcePath, "importing needs the Graphics module (editor or JGConsole -graphics)");
	}

	const HHashMap<PString, fs::file_time_type> before = snapshotAssets(destination);

	PSharedPtr<PFBXAssetImportArguments> args = Allocate<PFBXAssetImportArguments>();
	args->SrcPath  = source;
	args->DestPath = destination;
	args->Flags    = EFBXAssetImportFlags::Import_StaticMesh | EFBXAssetImportFlags::Import_Texture;
	if (Allocate<JGFBXAssetImporter>()->Import(args) == false)
	{
		return importFailed(result, inSourcePath, "the importer failed (see the Asset log)");
	}

	for (const HPair<const PString, fs::file_time_type>& entry : snapshotAssets(destination))
	{
		const auto previous = before.find(entry.first);
		if (previous == before.end() || previous->second != entry.second)
		{
			result->CreatedAssets.push_back(joinToken(inTokenFolder, entry.first));
		}
	}
	std::sort(result->CreatedAssets.begin(), result->CreatedAssets.end(), [](const PString& a, const PString& b) { return a.GetRawString() < b.GetRawString(); });
	if (result->CreatedAssets.empty())
	{
		return importFailed(result, inSourcePath, "the importer wrote no asset");
	}

	// 새 에셋을 바로 올린다. 이미 올라온 경로(다시 가져오기)는 에셋 DB 가 그 에셋을 그대로 둔다 — 다시 읽기는 첫 판 밖.
	if (GAssetDatabase::IsValid())
	{
		for (const PString& created : result->CreatedAssets)
		{
			GAssetDatabase::GetInstance().LoadAssetAsync(HAssetPath(created));
		}
	}

	JG_LOG(Asset, ELogLevel::Info, "ImportAssetFile: %s -> %s (%d asset(s))", inSourcePath, inTokenFolder, (int32)result->CreatedAssets.size());
	return true;
}
