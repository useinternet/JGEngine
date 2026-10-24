#include "PCH/PCH.h"
#include "Widgets/ContentBrowser.h"
#include "Widgets/DataTableEditor.h"
#include "Asset/AssetDatabase.h"
#include "Asset/AssetPath.h"
#include "Asset/TextureAsset.h"
#include "Asset/StaticMeshAsset.h"
#include "GUIModule.h"
#include "GUI.h"

namespace
{
	// 100%(96 DPI) 기준 — 쓸 때 화면 배율을 곱한다(D-22 ②)
	constexpr float32 TreeWidth      = 220.0f;
	constexpr float32 ThumbnailSize  = 80.0f;
	constexpr float32 TilePadding    = 12.0f;   // HGUI::BeginPanel 의 가로 안쪽 여백
	constexpr float32 TileGap        = 8.0f;
	constexpr float32 DetailsHeight  = 112.0f;
	constexpr float32 MinListHeight  = 80.0f;   // 창이 작아도 트리 · 타일 칸은 이만큼 둔다(세부 칸이 밀려 내려간다)
	constexpr float32 DetailsLabelX  = 72.0f;
	constexpr float32 SearchWidth    = 180.0f;
	constexpr float64 RelistInterval = 1.0;     // 에셋 DB 가 아직 올리는 중인 에셋(클래스 이름이 빈 것)이 있으면 이 간격으로 다시 읽는다

	const char* const EngineRoot = JG_ASSET_ENGINE_PATH_RECOGNITION_TOEKN;
	const char* const GameRoot   = JG_ASSET_GAME_PATH_RECOGNITION_TOEKN;

	enum class ETileKind
	{
		Folder,
		Mesh,
		Texture,
		Table,
		Source,
		Other,   // 다른 클래스 · 에셋 DB 가 아직 올리지 않은 에셋
	};

	ETileKind tileKind(const HAssetFolderEntry& entry)
	{
		switch (entry.Kind)
		{
		case EAssetFolderEntryKind::Folder: return ETileKind::Folder;
		case EAssetFolderEntryKind::Source: return ETileKind::Source;
		default: break;
		}
		if (entry.ClassName == PString("JGStaticMesh"))
		{
			return ETileKind::Mesh;
		}
		if (entry.ClassName == PString("JGTexture"))
		{
			return ETileKind::Texture;
		}
		if (entry.ClassName == PString("JGDataTable"))
		{
			return ETileKind::Table;
		}
		return ETileKind::Other;
	}

	// 디자인 시안(Saved/UIShots/Engine-24_ContentBrowser_mockups.png)의 종류 색
	uint32 kindColor(ETileKind kind)
	{
		switch (kind)
		{
		case ETileKind::Folder:  return 0xc8a45a;
		case ETileKind::Mesh:    return 0x5bb08a;
		case ETileKind::Texture: return 0xd39b45;
		case ETileKind::Table:   return 0x8f7fe0;
		default:                 return 0x8a8a8a;
		}
	}

	uint32 kindBackground(ETileKind kind)
	{
		switch (kind)
		{
		case ETileKind::Folder: return 0x2e2a20;
		case ETileKind::Mesh:   return 0x1f2a25;
		case ETileKind::Table:  return 0x25223a;
		default:                return 0x262626;
		}
	}

	PString fileName(const PString& tokenPath)
	{
		PString name;
		const uint64 slash = tokenPath.FindLastOf("/");
		tokenPath.SubString(&name, slash == PString::NPOS ? 0 : slash + 1);
		return name;
	}

	// 타일 이름: 에셋은 확장자를 떼고, 원본 파일은 확장자까지 보인다(XBot.fbx 와 XBot.jgasset 을 가른다).
	PString entryName(const HAssetFolderEntry& entry)
	{
		PString name = fileName(entry.TokenPath);
		if (entry.Kind == EAssetFolderEntryKind::Asset && name.EndWidth(JG_ASSET_FORMAT))
		{
			name.Remove(name.Length() - PString(JG_ASSET_FORMAT).Length(), PString(JG_ASSET_FORMAT).Length());
		}
		return name;
	}

