#pragma once
#include "GUIDefines.h"

// 스프레드시트형 그리드 컨트롤. 데이터를 모른다 — 칸 글자 · 편집기 종류는 원본(IGUIGridSource)이 주고,
// 그리드는 이번 프레임에 일어난 일(편집 확정 · 단축키 · 선택)을 이벤트로 돌려준다. 원본은 Draw 뒤에 고친다(즉시 모드).
// 그리드 행 · 열은 원본 좌표다. 화면 맨 왼쪽의 행 번호 칸은 원본 열이 아니다.
//   마우스: 클릭 = 칸, Shift+클릭 · 끌기 = 사각 범위, 행 번호 = 행 전체, 열 머리 = 열 전체, 두 번 클릭 = 편집, 오른쪽 클릭 = 메뉴 이벤트
//   키보드(그리드에 초점이 있을 때): 방향키 · Shift(범위) · Ctrl(끝까지), Tab/Shift+Tab, Enter/Shift+Enter, Home/End, PageUp/PageDown,
//     글자 = 덮어쓰며 편집, F2 = 이어서 편집, Backspace = 비우고 편집, Esc = 편집 취소, Space = 체크박스, Delete,
//     Ctrl+C/X/V/Z/Y/S/F/A, Ctrl+Shift+= (행 삽입), Ctrl+- (행 삭제)
// 열 0 과 머리 행은 가로 · 세로로 스크롤해도 고정된다(키 열 자리).

enum class EGUIGridEditor : int32
{
	None,       // 읽기 전용
	Text,       // 한 줄 글
	Checkbox,   // 참거짓
	Combo,      // 목록에서 고르기(글자로 거른다)
};

struct GUI_API HGUIGridCell
{
	PString Text;
	PString Tooltip;
	bool    bAlignRight = false;   // 숫자
	bool    bMuted      = false;   // 기본값과 같음 → 흐리게
	bool    bError      = false;   // 문제 칸 → 빨간 테두리
	bool    bChecked    = false;   // Checkbox
};

// 사각 선택. Anchor = 처음 고른 칸, Focus = 지금 칸(키보드 이동 · 편집 대상).
struct GUI_API HGUIGridSelection
{
	int32 AnchorRow    = 0;
	int32 AnchorColumn = 0;
	int32 FocusRow     = 0;
	int32 FocusColumn  = 0;

	int32 GetFirstRow() const { return (AnchorRow < FocusRow) ? AnchorRow : FocusRow; }
	int32 GetLastRow() const { return (AnchorRow > FocusRow) ? AnchorRow : FocusRow; }
	int32 GetFirstColumn() const { return (AnchorColumn < FocusColumn) ? AnchorColumn : FocusColumn; }
	int32 GetLastColumn() const { return (AnchorColumn > FocusColumn) ? AnchorColumn : FocusColumn; }
	int32 GetRowCount() const { return GetLastRow() - GetFirstRow() + 1; }
	int32 GetColumnCount() const { return GetLastColumn() - GetFirstColumn() + 1; }
	bool  Contains(int32 inRow, int32 inColumn) const;
	bool  operator==(const HGUIGridSelection& inOther) const;
};

enum class EGUIGridEventType : int32
{
	SelectionChanged,
	CommitText,          // Row · Column · Text — 글 편집 확정
	ToggleCheckbox,      // Row · Column
	PickComboItem,       // Row · Column · Text — 목록에서 고름
	Copy,
	Cut,
	Paste,
	Delete,
	Undo,
	Redo,
	Save,
	Find,
	InsertRow,           // Row — 이 행 앞에 (행이 없으면 0)
	DeleteRows,          // 선택한 행들
	HeaderClicked,       // Column
	HeaderContextMenu,   // Column — 열 머리 오른쪽 클릭
	CellContextMenu,     // Row · Column — 칸 · 행 번호 오른쪽 클릭(Column -1 = 행 번호)
};

struct GUI_API HGUIGridEvent
{
	EGUIGridEventType Type   = EGUIGridEventType::SelectionChanged;
	int32             Row    = -1;
	int32             Column = -1;
	PString           Text;
};

class GUI_API IGUIGridSource
{
public:
	virtual ~IGUIGridSource() = default;

