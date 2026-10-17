#include "PCH/PCH.h"
#include "Data/ContentCookCheck.h"
#include "Data/DataTable.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include <chrono>
#include <fstream>

// cookcheck.selftest — 쿠킹 검사(ValidateContentForCook, GP-A1) 자체 검사. GPU · 에셋 DB 없이 돈다.
//   엔진 Temp 아래에 두 마운트(Engine · Game) 픽스처를 만들어 정상 · 깨진 참조 · GUID 중복 · 형식 오류 · 데이터 테이블 오류를 보고,
//   실제 엔진 Content(RawResources · TempAsset 제외 — 쿠킹 기본 제외, 설계 4-10)가 통과하는지 본다.
// 결과는 JG_LOG(CookCheck): 실패마다 Error 한 줄, 끝에 "cookcheck.selftest: OK (통과/전체)".
namespace
{
	int32 GCheckCount   = 0;
	int32 GFailureCount = 0;

	void check(bool bCondition, const char* inCaseName)
	{
		++GCheckCount;
		if (bCondition == false)
		{
			++GFailureCount;
			JG_LOG(CookCheck, ELogLevel::Error, "cookcheck.selftest FAILED: %s", PString(inCaseName));
		}
	}

	bool anyContains(const HList<PString>& inLines, const char* inText)
	{
		for (const PString& line : inLines)
		{
			if (line.Contains(inText))
			{
				return true;
			}
		}
		return false;
	}

	HDataTableColumn makeColumn(const char* inName, EDataTableColumnType inType)
	{
		return HDataTableColumn(PString(inName), inType);
	}

	HDataTableRow makeRow(const char* inKey, const HList<HDataTableValue>& inValues)
	{
		HDataTableRow row;
		row.Key    = inKey;
		row.Values = inValues;
		return row;
	}

	// 데이터 테이블 파일 글(실제 저장 형식). 자기 경로 칸은 /JGEngine/ 으로 둔다 — 쿠킹 검사는 그 칸을 참조로 보지 않는다
	PString tableJson(const char* inName, const HDataTableContent& inContent)
	{
		PSharedPtr<JGDataTable> table = Allocate<JGDataTable>();
		table->InitializeNew(PString::Format("/JGEngine/CookCheckSelfTest/%s", PString(inName)), inContent);
		PString text;
		table->ToJsonText(&text);
		return text;
	}

	// 데이터 테이블이 아닌 에셋 껍데기. inBody 는 JGObject 안의 타입 절("\"JGTexture\": { … }")
	PString assetJson(const char* inType, const PString& inGuid, const char* inSelfPath, const PString& inBody)
	{
		return PString::Format(
			"{\n"
			"    \"JGObjectType\": \"%s\",\n"
			"    \"JGObject\": {\n"
			"        \"JGObject\": { \"Name\": \"%s\" },\n"
			"        \"JGAsset\": { \"_guid\": \"%s\", \"AssetPath\": { \"AssetPath\": \"%s\" }, \"Version\": 0 },\n"
			"        %s\n"
			"    }\n"
			"}\n",
			PString(inType), PString(inType), inGuid, PString(inSelfPath), inBody);
	}

	// 두 마운트(Engine · Game)를 가진 픽스처. 파일을 쓰면 쿠킹 대상 목록에도 들어간다
	class HCookFixture
	{
	public:
		HRawString   Root;
		HList<PString> Files;

		explicit HCookFixture(const HRawString& inRoot)
			: Root(inRoot)
		{
			std::error_code errCode;
			fs::remove_all(fs::path(Root), errCode);
		}

		PString MountDirectory(int32 inMount) const
		{
			return PString((Root + (inMount == 0 ? "/Engine/" : "/Game/")).c_str());
		}

		HList<HPair<PString, PString>> Mounts() const
		{
			return { HPair<PString, PString>(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN, MountDirectory(0)),
					 HPair<PString, PString>(JG_ASSET_GAME_PATH_RECOGNITION_TOEKN, MountDirectory(1)) };
		}