	// 타일의 종류 글자(텍스처는 그림이 대신한다)
	PString kindLabel(const HAssetFolderEntry& entry, ETileKind kind)
	{
		switch (kind)
		{
		case ETileKind::Folder:  return "FOLDER";
		case ETileKind::Mesh:    return "MESH";
		case ETileKind::Texture: return "TEXTURE";
		case ETileKind::Table:   return "TABLE";
		case ETileKind::Source:
		{
			PString extension;
			HFileHelper::FileExtension(entry.TokenPath, &extension);
			extension.ReplaceAll(".", "");
			return PString::ToUpper(extension);
		}
		default:
			return entry.ClassName.Empty() ? PString("LOADING") : PString("ASSET");
		}
	}

	PString className(const HAssetFolderEntry& entry, ETileKind kind)
	{
		switch (kind)
		{
		case ETileKind::Folder: return "Folder";
		case ETileKind::Source: return "Source file (Import makes .jgasset files in this folder)";
		default:
			return entry.ClassName.Empty() ? PString("(the asset database has not loaded it yet)") : entry.ClassName;
		}
	}

	bool rootOf(const PString& folder, PString* outRoot)
	{
		if (folder.StartWidth(EngineRoot))
		{
			*outRoot = EngineRoot;
			return true;
		}
		if (folder.StartWidth(GameRoot))
		{
			*outRoot = GameRoot;
			return true;
		}
		return false;
	}

	PSharedPtr<JGAsset> loadedAsset(const PString& tokenPath)
	{
		if (GAssetDatabase::IsValid() == false)
		{
			return nullptr;
		}
		return GAssetDatabase::GetInstance().GetLoadedAsset(HAssetPath(tokenPath)).Pin();
	}

	// 텍스처 타일의 GPU 텍스처. 아직 올라오지 않았거나 GPU 텍스처가 없으면 0.
	uint64 thumbnailTexture(const HAssetFolderEntry& entry)
	{
		PSharedPtr<JGAsset> asset = loadedAsset(entry.TokenPath);
		// Cast<> 는 정적 캐스트라 타입을 먼저 확인한다.
		if (asset == nullptr || (asset->GetType() == JGType::GenerateType<JGTexture>()) == false)
		{
			return 0;
		}
		PSharedPtr<JGTexture> texture = Cast<JGTexture>(asset);
		if (texture->IsValid() == false)
		{
			return 0;
		}
		return texture->GetRawTexture()->GetTextureID();
	}

	bool containsIgnoreCase(const PString& text, const PString& pattern)
	{
		return PString::ToLower(text).Contains(PString::ToLower(pattern));
	}

	void detailRow(const PString& label, const PString& value)
	{
		HGUI::Text(label, HGUI::DisplayColor(0x9a9a9a));
		HGUI::SameLineAt(DetailsLabelX * HGUI::GetDpiScale());
		HGUI::Text(value);
	}

	struct HTileInput
	{
		bool bClicked       = false;
		bool bDoubleClicked = false;
	};

