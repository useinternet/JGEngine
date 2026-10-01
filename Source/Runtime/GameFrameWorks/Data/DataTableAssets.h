#pragma once
#include "Data/DataTable.h"
#include "Data/DataTableValidation.h"

struct GAMEFRAMEWORKS_API HDataTableAssetEntry
{
	PString                 TokenPath;   // /JGGame/….jgasset
	PSharedPtr<JGDataTable> Table;
};

// 에셋 DB 에 올라온 데이터 테이블(경로 순). 에셋 DB 가 없으면(헤드리스) 빈 목록.
GAMEFRAMEWORKS_API void GetLoadedDataTables(HList<HDataTableAssetEntry>& outTables);
// 에셋 DB 에 올라온 에셋 경로(경로 순). inClassName(예: JGStaticMesh)이 있으면 그 클래스와 파생만.
GAMEFRAMEWORKS_API void GetLoadedAssetPaths(const PString& inClassName, HList<PString>& outPaths);

// 새 테이블 경로: inContentToken(/JGGame/ · /JGEngine/) + inRelativePath(폴더/이름). ASCII 글자 · 숫자 · _ · - · / 만 받는다
// (파일 입출력이 좁은 문자 경로라 한글 경로는 열리지 않는다). 확장자는 붙인다.
GAMEFRAMEWORKS_API bool MakeDataTableTokenPath(const PString& inContentToken, const PString& inRelativePath, PString* outTokenPath, PString* outError);

// 에셋 DB 로 참조를 찾는다. 테이블은 올라온 것만, 에셋은 올라온 것 또는 파일이 있는 것.
class GAMEFRAMEWORKS_API HDataTableAssetResolver : public IDataTableReferenceResolver
{
public:
	virtual ~HDataTableAssetResolver() = default;

	virtual bool GetTableKeys(const PString& inTablePath, HList<PString>* outKeys) const override;
	virtual bool HasAsset(const PString& inAssetPath) const override;
};