		PString Write(int32 inMount, const char* inRelativePath, const PString& inText, bool bAddToCookSet = true)
		{
			const PString path = MountDirectory(inMount) + inRelativePath;
			writeFile(path, inText);
			if (bAddToCookSet)
			{
				Files.push_back(path);
			}
			return path;
		}

		// 마운트 밖 파일
		PString WriteOutside(const char* inRelativePath, const PString& inText)
		{
			const PString path = PString((Root + "/Outside/").c_str()) + inRelativePath;
			writeFile(path, inText);
			Files.push_back(path);
			return path;
		}

		void RemoveFromCookSet(const PString& inPath)
		{
			Files.erase(std::remove_if(Files.begin(), Files.end(), [&inPath](const PString& file)
				{
					return file.GetRawString() == inPath.GetRawString();
				}), Files.end());
		}

		bool Run(HList<PString>& outErrors, HList<PString>& outWarnings) const
		{
			outErrors.clear();
			outWarnings.clear();
			return ValidateContentForCook(Files, Mounts(), outErrors, outWarnings);
		}

		void Cleanup() const
		{
			std::error_code errCode;
			fs::remove_all(fs::path(Root), errCode);
		}

	private:
		static void writeFile(const PString& inPath, const PString& inText)
		{
			std::error_code errCode;
			fs::create_directories(fs::path(inPath.GetRawString()).parent_path(), errCode);
			std::ofstream stream(inPath.GetRawString(), std::ios::binary);
			stream << inText.GetRawString();
		}
	};

	// 정상 픽스처 — 엔진 · 게임 마운트를 오가는 참조, 확장자 없는 참조, 에셋이 아닌 파일 참조, 다른 테이블 참조
	//   Engine: Fonts/Default.ttf · Textures/Base.jgasset
	//   Game  : Meshes/Box.jgasset(→ /JGEngine/Textures/Base, 자기 경로 칸은 옛 경로) · Data/Items.jgasset(→ Box · 글꼴 · Data/Other) · Data/Other.jgasset
	// 경고 1: Items 의 Next 'Other_Z' 가 Other 에 없다(데이터 테이블 경고는 쿠킹을 막지 않는다)
	struct HGoodSet
	{
		PString BoxGuid;
		PString FontPath;
		PString BoxText;
	};

	HDataTableContent makeItems(const char* inMesh, const char* inNextTable, const char* inSecondKey)
	{
		HDataTableContent content;
		content.Schema.KeyColumn = "Id";
		HDataTableColumn mesh = makeColumn("Mesh", EDataTableColumnType::AssetRef);
		mesh.AssetClass = "JGStaticMesh";
		HDataTableColumn font = makeColumn("Font", EDataTableColumnType::String);
		HDataTableColumn next = makeColumn("Next", EDataTableColumnType::RowRef);
		next.Table = inNextTable;
		content.Schema.Columns = { mesh, font, next };
		content.Rows.push_back(makeRow("Item_A", { HDataTableValue::MakeText(inMesh), HDataTableValue::MakeText("/JGEngine/Fonts/Default.ttf"), HDataTableValue::MakeText("Other_A") }));
		content.Rows.push_back(makeRow(inSecondKey, { HDataTableValue::MakeText(inMesh), HDataTableValue::MakeText(""), HDataTableValue::MakeText("Other_Z") }));
		content.Rows.push_back(makeRow("Item_C", { HDataTableValue::MakeText(inMesh), HDataTableValue::MakeText(""), HDataTableValue::MakeText("") }));
		return content;
	}

	HDataTableContent makeOther()
	{
		HDataTableContent content;
		content.Schema.KeyColumn = "Id";
		content.Schema.Columns   = { makeColumn("Label", EDataTableColumnType::String) };
		content.Rows.push_back(makeRow("Other_A", { HDataTableValue::MakeText("first") }));
		content.Rows.push_back(makeRow("Other_B", { HDataTableValue::MakeText("second") }));
		return content;
	}