	// 타일 하나: 테두리 있는 구역 안에 썸네일(텍스처 그림 또는 종류 글자)과 이름. 누름은 썸네일이 받는다.
	HTileInput drawTile(const HAssetFolderEntry& entry, ETileKind kind, bool bSelected)
	{
		const float32 scale = HGUI::GetDpiScale();
		const float32 thumbnail = ThumbnailSize * scale;
		const HLinearColor background = HGUI::DisplayColor(bSelected ? 0x24456f : 0x2b2b2e);
		const HLinearColor border     = HGUI::DisplayColor(bSelected ? 0x3d7bd9 : 0x3c3c40);
		const PString name = entryName(entry);

		HTileInput input;
		HGUI::BeginPanel(PString("##Tile") + entry.TokenPath, HVector2(thumbnail + TilePadding * 2.0f * scale, 0.0f), background, border);
		const uint64 textureID = (kind == ETileKind::Texture) ? thumbnailTexture(entry) : 0;
		if (textureID != 0)
		{
			const HGUIImageInput image = HGUI::InteractiveImage("##Thumbnail", textureID, HVector2(thumbnail, thumbnail));
			input.bClicked = image.bClicked[(int32)EGUIMouseButton::Left];
		}
		else
		{
			HGUI::PushStyleColor(EGUIColor::Button, HGUI::DisplayColor(kindBackground(kind)));
			HGUI::PushStyleColor(EGUIColor::ButtonHovered, HGUI::DisplayColor(0x34343a));
			HGUI::PushStyleColor(EGUIColor::ButtonActive, HGUI::DisplayColor(0x3c3c44));
			HGUI::PushStyleColor(EGUIColor::Text, HGUI::DisplayColor(kindColor(kind)));
			input.bClicked = HGUI::Button(kindLabel(entry, kind) + "##Thumbnail", HVector2(thumbnail, thumbnail));
			HGUI::PopStyleColor(4);
		}
		input.bDoubleClicked = HGUI::IsItemDoubleClicked();
		HGUI::ItemTooltip(name);
		HGUI::Text(name);
		HGUI::EndPanel();
		return input;
	}
}

PString JGContentBrowser::GetTitleName() const
{
	return "Content Browser";
}

void JGContentBrowser::OnOpen()
{
	// 창을 닫은 동안 바뀐 디스크를 다시 읽는다.
	_folderCache.clear();
}

void JGContentBrowser::OnGenerateGUI()
{
	if (_currentFolder.Empty())
	{
		_currentFolder = folderListing(GameRoot).bFound ? GameRoot : EngineRoot;
	}

	const float32 scale = HGUI::GetDpiScale();
	drawToolbar();

	const float32 middleHeight = HMath::Max(HGUI::GetContentRegionAvail().y - DetailsHeight * scale, MinListHeight * scale);
	HGUI::BeginChild("##ContentBrowserTree", HVector2(TreeWidth * scale, middleHeight));
	drawTreeNode(EngineRoot, EngineRoot, true);
	drawTreeNode(GameRoot, GameRoot, true);
	HGUI::EndChild();

	HGUI::SameLine();
	HGUI::BeginChild("##ContentBrowserTiles", HVector2(0.0f, middleHeight));
	drawTiles();
	HGUI::EndChild();

	HGUI::Separator();
	HGUI::BeginChild("##ContentBrowserDetails", HVector2(0.0f, 0.0f));
	drawDetails();
	HGUI::EndChild();
}

void JGContentBrowser::drawToolbar()
{
	const HAssetFolderEntry* selected = findSelectedEntry();
	const bool bCanImport = selected != nullptr && selected->Kind == EAssetFolderEntryKind::Source;
	HGUI::BeginDisabled(bCanImport == false);
	if (HGUI::Button("Import"))
	{
		importSelected();
	}
	HGUI::EndDisabled();
	HGUI::ItemTooltip("Import the selected source file (.fbx) into this folder");

	HGUI::SameLine();
	if (HGUI::Button("Refresh"))
	{
		_folderCache.clear();
		setStatus(PString(), false);
	}

	HGUI::SameLine();
	drawPathBar();

	HGUI::SameLine();
	HGUI::InputTextWithHint("##ContentBrowserSearch", "Search assets", _searchText, SearchWidth * HGUI::GetDpiScale());

	int32 counts[(int32)ETileKind::Other + 1] = {};
	for (const HAssetFolderEntry& entry : folderListing(_currentFolder).Entries)
	{
		++counts[(int32)tileKind(entry)];
	}
	HGUI::SameLine();
	HGUI::ToggleChip("##ChipMesh", "Mesh", PString::Format("%d", counts[(int32)ETileKind::Mesh]), HGUI::DisplayColor(kindColor(ETileKind::Mesh)), _bShowMesh);
	HGUI::SameLine();
	HGUI::ToggleChip("##ChipTexture", "Texture", PString::Format("%d", counts[(int32)ETileKind::Texture]), HGUI::DisplayColor(kindColor(ETileKind::Texture)), _bShowTexture);
	HGUI::SameLine();
	HGUI::ToggleChip("##ChipTable", "Table", PString::Format("%d", counts[(int32)ETileKind::Table]), HGUI::DisplayColor(kindColor(ETileKind::Table)), _bShowTable);
	HGUI::SameLine();
	HGUI::ToggleChip("##ChipSource", "Source", PString::Format("%d", counts[(int32)ETileKind::Source]), HGUI::DisplayColor(kindColor(ETileKind::Source)), _bShowSource);
}

