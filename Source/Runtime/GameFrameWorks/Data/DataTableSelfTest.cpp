#include "PCH/PCH.h"
#include "Data/DataTableView.h"
#include "Data/DataTableDocument.h"
#include "Data/DataTableClipboard.h"
#include "Data/DataTableAssets.h"
#include "JGGraphicsDefine.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include <chrono>
#include <thread>

// datatable.selftest — 데이터 테이블 자체 검사. GPU · 에셋 DB 없이 돈다(JGConsole 과 에디터 콘솔 양쪽).
//   값 · 텍스트 · JSON 왕복, 읽기 문제, 검증, C++ 바인딩, 편집 · 실행 취소, TSV · 붙여넣기, 파일 저장 · 다시 읽기 · 바깥 수정, 1만 행 시간
// 결과는 JG_LOG(DataTable): 실패마다 Error 한 줄, 끝에 "datatable.selftest: OK (통과/전체)".
// datatable.validate — 데이터 테이블 파일을 읽어 검증한다(경로를 주면 그 파일, 없으면 두 Content 의 모든 데이터 테이블). 오류가 있으면 실패.
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
			JG_LOG(DataTable, ELogLevel::Error, "datatable.selftest FAILED: %s", PString(inCaseName));
		}
	}

	PString describeValue(const HDataTableValue& inValue)
	{
		return PString::Format("kind %d '%s'", (int32)inValue.GetKind(), HDataTableColumn("V", EDataTableColumnType::String).FormatText(inValue));
	}

	// 두 내용의 첫 차이(실패 로그용). 같으면 빈 글
	PString describeDifference(const HDataTableContent& a, const HDataTableContent& b)
	{
		if (a.Schema.KeyColumn != b.Schema.KeyColumn)
		{
			return PString::Format("key column '%s' vs '%s'", a.Schema.KeyColumn, b.Schema.KeyColumn);
		}
		if (a.Schema.Columns.size() != b.Schema.Columns.size())
		{
			return PString::Format("column count %d vs %d", (int32)a.Schema.Columns.size(), (int32)b.Schema.Columns.size());
		}
		for (int32 c = 0; c < (int32)a.Schema.Columns.size(); ++c)
		{
			const HDataTableColumn& x = a.Schema.Columns[c];
			const HDataTableColumn& y = b.Schema.Columns[c];
			if (x != y)
			{
				return PString::Format("column %d '%s': type %d/%d default %s/%s desc '%s'/'%s' min %d/%d max %d/%d enum %d/%d enumType '%s'/'%s' class '%s'/'%s' table '%s'/'%s'",
					c, x.Name, (int32)x.Type, (int32)y.Type, describeValue(x.Default), describeValue(y.Default), x.Description, y.Description,
					x.bHasMin ? 1 : 0, y.bHasMin ? 1 : 0, x.bHasMax ? 1 : 0, y.bHasMax ? 1 : 0, (int32)x.EnumValues.size(), (int32)y.EnumValues.size(),
					x.EnumType, y.EnumType, x.AssetClass, y.AssetClass, x.Table, y.Table);
			}
		}
		if (a.Rows.size() != b.Rows.size())
		{
			return PString::Format("row count %d vs %d", (int32)a.Rows.size(), (int32)b.Rows.size());
		}
		for (int32 r = 0; r < (int32)a.Rows.size(); ++r)
		{
			if (a.Rows[r].Key != b.Rows[r].Key)
			{
				return PString::Format("row %d key '%s' vs '%s'", r, a.Rows[r].Key, b.Rows[r].Key);
			}
			for (int32 c = 0; c < (int32)a.Rows[r].Values.size() && c < (int32)b.Rows[r].Values.size(); ++c)
			{
				if (a.Rows[r].Values[c] != b.Rows[r].Values[c])
				{
					return PString::Format("row %d column %d: %s vs %s", r, c, describeValue(a.Rows[r].Values[c]), describeValue(b.Rows[r].Values[c]));
				}
			}
		}
		return PString();
	}

	void checkSameContent(const HDataTableContent& a, const HDataTableContent& b, const char* inCaseName)
	{
		const bool bSame = (a == b);
		check(bSame, inCaseName);
		if (bSame == false)
		{
			JG_LOG(DataTable, ELogLevel::Error, "datatable.selftest   difference: %s", describeDifference(a, b));
		}
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

	// 열 7종 · 한글 · 따옴표 · 줄바꿈 · 앞뒤 공백 · 0.1 같은 실수가 든 표본
	HDataTableContent makeSampleContent()
	{
		HDataTableContent content;
		content.Schema.KeyColumn = "Id";

		HDataTableColumn count = makeColumn("Count", EDataTableColumnType::Int);
		count.bHasMin = true;
		count.Min     = 0.0;
		count.bHasMax = true;
		count.Max     = 100.0;
		HDataTableColumn rate = makeColumn("Rate", EDataTableColumnType::Float);
		rate.Default = HDataTableValue::MakeFloat(1.0);
		HDataTableColumn enabled = makeColumn("Enabled", EDataTableColumnType::Bool);
		enabled.Default = HDataTableValue::MakeBool(true);
		HDataTableColumn label = makeColumn("Label", EDataTableColumnType::String);
		label.Description = "표시 이름";
		HDataTableColumn kind = makeColumn("Kind", EDataTableColumnType::Enum);
		kind.EnumValues = { "A", "B", "C" };
		kind.Default    = HDataTableValue::MakeText("A");
		HDataTableColumn mesh = makeColumn("Mesh", EDataTableColumnType::AssetRef);
		mesh.AssetClass = "JGStaticMesh";
		HDataTableColumn next = makeColumn("Next", EDataTableColumnType::RowRef);

		content.Schema.Columns = { count, rate, enabled, label, kind, mesh, next };

		content.Rows.push_back(makeRow("Row_A", { HDataTableValue::MakeInt(3), HDataTableValue::MakeFloat(1.5), HDataTableValue::MakeBool(true),
			HDataTableValue::MakeText("첫 행"), HDataTableValue::MakeText("B"), HDataTableValue::MakeText("/JGEngine/Meshes/Box"), HDataTableValue::MakeText("Row_B") }));
		content.Rows.push_back(makeRow("Row_B", { HDataTableValue::MakeInt(0), HDataTableValue::MakeFloat(1.0), HDataTableValue::MakeBool(true),
			HDataTableValue::MakeText("둘째 \"따옴표\"\n줄바꿈\t탭"), HDataTableValue::MakeText("A"), HDataTableValue::MakeText(""), HDataTableValue::MakeText("") }));
		content.Rows.push_back(makeRow("Row_C", { HDataTableValue::MakeInt(12), HDataTableValue::MakeFloat(0.1), HDataTableValue::MakeBool(false),
			HDataTableValue::MakeText("  공백 유지  "), HDataTableValue::MakeText("C"), HDataTableValue::MakeText(""), HDataTableValue::MakeText("Row_A") }));
		return content;
	}

	PSharedPtr<JGDataTable> makeTable(const HDataTableContent& inContent)
	{
		PSharedPtr<JGDataTable> table = Allocate<JGDataTable>();
		table->InitializeNew("/JGEngine/DataTableSelfTest/Sample", inContent);
		return table;
	}

	bool parse(const HDataTableColumn& inColumn, const char* inText, HDataTableValue* outValue)
	{
		return inColumn.ParseText(PString(inText), outValue, nullptr);
	}

	// 셀프 테스트용 파일 자리(엔진 Temp). Content 밖이라 에셋 DB 가 올리지 않는다
	PString tempFilePath(const char* inName)
	{
		PString directory;
		HFileHelper::CombinePath(HFileHelper::EngineTempDirectory(), "DataTableSelfTest", &directory);
		std::error_code errCode;
		fs::create_directories(fs::path(directory.GetRawString()), errCode);

		PString path;
		HFileHelper::CombinePath(directory, inName, &path);
		return path;
	}

	class HFakeResolver : public IDataTableReferenceResolver
	{
	public:
		virtual bool GetTableKeys(const PString& inTablePath, HList<PString>* outKeys) const override
		{
			if (inTablePath == "/JGGame/Data/Other")
			{
				if (outKeys != nullptr)
				{
					*outKeys = { "X1", "X2" };
				}
				return true;
			}
			return false;
		}

		virtual bool HasAsset(const PString& inAssetPath) const override
		{
			return inAssetPath == "/JGEngine/Meshes/Box";
		}
	};

	// 이 검사용 행 구조체. 지원하는 멤버 타입 전부
	struct HSelfTestRow
	{
		int32              Count   = -1;
		int64              Count64 = -1;
		float32            Rate    = -1.0f;
		float64            Rate64  = -1.0;
		bool               Enabled = false;
		PString            Label;
		PName              Kind;
		HAssetPath         Mesh;
		PName              Next;
		ETextureFilterMode Filter  = ETextureFilterMode::Point;

		static void BindColumns(HDataTableRowBinder<HSelfTestRow>& binder)
		{
			binder.Bind("Count", &HSelfTestRow::Count);
			binder.Bind("Count", &HSelfTestRow::Count64);
			binder.Bind("Rate", &HSelfTestRow::Rate);
			binder.Bind("Count", &HSelfTestRow::Rate64);   // Int 열을 실수 멤버로
			binder.Bind("Enabled", &HSelfTestRow::Enabled);
			binder.Bind("Label", &HSelfTestRow::Label);
			binder.Bind("Kind", &HSelfTestRow::Kind);
			binder.Bind("Mesh", &HSelfTestRow::Mesh);
			binder.Bind("Next", &HSelfTestRow::Next);
			binder.Bind("Filter", &HSelfTestRow::Filter);
		}
	};

	struct HMissingColumnRow
	{
		int32 Missing = 0;

		static void BindColumns(HDataTableRowBinder<HMissingColumnRow>& binder)
		{
			binder.Bind("NoSuchColumn", &HMissingColumnRow::Missing);
		}
	};

	struct HWrongTypeRow
	{
		bool Label = false;

		static void BindColumns(HDataTableRowBinder<HWrongTypeRow>& binder)
		{
			binder.Bind("Label", &HWrongTypeRow::Label);   // String 열을 bool 로
		}
	};

	// 결정론 검사용 간단한 난수(LCG)
	struct HTestRandom
	{
		uint64 State = 0x2545F4914F6CDD1Dull;

		uint32 Next()
		{
			State = State * 6364136223846793005ull + 1442695040888963407ull;
			return (uint32)(State >> 33);
		}

		int32 Range(int32 inCount)
		{
			return (inCount <= 0) ? 0 : (int32)(Next() % (uint32)inCount);
		}
	};

	void testValues()
	{
		HDataTableValue value;

		const HDataTableColumn boolColumn = makeColumn("B", EDataTableColumnType::Bool);
		check(parse(boolColumn, "TRUE", &value) && value.GetBool() == true, "value: Bool accepts TRUE");
		check(parse(boolColumn, " false ", &value) && value.GetBool() == false, "value: Bool trims and accepts false");
		check(parse(boolColumn, "1", &value) && value.GetBool() == true, "value: Bool accepts 1");
		check(parse(boolColumn, "yes", &value) == false, "value: Bool rejects yes");

		const HDataTableColumn intColumn = makeColumn("I", EDataTableColumnType::Int);
		check(parse(intColumn, "42", &value) && value.GetInt() == 42, "value: Int 42");
		check(parse(intColumn, "-7", &value) && value.GetInt() == -7, "value: Int -7");
		check(parse(intColumn, "+3", &value) && value.GetInt() == 3, "value: Int +3");
		check(parse(intColumn, "1.5", &value) == false, "value: Int rejects 1.5");
		check(parse(intColumn, "abc", &value) == false, "value: Int rejects abc");
		check(parse(intColumn, "", &value) == false, "value: Int rejects empty");
		check(parse(intColumn, "99999999999999999999", &value) == false, "value: Int rejects overflow");
		check(intColumn.FormatText(HDataTableValue::MakeInt(-123)) == "-123", "value: Int formats -123");

		const HDataTableColumn floatColumn = makeColumn("F", EDataTableColumnType::Float);
		check(parse(floatColumn, "1.5", &value) && value.GetFloat() == 1.5, "value: Float 1.5");
		check(parse(floatColumn, "2", &value) && value.GetFloat() == 2.0, "value: Float accepts integer text");
		check(parse(floatColumn, "1e3", &value) && value.GetFloat() == 1000.0, "value: Float 1e3");
		check(parse(floatColumn, "nan", &value) == false && parse(floatColumn, "inf", &value) == false, "value: Float rejects nan / inf");
		check(floatColumn.FormatText(HDataTableValue::MakeFloat(1.0)) == "1.0", "value: Float 1.0 formats as 1.0");
		check(floatColumn.FormatText(HDataTableValue::MakeFloat(1.5)) == "1.5", "value: Float 1.5 formats as 1.5");
		{
			HDataTableValue roundTrip;
			const PString text = floatColumn.FormatText(HDataTableValue::MakeFloat(0.1));
			check(floatColumn.ParseText(text, &roundTrip, nullptr) && roundTrip == HDataTableValue::MakeFloat(0.1), "value: Float 0.1 text round trip keeps the bits");
		}

		const HDataTableColumn stringColumn = makeColumn("S", EDataTableColumnType::String);
		check(parse(stringColumn, "  keep  ", &value) && value.GetText() == "  keep  ", "value: String keeps spaces");

		HDataTableColumn enumColumn = makeColumn("E", EDataTableColumnType::Enum);
		enumColumn.EnumValues = { "Alpha", "Beta" };
		check(parse(enumColumn, "beta", &value) && value.GetText() == "Beta", "value: Enum is case-insensitive and canonical");
		check(parse(enumColumn, "Gamma", &value) == false, "value: Enum rejects a name not in the list");

		HDataTableColumn reflectedEnumColumn = makeColumn("RE", EDataTableColumnType::Enum);
		reflectedEnumColumn.EnumType = "ETextureFilterMode";
		HList<PString> names;
		check(reflectedEnumColumn.GetEnumNames(names) && names.size() == 3 && names[1] == "Linear", "value: EnumType lists reflected enum names");

		const HDataTableColumn assetColumn = makeColumn("A", EDataTableColumnType::AssetRef);
		check(parse(assetColumn, "/JGGame/Meshes/Box", &value) && value.GetText() == "/JGGame/Meshes/Box", "value: AssetRef token path");
		check(parse(assetColumn, "", &value) && value.GetText().Empty(), "value: AssetRef empty is none");
		check(parse(assetColumn, "Meshes/Box", &value) == false, "value: AssetRef rejects a path without token");

		const HDataTableColumn rowColumn = makeColumn("R", EDataTableColumnType::RowRef);
		check(parse(rowColumn, "Row_A", &value) && value.GetText() == "Row_A", "value: RowRef key");
		check(parse(rowColumn, "Row A", &value) == false, "value: RowRef rejects a key with a space");

		EDataTableColumnType type = EDataTableColumnType::Bool;
		check(DataTableColumnTypeFromString("assetref", &type) && type == EDataTableColumnType::AssetRef, "type: names are case-insensitive");
		check(DataTableColumnTypeFromString("Vector3", &type) == false, "type: unknown name fails");
		check(DataTableToLowerAscii("ABC한글") == "abc한글", "text: ASCII lower keeps UTF-8 bytes");

		HDataTableValue converted;
		check(intColumn.ConvertFrom(HDataTableValue::MakeFloat(3.0), EDataTableColumnType::Float, &converted) && converted.GetInt() == 3, "convert: Float 3.0 -> Int 3");
		check(intColumn.ConvertFrom(HDataTableValue::MakeFloat(3.5), EDataTableColumnType::Float, &converted) == false, "convert: Float 3.5 -> Int fails");
		check(stringColumn.ConvertFrom(HDataTableValue::MakeBool(true), EDataTableColumnType::Bool, &converted) && converted.GetText() == "true", "convert: Bool -> String");
		check(floatColumn.ConvertFrom(HDataTableValue::MakeText("2.5"), EDataTableColumnType::String, &converted) && converted.GetFloat() == 2.5, "convert: String -> Float");
	}

	void testJsonRoundTrip()
	{
		const HDataTableContent content = makeSampleContent();
		PSharedPtr<JGDataTable> table = makeTable(content);

		PString text;
		check(table->ToJsonText(&text), "json: table to text");
		check(text.Contains("\"JGObjectType\": \"JGDataTable\""), "json: shell has JGObjectType");

		PString error;
		PSharedPtr<JGDataTable> loaded = JGDataTable::FromJsonText(text, &error);
		check(loaded != nullptr, "json: text to table");
		if (loaded == nullptr)
		{
			return;
		}
		checkSameContent(loaded->GetContent(), content, "json: content round trip is equal");
		check(loaded->GetGuid() == table->GetGuid(), "json: guid round trip");
		check(loaded->GetLoadIssues().empty(), "json: clean file has no load issues");

		PString secondText;
		check(loaded->ToJsonText(&secondText) && secondText == text, "json: saving again gives the same bytes");

		check(loaded->FindRowIndex(PName("Row_C")) == 2 && loaded->FindRowIndex(PString("Row_X")) == -1, "json: key index");
		check(loaded->ComputeContentHash() == table->ComputeContentHash(), "json: hash round trip");

		// BOM 이 붙은 파일(메모장)도 읽힌다
		PString bomText = PString("\xEF\xBB\xBF") + text;
		check(JGDataTable::FromJsonText(bomText, &error) != nullptr, "json: UTF-8 BOM is skipped");

		// 문법 오류는 줄 · 칸과 함께 실패한다
		PString broken = text;
		broken.ReplaceAll("\"Rows\": [", "\"Rows\": [ ,");
		PSharedPtr<JGDataTable> brokenTable = JGDataTable::FromJsonText(broken, &error);
		check(brokenTable == nullptr && error.Contains("line "), "json: syntax error reports a line");

		check(JGDataTable::FromJsonText("{ \"JGObjectType\": \"JGTexture\", \"JGObject\": {} }", &error) == nullptr, "json: another asset type is rejected");
	}

	void testLoadIssues()
	{
		// 모르는 키 · 빠진 값 · 타입이 다른 값 · 숫자 키
		const char* text =
			"{ \"JGObjectType\": \"JGDataTable\", \"JGObject\": { \"JGObject\": { \"Name\": \"JGDataTable\" }, \"JGDataTable\": {"
			" \"FormatVersion\": 1, \"KeyColumn\": \"Id\","
			" \"Columns\": [ { \"Name\": \"Count\", \"Type\": \"Int\", \"Default\": 5 }, { \"Name\": \"Rate\", \"Type\": \"Float\", \"Default\": 2 } ],"
			" \"Rows\": [ { \"Id\": \"R1\", \"Count\": 1, \"Rate\": 3, \"Extra\": true },"
			"            { \"Id\": 101, \"Count\": \"many\" },"
			"            { \"Count\": 2, \"Rate\": 0.5 } ] } } }";

		PString error;
		PSharedPtr<JGDataTable> table = JGDataTable::FromJsonText(text, &error);
		check(table != nullptr, "load: table with issues still loads");
		if (table == nullptr)
		{
			return;
		}

		const HList<HDataTableIssue>& issues = table->GetLoadIssues();
		bool bUnknown = false;
		bool bMissing = false;
		bool bWrongType = false;
		bool bNumberKey = false;
		bool bNoKey = false;
		for (const HDataTableIssue& issue : issues)
		{
			bUnknown   |= issue.Message.Contains("'Extra'");
			bMissing   |= issue.Message.Contains("have no 'Rate'");
			bWrongType |= issue.Message.Contains("that is not Int");
			bNumberKey |= issue.Message.Contains("is a number");
			bNoKey     |= (issue.Severity == EDataTableIssueSeverity::Error && issue.Message.Contains("no key"));
		}
		check(bUnknown, "load: unknown key is reported");
		check(bMissing, "load: missing value is reported");
		check(bWrongType, "load: wrong-type value is reported");
		check(bNumberKey && table->GetRowKey(1) == "101", "load: number key becomes text");
		check(bNoKey, "load: row without key is an error");
		check(table->GetValue(0, 1).GetFloat() == 3.0, "load: integer JSON in a Float column");
		check(table->GetValue(1, 0).GetInt() == 5 && table->GetValue(1, 1).GetFloat() == 2.0, "load: bad and missing values use the column default");
	}

	void testValidation()
	{
		HDataTableContent content = makeSampleContent();
		HFakeResolver resolver;
		HList<HDataTableIssue> issues;
		ValidateDataTableContent(content, &resolver, issues);
		check(issues.empty(), "validate: sample content is clean");

		content.Rows[1].Key = "Row_A";              // 중복
		content.Rows[2].Key = "Row C";              // 공백
		content.Rows[0].Values[0] = HDataTableValue::MakeInt(500);                 // 범위 밖
		content.Rows[0].Values[4] = HDataTableValue::MakeText("Z");                 // 목록 밖
		content.Rows[0].Values[5] = HDataTableValue::MakeText("/JGEngine/Missing"); // 없는 에셋
		content.Rows[0].Values[6] = HDataTableValue::MakeText("Row_Missing");       // 없는 행
		content.Schema.Columns[6].Table.Reset();

		HDataTableColumn other = makeColumn("Other", EDataTableColumnType::RowRef);
		other.Table = "/JGGame/Data/Other";
		content.Schema.Columns.push_back(other);
		for (HDataTableRow& row : content.Rows)
		{
			row.Values.push_back(HDataTableValue::MakeText("X1"));
		}
		content.Rows[2].Values[7] = HDataTableValue::MakeText("X9");

		issues.clear();
		ValidateDataTableContent(content, &resolver, issues);
		int32 errors = 0;
		bool bDuplicate = false, bSpace = false, bRange = false, bEnum = false, bAsset = false, bRow = false, bOtherRow = false;
		for (const HDataTableIssue& issue : issues)
		{
			errors     += (issue.Severity == EDataTableIssueSeverity::Error) ? 1 : 0;
			bDuplicate |= issue.Message.Contains("duplicate") && issue.Row == 1;
			bSpace     |= issue.Message.Contains("space") && issue.Row == 2;
			bRange     |= issue.Message.Contains("out of range") && issue.Row == 0 && issue.Column == 0;
			bEnum      |= issue.Message.Contains("enum list") && issue.Column == 4;
			bAsset     |= issue.Message.Contains("asset '/JGEngine/Missing'");
			bRow       |= issue.Message.Contains("row 'Row_Missing'");
			bOtherRow  |= issue.Message.Contains("row 'X9'") && issue.Column == 7;
		}
		check(errors == 2, "validate: duplicate and space keys are the only errors");
		check(bDuplicate && bSpace, "validate: key errors point at their rows");
		check(bRange && bEnum && bAsset && bRow && bOtherRow, "validate: value warnings (range, enum, asset, same-table row, other-table row)");
		check(HasDataTableErrors(issues), "validate: HasDataTableErrors");

		HDataTableContent schemaErrors = makeSampleContent();
		schemaErrors.Schema.Columns[1].Name = "Count";
		schemaErrors.Schema.Columns[2].Name = "Id";
		issues.clear();
		ValidateDataTableContent(schemaErrors, nullptr, issues);
		int32 schemaErrorCount = 0;
		for (const HDataTableIssue& issue : issues)
		{
			schemaErrorCount += (issue.Severity == EDataTableIssueSeverity::Error) ? 1 : 0;
		}
		check(schemaErrorCount == 2, "validate: duplicate column name and key-column name are errors");
	}

	void testBinding()
	{
		HDataTableContent content = makeSampleContent();
		HDataTableColumn filter = makeColumn("Filter", EDataTableColumnType::Enum);
		filter.EnumType = "ETextureFilterMode";
		filter.Default  = HDataTableValue::MakeText("Point");
		content.Schema.Columns.push_back(filter);
		content.Rows[0].Values.push_back(HDataTableValue::MakeText("Linear"));
		content.Rows[1].Values.push_back(HDataTableValue::MakeText("Anisotropic"));
		content.Rows[2].Values.push_back(HDataTableValue::MakeText("Point"));
		PSharedPtr<JGDataTable> table = makeTable(content);

		HDataTableView<HSelfTestRow> view;
		HList<HDataTableIssue> issues;
		check(view.Bind(table, &issues), "bind: all supported member types");
		check(view.GetCount() == 3 && view.GetKey(0) == PName("Row_A") && view.GetKey(2) == PName("Row_C"), "bind: rows keep file order");

		const HSelfTestRow* rowA = view.Find(PName("Row_A"));
		check(rowA != nullptr && rowA->Count == 3 && rowA->Count64 == 3 && rowA->Rate == 1.5f && rowA->Rate64 == 3.0 && rowA->Enabled, "bind: numbers and bool");
		check(rowA != nullptr && rowA->Label == "첫 행" && rowA->Kind == PName("B") && rowA->Next == PName("Row_B"), "bind: text, enum name, row key");
		check(rowA != nullptr && rowA->Mesh.GetAssetPath() == PName("/JGEngine/Meshes/Box.jgasset"), "bind: asset path");
		check(rowA != nullptr && rowA->Filter == ETextureFilterMode::Linear && view.Find(PName("Row_B"))->Filter == ETextureFilterMode::Anisotropic, "bind: reflected enum by name");
		check(view.Find(PName("Row_B")) != nullptr && view.Find(PName("Row_B"))->Next == PName(), "bind: empty row ref is none");
		check(view.Find(PName("Row_X")) == nullptr, "bind: unknown key is null");
		check(view.IsUpToDate(), "bind: up to date after bind");

		table->ApplyContent(content);
		check(view.IsUpToDate() == false, "bind: table change makes the view stale");

		HDataTableView<HMissingColumnRow> missingView;
		issues.clear();
		check(missingView.Bind(table, &issues) == false && issues.empty() == false && issues[0].Message.Contains("NoSuchColumn"), "bind: missing column fails with its name");

		HDataTableView<HWrongTypeRow> wrongView;
		issues.clear();
		check(wrongView.Bind(table, &issues) == false && issues.empty() == false && issues[0].Message.Contains("String"), "bind: wrong column type fails");

		HDataTableContent badEnum = content;
		badEnum.Rows[2].Values[7] = HDataTableValue::MakeText("Cubic");
		HDataTableView<HSelfTestRow> badEnumView;
		issues.clear();
		check(badEnumView.Bind(makeTable(badEnum), &issues) == false && badEnumView.GetCount() == 0, "bind: enum name missing in C++ fails and leaves the view empty");

		HDataTableContent overflow = content;
		overflow.Rows[0].Values[0] = HDataTableValue::MakeInt(5000000000ll);
		HDataTableView<HSelfTestRow> overflowView;
		issues.clear();
		check(overflowView.Bind(makeTable(overflow), &issues) == false, "bind: int32 overflow fails");
	}

	void testEdits()
	{
		const HDataTableContent original = makeSampleContent();
		PSharedPtr<PDataTableDocument> document = PDataTableDocument::FromContent(original);
		PString error;

		check(document->IsDirty() == false && document->CanUndo() == false, "edit: new document is clean");

		HDataTableCellAssignment cell;
		cell.Row    = 0;
		cell.Column = 0;
		cell.Value  = HDataTableValue::MakeInt(77);
		check(document->Apply({ HDataTableEdit::MakeSetCells({ cell }) }, "Set Count", &error), "edit: set cell");
		check(document->GetContent().Rows[0].Values[0].GetInt() == 77 && document->IsDirty(), "edit: cell changed and dirty");

		// 종류가 다른 값 · 범위 밖은 실패하고 아무것도 바꾸지 않는다
		HDataTableCellAssignment badCell = cell;
		badCell.Value = HDataTableValue::MakeText("77");
		const uint64 revision = document->GetRevision();
		check(document->Apply({ HDataTableEdit::MakeSetCells({ badCell }) }, "Bad", &error) == false && document->GetRevision() == revision, "edit: wrong kind is rejected");

		// 묶음 중 하나가 실패하면 앞의 것도 되돌린다
		HDataTableCellAssignment goodCell = cell;
		goodCell.Value = HDataTableValue::MakeInt(1);
		HDataTableCellAssignment outOfRange = cell;
		outOfRange.Row = 99;
		check(document->Apply({ HDataTableEdit::MakeSetCells({ goodCell }), HDataTableEdit::MakeSetCells({ outOfRange }) }, "Mixed", &error) == false
			&& document->GetContent().Rows[0].Values[0].GetInt() == 77, "edit: failed batch rolls back");

		// 행 · 열 · 키 편집
		check(document->Apply({ HDataTableEdit::MakeSetKeys({ HPair<int32, PString>(1, "Row_B2") }) }, "Rename", &error)
			&& document->GetContent().Rows[1].Key == "Row_B2", "edit: set key");
		HDataTableRow newRow = document->GetContent().MakeDefaultRow("Row_New");
		check(document->Apply({ HDataTableEdit::MakeInsertRows({ HPair<int32, HDataTableRow>(1, newRow) }) }, "Insert", &error)
			&& document->GetContent().Rows[1].Key == "Row_New" && document->GetContent().GetRowCount() == 4, "edit: insert row");
		check(document->Apply({ HDataTableEdit::MakeRemoveRows({ 0, 2 }) }, "Remove", &error)
			&& document->GetContent().GetRowCount() == 2 && document->GetContent().Rows[0].Key == "Row_New", "edit: remove rows");
		check(document->Apply({ HDataTableEdit::MakeReorderRows({ 1, 0 }) }, "Reorder", &error)
			&& document->GetContent().Rows[0].Key == "Row_C", "edit: reorder rows");
		check(document->Apply({ HDataTableEdit::MakeInsertColumn(1, makeColumn("Extra", EDataTableColumnType::Int)) }, "Add column", &error)
			&& document->GetContent().Schema.Columns[1].Name == "Extra" && document->GetContent().Rows[0].Values.size() == 8, "edit: insert column");
		check(document->Apply({ HDataTableEdit::MakeMoveColumn(1, 7) }, "Move column", &error)
			&& document->GetContent().Schema.Columns[7].Name == "Extra", "edit: move column");

		HDataTableColumn rateAsInt = document->GetContent().Schema.Columns[1];
		check(rateAsInt.Name == "Rate", "edit: rate column position");
		rateAsInt.Type    = EDataTableColumnType::Int;
		rateAsInt.Default = HDataTableValue::MakeInt(0);
		HList<int32> failedRows;
		const HList<HDataTableValue> converted = ConvertDataTableColumnValues(document->GetContent(), 1, rateAsInt, &failedRows);
		check(failedRows.size() == 1 && failedRows[0] == 0, "edit: Float -> Int converts 1.0 and fails only 0.1");
		check(document->Apply({ HDataTableEdit::MakeReplaceColumn(1, rateAsInt, converted) }, "Change type", &error)
			&& document->GetContent().Schema.Columns[1].Type == EDataTableColumnType::Int, "edit: replace column (type change)");
		check(document->Apply({ HDataTableEdit::MakeRemoveColumn(7) }, "Remove column", &error) && document->GetContent().GetColumnCount() == 7, "edit: remove column");
		check(document->Apply({ HDataTableEdit::MakeSetKeyColumn("Key") }, "Key column", &error) && document->GetContent().Schema.KeyColumn == "Key", "edit: key column name");

		const HDataTableContent edited = document->GetContent();
		int32 undoCount = 0;
		while (document->Undo())
		{
			++undoCount;
		}
		check(undoCount == 10 && document->GetContent() == original && document->IsDirty() == false, "edit: undo all returns to the original");
		while (document->Redo())
		{
		}
		check(document->GetContent() == edited && document->IsDirty(), "edit: redo all returns to the edited content");
		check(document->GetUndoLabel() == "Key column", "edit: undo label");

		// 무작위 편집 300번 → 전부 되돌리면 원본(지문 · 내용)
		PSharedPtr<PDataTableDocument> randomDocument = PDataTableDocument::FromContent(original);
		HTestRandom random;
		int32 applied = 0;
		for (int32 i = 0; i < 300; ++i)
		{
			const HDataTableContent& current = randomDocument->GetContent();
			const int32 rowCount = current.GetRowCount();
			const int32 columnCount = current.GetColumnCount();
			HDataTableEdit edit;
			switch (random.Range(6))
			{
			case 0:
				if (rowCount > 0 && columnCount > 0)
				{
					HDataTableCellAssignment randomCell;
					randomCell.Row    = random.Range(rowCount);
					randomCell.Column = random.Range(columnCount);
					randomCell.Value  = current.Schema.Columns[randomCell.Column].Default;
					if (current.Schema.Columns[randomCell.Column].Type == EDataTableColumnType::Int)
					{
						randomCell.Value = HDataTableValue::MakeInt(random.Range(1000));
					}
					edit = HDataTableEdit::MakeSetCells({ randomCell });
				}
				break;
			case 1:
				edit = HDataTableEdit::MakeInsertRows({ HPair<int32, HDataTableRow>(random.Range(rowCount + 1), current.MakeDefaultRow(current.MakeUniqueKey("Rnd"))) });
				break;
			case 2:
				if (rowCount > 1)
				{
					edit = HDataTableEdit::MakeRemoveRows({ random.Range(rowCount) });
				}
				break;
			case 3:
				if (rowCount > 1)
				{
					HList<int32> order;
					for (int32 r = rowCount - 1; r >= 0; --r)
					{
						order.push_back(r);
					}
					edit = HDataTableEdit::MakeReorderRows(order);
				}
				break;
			case 4:
				if (columnCount > 1)
				{
					edit = HDataTableEdit::MakeMoveColumn(random.Range(columnCount), random.Range(columnCount));
				}
				break;
			default:
				if (rowCount > 0)
				{
					edit = HDataTableEdit::MakeSetKeys({ HPair<int32, PString>(random.Range(rowCount), current.MakeUniqueKey("K")) });
				}
				break;
			}
			if (randomDocument->Apply({ edit }, "Random", &error))
			{
				++applied;
			}
		}
		const uint64 editedHash = ComputeDataTableContentHash(randomDocument->GetContent());
		while (randomDocument->Undo())
		{
		}
		check(applied > 200 && randomDocument->GetContent() == original, "edit: 300 random edits undo back to the original");
		while (randomDocument->Redo())
		{
		}
		check(ComputeDataTableContentHash(randomDocument->GetContent()) == editedHash, "edit: redo of random edits gives the same hash");
	}

	void testClipboard()
	{
		HDataTableTextGrid cells = { { "a", "b\tc" }, { "줄\n바꿈", "\"q\"" } };
		const PString tsv = HDataTableClipboard::EncodeTsv(cells);
		check(tsv == "a\t\"b\tc\"\r\n\"줄\n바꿈\"\t\"\"\"q\"\"\"\r\n", "tsv: encode quotes tab, newline and quote cells");
		HDataTableTextGrid decoded;
		check(HDataTableClipboard::DecodeTsv(tsv, &decoded) && decoded == cells, "tsv: decode round trip");

		check(HDataTableClipboard::DecodeTsv("1\t2\r\n3\t4\r\n", &decoded) && decoded.size() == 2 && decoded[1][1] == "4", "tsv: Excel 2x2 with trailing CRLF");
		check(HDataTableClipboard::DecodeTsv("x", &decoded) && decoded.size() == 1 && decoded[0].size() == 1 && decoded[0][0] == "x", "tsv: single cell without newline");
		check(HDataTableClipboard::DecodeTsv("a\tb\nc", &decoded) && decoded.size() == 2 && decoded[1].size() == 2 && decoded[1][1].Empty(), "tsv: ragged rows are padded");
		check(HDataTableClipboard::DecodeTsv("\"multi\r\nline\"\tz\r\n", &decoded) && decoded.size() == 1 && decoded[0][0] == "multi\r\nline", "tsv: quoted CRLF stays in the cell");
		check(HDataTableClipboard::DecodeTsv("", &decoded) == false, "tsv: empty text fails");

		const HDataTableContent content = makeSampleContent();
		const HList<int32> fileOrder = { 0, 1, 2 };
		HList<HDataTableEdit> edits;
		HList<PString> errors;

		// 기존 행 2x2 (Count, Rate)
		HDataTableGridRange range;
		range.FirstViewRow    = 0;
		range.FirstGridColumn = 1;
		check(PlanDataTablePaste(content, fileOrder, true, range, { { "10", "0.5" }, { "20", "2" } }, &edits, &errors) && edits.size() == 1 && edits[0].Cells.size() == 4, "paste: 2x2 over existing rows");

		// 한 칸을 3x2 선택에 채우기
		range.RowCount    = 3;
		range.ColumnCount = 1;
		check(PlanDataTablePaste(content, fileOrder, true, range, { { "9" } }, &edits, &errors) && edits.size() == 1 && edits[0].Cells.size() == 3, "paste: one cell fills the selection");

		// 키 열부터 아래로 넘치면 새 행
		range.FirstViewRow    = 2;
		range.FirstGridColumn = 0;
		range.RowCount        = 1;
		range.ColumnCount     = 1;
		check(PlanDataTablePaste(content, fileOrder, true, range, { { "Row_C", "1" }, { "Row_D", "2" }, { "Row_E", "3" } }, &edits, &errors)
			&& edits.size() == 3 && edits[2].Type == EDataTableEditType::InsertRows && edits[2].Rows.size() == 2 && edits[2].Rows[0].first == 3 && edits[2].Rows[1].second.Values[0].GetInt() == 3,
			"paste: block from the key column appends rows");

		// 키 열이 아니면 넘칠 때 거부, 정렬 · 필터 중에도 거부
		range.FirstGridColumn = 1;
		errors.clear();
		check(PlanDataTablePaste(content, fileOrder, true, range, { { "1" }, { "2" } }, &edits, &errors) == false && errors.empty() == false, "paste: overflow without key column is rejected");
		range.FirstGridColumn = 0;
		errors.clear();
		check(PlanDataTablePaste(content, fileOrder, false, range, { { "Row_C" }, { "Row_F" } }, &edits, &errors) == false, "paste: no new rows in a sorted or filtered view");

		// 타입 오류 → 전체 거부 · 열 넘침 → 거부
		range.FirstViewRow    = 0;
		range.FirstGridColumn = 1;
		errors.clear();
		check(PlanDataTablePaste(content, fileOrder, true, range, { { "abc" } }, &edits, &errors) == false && edits.empty() && errors[0].Contains("Count"), "paste: a type error rejects the whole paste");
		range.FirstGridColumn = 7;
		errors.clear();
		check(PlanDataTablePaste(content, fileOrder, true, range, { { "x", "y" } }, &edits, &errors) == false, "paste: too many columns is rejected");

		// 복사 · 지우기
		range.FirstViewRow    = 0;
		range.FirstGridColumn = 0;
		range.RowCount        = 2;
		range.ColumnCount     = 3;
		const HDataTableTextGrid copied = CopyDataTableRange(content, fileOrder, range);
		check(copied.size() == 2 && copied[0][0] == "Row_A" && copied[0][1] == "3" && copied[1][2] == "1.0", "copy: key and formatted values");
		const HList<HDataTableEdit> clear = PlanDataTableClear(content, fileOrder, range);
		check(clear.size() == 1 && clear[0].Cells.size() == 2, "clear: only non-default cells, key column untouched");

		// 보기 순서(정렬)로 붙이면 데이터 행 번호로 바뀐다
		const HList<int32> reversedView = { 2, 1, 0 };
		range.FirstViewRow    = 0;
		range.FirstGridColumn = 1;
		range.RowCount        = 1;
		range.ColumnCount     = 1;
		check(PlanDataTablePaste(content, reversedView, false, range, { { "5" } }, &edits, &errors) && edits[0].Cells[0].Row == 2, "paste: view rows map to data rows");
	}

	void testFiles()
	{
		const PString path = tempFilePath("Sample.jgasset");
		HFileHelper::RemoveFileOrDirectory(path);

		PSharedPtr<JGDataTable> table = makeTable(makeSampleContent());
		PString error;
		check(table->SaveToFile(path, &error), "file: save");

		PString firstBytes;
		HFileHelper::ReadAllText(path, &firstBytes);

		// LoadObject(에셋 DB 가 쓰는 길)로도 읽힌다 — 리플렉션 등록 확인
		PSharedPtr<JGDataTable> loadedByReflection = LoadObject<JGDataTable>(path);
		check(loadedByReflection != nullptr, "file: LoadObject reads the table");
		if (loadedByReflection != nullptr)
		{
			checkSameContent(loadedByReflection->GetContent(), table->GetContent(), "file: LoadObject content is equal");
		}

		PSharedPtr<PDataTableDocument> document = PDataTableDocument::OpenRawFile(path, &error);
		check(document != nullptr, "file: open document");
		if (document == nullptr)
		{
			return;
		}

		check(document->Save(nullptr, &error), "file: save unchanged document");
		PString secondBytes;
		HFileHelper::ReadAllText(path, &secondBytes);
		check(secondBytes == firstBytes, "file: unchanged save keeps the bytes");
		check(HFileHelper::Exists(path + ".tmp") == false, "file: no temp file left");

		HDataTableCellAssignment cell;
		cell.Row    = 0;
		cell.Column = 3;
		cell.Value  = HDataTableValue::MakeText("바뀐 값");
		document->Apply({ HDataTableEdit::MakeSetCells({ cell }) }, "Edit", &error);
		check(document->Save(nullptr, &error) && document->IsDirty() == false, "file: save edit");
		PSharedPtr<JGDataTable> reloaded = JGDataTable::LoadFromFile(path, &error);
		check(reloaded != nullptr && reloaded->GetValue(0, 3).GetText() == "바뀐 값", "file: edit is on disk");

		// 오류가 있으면 저장하지 않는다
		document->Apply({ HDataTableEdit::MakeSetKeys({ HPair<int32, PString>(1, "Row_A") }) }, "Duplicate", &error);
		check(document->Save(nullptr, &error) == false && error.Contains("duplicate"), "file: duplicate key blocks save");
		document->Undo();

		// 바깥에서 고치면 감지하고 다시 읽는다
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		PString external = secondBytes;
		external.ReplaceAll("\"Row_C\"", "\"Row_Z\"");
		HFileHelper::WriteAllText(path, external);
		check(document->HasExternalChange(), "file: external change is detected");
		check(document->Reload(&error) && document->GetContent().Rows[2].Key == "Row_Z" && document->CanUndo() == false, "file: reload reads the external change");
		check(document->HasExternalChange() == false, "file: reload clears the external change");

		HFileHelper::RemoveFileOrDirectory(path);
	}

	void testScale()
	{
		// 1만 행 × 열 7 — 읽기 · 쓰기 · 검증 · 바인딩 시간(결과 로그에 남긴다)
		HDataTableContent content = makeSampleContent();
		const HDataTableRow templateRow = content.Rows[0];
		content.Rows.clear();
		content.Rows.reserve(10000);
		for (int32 i = 0; i < 10000; ++i)
		{
			HDataTableRow row = templateRow;
			row.Key = PString::Format("Row_%d", i);
			row.Values[0] = HDataTableValue::MakeInt(i % 100);
			row.Values[6] = HDataTableValue::MakeText(PString::Format("Row_%d", (i + 1) % 10000));
			content.Rows.push_back(row);
		}

		using HClock = std::chrono::steady_clock;
		const HClock::time_point start = HClock::now();
		PSharedPtr<JGDataTable> table = makeTable(content);
		PString text;
		table->ToJsonText(&text);
		const HClock::time_point written = HClock::now();
		PString error;
		PSharedPtr<JGDataTable> loaded = JGDataTable::FromJsonText(text, &error);
		const HClock::time_point read = HClock::now();
		HList<HDataTableIssue> issues;
		ValidateDataTableContent(content, nullptr, issues);
		const HClock::time_point validated = HClock::now();
		// 이 표본에는 Filter 열이 없다 — 시간을 재는 바인딩은 있는 열만 묶는 구조체로
		struct HScaleRow
		{
			int32   Count = 0;
			PString Label;
			PName   Next;
			static void BindColumns(HDataTableRowBinder<HScaleRow>& binder)
			{
				binder.Bind("Count", &HScaleRow::Count);
				binder.Bind("Label", &HScaleRow::Label);
				binder.Bind("Next", &HScaleRow::Next);
			}
		};
		HDataTableView<HScaleRow> scaleView;
		const bool bBound = (loaded != nullptr) && scaleView.Bind(loaded, &issues);
		const HClock::time_point bound = HClock::now();

		auto ms = [](HClock::time_point a, HClock::time_point b)
		{
			return (int32)std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
		};
		check(loaded != nullptr && loaded->GetRowCount() == 10000 && issues.empty() && bBound && scaleView.GetCount() == 10000, "scale: 10000 rows load, validate and bind");
		JG_LOG(DataTable, ELogLevel::Info, "datatable.selftest scale: 10000 rows, %d KB JSON, write %d ms, read %d ms, validate %d ms, bind %d ms",
			(int32)(text.Length() / 1024), ms(start, written), ms(written, read), ms(read, validated), ms(validated, bound));
	}

	bool executeSelfTest(const HConsoleCommandArgs& args)
	{
		GCheckCount   = 0;
		GFailureCount = 0;

		testValues();
		testJsonRoundTrip();
		testLoadIssues();
		testValidation();
		testBinding();
		testEdits();
		testClipboard();
		testFiles();
		testScale();

		if (GFailureCount == 0)
		{
			JG_LOG(DataTable, ELogLevel::Info, "datatable.selftest: OK (%d/%d)", GCheckCount, GCheckCount);
			return true;
		}
		JG_LOG(DataTable, ELogLevel::Error, "datatable.selftest: FAILED (%d/%d)", GCheckCount - GFailureCount, GCheckCount);
		return false;
	}

	// 두 Content 의 .jgasset 중 데이터 테이블을 모두 읽어 검증한다. 다른 테이블 참조는 읽은 테이블끼리 찾는다.
	class HFileSetResolver : public IDataTableReferenceResolver
	{
	public:
		HHashMap<PString, HList<PString>> KeysByTable;   // 토큰 경로(확장자 포함) → 키

		virtual bool GetTableKeys(const PString& inTablePath, HList<PString>* outKeys) const override
		{
			const HAssetPath assetPath(inTablePath);
			if (assetPath.IsValid() == false)
			{
				return false;
			}
			const PString key = assetPath.GetAssetPath().ToString();
			if (KeysByTable.contains(key) == false)
			{
				return false;
			}
			if (outKeys != nullptr)
			{
				*outKeys = KeysByTable.at(key);
			}
			return true;
		}

		virtual bool HasAsset(const PString& inAssetPath) const override
		{
			const HAssetPath assetPath(inAssetPath);
			return assetPath.IsValid() && HFileHelper::Exists(assetPath.GetRawAssetPath().ToString());
		}
	};

	bool executeValidate(const HConsoleCommandArgs& args)
	{
		HList<PString> files;
		PString onePath;
		if (args.TryGetString("path", onePath))
		{
			const HAssetPath assetPath(onePath);
			files.push_back(assetPath.IsValid() ? assetPath.GetRawAssetPath().ToString() : onePath);
		}
		else
		{
			HFileHelper::FileListInDirectory(HFileHelper::EngineContentDirectory(), &files, true, { JG_ASSET_FORMAT });
			if (HFileHelper::GameContentDirectory().Empty() == false)
			{
				HFileHelper::FileListInDirectory(HFileHelper::GameContentDirectory(), &files, true, { JG_ASSET_FORMAT });
			}
		}

		// 먼저 데이터 테이블만 읽는다(다른 에셋은 JGObjectType 만 보고 건너뛴다)
		HList<HPair<PString, PSharedPtr<JGDataTable>>> tables;
		HFileSetResolver resolver;
		int32 readFailures = 0;
		for (const PString& file : files)
		{
			PString text;
			if (HFileHelper::ReadAllText(file, &text) == false)
			{
				continue;
			}
			if (text.Contains("\"JGObjectType\": \"JGDataTable\"") == false)
			{
				continue;
			}

			PString error;
			PSharedPtr<JGDataTable> table = JGDataTable::FromJsonText(text, &error);
			if (table == nullptr)
			{
				JG_LOG(DataTable, ELogLevel::Error, "datatable.validate: %s : %s", file, error);
				++readFailures;
				continue;
			}

			const HAssetPath assetPath(file);
			const PString tokenPath = assetPath.IsValid() ? assetPath.GetAssetPath().ToString() : file;
			HList<PString> keys;
			for (const HDataTableRow& row : table->GetContent().Rows)
			{
				keys.push_back(row.Key);
			}
			resolver.KeysByTable[tokenPath] = keys;
			tables.push_back(HPair<PString, PSharedPtr<JGDataTable>>(tokenPath, table));
		}

		int32 errorTables = 0;
		for (const HPair<PString, PSharedPtr<JGDataTable>>& entry : tables)
		{
			HList<HDataTableIssue> issues = entry.second->GetLoadIssues();
			ValidateDataTableContent(entry.second->GetContent(), &resolver, issues);
			if (issues.empty())
			{
				JG_LOG(DataTable, ELogLevel::Info, "datatable.validate: %s : OK (%d rows)", entry.first, entry.second->GetRowCount());
				continue;
			}

			LogDataTableIssues(PString::Format("datatable.validate: %s", entry.first), issues, 20);
			if (HasDataTableErrors(issues))
			{
				++errorTables;
			}
		}

		JG_LOG(DataTable, ELogLevel::Info, "datatable.validate: %d table(s), %d with errors, %d unreadable", (int32)tables.size(), errorTables, readFailures);
		return errorTables == 0 && readFailures == 0;
	}

	HAutoConsoleCommand DataTableSelfTestCommand(
		"datatable.selftest",
		"datatable.selftest",
		"Run the data table self tests (values, JSON, binding, edits, undo, TSV paste, files). No GPU or asset database needed",
		&executeSelfTest);

	HAutoConsoleCommand DataTableValidateCommand(
		"datatable.validate",
		"datatable.validate [-path=<token or file path>]",
		"Validate data table files (all tables in the engine and game Content when no path). Fails when a table has errors",
		&executeValidate);
}
