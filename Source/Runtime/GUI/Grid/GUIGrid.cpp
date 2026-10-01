#include "PCH/PCH.h"
#include "Grid/GUIGrid.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_internal.h"

namespace
{
	constexpr float32 RowLabelColumnWidth = 48.0f;
	constexpr float32 DefaultColumnWidth  = 120.0f;
	constexpr float32 CellPaddingX        = 4.0f;

	const ImU32 ErrorBorderColor = IM_COL32(235, 87, 87, 255);

	// 원본 행 · 열을 그리드 안으로
	int32 clampIndex(int32 inValue, int32 inCount)
	{
		if (inCount <= 0)
		{
			return 0;
		}
		if (inValue < 0)
		{
			return 0;
		}
		return (inValue >= inCount) ? inCount - 1 : inValue;
	}

	// std::string 을 ImGui 입력란 버퍼로 쓴다(imgui_stdlib 와 같은 방식). 첫 프레임에 커서를 끝으로 옮긴다.
	struct HInputTextState
	{
		HRawString* Buffer       = nullptr;
		bool*       bCursorToEnd = nullptr;
	};

	int inputTextCallback(ImGuiInputTextCallbackData* data)
	{
		HInputTextState* state = (HInputTextState*)data->UserData;
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			state->Buffer->resize((size_t)data->BufTextLen);
			data->Buf = state->Buffer->data();
		}
		else if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways)
		{
			if (state->bCursorToEnd != nullptr && *state->bCursorToEnd)
			{
				// 입력란은 키보드로 켜지면 전부 고른다 — 바로 입력한 첫 글자가 다음 글자에 덮이지 않게 끝으로
				data->CursorPos      = data->BufTextLen;
				data->SelectionStart = data->BufTextLen;
				data->SelectionEnd   = data->BufTextLen;
				*state->bCursorToEnd = false;
			}
		}
		return 0;
	}

	bool inputText(const char* inLabel, HRawString& buffer, ImGuiInputTextFlags inFlags, bool* bCursorToEnd)
	{
		HInputTextState state;
		state.Buffer       = &buffer;
		state.bCursorToEnd = bCursorToEnd;
		if (buffer.capacity() < 16)
		{
			buffer.reserve(16);
		}
		return ImGui::InputText(inLabel, buffer.data(), buffer.capacity() + 1, inFlags | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackAlways, &inputTextCallback, &state);
	}

	// 첫 줄만, 넘치면 …
	HRawString firstLine(const PString& inText, bool* outHasMore)
	{
		const HRawString& raw = inText.GetRawString();
		const size_t lineEnd = raw.find_first_of("\r\n");
		*outHasMore = (lineEnd != HRawString::npos);
		return (lineEnd == HRawString::npos) ? raw : raw.substr(0, lineEnd);
	}

	bool containsIgnoreCase(const HRawString& inText, const HRawString& inPattern)
	{
		if (inPattern.empty())
		{
			return true;
		}
		HRawString text = inText;
		HRawString pattern = inPattern;
		for (char& c : text)
		{
			if (c >= 'A' && c <= 'Z')
			{
				c = (char)(c - 'A' + 'a');
			}
		}
		for (char& c : pattern)
		{
			if (c >= 'A' && c <= 'Z')
			{
				c = (char)(c - 'A' + 'a');
			}
		}
		return text.find(pattern) != HRawString::npos;
	}

	void pushEvent(HList<HGUIGridEvent>& outEvents, EGUIGridEventType inType, int32 inRow = -1, int32 inColumn = -1, const PString& inText = PString())
	{
		HGUIGridEvent event;
		event.Type   = inType;
		event.Row    = inRow;
		event.Column = inColumn;
		event.Text   = inText;
		outEvents.push_back(event);
	}

	// 그리드가 초점일 때 ImGui 키보드 내비게이션이 쓰지 못하게 가져오는 키
	const ImGuiKey GridOwnedKeys[] =
	{
		ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow,
		ImGuiKey_PageUp, ImGuiKey_PageDown, ImGuiKey_Home, ImGuiKey_End,
		ImGuiKey_Tab, ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_Space,
	};
}

