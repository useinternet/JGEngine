#include "PCH/PCH.h"
#include "Asset/AssetFolder.h"
#include "Asset/Asset.h"
#include "Asset/AssetDatabase.h"
#include "SelfTest/GameFrameWorkSelfTest.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "FileIO/FileHelper.h"
#include "Misc/Module.h"
#include <fstream>

// 컨텐츠 브라우저용 폴더 읽기 · 가져오기(ListAssetFolder · ImportAssetFile, Engine-30) 자체 검사. gfw.selftest 그룹 asset.
//   엔진 Content 의 저장소 파일(RawResources · TempAsset)과, 시험 동안만 엔진 Content 에 만드는 임시 폴더(__AssetFolderSelfTest)를 읽는다.
//   가져오기 성공 길은 Graphics 가 있을 때만(에디터 · JGConsole -graphics) 삼각형 하나짜리 ASCII FBX 로 본다. 없으면 실패 메시지를 보고 그 절은 건너뛴다.
//   에셋 DB 가 있으면(에디터) 새 에셋이 올라오는지는 다음 프레임에 안다. 로드 콜백이 "assetfolder selftest (load): OK/FAILED" 를 따로 남기고 임시 폴더를 지운다.
//   에셋 DB 에 올라온 시험 에셋은 남는다(에디터를 다시 켜면 없음).
namespace
{
	int32 GCheckCount   = 0;
	int32 GFailureCount = 0;
	int32 GSkipCount    = 0;

	void check(bool bCondition, const char* caseName)
	{
		++GCheckCount;
		if (bCondition == false)
		{
			++GFailureCount;
			JG_LOG(Asset, ELogLevel::Error, "assetfolder selftest FAILED: %s", caseName);
		}
	}

	const char* const TEST_FOLDER = "/JGEngine/__AssetFolderSelfTest";
	bool bKeepTestFolder = false;   // 에셋 DB 가 새 에셋을 읽는 동안 임시 폴더를 남긴다

	// 삼각형 하나(정점 3 · 인덱스 3). 마지막 인덱스의 음수(-3 = ~2)가 다각형 끝을 뜻한다.
	const char* const TRIANGLE_FBX =
		"; FBX 7.4.0 project file\n"
		"FBXHeaderExtension:  {\n"
		"\tFBXHeaderVersion: 1003\n"
		"\tFBXVersion: 7400\n"
		"}\n"
		"GlobalSettings:  {\n"
		"\tVersion: 1000\n"
		"\tProperties70:  {\n"
		"\t\tP: \"UpAxis\", \"int\", \"Integer\", \"\",1\n"
		"\t\tP: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n"
		"\t\tP: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1\n"
		"\t}\n"
		"}\n"
		"Objects:  {\n"
		"\tGeometry: 100, \"Geometry::Triangle\", \"Mesh\" {\n"
		"\t\tVertices: *9 {\n"
		"\t\t\ta: 0,0,0,100,0,0,0,100,0\n"
		"\t\t}\n"
		"\t\tPolygonVertexIndex: *3 {\n"
		"\t\t\ta: 0,1,-3\n"
		"\t\t}\n"
		"\t\tGeometryVersion: 124\n"
		"\t}\n"
		"\tModel: 200, \"Model::Triangle\", \"Mesh\" {\n"
		"\t\tVersion: 232\n"
		"\t}\n"
		"}\n"
		"Connections:  {\n"
		"\tC: \"OO\",100,200\n"
		"\tC: \"OO\",200,0\n"
		"}\n";

	PString diskPath(const PString& inTokenPath)
	{
		PString relative = inTokenPath;
		relative.Remove(0, PString(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN).Length());
		PString path;
		HFileHelper::CombinePath(HFileHelper::EngineContentDirectory(), relative, &path);
		return path;
	}

	void writeText(const PString& inTokenPath, const char* inText)
	{
		std::ofstream file(diskPath(inTokenPath).GetRawString(), std::ios::binary | std::ios::trunc);
		file.write(inText, (std::streamsize)strlen(inText));
	}

	const HAssetFolderEntry* findEntry(const HList<HAssetFolderEntry>& inEntries, const PString& inTokenPath)
	{
		for (const HAssetFolderEntry& entry : inEntries)
		{
			if (entry.TokenPath == inTokenPath)
			{
				return &entry;
			}
		}
		return nullptr;
	}