// 경로 막대: 루트 · 폴더 이름마다 단추. 누르면 그 폴더로 간다.
void JGContentBrowser::drawPathBar()
{
	PString root;
	if (rootOf(_currentFolder, &root) == false)
	{
		HGUI::AlignTextToFramePadding();
		HGUI::Text(_currentFolder);
		return;
	}

	HGUI::PushStyleColor(EGUIColor::Button, HGUI::DisplayColor(0x000000, 0.0f));
	if (HGUI::Button(root + "##PathRoot"))
	{
		navigate(root);
	}

	PString relative;
	_currentFolder.SubString(&relative, root.Length());
	PString folder = root;
	int32 index = 0;
	for (const PString& part : relative.Split('/'))
	{
		if (part.Empty())
		{
			continue;
		}
		folder = folder.EndWidth("/") ? folder + part : folder + "/" + part;
		HGUI::SameLine(0.0f);
		HGUI::AlignTextToFramePadding();
		HGUI::Text(">", HGUI::DisplayColor(0x808080));
		HGUI::SameLine(0.0f);
		if (HGUI::Button(part + PString::Format("##PathPart%d", index)))
		{
			navigate(folder);
		}
		++index;
	}
	HGUI::PopStyleColor();
}

void JGContentBrowser::drawTreeNode(const PString& inFolder, const PString& inLabel, bool bRoot)
{
	const HContentFolderListing& listing = folderListing(inFolder);
	HList<PString> subFolders;
	for (const HAssetFolderEntry& entry : listing.Entries)
	{
		if (entry.Kind == EAssetFolderEntryKind::Folder)
		{
			subFolders.push_back(entry.TokenPath);
		}
	}

	bool bClicked = false;
	const bool bOpen = HGUI::TreeNode(inLabel + "##" + inFolder, _currentFolder == inFolder, bClicked, subFolders.empty(), bRoot);
	if (bClicked)
	{
		navigate(inFolder);
	}
	if (bOpen == false)
	{
		return;
	}
	// subFolders 는 복사본이다 — 아래 자식이 folderListing 으로 캐시를 늘려도 그대로다.
	for (const PString& subFolder : subFolders)
	{
		drawTreeNode(subFolder, fileName(subFolder), false);
	}
	HGUI::TreePop();
}