bool HGUIGridSelection::Contains(int32 inRow, int32 inColumn) const
{
	return inRow >= GetFirstRow() && inRow <= GetLastRow() && inColumn >= GetFirstColumn() && inColumn <= GetLastColumn();
}

bool HGUIGridSelection::operator==(const HGUIGridSelection& inOther) const
{
	return AnchorRow == inOther.AnchorRow && AnchorColumn == inOther.AnchorColumn && FocusRow == inOther.FocusRow && FocusColumn == inOther.FocusColumn;
}

PString IGUIGridSource::GetRowLabel(int32 inRow) const
{
	return PString::FromInt32(inRow + 1);
}

PString IGUIGridSource::GetEditText(int32 inRow, int32 inColumn) const
{
	HGUIGridCell cell;
	GetCell(inRow, inColumn, cell);
	return cell.Text;
}

void PGUIGrid::SetSelection(const HGUIGridSelection& inSelection, bool bScrollToFocus)
{
	_selection = inSelection;
	if (bScrollToFocus)
	{
		_bScrollToFocus = true;
	}
}

void PGUIGrid::SetFocusCell(int32 inRow, int32 inColumn, bool bScrollToFocus)
{
	HGUIGridSelection selection;
	selection.AnchorRow    = inRow;
	selection.AnchorColumn = inColumn;
	selection.FocusRow     = inRow;
	selection.FocusColumn  = inColumn;
	SetSelection(selection, bScrollToFocus);
}

void PGUIGrid::CancelEdit()
{
	_bEditing   = false;
	_editRow    = -1;
	_editColumn = -1;
	_editBuffer.clear();
	if (_bComboOpen)
	{
		_bComboOpen = false;
	}
	_bComboOpenRequest = false;
}

void PGUIGrid::clampSelection(int32 inRowCount, int32 inColumnCount)
{
	_selection.AnchorRow    = clampIndex(_selection.AnchorRow, inRowCount);
	_selection.FocusRow     = clampIndex(_selection.FocusRow, inRowCount);
	_selection.AnchorColumn = clampIndex(_selection.AnchorColumn, inColumnCount);
	_selection.FocusColumn  = clampIndex(_selection.FocusColumn, inColumnCount);

	if (_bEditing && (_editRow >= inRowCount || _editColumn >= inColumnCount))
	{
		CancelEdit();
	}
}

void PGUIGrid::moveFocus(int32 inRow, int32 inColumn, bool bExtend, int32 inRowCount, int32 inColumnCount, HList<HGUIGridEvent>& outEvents)
{
	const HGUIGridSelection before = _selection;
	_selection.FocusRow    = clampIndex(inRow, inRowCount);
	_selection.FocusColumn = clampIndex(inColumn, inColumnCount);
	if (bExtend == false)
	{
		_selection.AnchorRow    = _selection.FocusRow;
		_selection.AnchorColumn = _selection.FocusColumn;
	}
	_bScrollToFocus = true;

	if ((before == _selection) == false)
	{
		pushEvent(outEvents, EGUIGridEventType::SelectionChanged);
	}
}

void PGUIGrid::beginEdit(const IGUIGridSource& inSource, int32 inRow, int32 inColumn, const HRawString& inInitialText, bool bReplace, HList<HGUIGridEvent>& outEvents)
{
	const EGUIGridEditor editor = inSource.GetEditor(inRow, inColumn);
	if (editor == EGUIGridEditor::None)
	{
		return;
	}
	if (editor == EGUIGridEditor::Checkbox)
	{
		pushEvent(outEvents, EGUIGridEventType::ToggleCheckbox, inRow, inColumn);
		return;
	}

	if (editor == EGUIGridEditor::Combo)
	{
		_comboRow           = inRow;
		_comboColumn        = inColumn;
		_comboFilter        = bReplace ? inInitialText : HRawString();
		_comboHighlight     = 0;
		_bComboOpenRequest  = true;
		_bComboFocusRequest = true;
		_bComboCursorToEnd  = true;
		return;
	}

	_bEditing          = true;
	_editRow           = inRow;
	_editColumn        = inColumn;
	_editBuffer        = bReplace ? inInitialText : inSource.GetEditText(inRow, inColumn).GetRawString();
	_bEditFocusRequest = true;
	_bEditCursorToEnd  = true;
	_bScrollToFocus    = true;
}