	void removeTestFolder()
	{
		const PString folder = diskPath(TEST_FOLDER);
		if (HFileHelper::Exists(folder))
		{
			HFileHelper::RemoveFileOrDirectory(folder);
		}
	}

	void testListRepositoryContent()
	{
		HList<HAssetFolderEntry> entries;
		check(ListAssetFolder("/JGEngine", entries), "list: /JGEngine (no slash) is the engine Content root");
		const HAssetFolderEntry* rawResources = findEntry(entries, "/JGEngine/RawResources");
		check(rawResources != nullptr && rawResources->Kind == EAssetFolderEntryKind::Folder, "list: root has the RawResources folder");
		check(findEntry(entries, "/JGEngine/TempAsset") != nullptr, "list: root has the TempAsset folder");

		check(ListAssetFolder("/JGEngine/RawResources/", entries), "list: RawResources with a trailing slash");
		const HAssetFolderEntry* xbot = findEntry(entries, "/JGEngine/RawResources/X Bot.fbx");
		check(xbot != nullptr && xbot->Kind == EAssetFolderEntryKind::Source, "list: X Bot.fbx is a source file");
		bool bPngListed = false;
		for (const HAssetFolderEntry& entry : entries)
		{
			bPngListed = bPngListed || entry.TokenPath.EndWidth(".png");
		}
		check(bPngListed == false, "list: png has no importer yet, so it is not listed");

		check(ListAssetFolder("/JGEngine/TempAsset", entries), "list: TempAsset");
		const HAssetFolderEntry* sample = findEntry(entries, "/JGEngine/TempAsset/Sample.jgasset");
		check(sample != nullptr && sample->Kind == EAssetFolderEntryKind::Asset, "list: Sample.jgasset is an asset");
		if (GAssetDatabase::IsValid() && sample != nullptr && sample->ClassName.Empty() == false)
		{
			check(sample->ClassName == PString("JGStaticMesh"), "list: Sample.jgasset class name from the asset database");
		}
		else
		{
			++GSkipCount;   // 에셋 DB 가 없거나(JGConsole) 아직 읽는 중
		}
	}

	void testListFixture()
	{
		HFileHelper::CreateDirectory(diskPath(TEST_FOLDER));
		HFileHelper::CreateDirectory(diskPath(PString(TEST_FOLDER) + "/Sub"));
		writeText(PString(TEST_FOLDER) + "/b.jgasset", "{}");
		writeText(PString(TEST_FOLDER) + "/A.FBX", "not an fbx");
		writeText(PString(TEST_FOLDER) + "/notes.txt", "ignored");

		HList<HAssetFolderEntry> entries;
		check(ListAssetFolder(TEST_FOLDER, entries), "fixture: list the temporary folder");
		check(entries.size() == 3, "fixture: folder, asset and source only (txt is skipped)");
		if (entries.size() == 3)
		{
			check(entries[0].Kind == EAssetFolderEntryKind::Folder && entries[0].TokenPath == PString(TEST_FOLDER) + "/Sub", "fixture: the folder comes first");
			check(entries[1].Kind == EAssetFolderEntryKind::Asset && entries[1].TokenPath == PString(TEST_FOLDER) + "/b.jgasset", "fixture: then the asset");
			check(entries[1].ClassName.Empty(), "fixture: an asset that is not loaded has no class name");
			check(entries[2].Kind == EAssetFolderEntryKind::Source && entries[2].TokenPath == PString(TEST_FOLDER) + "/A.FBX", "fixture: then the source (extension is case-insensitive)");
		}

		check(ListAssetFolder("/JGEngine/__NoSuchFolder__", entries) == false && entries.empty(), "invalid: a missing folder");
		check(ListAssetFolder("/Unknown/Folder", entries) == false, "invalid: an unknown mount token");
		check(ListAssetFolder("/JGEngine/../Source", entries) == false, "invalid: .. cannot leave the Content folder");
		if (HFileHelper::IsProjectMode() == false)
		{
			check(ListAssetFolder("/JGGame/", entries) == false, "invalid: /JGGame/ without a game project");
		}
		else
		{
			++GSkipCount;
		}
	}

