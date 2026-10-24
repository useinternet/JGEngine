// 6-1. PGUIGrid와 IGUIGridSource — 원본이 칸을 주고, Draw 가 이번 프레임의 일을 이벤트로 돌려주며, 사용하는 창이 Draw 뒤에 데이터를 고친다
// 코드: Runtime/GUI/Grid/GUIGrid.h · GUIGrid.cpp, Editor/JGEditor/Widgets/DataTableEditor.cpp(HDataTableEditorTab)
module.exports = {
  file: 'GUI_06_그리드흐름',
  alt: 'PGUIGrid와 IGUIGridSource — 사용하는 창이 구현한 IGUIGridSource가 칸 글 · 편집기 종류를 주고, PGUIGrid::Draw는 선택 · 편집 상태를 보관하며 이번 프레임의 일을 HGUIGridEvent 목록으로 돌려주고, 창은 Draw 뒤에 이벤트를 데이터에 반영해 다음 프레임 원본에 나타난다',
  type: 'flow',
  w: 1000,
  h: 330,
  nodes: [
    { id: 'SRC', x: 20, y: 60, w: 280, h: 86, color: 'yellow', title: 'IGUIGridSource', lines: ['GetRowCount · GetColumnCount', 'GetCell · GetEditor', 'GetComboItems · GetEditText'] },
    { id: 'GRID', x: 370, y: 50, w: 280, h: 106, color: 'green', title: 'PGUIGrid::Draw', lines: ['(id, source, size, outEvents)', '선택 · 칸 편집 · 목록 고르기 상태 보관', '열 0 · 머리 행 고정'] },
    { id: 'EVT', x: 750, y: 60, w: 230, h: 86, color: 'green', title: 'HGUIGridEvent 목록', lines: ['CommitText · ToggleCheckbox', 'PickComboItem · Copy · Paste', 'Undo · InsertRow · DeleteRows 등'] },
    { id: 'OWN', x: 370, y: 226, w: 280, h: 76, color: 'purple', title: '사용하는 창', lines: ['예: JGDataTableEditor 탭', 'Draw 뒤 이벤트를 데이터에 반영'] }
  ],
  edges: [
    { from: 'SRC', to: 'GRID', fs: 'r', ts: 'l', label: '칸 읽기' },
    { from: 'GRID', to: 'EVT', fs: 'r', ts: 'l', label: '이번 프레임 일' },
    { points: [[865, 146], [865, 264], [650, 264]], label: '처리', lx: 865, ly: 205 },
    { points: [[370, 264], [160, 264], [160, 146]], label: '원본 구현 · 데이터 수정', lx: 265, ly: 264 }
  ]
};