void PGUIGrid::Draw(const PString& inId, const IGUIGridSource& inSource, const HVector2& inSize, HList<HGUIGridEvent>& outEvents)
{
	const int32 rowCount    = inSource.GetRowCount();
	const int32 columnCount = (inSource.GetColumnCount() < IMGUI_TABLE_MAX_COLUMNS - 1) ? inSource.GetColumnCount() : IMGUI_TABLE_MAX_COLUMNS - 1;
	clampSelection(rowCount, columnCount);

	ImGui::PushID(inId.GetCStr());

	const float32 rowHeight = ImGui::GetFrameHeight();
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable
		| ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;

	bool bGridHovered = false;
	if (columnCount > 0 && ImGui::BeginTable("##grid", columnCount + 1, tableFlags, ImVec2(inSize.x, inSize.y)))
	{
		// 행 번호 칸 + 열 0(키 열) + 머리 행을 고정한다
		ImGui::TableSetupScrollFreeze(2, 1);
		ImGui::TableSetupColumn("##row", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoResize, RowLabelColumnWidth);
		for (int32 c = 0; c < columnCount; ++c)
		{
			const float32 width = inSource.GetColumnWidth(c);
			const ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_WidthFixed | ((c == 0) ? ImGuiTableColumnFlags_NoHide : ImGuiTableColumnFlags_None);
			ImGui::TableSetupColumn(inSource.GetColumnHeader(c).GetCStr(), columnFlags, (width > 0.0f) ? width : DefaultColumnWidth);
		}

		drawHeader(inSource, rowCount, columnCount, rowHeight, outEvents);

		ImGuiListClipper clipper;
		clipper.Begin(rowCount, rowHeight);
		if (_bScrollToFocus && rowCount > 0)
		{
			clipper.IncludeItemByIndex(_selection.FocusRow);
		}
		if (_bEditing && _editRow >= 0 && _editRow < rowCount)
		{
			clipper.IncludeItemByIndex(_editRow);
		}
		while (clipper.Step())
		{
			for (int32 row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
			{
				ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
				ImGui::PushID(row);

				ImGui::TableSetColumnIndex(0);
				drawRowLabel(inSource, row, columnCount, rowHeight, outEvents);

				for (int32 c = 0; c < columnCount; ++c)
				{
					if (ImGui::TableSetColumnIndex(c + 1) == false && (_bScrollToFocus == false || row != _selection.FocusRow || c != _selection.FocusColumn))
					{
						continue;
					}
					ImGui::PushID(c);
					drawCell(inSource, row, c, rowCount, columnCount, rowHeight, outEvents);
					ImGui::PopID();
				}

				ImGui::PopID();
			}
		}

		ImGuiWindow* innerWindow = ImGui::GetCurrentWindow();
		_visibleRowCount = (int32)((innerWindow->InnerRect.GetHeight() - rowHeight) / rowHeight);
		if (_visibleRowCount < 1)
		{
			_visibleRowCount = 1;
		}
		ImGui::EndTable();

		bGridHovered = ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}
	_bScrollToFocus = false;

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		_bDragSelecting = false;
	}
	// 그리드 밖(같은 창의 도구 막대 등)을 누르면 키보드 초점을 놓는다
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && bGridHovered == false && _bComboOpen == false)
	{
		_bHasFocus = false;
	}

	drawCombo(inSource, outEvents);
	handleKeyboard(inSource, rowCount, columnCount, outEvents);

	ImGui::PopID();
}

