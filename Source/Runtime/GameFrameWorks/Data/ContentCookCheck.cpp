#include "PCH/PCH.h"
#include "Data/ContentCookCheck.h"
#include "Data/DataTable.h"
#include "Data/DataTableValidation.h"
#include "Cook/AssetCookValidator.h"

namespace
{
	// 데이터 테이블의 다른 테이블 참조(RowRef 열의 Table)는 쿠킹 대상 안에서 찾는다.
	// 에셋 참조(AssetRef 칸 · Table 경로 자체)는 쿠킹 검사 3(문자열 참조)이 오류로 보므로 HasAsset 은 늘 있다고 답한다 — 같은 칸을 경고로 또 내지 않는다
	class HCookDataTableResolver : public IDataTableReferenceResolver
	{
	private:
		const HAssetCookContext& _context;

	public:
		explicit HCookDataTableResolver(const HAssetCookContext& inContext)
			: _context(inContext)
		{
		}

		virtual bool GetTableKeys(const PString& inTablePath, HList<PString>* outKeys) const override
		{
			// 데이터 테이블만 JsonText 를 갖는다(ValidateAssetsForCook 은 타입별 검사가 있는 타입만 글을 남긴다)
			const HAssetCookAsset* asset = _context.FindAsset(inTablePath);
			if (asset == nullptr || asset->JsonText.Empty())
			{
				return false;
			}

			// 그 테이블 자체의 형식 오류는 그 테이블의 검사가 낸다
			PString error;
			PSharedPtr<JGDataTable> table = JGDataTable::FromJsonText(asset->JsonText, &error);
			if (table == nullptr)
			{
				return false;
			}

			if (outKeys != nullptr)
			{
				outKeys->clear();
				for (int32 r = 0; r < table->GetRowCount(); ++r)
				{
					outKeys->push_back(table->GetRowKey(r));
				}
			}
			return true;
		}

		virtual bool HasAsset(const PString& inAssetPath) const override
		{
			return true;
		}
	};

	// "data table row 3 'Row_C', column 'Count': <이유>"
	PString describeIssue(const HDataTableContent& inContent, const HDataTableIssue& inIssue)
	{
		HList<PString> places;
		if (inIssue.Row >= 0)
		{
			const bool bKnownKey = inIssue.Row < inContent.GetRowCount() && inContent.Rows[inIssue.Row].Key.Empty() == false;
			places.push_back(bKnownKey
				? PString::Format("row %d '%s'", inIssue.Row + 1, inContent.Rows[inIssue.Row].Key)
				: PString::Format("row %d", inIssue.Row + 1));
		}
		if (inIssue.Column >= 0 && inIssue.Column < inContent.GetColumnCount())
		{
			places.push_back(PString::Format("column '%s'", inContent.Schema.Columns[inIssue.Column].Name));
		}
		else if (inIssue.Column == -1 && inIssue.Row >= 0)
		{
			places.push_back(PString::Format("key '%s'", inContent.Schema.KeyColumn));
		}

		PString place;
		for (const PString& part : places)
		{
			place += place.Empty() ? " " : ", ";
			place += part;
		}
		return PString::Format("data table%s: %s", place, inIssue.Message);
	}

	// 4 데이터 테이블 — datatable.validate 와 같은 규칙(읽기 문제 + ValidateDataTableContent, 오류면 실패 · 경고는 경고)
	void checkDataTable(const HAssetCookContext& inContext, const HAssetCookAsset& inAsset, HList<HAssetCookIssue>& outIssues)
	{
		const auto addIssue = [&inAsset, &outIssues](EAssetCookIssueSeverity inSeverity, const PString& inMessage)
			{
				HAssetCookIssue issue;
				issue.Severity = inSeverity;
				issue.File     = inAsset.TokenPath;
				issue.Message  = inMessage;
				outIssues.push_back(issue);
			};

		PString error;
		PSharedPtr<JGDataTable> table = JGDataTable::FromJsonText(inAsset.JsonText, &error);
		if (table == nullptr)
		{
			addIssue(EAssetCookIssueSeverity::Error, PString::Format("data table: %s", error));
			return;
		}

		HList<HDataTableIssue> issues = table->GetLoadIssues();
		HCookDataTableResolver resolver(inContext);
		ValidateDataTableContent(table->GetContent(), &resolver, issues);
		for (const HDataTableIssue& issue : issues)
		{
			const EAssetCookIssueSeverity severity = (issue.Severity == EDataTableIssueSeverity::Error) ? EAssetCookIssueSeverity::Error : EAssetCookIssueSeverity::Warning;
			addIssue(severity, describeIssue(table->GetContent(), issue));
		}
	}
}

bool ValidateContentForCook(const HList<PString>& inFiles, const HList<HPair<PString, PString>>& inMounts,
	HList<PString>& outErrors, HList<PString>& outWarnings)
{
	HList<HAssetCookMount> mounts;
	for (const HPair<PString, PString>& mount : inMounts)
	{
		HAssetCookMount cookMount;
		cookMount.Token     = mount.first;
		cookMount.Directory = mount.second;
		mounts.push_back(cookMount);
	}

	HAssetCookTypeCheck dataTableCheck;
	dataTableCheck.ObjectType = JGTYPE(JGDataTable).GetName().ToString();
	dataTableCheck.Check      = &checkDataTable;

	HList<HAssetCookIssue> issues;
	const bool bOk = ValidateAssetsForCook(inFiles, mounts, { dataTableCheck }, issues);
	for (const HAssetCookIssue& issue : issues)
	{
		const PString line = PString::Format("%s : %s", issue.File, issue.Message);
		if (issue.Severity == EAssetCookIssueSeverity::Error)
		{
			outErrors.push_back(line);
		}
		else
		{
			outWarnings.push_back(line);
		}
	}
	return bOk;
}
