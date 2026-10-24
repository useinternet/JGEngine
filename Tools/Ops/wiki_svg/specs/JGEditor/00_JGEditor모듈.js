// 0. JGEditor 모듈 — JGLauncher가 실행 모듈로 연결 / JGEditor.dll(HJGEditorModule + Widgets 창 6개) / 프로젝트 모듈 연결 / 의존 모듈
// 코드(구조 개편 뒤): Editor/JGEditor/JGEditor.h · .cpp · JGEditor.module.json · Widgets/*.h, Programs/JGLauncher/Main.cpp
module.exports = {
  file: 'JGEditor_00_JGEditor모듈',
  alt: 'JGEditor 모듈 — JGLauncher가 실행 모듈로 연결하는 JGEditor.dll은 에디터 호스트 HJGEditorModule과 Widgets의 에디터 창 6개(JGSceneViewport, JGGameplayDevView, JGDataTableEditor, JGSessionLobby, JGDevConsole, JGMemoryStatistcs)로 구성되고, 프로젝트 모드에서는 .jgproject의 게임 모듈과 게임 에디터 모듈을 연결하며, Core · Graphics · GUI · GameFrameWork에 의존한다',
  type: 'class',
  w: 1100,
  h: 710,
  groups: [
    { x: 20, y: 110, w: 740, h: 410, label: 'JGEditor 모듈 (JGEditor.dll)' },
    { x: 40, y: 300, w: 700, h: 200, label: 'Widgets — 에디터 창 (JGWidget 상속)', dash: true },
    { x: 800, y: 110, w: 280, h: 300, label: '프로젝트 모듈 (.jgproject)', color: 'green' },
    { x: 20, y: 560, w: 1060, h: 130, label: 'JGEditor가 의존하는 모듈' }
  ],
  classes: [
    { id: 'H', x: 200, y: 150, w: 380, name: 'HJGEditorModule', stereo: '에디터 호스트 · 모듈 클래스', color: 'blue',
      members: ['#StartupModule() · ShutdownModule()', '-openDefaultWidgets()  // 메뉴 등록', '-connectProjectModules()'] },
    { id: 'W1', x: 60, y: 340, w: 210, name: 'JGSceneViewport', stereo: '씬 뷰포트', color: 'green' },
    { id: 'W2', x: 290, y: 340, w: 210, name: 'JGGameplayDevView', stereo: 'GameMaster 관찰', color: 'green' },
    { id: 'W3', x: 520, y: 340, w: 200, name: 'JGDataTableEditor', stereo: '데이터 테이블 편집', color: 'green' },
    { id: 'W4', x: 60, y: 440, w: 210, name: 'JGSessionLobby', stereo: '세션 로비', color: 'green' },
    { id: 'W5', x: 290, y: 440, w: 210, name: 'JGDevConsole', stereo: '로그 · 콘솔 명령', color: 'green' },
    { id: 'W6', x: 520, y: 440, w: 200, name: 'JGMemoryStatistcs', stereo: '메모리 통계', color: 'green' }
  ],
  nodes: [
    { id: 'L', x: 230, y: 20, w: 320, h: 56, color: 'purple', title: 'JGLauncher.exe', lines: ['실행 모듈 = JGEditor'] },
    { id: 'GM', x: 820, y: 150, w: 240, h: 74, color: 'green', title: '게임 모듈', lines: ['GameModules', '먼저 연결'] },
    { id: 'EM', x: 820, y: 300, w: 240, h: 90, color: 'green', title: '게임 에디터 모듈', lines: ['EditorModules', '게임 전용 메뉴 · 창(GUI 사용)'] },
    { id: 'C1', x: 40, y: 600, w: 240, h: 74, color: 'gray', title: 'Core', lines: ['모듈 · 창 · 콘솔 명령', '로그 · 메모리'] },
    { id: 'C2', x: 300, y: 600, w: 240, h: 74, color: 'gray', title: 'Graphics', lines: ['텍스처 · 렌더러'] },
    { id: 'C3', x: 560, y: 600, w: 240, h: 74, color: 'gray', title: 'GUI', lines: ['JGWidget · 메뉴 · 레이아웃'] },
    { id: 'C4', x: 820, y: 600, w: 240, h: 74, color: 'gray', title: 'GameFrameWork', lines: ['게임 인스턴스 · 게임 뷰포트', '데이터 테이블'] }
  ],
  edges: [
    { from: 'L', to: 'H', fs: 'b', ts: 't', label: '연결' },
    { points: [[390, 262], [390, 300]], label: '메뉴 등록 · 열기' },
    { from: 'H', to: 'GM', fs: 'r', ts: 'l', label: '연결' },
    { from: 'GM', to: 'EM', fs: 'b', ts: 't', label: '다음 연결' },
    { points: [[390, 520], [390, 560]], label: '의존' }
  ]
};