void JGContentBrowser::drawTiles()
{
	const HContentFolderListing* listing = &folderListing(_currentFolder);
	if (listing->bFound == false)
	{
		HGUI::Text(PString("Folder not found: ") + _currentFolder, HGUI::DisplayColor(0x808080));
		return;
	}

	// 에셋 DB 는 시작할 때 에셋을 비동기로 올린다. 클래스 이름이 빈 에셋이 있으면 잠시 뒤 다시 읽어 종류 · 그림을 채운다.
	bool bLoading = false;
	for (const HAssetFolderEntry& entry : listing->Entries)
	{
		if (entry.Kind == EAssetFolderEntryKind::Asset && entry.ClassName.Empty())
		{
			bLoading = true;
			break;
		}
	}
	if (bLoading && HGUI::GetTime() - _lastRelistTime > RelistInterval)
	{
		_lastRelistTime = HGUI::GetTime();
		_folderCache.erase(_currentFolder);
		listing = &folderListing(_currentFolder);
	}

	const bool bAnyChip = _bShowMesh || _bShowTexture || _bShowTable || _bShowSource;
	const float32 scale = HGUI::GetDpiScale();
	const float32 tileWidth = (ThumbnailSize + TilePadding * 2.0f) * scale;
	const float32 gap = TileGap * scale;
	const int32 columns = HMath::Max(1, (int32)((HGUI::GetContentRegionAvail().x + gap) / (tileWidth + gap)));

	// 타일을 누르면 navigate 가 목록을 바꿀 수 있어 그린 뒤에 처리한다.
	HAssetFolderEntry opened;
	bool bOpened = false;
	int32 drawn = 0;
	for (const HAssetFolderEntry& entry : listing->Entries)
	{
		const ETileKind kind = tileKind(entry);
		if (_searchText.Empty() == false && containsIgnoreCase(entryName(entry), _searchText) == false)
		{
			continue;
		}
		if (bAnyChip)
		{
			const bool bShown = (kind == ETileKind::Mesh && _bShowMesh) || (kind == ETileKind::Texture && _bShowTexture)
				|| (kind == ETileKind::Table && _bShowTable) || (kind == ETileKind::Source && _bShowSource);
			if (bShown == false)
			{
				continue;
			}
		}

		if (drawn % columns != 0)
		{
			HGUI::SameLine(gap);
		}
		const HTileInput input = drawTile(entry, kind, entry.TokenPath == _selectedPath);
		if (input.bClicked)
		{
			_selectedPath = entry.TokenPath;
		}
		if (input.bDoubleClicked && bOpened == false)
		{
			opened  = entry;
			bOpened = true;
		}
		++drawn;
	}

	if (drawn == 0)
	{
		HGUI::Text(listing->Entries.empty() ? "This folder is empty." : "No item matches the search or the filters.", HGUI::DisplayColor(0x808080));
	}
	if (bOpened)
	{
		openEntry(opened);
	}
}

void JGContentBrowser::drawDetails()
{
	const HAssetFolderEntry* selected = findSelectedEntry();
	if (selected == nullptr)
	{
		HGUI::Text("Select an item to see its details. Double-click a folder to enter it, a data table to open it.", HGUI::DisplayColor(0x808080));
		if (_statusText.Empty() == false)
		{
			HGUI::Text(_statusText, HGUI::DisplayColor(_bStatusError ? 0xe06c6c : 0x9a9a9a));
		}
	}
	else
	{
		const ETileKind kind = tileKind(*selected);
		HGUI::AlignTextToFramePadding();
		HGUI::Text(entryName(*selected), HGUI::DisplayColor(0xffffff));
		HGUI::SameLine();
		if (HGUI::Button("Copy Path"))
		{
			HGUI::SetClipboardText(selected->TokenPath);
			setStatus(PString("Copied ") + selected->TokenPath, false);
		}
		if (kind == ETileKind::Folder || kind == ETileKind::Table)
		{
			HGUI::SameLine();
			if (HGUI::Button("Open"))
			{
				const HAssetFolderEntry entry = *selected;
				openEntry(entry);
				return;
			}
		}
		// 상태 글(가져오기 · 복사 · 열기 결과)은 이름 줄 끝에 둔다 — 세부 줄이 늘어도 가려지지 않는다.
		if (_statusText.Empty() == false)
		{
			HGUI::SameLine();
			HGUI::Text(_statusText, HGUI::DisplayColor(_bStatusError ? 0xe06c6c : 0x9a9a9a));
		}

		detailRow("Class", className(*selected, kind));
		detailRow("Path", selected->TokenPath);
		if (kind == ETileKind::Mesh)
		{
			PSharedPtr<JGAsset> asset = loadedAsset(selected->TokenPath);
			if (asset != nullptr && asset->GetType() == JGType::GenerateType<JGStaticMesh>())
			{
				PSharedPtr<JGStaticMesh> mesh = Cast<JGStaticMesh>(asset);
				detailRow("SubMesh", PString::Format("%llu (vertices %llu)", mesh->GetSubMeshCount(), mesh->GetTotalVertexCount()));
				HBBox bounds;
				if (mesh->GetBounds(bounds))
				{
					detailRow("Bounds", PString::Format("(%.1f, %.1f, %.1f) ~ (%.1f, %.1f, %.1f)",
						bounds.min.x, bounds.min.y, bounds.min.z, bounds.max.x, bounds.max.y, bounds.max.z));
				}
			}
		}
		else if (kind == ETileKind::Texture)
		{
			PSharedPtr<JGAsset> asset = loadedAsset(selected->TokenPath);
			if (asset != nullptr && asset->GetType() == JGType::GenerateType<JGTexture>())
			{
				PSharedPtr<JGTexture> texture = Cast<JGTexture>(asset);
				if (texture->IsValid())
				{
					const HTextureInfo& info = texture->GetRawTexture()->GetTextureInfo();
					detailRow("Size", PString::Format("%u x %u", info.Width, info.Height));
				}
			}
		}
	}
}