	virtual int32   GetRowCount() const = 0;
	virtual int32   GetColumnCount() const = 0;
	virtual PString GetColumnHeader(int32 inColumn) const = 0;
	virtual PString GetColumnTooltip(int32 inColumn) const { return PString(); }
	// 처음 폭(픽셀). 0 이면 기본 폭
	virtual float32 GetColumnWidth(int32 inColumn) const { return 0.0f; }
	virtual PString GetRowLabel(int32 inRow) const;
	virtual void    GetCell(int32 inRow, int32 inColumn, HGUIGridCell& outCell) const = 0;
	virtual EGUIGridEditor GetEditor(int32 inRow, int32 inColumn) const = 0;
	// Combo 편집기의 목록. 빈 글 항목은 "(none)" 으로 보이고 고르면 빈 글이다
	virtual void    GetComboItems(int32 inRow, int32 inColumn, HList<PString>& outItems) const {}
	// F2 · 두 번 클릭으로 편집을 시작할 때의 글. 기본은 칸에 보이는 글
	virtual PString GetEditText(int32 inRow, int32 inColumn) const;
};

class GUI_API PGUIGrid : public IMemoryObject
{
	HGUIGridSelection _selection;
	bool  _bHasFocus      = false;
	bool  _bDragSelecting = false;
	bool  _bScrollToFocus = false;
	int32 _visibleRowCount = 20;

	// 글 편집
	bool       _bEditing           = false;
	int32      _editRow            = -1;
	int32      _editColumn         = -1;
	HRawString _editBuffer;
	bool       _bEditFocusRequest  = false;
	bool       _bEditCursorToEnd   = false;

	// 목록 고르기
	bool       _bComboOpen         = false;
	bool       _bComboOpenRequest  = false;
	bool       _bComboFocusRequest = false;
	int32      _comboRow           = -1;
	int32      _comboColumn        = -1;
	HRawString _comboFilter;
	int32      _comboHighlight     = 0;
	bool       _bComboCursorToEnd  = false;
	HVector2   _focusCellBottomLeft;   // 마지막으로 그린 초점 칸의 왼쪽 아래(목록을 그 아래에 연다)
	float32    _focusCellWidth     = 0.0f;

public:
	PGUIGrid() = default;
	virtual ~PGUIGrid() = default;

	// inSize: 0 이면 남은 폭 · 높이 전부
	void Draw(const PString& inId, const IGUIGridSource& inSource, const HVector2& inSize, HList<HGUIGridEvent>& outEvents);

	const HGUIGridSelection& GetSelection() const { return _selection; }
	void SetSelection(const HGUIGridSelection& inSelection, bool bScrollToFocus = true);
	void SetFocusCell(int32 inRow, int32 inColumn, bool bScrollToFocus = true);

	bool IsEditing() const { return _bEditing || _bComboOpen; }
	void CancelEdit();
	bool HasKeyboardFocus() const { return _bHasFocus; }
	void SetKeyboardFocus(bool bFocus) { _bHasFocus = bFocus; }

private:
	void clampSelection(int32 inRowCount, int32 inColumnCount);
	void moveFocus(int32 inRow, int32 inColumn, bool bExtend, int32 inRowCount, int32 inColumnCount, HList<HGUIGridEvent>& outEvents);
	void beginEdit(const IGUIGridSource& inSource, int32 inRow, int32 inColumn, const HRawString& inInitialText, bool bReplace, HList<HGUIGridEvent>& outEvents);
	void drawHeader(const IGUIGridSource& inSource, int32 inRowCount, int32 inColumnCount, float32 inRowHeight, HList<HGUIGridEvent>& outEvents);
	void drawRowLabel(const IGUIGridSource& inSource, int32 inRow, int32 inColumnCount, float32 inRowHeight, HList<HGUIGridEvent>& outEvents);
	void drawCell(const IGUIGridSource& inSource, int32 inRow, int32 inColumn, int32 inRowCount, int32 inColumnCount, float32 inRowHeight, HList<HGUIGridEvent>& outEvents);
	void drawEditor(const IGUIGridSource& inSource, int32 inRowCount, int32 inColumnCount, float32 inRowHeight, HList<HGUIGridEvent>& outEvents);
	void drawCombo(const IGUIGridSource& inSource, HList<HGUIGridEvent>& outEvents);
	void handleKeyboard(const IGUIGridSource& inSource, int32 inRowCount, int32 inColumnCount, HList<HGUIGridEvent>& outEvents);
};