void PGUIGrid::drawHeader(const IGUIGridSource& inSource, int32 inRowCount, int32 inColumnCount, float32 inRowHeight, HList<HGUIGridEvent>& outEvents)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_Headers, inRowHeight);

	ImGui::TableSetColumnIndex(0);
	ImGui::TableHeader("##row");

	for (int32 c = 0; c < inColumnCount; ++c)
	{
		if (ImGui::TableSetColumnIndex(c + 1) == false)
		{
			continue;
		}

		ImGui::PushID(c);
		ImGui::TableHeader(ImGui::TableGetColumnName(c + 1));
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			// 열 전체를 고른다
			_selection.AnchorRow    = 0;
			_selection.AnchorColumn = c;
			_selection.FocusRow     = (inRowCount > 0) ? inRowCount - 1 : 0;
			_selection.FocusColumn  = c;
			_bHasFocus = true;
			pushEvent(outEvents, EGUIGridEventType::SelectionChanged);
			pushEvent(outEvents, EGUIGridEventType::HeaderClicked, -1, c);
		}
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			_bHasFocus = true;
			pushEvent(outEvents, EGUIGridEventType::HeaderContextMenu, -1, c);
		}

		const PString tooltip = inSource.GetColumnTooltip(c);
		if (tooltip.Empty() == false && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip.GetCStr());
		}
		ImGui::PopID();
	}
}

void PGUIGrid::drawRowLabel(const IGUIGridSource& inSource, int32 inRow, int32 inColumnCount, float32 inRowHeight, HList<HGUIGridEvent>& outEvents)
{
	const ImVec2 cellMin = ImGui::GetCursorScreenPos();
	const float32 width = ImGui::GetContentRegionAvail().x;
	ImGui::InvisibleButton("##rowlabel", ImVec2((width > 1.0f) ? width : 1.0f, inRowHeight));

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		// 행 전체. Shift 면 기준 행에서 이 행까지
		const bool bExtend = ImGui::GetIO().KeyShift;
		if (bExtend == false)
		{
			_selection.AnchorRow = inRow;
		}
		_selection.AnchorColumn = 0;
		_selection.FocusRow     = inRow;
		_selection.FocusColumn  = (inColumnCount > 0) ? inColumnCount - 1 : 0;
		_bHasFocus = true;
		pushEvent(outEvents, EGUIGridEventType::SelectionChanged);
	}
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
	{
		if (_selection.Contains(inRow, 0) == false)
		{
			_selection.AnchorRow    = inRow;
			_selection.AnchorColumn = 0;
			_selection.FocusRow     = inRow;
			_selection.FocusColumn  = (inColumnCount > 0) ? inColumnCount - 1 : 0;
			pushEvent(outEvents, EGUIGridEventType::SelectionChanged);
		}
		_bHasFocus = true;
		pushEvent(outEvents, EGUIGridEventType::CellContextMenu, inRow, -1);
	}

	const PString label = inSource.GetRowLabel(inRow);
	const ImVec2 textSize = ImGui::CalcTextSize(label.GetCStr());
	const ImVec2 textPosition(cellMin.x + width - textSize.x - CellPaddingX, cellMin.y + (inRowHeight - textSize.y) * 0.5f);
	ImGui::GetWindowDrawList()->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_TextDisabled), label.GetCStr());
}