const HContentFolderListing& JGContentBrowser::folderListing(const PString& inFolder)
{
	auto found = _folderCache.find(inFolder);
	if (found != _folderCache.end())
	{
		return found->second;
	}

	HContentFolderListing listing;
	listing.bFound = ListAssetFolder(inFolder, listing.Entries);
	return _folderCache.emplace(inFolder, std::move(listing)).first->second;
}

const HAssetFolderEntry* JGContentBrowser::findSelectedEntry()
{
	if (_selectedPath.Empty())
	{
		return nullptr;
	}
	for (const HAssetFolderEntry& entry : folderListing(_currentFolder).Entries)
	{
		if (entry.TokenPath == _selectedPath)
		{
			return &entry;
		}
	}
	return nullptr;
}

void JGContentBrowser::navigate(const PString& inFolder)
{
	if (_currentFolder == inFolder)
	{
		return;
	}
	_currentFolder = inFolder;
	_selectedPath.Reset();
}

void JGContentBrowser::openEntry(const HAssetFolderEntry& inEntry)
{
	const ETileKind kind = tileKind(inEntry);
	if (kind == ETileKind::Folder)
	{
		navigate(inEntry.TokenPath);
		return;
	}
	if (kind != ETileKind::Table)
	{
		return;
	}

	HGUIModule* GUIModule = GModuleGlobalSystem::GetInstance().FindModule<HGUIModule>();
	if (GUIModule == nullptr)
	{
		return;
	}
	GUIModule->OpenWidget<JGDataTableEditor>();
	PSharedPtr<JGDataTableEditor> tableEditor = GUIModule->FindWidget<JGDataTableEditor>();
	if (tableEditor != nullptr && tableEditor->OpenTable(inEntry.TokenPath))
	{
		setStatus(PString("Opened ") + inEntry.TokenPath + " in Data Table Editor", false);
	}
	else
	{
		setStatus(PString("Cannot open ") + inEntry.TokenPath + " (see Data Table Editor)", true);
	}
}

void JGContentBrowser::importSelected()
{
	const HAssetFolderEntry* selected = findSelectedEntry();
	if (selected == nullptr)
	{
		return;
	}

	const PString sourcePath = selected->TokenPath;
	HAssetImportResult result;
	if (ImportAssetFile(sourcePath, _currentFolder, &result) == false)
	{
		setStatus(PString("Import failed: ") + sourcePath + " : " + result.Error, true);
		return;
	}

	_folderCache.clear();
	if (result.CreatedAssets.empty() == false)
	{
		_selectedPath = result.CreatedAssets.front();
	}
	setStatus(PString::Format("Imported %s : %d asset(s)", sourcePath, (int32)result.CreatedAssets.size()), false);
}

void JGContentBrowser::setStatus(const PString& inText, bool bError)
{
	_statusText   = inText;
	_bStatusError = bError;
}
