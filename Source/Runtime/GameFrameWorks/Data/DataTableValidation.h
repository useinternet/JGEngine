#pragma once
#include "Data/DataTableContent.h"

// 다른 테이블 · 에셋을 찾는 쪽. 런타임 · 에디터는 에셋 DB 로(HDataTableAssetResolver), 셀프 테스트는 가짜로.
class GAMEFRAMEWORKS_API IDataTableReferenceResolver
{
public:
	virtual ~IDataTableReferenceResolver() = default;

	// inTablePath(토큰 경로) 테이블의 키 목록(행 순서). 테이블을 못 찾으면 false.
	virtual bool GetTableKeys(const PString& inTablePath, HList<PString>* outKeys) const = 0;
	// inAssetPath(토큰 경로) 에셋이 있나
	virtual bool HasAsset(const PString& inAssetPath) const = 0;
};

// 내용을 검사해 문제를 더한다. inResolver 가 null 이면 다른 테이블 · 에셋 참조는 보지 않는다(같은 테이블 RowRef 는 본다).
//   오류(저장 막음): 빈 키 · 중복 키 · 키에 공백 · 열 이름 규칙 · 열 이름 중복 · 키 열과 같은 열 이름
//   경고: 범위 밖 · Enum 목록 밖 · 목록을 모르는 Enum 열 · 없는 행 · 없는 에셋 · 없는 대상 테이블
GAMEFRAMEWORKS_API void ValidateDataTableContent(const HDataTableContent& inContent, const IDataTableReferenceResolver* inResolver, HList<HDataTableIssue>& outIssues);