	HGoodSet writeGoodSet(HCookFixture& fixture, const char* inItemsMesh = "/JGGame/Meshes/Box.jgasset",
		const char* inItemsNextTable = "/JGGame/Data/Other", const char* inItemsSecondKey = "Item_B")
	{
		HGoodSet set;
		set.BoxGuid  = HGuid::New().ToString();
		set.FontPath = fixture.Write(0, "Fonts/Default.ttf", "not a real font");
		fixture.Write(0, "Textures/Base.jgasset", assetJson("JGTexture", HGuid::New().ToString(), "/JGEngine/Textures/Base.jgasset", "\"JGTexture\": { \"Format\": \"RGBA8\" }"));
		set.BoxText = assetJson("JGStaticMesh", set.BoxGuid, "/JGGame/OldFolder/Box.jgasset", "\"JGStaticMesh\": { \"Material\": \"/JGEngine/Textures/Base\" }");
		fixture.Write(1, "Meshes/Box.jgasset", set.BoxText);
		fixture.Write(1, "Data/Items.jgasset", tableJson("Items", makeItems(inItemsMesh, inItemsNextTable, inItemsSecondKey)));
		fixture.Write(1, "Data/Other.jgasset", tableJson("Other", makeOther()));
		return set;
	}

	HRawString fixtureRoot(const char* inCase)
	{
		std::error_code errCode;
		const fs::path temp = fs::absolute(fs::path(HFileHelper::EngineTempDirectory().GetRawString()), errCode);
		return (temp / "CookCheckSelfTest" / inCase).lexically_normal().generic_string();
	}

	void logLines(const char* inCase, const HList<PString>& inErrors, const HList<PString>& inWarnings)
	{
		for (const PString& line : inErrors)
		{
			JG_LOG(CookCheck, ELogLevel::Info, "cookcheck.selftest [%s] error   %s", PString(inCase), line);
		}
		for (const PString& line : inWarnings)
		{
			JG_LOG(CookCheck, ELogLevel::Info, "cookcheck.selftest [%s] warning %s", PString(inCase), line);
		}
	}

	void testGood()
	{
		HCookFixture fixture(fixtureRoot("good"));
		writeGoodSet(fixture);

		HList<PString> errors;
		HList<PString> warnings;
		const bool bOk = fixture.Run(errors, warnings);
		logLines("good", errors, warnings);
		check(bOk && errors.empty(), "good: passes with no errors");
		check(warnings.size() == 1 && anyContains(warnings, "/JGGame/Data/Items.jgasset : ") && anyContains(warnings, "Other_Z"),
			"good: one data table warning (missing row Other_Z) does not fail the cook");

		HList<PString> secondErrors;
		HList<PString> secondWarnings;
		fixture.Run(secondErrors, secondWarnings);
		check(secondErrors == errors && secondWarnings.size() == warnings.size(), "good: same result twice");
		fixture.Cleanup();
	}