void PGUIGrid::drawCell(const IGUIGridSource& inSource, int32 inRow, int32 inColumn, int32 inRowCount, int32 inColumnCount, float32 inRowHeight, HList<HGUIGridEvent>& outEvents)
{
	const bool bSelected = _selection.Contains(inRow, inColumn);
	const bool bFocus    = (inRow == _selection.FocusRow && inColumn == _selection.FocusColumn);

	if (bSelected)
	{
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImGuiCol_HeaderHovered, 0.45f));
	}

	const ImVec2 cellMin = ImGui::GetCursorScreenPos();
	const float32 width = ImGui::GetContentRegionAvail().x;
	const ImVec2 cellMax(cellMin.x + width, cellMin.y + inRowHeight);
	if (bFocus)
	{
		_focusCellBottomLeft = HVector2(cellMin.x, cellMax.y);
		_focusCellWidth      = width;
	}

	// 고른 칸이 보이게 스크롤한다(고정 머리 행 · 고정 열 아래로 숨지 않게)
	if (_bScrollToFocus && bFocus)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImGuiTable*  table  = ImGui::GetCurrentTable();
		const float32 visibleTop    = window->InnerRect.Min.y + inRowHeight;
		const float32 visibleBottom = window->InnerRect.Max.y;
		if (cellMin.y < visibleTop)
		{
			ImGui::SetScrollY(window->Scroll.y - (visibleTop - cellMin.y));
		}
		else if (cellMax.y > visibleBottom)
		{
			ImGui::SetScrollY(window->Scroll.y + (cellMax.y - visibleBottom));
		}

		if (inColumn > 0 && table != nullptr)
		{
			const float32 frozenWidth = table->Columns[0].WidthGiven + table->Columns[1].WidthGiven + table->CellPaddingX * 4.0f;
			const float32 visibleLeft  = window->InnerRect.Min.x + frozenWidth;
			const float32 visibleRight = window->InnerRect.Max.x;
			if (cellMin.x < visibleLeft)
			{
				ImGui::SetScrollX(window->Scroll.x - (visibleLeft - cellMin.x));
			}
			else if (cellMax.x > visibleRight)
			{
				ImGui::SetScrollX(window->Scroll.x + (cellMax.x - visibleRight));
			}
		}
	}

	if (_bEditing && inRow == _editRow && inColumn == _editColumn)
	{
		drawEditor(inSource, inRowCount, inColumnCount, inRowHeight, outEvents);
		return;
	}

	HGUIGridCell cell;
	inSource.GetCell(inRow, inColumn, cell);
	const EGUIGridEditor editor = inSource.GetEditor(inRow, inColumn);

	ImGui::InvisibleButton("##cell", ImVec2((width > 1.0f) ? width : 1.0f, inRowHeight));
	const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		const bool bWasFocus = bFocus && _bHasFocus;
		moveFocus(inRow, inColumn, ImGui::GetIO().KeyShift, inRowCount, inColumnCount, outEvents);
		_bScrollToFocus = false;
		_bHasFocus      = true;
		_bDragSelecting = true;

		// 이미 고른 체크박스 칸을 다시 누르면 뒤집는다
		if (bWasFocus && editor == EGUIGridEditor::Checkbox && ImGui::GetIO().KeyShift == false)
		{
			pushEvent(outEvents, EGUIGridEventType::ToggleCheckbox, inRow, inColumn);
		}
	}
	else if (_bDragSelecting && bHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left) && bFocus == false)
	{
		moveFocus(inRow, inColumn, true, inRowCount, inColumnCount, outEvents);
		_bScrollToFocus = false;
	}

	if (bHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		beginEdit(inSource, inRow, inColumn, HRawString(), false, outEvents);
	}

	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
	{
		if (bSelected == false)
		{
			moveFocus(inRow, inColumn, false, inRowCount, inColumnCount, outEvents);
			_bScrollToFocus = false;
		}
		_bHasFocus = true;
		pushEvent(outEvents, EGUIGridEventType::CellContextMenu, inRow, inColumn);
	}

	if (bHovered && cell.Tooltip.Empty() == false && _bDragSelecting == false)
	{
		ImGui::SetTooltip("%s", cell.Tooltip.GetCStr());
	}

	// 내용
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 textColor = ImGui::GetColorU32(cell.bMuted ? ImGuiCol_TextDisabled : ImGuiCol_Text);
	if (editor == EGUIGridEditor::Checkbox)
	{
		const float32 boxSize = ImGui::GetFontSize();
		const ImVec2 boxMin(cellMin.x + CellPaddingX, cellMin.y + (inRowHeight - boxSize) * 0.5f);
		const ImVec2 boxMax(boxMin.x + boxSize, boxMin.y + boxSize);
		drawList->AddRect(boxMin, boxMax, textColor, 2.0f);
		if (cell.bChecked)
		{
			ImGui::RenderCheckMark(drawList, ImVec2(boxMin.x + boxSize * 0.15f, boxMin.y + boxSize * 0.15f), textColor, boxSize * 0.7f);
		}
	}
	else
	{
		bool bHasMore = false;
		HRawString text = firstLine(cell.Text, &bHasMore);
		if (bHasMore)
		{
			text += " ...";
		}

		const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
		const float32 textX = cell.bAlignRight ? (cellMax.x - textSize.x - CellPaddingX) : (cellMin.x + CellPaddingX);
		drawList->PushClipRect(cellMin, cellMax, true);
		drawList->AddText(ImVec2(textX, cellMin.y + (inRowHeight - textSize.y) * 0.5f), textColor, text.c_str());
		drawList->PopClipRect();
	}

	if (cell.bError)
	{
		drawList->AddRect(ImVec2(cellMin.x + 1.0f, cellMin.y + 1.0f), ImVec2(cellMax.x - 1.0f, cellMax.y - 1.0f), ErrorBorderColor, 0.0f, 0, 1.5f);
	}
	if (bFocus && _bHasFocus)
	{
		drawList->AddRect(cellMin, cellMax, ImGui::GetColorU32(ImGuiCol_CheckMark), 0.0f, 0, 2.0f);
	}
}

