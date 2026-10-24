#pragma once
#include "Core.h"
#include "Asset/AssetDefines.h"
#include <functional>

// 쿠킹 검사(게임 빌드 파이프라인 GP-A1, DevOps/Files/게임빌드파이프라인_설계_2026-10-02.md 4-10).
// 패키지에 넣을 파일을 에셋 DB 에 올리지 않고 글로 검사한다. 오류가 하나라도 있으면 쿠킹이 실패한다.
//   1 .jgasset 이 JSON 으로 읽히고 JGObjectType · JGObject · JGAsset._guid 가 있다
//   2 GUID 가 모든 마운트를 통틀어 하나뿐이다(GAssetDatabase 는 같은 GUID 의 둘째 에셋을 버린다)
//   3 에셋 안의 마운트 토큰 문자열(/JGEngine/… · /JGGame/…)이 쿠킹 대상 파일을 가리킨다.
//     확장자가 없으면 .jgasset 을 붙인다(HAssetPath 와 같은 규칙). 런타임 찾기가 대소문자를 구분하므로 여기서도 구분한다.
//     에셋 자기 경로 칸(JGAsset.AssetPath)은 참조가 아니다 — 에셋 DB 는 파일 위치로 경로를 정한다
//   + 타입별 검사: JGObjectType 이 같은 에셋마다 부른다. 데이터 테이블 검사는 Data/ContentCookCheck 가 끼운다
// 경로는 HAssetPath · HFileHelper 의 Content 경로를 쓰지 않고 inMounts 로만 푼다 — 쿠킹하는 프로세스의 Content 와 상관없다.

struct GAMEFRAMEWORK_API HAssetCookMount
{
	PString Token;       // JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN · JG_ASSET_GAME_PATH_RECOGNITION_TOEKN
	PString Directory;   // 그 토큰의 원본 Content 폴더
};

enum class EAssetCookIssueSeverity : uint8
{
	Warning,
	Error,
};

struct GAMEFRAMEWORK_API HAssetCookIssue
{
	EAssetCookIssueSeverity Severity = EAssetCookIssueSeverity::Error;
	PString File;      // 토큰 경로(마운트 밖 파일이면 원본 경로)
	PString Message;
};

// 쿠킹 대상 .jgasset 하나
struct GAMEFRAMEWORK_API HAssetCookAsset
{
	PString File;         // 원본 경로(절대)
	PString TokenPath;    // /JGGame/Data/Items.jgasset
	PString ObjectType;   // JGObjectType
	PString JsonText;     // 파일 글. 타입별 검사가 있는 타입만 채운다
};

// 타입별 검사가 보는 쿠킹 대상 전체(읽기 전용). 채우는 쪽은 ValidateAssetsForCook 이다
class GAMEFRAMEWORK_API HAssetCookContext
{
protected:
	HList<HRawString>                _tokens;             // 마운트 토큰
	HHashSet<HRawString>             _files;              // 쿠킹 대상 파일의 토큰 경로
	HHashMap<HRawString, HRawString> _filesByLowerPath;   // 소문자 토큰 경로 → 실제 토큰 경로(대소문자만 다른 참조를 알려 준다)
	HList<HAssetCookAsset>           _assets;             // 토큰 경로 순
	HHashMap<HRawString, int32>      _assetIndexByPath;

public:
	virtual ~HAssetCookContext() = default;

	// inPath(토큰 경로)를 HAssetPath 규칙으로 고친다(역슬래시 → '/', 토큰 뒤 '/' 정리, 확장자가 없으면 .jgasset). 토큰으로 시작하지 않으면 빈 글
	PString NormalizeTokenPath(const PString& inPath) const;
	// 쿠킹 대상에 그 파일이 있는가(inPath 는 NormalizeTokenPath 로 고쳐 찾는다)
	bool HasFile(const PString& inPath) const;
	// 쿠킹 대상 .jgasset 하나. 없으면 nullptr
	const HAssetCookAsset* FindAsset(const PString& inPath) const;
	const HList<HAssetCookAsset>& GetAssets() const { return _assets; }
};

struct GAMEFRAMEWORK_API HAssetCookTypeCheck
{
	PString ObjectType;   // 이 JGObjectType 의 에셋에만 부른다
	std::function<void(const HAssetCookContext& inContext, const HAssetCookAsset& inAsset, HList<HAssetCookIssue>& outIssues)> Check;
};

// inFiles      : 패키지에 넣을 모든 파일(원본 경로). .jgasset 은 검사하고, 다른 파일은 참조 대상으로만 쓴다
// inMounts     : 토큰 → 원본 Content 폴더(보통 /JGEngine/ · /JGGame/ 두 개). 모든 파일이 어느 마운트 안에 있어야 한다
// inTypeChecks : 타입별 검사(없으면 빈 목록)
// outIssues    : 덧붙인다. 파일은 토큰 경로 순
// 반환: 오류(EAssetCookIssueSeverity::Error)가 없으면 true
GAMEFRAMEWORK_API bool ValidateAssetsForCook(const HList<PString>& inFiles, const HList<HAssetCookMount>& inMounts,
	const HList<HAssetCookTypeCheck>& inTypeChecks, HList<HAssetCookIssue>& outIssues);
