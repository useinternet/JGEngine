#pragma once
#include "Core.h"
#include "Asset/AssetDefines.h"

// 컨텐츠 브라우저(JGEditor)가 쓰는 폴더 읽기 · 가져오기(설계 Engine-24, 구현 Engine-30).
// 폴더 · 에셋 · 원본 파일 경로는 모두 토큰 경로다: /JGEngine/… = 엔진 Content, /JGGame/… = 게임 프로젝트 Content.

enum class EAssetFolderEntryKind : uint8
{
	Folder,
	Asset,    // .jgasset
	Source,   // 가져오기 클래스가 받는 원본 파일(첫 판은 .fbx — PNG · TGA 는 Engine-28)
};

struct GAMEFRAMEWORK_API HAssetFolderEntry
{
	EAssetFolderEntryKind Kind = EAssetFolderEntryKind::Folder;
	PString TokenPath;    // /JGGame/Meshes/XBot.jgasset
	PString ClassName;    // 에셋만 — 에셋 DB 에 올라온 에셋의 클래스 이름(예: JGStaticMesh). 올라오지 않았으면 비어 있다
};

struct GAMEFRAMEWORK_API HAssetImportResult
{
	HList<PString> CreatedAssets;   // 새로 쓰거나 다시 쓴 .jgasset 토큰 경로
	PString        Error;
};

// 폴더 하나 바로 아래의 하위 폴더 · 에셋 · 원본 파일. 순서는 폴더 → 에셋 → 원본, 그 안에서 이름 순.
// 토큰이 틀렸거나 폴더가 없으면 false.
GAMEFRAMEWORK_API bool ListAssetFolder(const PString& inTokenFolder, HList<HAssetFolderEntry>& outEntries);

// 원본 파일(토큰 경로 또는 디스크 경로)을 저장 폴더(토큰 경로)로 가져온다. 확장자로 가져오기 클래스를 고르고, 새 에셋은 에셋 DB 에 바로 올린다.
// 메시 · 텍스처 에셋은 GPU 리소스를 만들므로 Graphics 가 연결된 프로세스에서만 된다. 실패하면 false 와 outResult->Error.
GAMEFRAMEWORK_API bool ImportAssetFile(const PString& inSourcePath, const PString& inTokenFolder, HAssetImportResult* outResult);
