#pragma once
#include "JGEditorDefine.h"
#include "Widget.h"
#include "Asset/AssetFolder.h"
#include "ContentBrowser.generation.h"

// 컨텐츠 브라우저(Engine-31, 설계 Engine-24 시안 A). 엔진 · 게임 Content 의 폴더 · 에셋 · 원본 파일을 둘러본다.
//   도구 줄: Import(고른 원본 파일을 지금 폴더로 가져오기) · Refresh · 경로 막대 · 찾기(이름에 든 글자) · 종류 칩(Mesh · Texture · Table · Source)
//   왼쪽 폴더 트리(/JGEngine/ · /JGGame/), 가운데 타일(텍스처는 실제 그림, 그 밖은 종류 이름), 아래 세부(종류 · 경로 · 경로 복사 · 열기)
//   더블클릭: 폴더는 들어가고, 데이터 테이블은 Data Table Editor 로 연다. 그 밖은 고르기만 한다.
// 폴더 읽기 · 가져오기는 GameFrameWork(ListAssetFolder · ImportAssetFile)가 하고, 이 창은 읽은 목록을 폴더마다 들고 있다가 Refresh · 가져오기 · 창 열기 때 버린다.

// 폴더 하나를 읽은 결과. bFound 가 false 면 토큰이 틀렸거나 폴더가 없다.
struct HContentFolderListing
{
	bool                     bFound = false;
	HList<HAssetFolderEntry> Entries;
};

JGCLASS()
class JGEDITOR_API JGContentBrowser : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

private:
	HHashMap<PString, HContentFolderListing> _folderCache;
	PString _currentFolder;
	PString _selectedPath;
	PString _searchText;
	bool    _bShowMesh    = false;
	bool    _bShowTexture = false;
	bool    _bShowTable   = false;
	bool    _bShowSource  = false;
	PString _statusText;
	bool    _bStatusError = false;
	float64 _lastRelistTime = 0.0;

protected:
	virtual PString GetTitleName() const override;
	virtual void OnOpen() override;
	virtual void OnGenerateGUI() override;

private:
	void drawToolbar();
	void drawPathBar();
	void drawTreeNode(const PString& inFolder, const PString& inLabel, bool bRoot);
	void drawTiles();
	void drawDetails();
	const HContentFolderListing& folderListing(const PString& inFolder);
	const HAssetFolderEntry* findSelectedEntry();
	void navigate(const PString& inFolder);
	void openEntry(const HAssetFolderEntry& inEntry);
	void importSelected();
	void setStatus(const PString& inText, bool bError);
};
