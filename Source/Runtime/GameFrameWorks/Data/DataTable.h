#pragma once
#include "Asset.h"
#include "Data/DataTableContent.h"
#include "DataTable.generation.h"

class JGDataTable;
JG_DECLARE_MULTICAST_DELEGATE(POnDataTableChanged, const JGDataTable&);

// 데이터 테이블 에셋. 행(고유 키) × 열(타입) 정적 데이터를 .jgasset(JSON)으로 저장한다. 열 정의(스키마)도 같은 파일에 있다.
//   런타임: 읽기 전용. 키 → 행 번호는 색인, 순회는 행 순서(파일 순서). C++ 타입으로 읽으려면 HDataTableView<T>(DataTableView.h)
//   편집: 에디터의 PDataTableDocument 가 파일에서 읽은 사본을 고치고, 저장하면 이 에셋에 ApplyContent → Revision+1 · OnChanged
// 직렬화는 JGTexture 처럼 직접 쓴다("JGDataTable" 절, 형식은 Data/DataTableContent.h).
JGCLASS()
class GAMEFRAMEWORKS_API JGDataTable : public JGAsset
{
	JG_GENERATED_SIMPLE_BODY

	friend class PDataTableDocument;

private:
	HDataTableContent      _content;
	HHashMap<PName, int32> _rowIndexByKey;   // 빈 키 · 중복 키는 첫 행만
	HList<HDataTableIssue> _loadIssues;      // 마지막 읽기에서 나온 문제
	uint64                 _revision = 1;

public:
	// 내용이 바뀐 뒤(에디터 저장 · 다시 읽기) 메인 스레드에서 불린다. 묶어 둔 HDataTableView 는 여기서 다시 Bind 한다.
	POnDataTableChanged OnChanged;

	JGDataTable();
	virtual ~JGDataTable() = default;

	const HDataTableContent& GetContent() const { return _content; }
	const HDataTableSchema&  GetSchema() const { return _content.Schema; }
	int32 GetRowCount() const { return _content.GetRowCount(); }
	int32 GetColumnCount() const { return _content.GetColumnCount(); }
	int32 FindColumn(const PString& inName) const { return _content.Schema.FindColumn(inName); }

	// 범위 밖이면 빈 글 · 열 기본값
	const PString& GetRowKey(int32 inRowIndex) const;
	const HDataTableValue& GetValue(int32 inRowIndex, int32 inColumnIndex) const;
	// 없으면 -1
	int32 FindRowIndex(const PName& inKey) const;
	int32 FindRowIndex(const PString& inKey) const;

	const HList<HDataTableIssue>& GetLoadIssues() const { return _loadIssues; }
	uint64 GetRevision() const { return _revision; }
	uint64 ComputeContentHash() const;

	// 새 테이블. inTokenAssetPath(/JGGame/… · /JGEngine/…)가 에셋 경로가 된다. GUID 는 생성자가 새로 만든다.
	void InitializeNew(const PString& inTokenAssetPath, const HDataTableContent& inContent);
	// 내용 교체. 색인을 다시 만들고 Revision+1, OnChanged.
	void ApplyContent(const HDataTableContent& inContent);

	// 글 · 파일. AssetDatabase 를 거치지 않는다(셀프 테스트 · 편집 문서의 작업 사본 · 명령).
	static PSharedPtr<JGDataTable> FromJsonText(const PString& inText, PString* outError);
	static PSharedPtr<JGDataTable> LoadFromFile(const PString& inRawPath, PString* outError);
	bool ToJsonText(PString* outText) const;
	// 같은 폴더 임시 파일에 쓴 뒤 바꿔 넣는다. 폴더가 없으면 만든다.
	bool SaveToFile(const PString& inRawPath, PString* outError) const;

protected:
	// IJsonable
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
	// ~IJsonable

private:
	void rebuildIndex();
};