void PGUIGrid::drawEditor(const IGUIGridSource& inSource, int32 inRowCount, int32 inColumnCount, float32 inRowHeight, HList<HGUIGridEvent>& outEvents)
{
	if (_bEditFocusRequest)
	{
		ImGui::SetKeyboardFocusHere();
		_bEditFocusRequest = false;
	}

	ImGui::SetNextItemWidth(-FLT_MIN);
	const bool bEnter = inputText("##edit", _editBuffer, ImGuiInputTextFlags_EnterReturnsTrue, &_bEditCursorToEnd);
	const bool bDeactivated = ImGui::IsItemDeactivated();
	if (bEnter == false && bDeactivated == false)
	{
		return;
	}

	const int32 row    = _editRow;
	const int32 column = _editColumn;
	const PString text(_editBuffer.c_str());
	const bool bShift = ImGui::GetIO().KeyShift;

	if (bEnter == false && ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		CancelEdit();
		return;
	}

	CancelEdit();
	pushEvent(outEvents, EGUIGridEventType::CommitText, row, column, text);

	if (bEnter)
	{
		moveFocus(row + (bShift ? -1 : 1), column, false, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Tab))
	{
		moveFocus(row, column + (bShift ? -1 : 1), false, inRowCount, inColumnCount, outEvents);
	}
	_bHasFocus = true;
}

