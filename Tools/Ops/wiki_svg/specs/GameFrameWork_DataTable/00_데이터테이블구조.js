// 0-1. JGDataTable과 HDataTableView — 왼쪽 = 런타임(에셋 · C++ 바인딩), 오른쪽 = 편집(문서 · 검증), 아래 = 파일 형식
// 코드: Data/DataTable.h(JGDataTable : JGAsset · ApplyContent · OnChanged), Data/DataTableContent.h · DataTableSchema.h · DataTableTypes.h,
//       Data/DataTableView.h(HDataTableView<T> · BindColumns), Data/DataTableDocument.h(PDataTableDocument · 되돌리기 · 저장), Data/DataTableValidation.h
module.exports = {
  file: 'GameFrameWork_DataTable_00_데이터테이블구조',
  alt: 'JGDataTable과 HDataTableView — JGDataTable은 JGAsset을 상속해 스키마(키 열 · 열 7종)와 행을 .jgasset에 저장하고, 게임은 HDataTableView<T>로 구조체가 BindColumns로 묶은 열을 검사해 키로 조회하며, 에디터의 PDataTableDocument는 파일 사본을 편집 · 되돌리기하고 저장 시 ValidateDataTableContent로 검사한 뒤 파일을 교체하고 에셋 DB의 테이블에 ApplyContent로 반영한다',
  type: 'class',
  w: 1100,
  h: 640,
  classes: [
    { id: 'DT', x: 40, y: 110, w: 320, name: 'JGDataTable', stereo: '데이터 테이블 에셋 · 런타임 읽기 전용', color: 'purple', members: ['+GetContent() · FindRow(key)', '+ApplyContent(content)', 'OnChanged  // 대리자'] },
    { id: 'CT', x: 40, y: 390, w: 320, name: 'HDataTableContent', stereo: '스키마 + 행', color: 'gray', members: ['Schema : HDataTableSchema', 'Rows : HDataTableRow[]', 'Bool · Int · Float · String', 'Enum · AssetRef · RowRef  // 열 타입'] },
    { id: 'VW', x: 400, y: 110, w: 300, name: 'HDataTableView<T>', stereo: 'C++ 구조체로 읽기', color: 'blue', members: ['+Bind(table)  // 열 이름 · 타입 검사', '+Find(key)', 'T::BindColumns(binder)'] },
    { id: 'DOC', x: 760, y: 110, w: 300, name: 'PDataTableDocument', stereo: '편집 문서 · 에디터 탭 하나', color: 'green', members: ['+Apply(edit)  // 되돌리기 기록', '+Undo() · Redo()', '+Save()'] },
    { id: 'VAL', x: 760, y: 390, w: 300, name: 'ValidateDataTableContent', stereo: '내용 검사', color: 'green', members: ['오류: 빈 키 · 중복 키 · 열 이름', '경고: 범위 · Enum · 없는 참조'] }
  ],
  nodes: [
    { id: 'AST', x: 40, y: 20, w: 320, h: 50, color: 'gray', title: 'JGAsset', lines: ['.jgasset 에셋 기반'] },
    { id: 'ED', x: 760, y: 560, w: 300, h: 56, color: 'gray', title: 'Data Table Editor', lines: ['JGEditor 창 · 문서를 그림'] },
    { id: 'KEY', x: 400, y: 520, w: 300, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 사용', '+ public'] }
  ],
  edges: [
    { from: 'DT', to: 'AST', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'DT', to: 'CT', fs: 'b', ts: 't', fo: -60, to_off: -60, start: 'diamond', end: 'none', label: '내용' },
    { from: 'VW', to: 'DT', fs: 'l', ts: 'r', fo: -30, to_off: -30, label: '읽기' },
    { from: 'DOC', to: 'VAL', fs: 'b', ts: 't', label: '저장 전 검사' },
    { points: [[760, 200], [730, 200], [730, 340], [300, 340], [300, 254]], label: '저장 → ApplyContent', lx: 510, ly: 340 },
    { points: [[1060, 588], [1084, 588], [1084, 160], [1060, 160]] }
  ]
};