	void testBrokenReferences()
	{
		HList<PString> errors;
		HList<PString> warnings;
		{
			// 데이터 테이블 AssetRef 칸 3곳이 같은 없는 에셋 — 대상마다 한 줄
			HCookFixture fixture(fixtureRoot("missing_asset"));
			writeGoodSet(fixture, "/JGGame/Meshes/Missing.jgasset");
			const bool bOk = fixture.Run(errors, warnings);
			logLines("missing_asset", errors, warnings);
			check(bOk == false, "broken reference: data table AssetRef to a missing asset fails");
			check(errors.size() == 1 && anyContains(errors, "/JGGame/Data/Items.jgasset : reference '/JGGame/Meshes/Missing.jgasset'")
				&& anyContains(errors, "is not in the cook set") && anyContains(errors, "and 2 more place(s)"),
				"broken reference: one error per missing target with the first place and count");
			check(anyContains(errors, "JGObject.JGDataTable.Rows[0]"), "broken reference: error names the JSON place");
			fixture.Cleanup();
		}
		{
			// 데이터 테이블이 아닌 에셋의 참조(확장자 없음)
			HCookFixture fixture(fixtureRoot("missing_texture"));
			writeGoodSet(fixture);
			fixture.Write(1, "Meshes/Box.jgasset", assetJson("JGStaticMesh", HGuid::New().ToString(), "/JGGame/Meshes/Box.jgasset", "\"JGStaticMesh\": { \"Material\": \"/JGEngine/Textures/Gone\" }"), false);
			const bool bOk = fixture.Run(errors, warnings);
			logLines("missing_texture", errors, warnings);
			check(bOk == false && anyContains(errors, "/JGGame/Meshes/Box.jgasset : reference '/JGEngine/Textures/Gone'"),
				"broken reference: reference inside any asset type is checked");
			fixture.Cleanup();
		}
		{
			// 대소문자만 다른 참조 — 런타임 찾기가 대소문자를 구분한다
			HCookFixture fixture(fixtureRoot("case"));
			writeGoodSet(fixture, "/JGGame/meshes/box.jgasset");
			const bool bOk = fixture.Run(errors, warnings);
			logLines("case", errors, warnings);
			check(bOk == false && anyContains(errors, "differs in case from '/JGGame/Meshes/Box.jgasset'"), "broken reference: case mismatch fails with a hint");
			fixture.Cleanup();
		}
		{
			// 에셋이 아닌 파일 참조 — 그 파일이 쿠킹 대상에서 빠지면 실패
			HCookFixture fixture(fixtureRoot("missing_font"));
			const HGoodSet set = writeGoodSet(fixture);
			fixture.RemoveFromCookSet(set.FontPath);
			const bool bOk = fixture.Run(errors, warnings);
			logLines("missing_font", errors, warnings);
			check(bOk == false && anyContains(errors, "reference '/JGEngine/Fonts/Default.ttf'"), "broken reference: excluded non-asset file fails");
			fixture.Cleanup();
		}
		{
			// RowRef 열의 Table 이 없는 테이블 — 경로 문자열은 오류, 데이터 테이블은 경고(테이블 없음)
			HCookFixture fixture(fixtureRoot("missing_table"));
			writeGoodSet(fixture, "/JGGame/Meshes/Box.jgasset", "/JGGame/Data/Nope");
			const bool bOk = fixture.Run(errors, warnings);
			logLines("missing_table", errors, warnings);
			check(bOk == false && anyContains(errors, "reference '/JGGame/Data/Nope'"), "broken reference: missing referenced table fails");
			check(anyContains(warnings, "table '/JGGame/Data/Nope' of column 'Next' is not found"), "broken reference: data table also warns the table is missing");
			fixture.Cleanup();
		}
	}

	void testGuids()
	{
		HList<PString> errors;
		HList<PString> warnings;
		{
			// 게임 에셋을 엔진 마운트로 복사(GUID 도 복사됨) — 두 마운트를 통틀어 검사
			HCookFixture fixture(fixtureRoot("duplicate_guid"));
			const HGoodSet set = writeGoodSet(fixture);
			fixture.Write(0, "Meshes/BoxCopy.jgasset", set.BoxText);
			const bool bOk = fixture.Run(errors, warnings);
			logLines("duplicate_guid", errors, warnings);
			check(bOk == false && errors.size() == 1, "guid: duplicate across mounts fails once");
			check(anyContains(errors, PString::Format("/JGGame/Meshes/Box.jgasset : GUID %s is also used by /JGEngine/Meshes/BoxCopy.jgasset", set.BoxGuid).GetCStr()),
				"guid: error names both files (the second in token-path order is reported)");
			fixture.Cleanup();
		}
		{
			HCookFixture fixture(fixtureRoot("bad_guid"));
			writeGoodSet(fixture);
			fixture.Write(1, "Misc/NoGuid.jgasset", "{ \"JGObjectType\": \"JGTexture\", \"JGObject\": { \"JGAsset\": { \"Version\": 0 } } }");
			fixture.Write(1, "Misc/TextGuid.jgasset", assetJson("JGTexture", "abc-def", "/JGGame/Misc/TextGuid.jgasset", "\"JGTexture\": {}"));
			fixture.Write(1, "Misc/ZeroGuid.jgasset", assetJson("JGTexture", "0-1-2-3-4-5-6-7-8-9-10", "/JGGame/Misc/ZeroGuid.jgasset", "\"JGTexture\": {}"));
			fixture.Write(1, "Misc/WideGuid.jgasset", assetJson("JGTexture", "7-70000-2-3-4-5-6-7-8-9-10", "/JGGame/Misc/WideGuid.jgasset", "\"JGTexture\": {}"));
			const bool bOk = fixture.Run(errors, warnings);
			logLines("bad_guid", errors, warnings);
			check(bOk == false && anyContains(errors, "/JGGame/Misc/NoGuid.jgasset : JGAsset._guid is missing"), "guid: missing _guid fails");
			check(anyContains(errors, "/JGGame/Misc/TextGuid.jgasset : JGAsset._guid 'abc-def' is not a GUID"), "guid: non-numeric _guid fails without an exception");
			check(anyContains(errors, "/JGGame/Misc/ZeroGuid.jgasset : JGAsset._guid"), "guid: zero (invalid) GUID fails");
			check(anyContains(errors, "/JGGame/Misc/WideGuid.jgasset : JGAsset._guid"), "guid: out-of-range part fails");
			fixture.Cleanup();
		}
	}