void PGUIGrid::drawCombo(const IGUIGridSource& inSource, HList<HGUIGridEvent>& outEvents)
{
	if (_bComboOpenRequest)
	{
		// 목록은 고른 칸 아래에 연다
		_bComboOpenRequest = false;
		_bComboOpen        = true;
		ImGui::SetNextWindowPos(ImVec2(_focusCellBottomLeft.x, _focusCellBottomLeft.y));
		ImGui::OpenPopup("##gridcombo");
	}

	if (_bComboOpen == false)
	{
		return;
	}

	ImGui::SetNextWindowSizeConstraints(ImVec2((_focusCellWidth > 220.0f) ? _focusCellWidth : 220.0f, 0.0f), ImVec2(FLT_MAX, 360.0f));
	if (ImGui::BeginPopup("##gridcombo") == false)
	{
		// 바깥을 눌러 닫혔다
		_bComboOpen = false;
		return;
	}

	if (_bComboFocusRequest)
	{
		ImGui::SetKeyboardFocusHere();
		_bComboFocusRequest = false;
	}
	ImGui::SetNextItemWidth(-FLT_MIN);
	const bool bEnter = inputText("##filter", _comboFilter, ImGuiInputTextFlags_EnterReturnsTrue, &_bComboCursorToEnd);

	HList<PString> items;
	inSource.GetComboItems(_comboRow, _comboColumn, items);
	HList<int32> visible;
	for (int32 i = 0; i < (int32)items.size(); ++i)
	{
		if (containsIgnoreCase(items[i].GetRawString(), _comboFilter))
		{
			visible.push_back(i);
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		++_comboHighlight;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		--_comboHighlight;
	}
	_comboHighlight = clampIndex(_comboHighlight, (int32)visible.size());

	int32 picked = -1;
	if (bEnter && visible.empty() == false)
	{
		picked = visible[_comboHighlight];
	}

	const float32 itemHeight = ImGui::GetTextLineHeightWithSpacing();
	const float32 listHeight = (visible.size() > 12) ? itemHeight * 12.0f : itemHeight * (float32)(visible.size() + 0.5f);
	if (ImGui::BeginChild("##items", ImVec2(0.0f, listHeight)))
	{
		ImGuiListClipper clipper;
		clipper.Begin((int32)visible.size(), itemHeight);
		clipper.IncludeItemByIndex(_comboHighlight);
		while (clipper.Step())
		{
			for (int32 v = clipper.DisplayStart; v < clipper.DisplayEnd; ++v)
			{
				const PString& item = items[visible[v]];
				const char* label = item.Empty() ? "(none)" : item.GetCStr();
				ImGui::PushID(v);
				if (ImGui::Selectable(label, v == _comboHighlight))
				{
					picked = visible[v];
				}
				if (v == _comboHighlight && (ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_UpArrow)))
				{
					ImGui::SetScrollHereY();
				}
				ImGui::PopID();
			}
		}
	}
	ImGui::EndChild();

	if (picked >= 0)
	{
		pushEvent(outEvents, EGUIGridEventType::PickComboItem, _comboRow, _comboColumn, items[picked]);
		ImGui::CloseCurrentPopup();
		_bComboOpen = false;
		_bHasFocus  = true;
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		ImGui::CloseCurrentPopup();
		_bComboOpen = false;
		_bHasFocus  = true;
	}
	ImGui::EndPopup();
}