	void testImportErrors()
	{
		HAssetImportResult result;
		check(ImportAssetFile(PString(TEST_FOLDER) + "/notes.txt", TEST_FOLDER, &result) == false && result.Error.Empty() == false,
			"import: a file type without an importer is refused");
		check(ImportAssetFile(PString(TEST_FOLDER) + "/missing.fbx", TEST_FOLDER, &result) == false && result.Error.Empty() == false,
			"import: a missing source file is refused");
		check(ImportAssetFile(PString(TEST_FOLDER) + "/A.FBX", "/JGEngine/__NoSuchFolder__", &result) == false && result.Error.Empty() == false,
			"import: a missing destination folder is refused");
		check(ImportAssetFile(PString(TEST_FOLDER) + "/A.FBX", "/Unknown/Folder", &result) == false && result.CreatedAssets.empty(),
			"import: an unknown destination token is refused");
	}

	void testImportTriangle()
	{
		const PString source = PString(TEST_FOLDER) + "/Triangle.fbx";
		writeText(source, TRIANGLE_FBX);
		const bool bGraphics = GModuleGlobalSystem::GetInstance().FindModule(PName("Graphics")) != nullptr;

		HAssetImportResult result;
		const bool bImported = ImportAssetFile(source, TEST_FOLDER, &result);
		if (bGraphics == false)
		{
			check(bImported == false && result.Error.Contains("Graphics"), "import: without Graphics the import is refused with a reason");
			++GSkipCount;   // 성공 길은 Graphics 가 있을 때만
			return;
		}

		check(bImported, "import: the triangle FBX is imported");
		check(result.CreatedAssets.size() == 1, "import: one asset is created");
		if (result.CreatedAssets.size() != 1)
		{
			return;
		}
		const PString created = result.CreatedAssets[0];
		check(created.StartWidth(PString(TEST_FOLDER) + "/") && created.EndWidth(JG_ASSET_FORMAT), "import: the created asset is a token path in the destination folder");
		check(HFileHelper::Exists(diskPath(created)), "import: the created asset file exists");

		HList<HAssetFolderEntry> entries;
		ListAssetFolder(TEST_FOLDER, entries);
		const HAssetFolderEntry* entry = findEntry(entries, created);
		check(entry != nullptr && entry->Kind == EAssetFolderEntryKind::Asset, "import: the new asset is listed");

		if (GAssetDatabase::IsValid() == false)
		{
			++GSkipCount;   // 에셋 DB 가 없으면 올리기 확인을 건너뛴다(JGConsole -graphics)
			return;
		}
		// 로드 스레드가 읽고 메인 스레드 Update 가 올리므로 명령 안에서는 기다릴 수 없다. 이미 올라와 있으면(다시 실행) 콜백이 바로 불린다.
		// 임시 폴더는 로드가 끝난 뒤 지운다(먼저 지우면 로드 스레드가 파일을 못 읽는다).
		bKeepTestFolder = true;
		GAssetDatabase::GetInstance().LoadAssetAsync(HAssetPath(created), POnLoadCompelete::CreateLambda([created](PWeakPtr<JGAsset> inAsset)
			{
				PSharedPtr<JGAsset> asset = inAsset.Pin();
				const PString className = asset != nullptr ? asset->GetType().GetName().ToString() : PString();
				if (className == PString("JGStaticMesh"))
				{
					JG_LOG(Asset, ELogLevel::Info, "assetfolder selftest (load): OK - %s is loaded as %s", created, className);
				}
				else
				{
					JG_LOG(Asset, ELogLevel::Error, "assetfolder selftest (load): FAILED - %s is loaded as '%s'", created, className);
				}
				removeTestFolder();
			}));
	}

	bool executeSelfTest(const HConsoleCommandArgs&)
	{
		GCheckCount   = 0;
		GFailureCount = 0;
		GSkipCount    = 0;

		bKeepTestFolder = false;
		removeTestFolder();
		testListRepositoryContent();
		testListFixture();
		testImportErrors();
		testImportTriangle();
		if (bKeepTestFolder == false)
		{
			removeTestFolder();
		}

		if (GFailureCount == 0)
		{
			JG_LOG(Asset, ELogLevel::Info, "assetfolder selftest: OK (%d/%d)", GCheckCount, GCheckCount);
		}
		else
		{
			JG_LOG(Asset, ELogLevel::Error, "assetfolder selftest: FAILED (%d/%d)", GCheckCount - GFailureCount, GCheckCount);
		}
		return GFailureCount == 0;
	}
}

HSelfTestResult RunAssetFolderSelfTest(const HConsoleCommandArgs& inArgs)
{
	executeSelfTest(inArgs);
	return { GCheckCount - GFailureCount, GFailureCount, GSkipCount };
}