	void testFormat()
	{
		HList<PString> errors;
		HList<PString> warnings;
		HCookFixture fixture(fixtureRoot("format"));
		writeGoodSet(fixture);
		fixture.Write(1, "Misc/Broken.jgasset", "{\n    \"JGObjectType\": \"JGTexture\",\n    \"JGObject\": {\n");
		fixture.Write(1, "Misc/NoType.jgasset", "{ \"JGObject\": { \"JGAsset\": { \"_guid\": \"7-1-2-3-4-5-6-7-8-9-10\" } } }");
		fixture.Write(1, "Misc/NoObject.jgasset", "{ \"JGObjectType\": \"JGTexture\" }");
		fixture.Write(1, "Misc/notes.txt", "not an asset, copied as is");
		fixture.WriteOutside("Stray.jgasset", "{}");
		const bool bOk = fixture.Run(errors, warnings);
		logLines("format", errors, warnings);
		check(bOk == false && anyContains(errors, "/JGGame/Misc/Broken.jgasset : JSON line "), "format: parse error fails with line and column");
		check(anyContains(errors, "/JGGame/Misc/NoType.jgasset : JGObjectType is missing"), "format: missing JGObjectType fails");
		check(anyContains(errors, "/JGGame/Misc/NoObject.jgasset : JGObject is missing"), "format: missing JGObject fails");
		check(anyContains(errors, "Stray.jgasset : file is outside the content mounts"), "format: file outside the mounts fails");
		check(anyContains(errors, "notes.txt") == false, "format: non-asset files are not parsed");
		fixture.Cleanup();
	}

	void testDataTableErrors()
	{
		HList<PString> errors;
		HList<PString> warnings;
		{
			// 키 중복 — datatable.validate 와 같이 오류(저장을 막는 문제)는 쿠킹도 막는다
			HCookFixture fixture(fixtureRoot("duplicate_key"));
			writeGoodSet(fixture, "/JGGame/Meshes/Box.jgasset", "/JGGame/Data/Other", "Item_A");
			const bool bOk = fixture.Run(errors, warnings);
			logLines("duplicate_key", errors, warnings);
			check(bOk == false && anyContains(errors, "/JGGame/Data/Items.jgasset : data table row 2 'Item_A'") && anyContains(errors, "duplicate"),
				"data table: duplicate key fails with row and key");
			fixture.Cleanup();
		}
		{
			HCookFixture fixture(fixtureRoot("table_format"));
			writeGoodSet(fixture);
			fixture.Write(1, "Data/Wrong.jgasset", "{ \"JGObjectType\": \"JGDataTable\", \"JGObject\": { \"JGAsset\": { \"_guid\": \"7-11-2-3-4-5-6-7-8-9-10\" }, \"JGDataTable\": { \"FormatVersion\": 99 } } }");
			const bool bOk = fixture.Run(errors, warnings);
			logLines("table_format", errors, warnings);
			check(bOk == false && anyContains(errors, "/JGGame/Data/Wrong.jgasset : data table"), "data table: unreadable table fails");
			fixture.Cleanup();
		}
	}