void PGUIGrid::handleKeyboard(const IGUIGridSource& inSource, int32 inRowCount, int32 inColumnCount, HList<HGUIGridEvent>& outEvents)
{
	if (_bHasFocus == false || _bComboOpen || ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) == false)
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	const bool bCtrl  = io.KeyCtrl;
	const bool bShift = io.KeyShift;

	if (_bEditing)
	{
		// 편집 중에는 입력란이 키를 쓴다. 저장만 확정한 뒤 넘긴다
		if (bCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
		{
			const PString text(_editBuffer.c_str());
			const int32 row    = _editRow;
			const int32 column = _editColumn;
			CancelEdit();
			pushEvent(outEvents, EGUIGridEventType::CommitText, row, column, text);
			pushEvent(outEvents, EGUIGridEventType::Save);
		}
		return;
	}

	// 다른 입력란(도구 막대의 필터 등)이 글자를 받는 중이면 건드리지 않는다
	if (io.WantTextInput)
	{
		return;
	}

	// ImGui 키보드 내비게이션이 방향키 · Tab · Enter · Space 를 쓰지 못하게 매 프레임 가져온다
	const ImGuiID ownerId = ImGui::GetID("##gridkeys");
	for (ImGuiKey key : GridOwnedKeys)
	{
		ImGui::SetKeyOwner(key, ownerId);
	}

	if (inColumnCount <= 0)
	{
		return;
	}

	const int32 focusRow    = _selection.FocusRow;
	const int32 focusColumn = _selection.FocusColumn;
	const int32 lastRow     = (inRowCount > 0) ? inRowCount - 1 : 0;
	const int32 lastColumn  = inColumnCount - 1;

	// 단축키
	if (bCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_C, false))
		{
			pushEvent(outEvents, EGUIGridEventType::Copy);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_X, false))
		{
			pushEvent(outEvents, EGUIGridEventType::Cut);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_V, false))
		{
			pushEvent(outEvents, EGUIGridEventType::Paste);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_Z, true))
		{
			pushEvent(outEvents, bShift ? EGUIGridEventType::Redo : EGUIGridEventType::Undo);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_Y, true))
		{
			pushEvent(outEvents, EGUIGridEventType::Redo);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_S, false))
		{
			pushEvent(outEvents, EGUIGridEventType::Save);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_F, false))
		{
			pushEvent(outEvents, EGUIGridEventType::Find);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_A, false))
		{
			_selection.AnchorRow    = 0;
			_selection.AnchorColumn = 0;
			_selection.FocusRow     = lastRow;
			_selection.FocusColumn  = lastColumn;
			pushEvent(outEvents, EGUIGridEventType::SelectionChanged);
		}
		else if ((bShift && ImGui::IsKeyPressed(ImGuiKey_Equal, false)) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd, false))
		{
			pushEvent(outEvents, EGUIGridEventType::InsertRow, (inRowCount > 0) ? focusRow : 0);
		}
		else if (ImGui::IsKeyPressed(ImGuiKey_Minus, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false))
		{
			if (inRowCount > 0)
			{
				pushEvent(outEvents, EGUIGridEventType::DeleteRows);
			}
		}
	}

	if (inRowCount <= 0)
	{
		return;
	}

	// 이동
	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		moveFocus(bCtrl ? 0 : focusRow - 1, focusColumn, bShift, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		moveFocus(bCtrl ? lastRow : focusRow + 1, focusColumn, bShift, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		moveFocus(focusRow, bCtrl ? 0 : focusColumn - 1, bShift, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		moveFocus(focusRow, bCtrl ? lastColumn : focusColumn + 1, bShift, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Tab))
	{
		moveFocus(focusRow, focusColumn + (bShift ? -1 : 1), false, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
	{
		moveFocus(focusRow + (bShift ? -1 : 1), focusColumn, false, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_PageUp))
	{
		moveFocus(focusRow - _visibleRowCount, focusColumn, bShift, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_PageDown))
	{
		moveFocus(focusRow + _visibleRowCount, focusColumn, bShift, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Home))
	{
		moveFocus(bCtrl ? 0 : focusRow, 0, bShift, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_End))
	{
		moveFocus(bCtrl ? lastRow : focusRow, lastColumn, bShift, inRowCount, inColumnCount, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
	{
		pushEvent(outEvents, EGUIGridEventType::Delete);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
	{
		beginEdit(inSource, focusRow, focusColumn, HRawString(), false, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
	{
		beginEdit(inSource, focusRow, focusColumn, HRawString(), true, outEvents);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && inSource.GetEditor(focusRow, focusColumn) == EGUIGridEditor::Checkbox)
	{
		pushEvent(outEvents, EGUIGridEventType::ToggleCheckbox, focusRow, focusColumn);
	}
	else if (bCtrl == false && io.KeyAlt == false && io.InputQueueCharacters.Size > 0)
	{
		// 글자를 치면 그 글자로 덮어쓰며 편집을 시작한다(엑셀처럼). 한글 IME 는 확정된 글자가 여기로 온다
		HRawString typed;
		for (int32 i = 0; i < io.InputQueueCharacters.Size; ++i)
		{
			const unsigned int codepoint = (unsigned int)io.InputQueueCharacters[i];
			if (codepoint < 0x20 || codepoint == 0x7F)
			{
				continue;
			}
			char utf8[5] = {};
			ImTextCharToUtf8(utf8, codepoint);   // utf8 은 0 으로 끝난다
			typed.append(utf8);
		}

		if (typed.empty() == false && typed != " ")
		{
			beginEdit(inSource, focusRow, focusColumn, typed, true, outEvents);
		}
	}
}
