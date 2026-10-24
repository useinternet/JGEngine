// 4-1. 메뉴 등록과 창 복원 — JGEditor · 게임 에디터 모듈이 HGUIModule 에 메뉴를 등록하고(넣은 순서대로 표시),
//      열린 창은 imgui.ini 에 저장했다가 다음 실행 첫 프레임에 클래스 이름으로 다시 연다. 기본 창은 닫아 둔 레이아웃이면 열지 않는다.
// 코드(구조 개편 뒤): Editor/JGEditor/JGEditor.cpp(openDefaultWidgets · openSceneViewport), Runtime/GUI/GUIModule.h · .cpp
//       (AddMainMenuItem · RegisterLayoutSettings · RestoreLayoutWidgets · UpdateLayoutWidgetStates · OpenWidgetByDefault), Runtime/GUI/Menu/MenuTree.cpp
module.exports = {
  file: 'JGEditor_05_메뉴와창복원',
  alt: '메뉴 등록과 창 복원 — HJGEditorModule과 게임 에디터 모듈이 HGUIModule::AddMainMenuItem으로 메뉴를 등록하면 HMainMenuTree가 경로별로 넣은 순서대로 표시하고, HGUIModule은 열린 창을 실행 폴더의 imgui.ini에 클래스 이름으로 저장했다가 다음 실행 첫 프레임에 RestoreLayoutWidgets로 리플렉션 생성해 다시 열며, 씬 뷰포트 같은 기본 창은 저장된 레이아웃에서 닫혀 있으면 열지 않는다',
  type: 'flow',
  w: 1000,
  h: 600,
  groups: [
    { x: 20, y: 20, w: 960, h: 200, label: '메뉴 등록 (시작 시)' },
    { x: 20, y: 260, w: 960, h: 320, label: '열린 창 저장 · 복원 (실행 폴더 imgui.ini)' }
  ],
  nodes: [
    { id: 'M1', x: 40, y: 60, w: 280, h: 60, color: 'blue', title: 'HJGEditorModule', lines: ['Windows 메뉴 — 에디터 창'] },
    { id: 'M2', x: 40, y: 140, w: 280, h: 60, color: 'green', title: '게임 에디터 모듈', lines: ['게임 전용 메뉴'] },
    { id: 'G', x: 380, y: 98, w: 260, h: 64, color: 'purple', title: 'HGUIModule', lines: ['AddMainMenuItem(경로, 동작)'] },
    { id: 'T', x: 700, y: 98, w: 260, h: 64, color: 'purple', title: 'HMainMenuTree', lines: ['경로별 메뉴 · 넣은 순서대로 표시'] },
    { id: 'S1', x: 40, y: 300, w: 280, h: 74, color: 'purple', title: 'HGUIModule', lines: ['매 프레임 열린 창 상태 갱신', '종료 시 저장'] },
    { id: 'F', x: 380, y: 296, w: 260, h: 110, color: 'yellow', shape: 'note', title: 'imgui.ini', lines: ['[Window][창 제목] 위치 · 크기 · 도킹', '[JGWidget][Open] 클래스 이름=1'] },
    { id: 'R', x: 700, y: 300, w: 260, h: 90, color: 'purple', title: 'RestoreLayoutWidgets', lines: ['다음 실행 첫 프레임', '클래스 이름 → 리플렉션 생성', '없는 클래스는 건너뜀'] },
    { id: 'D', x: 40, y: 470, w: 280, h: 74, color: 'blue', title: 'OpenWidgetByDefault', lines: ['씬 뷰포트 같은 기본 창', '닫아 둔 레이아웃이면 열지 않음'] },
    { id: 'O', x: 700, y: 470, w: 260, h: 60, color: 'purple', title: 'OpenWidget', lines: ['저장된 창 다시 열기'] }
  ],
  edges: [
    { from: 'M1', to: 'G', fs: 'r', ts: 'l', to_off: -12, label: '등록' },
    { from: 'M2', to: 'G', fs: 'r', ts: 'l', to_off: 12 },
    { from: 'G', to: 'T', fs: 'r', ts: 'l' },
    { from: 'S1', to: 'F', fs: 'r', ts: 'l', label: '저장' },
    { from: 'F', to: 'R', fs: 'r', ts: 'l', label: '읽기' },
    { from: 'R', to: 'O', fs: 'b', ts: 't' },
    { from: 'D', to: 'F', fs: 'r', ts: 'b', dash: true, label: '열림 상태 확인', lx: 510, ly: 470, lpos: 'r' }
  ]
};