	// 엔진 Content 의 파일(폴더 항목은 뺀다 — FileListInDirectory 는 폴더도 돌려준다). bWithTempAsset 이 false 면 쿠킹 기본 제외(RawResources · TempAsset)를 뺀다
	HList<PString> engineContentFiles(bool bWithTempAsset, int32* outAssetCount)
	{
		HList<PString> allFiles;
		HFileHelper::FileListInDirectory(HFileHelper::EngineContentDirectory(), &allFiles, true);

		HList<PString> files;
		*outAssetCount = 0;
		for (const PString& file : allFiles)
		{
			std::error_code errCode;
			if (fs::is_regular_file(fs::path(file.GetRawString()), errCode) == false)
			{
				continue;
			}
			const HRawString path = fs::path(file.GetRawString()).generic_string();
			if (path.find("/RawResources/") != HRawString::npos || (bWithTempAsset == false && path.find("/TempAsset/") != HRawString::npos))
			{
				continue;
			}
			files.push_back(file);
			if (fs::path(path).extension() == JG_ASSET_FORMAT)
			{
				++(*outAssetCount);
			}
		}
		return files;
	}

	bool validateEngineContent(const char* inCase, bool bWithTempAsset)
	{
		int32 assetCount = 0;
		const HList<PString> files = engineContentFiles(bWithTempAsset, &assetCount);

		HList<PString> errors;
		HList<PString> warnings;
		const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		const bool bOk = ValidateContentForCook(files, { HPair<PString, PString>(JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN, HFileHelper::EngineContentDirectory()) }, errors, warnings);
		const int64 elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
		logLines(inCase, errors, warnings);
		JG_LOG(CookCheck, ELogLevel::Info, "cookcheck.selftest [%s] %d file(s), %d asset(s), %d error(s), %d warning(s), %d ms",
			PString(inCase), (int32)files.size(), assetCount, (int32)errors.size(), (int32)warnings.size(), (int32)elapsedMs);
		return bOk && errors.empty();
	}

	// 실제 엔진 Content — 쿠킹 기본 제외(RawResources · TempAsset)를 빼고 통과해야 한다.
	// TempAsset(Devkit 시험 메시, 36MB)도 넣어 실제 큰 에셋 하나를 읽어 본다(그 자체는 쿠킹 대상이 아니다)
	void testEngineContent()
	{
		check(validateEngineContent("engine_content", false), "engine content: passes without RawResources and TempAsset");
		check(validateEngineContent("engine_content_with_tempasset", true), "engine content: the 36MB TempAsset mesh also passes");
	}

	bool executeSelfTest(const HConsoleCommandArgs& args)
	{
		GCheckCount   = 0;
		GFailureCount = 0;

		testGood();
		testBrokenReferences();
		testGuids();
		testFormat();
		testDataTableErrors();
		testEngineContent();

		std::error_code errCode;
		fs::remove(fs::path(fixtureRoot("")), errCode);

		if (GFailureCount == 0)
		{
			JG_LOG(CookCheck, ELogLevel::Info, "cookcheck.selftest: OK (%d/%d)", GCheckCount, GCheckCount);
			return true;
		}
		JG_LOG(CookCheck, ELogLevel::Error, "cookcheck.selftest: FAILED (%d/%d)", GCheckCount - GFailureCount, GCheckCount);
		return false;
	}

	HAutoConsoleCommand CookCheckSelfTestCommand(
		"cookcheck.selftest",
		"cookcheck.selftest",
		"Run the cook content check self tests (parse, GUID, references, data tables) on temporary fixtures and the engine Content",
		&executeSelfTest);
}
