// 2. JGWidget 창 (에디터 창) — 창 6개는 GUI JGWidget 을 상속하고, 각 창은 런타임 모듈(GameFrameWork · Core)의 API 를 호출한다
// 코드(구조 개편 뒤): Editor/JGEditor/Widgets/*.h, Runtime/GUI/Widget.h
module.exports = {
  file: 'JGEditor_03_에디터창구조',
  alt: '에디터 창 구조 — Widgets의 에디터 창 6개는 GUI의 JGWidget을 상속하며, JGSceneViewport는 PGameViewport, JGGameplayDevView는 PGameMaster와 관찰자, JGDataTableEditor는 PDataTableDocument와 PGUIGrid, JGSessionLobby는 JGGameInstance와 PGameplaySession, JGDevConsole은 GConsoleCommandGlobalSystem과 GLogGlobalSystem, JGMemoryStatistcs는 GMemoryGlobalSystem을 호출한다',
  type: 'class',
  w: 1000,
  h: 640,
  groups: [
    { x: 20, y: 170, w: 400, h: 450, label: 'Widgets — 에디터 창' },
    { x: 500, y: 170, w: 480, h: 450, label: '창이 호출하는 런타임 API' }
  ],
  classes: [
    { id: 'JW', x: 260, y: 20, w: 400, name: 'JGWidget', stereo: 'GUI 창 기반 클래스', color: 'purple',
      members: ['#OnInitialize() · OnUpdate()', '#OnGenerateGUI()', '+GetTitleName()  // 창 제목'] }
  ],
  nodes: [
    { id: 'W1', x: 40, y: 210, w: 360, h: 52, color: 'green', title: 'JGSceneViewport', lines: ['씬 뷰포트'] },
    { id: 'W2', x: 40, y: 278, w: 360, h: 52, color: 'green', title: 'JGGameplayDevView', lines: ['GameMaster 상태 · 이벤트 로그'] },
    { id: 'W3', x: 40, y: 346, w: 360, h: 52, color: 'green', title: 'JGDataTableEditor', lines: ['데이터 테이블 편집'] },
    { id: 'W4', x: 40, y: 414, w: 360, h: 52, color: 'green', title: 'JGSessionLobby', lines: ['세션 호스트 · 참가 · 새 판'] },
    { id: 'W5', x: 40, y: 482, w: 360, h: 52, color: 'green', title: 'JGDevConsole', lines: ['로그 필터 · 콘솔 명령 입력'] },
    { id: 'W6', x: 40, y: 550, w: 360, h: 52, color: 'green', title: 'JGMemoryStatistcs', lines: ['메모리 풀 통계'] },
    { id: 'R1', x: 520, y: 210, w: 440, h: 52, color: 'blue', title: 'PGameViewport', lines: ['GameFrameWork — 게임 화면 · 입력'] },
    { id: 'R2', x: 520, y: 278, w: 440, h: 52, color: 'blue', title: 'PGameMaster · 관찰자', lines: ['GameFrameWork — 규칙 상태 · 이벤트'] },
    { id: 'R3', x: 520, y: 346, w: 440, h: 52, color: 'blue', title: 'PDataTableDocument · PGUIGrid', lines: ['GameFrameWork 편집 모델 · GUI 그리드'] },
    { id: 'R4', x: 520, y: 414, w: 440, h: 52, color: 'blue', title: 'JGGameInstance · PGameplaySession', lines: ['GameFrameWork — 게임 인스턴스 · 세션'] },
    { id: 'R5', x: 520, y: 482, w: 440, h: 52, color: 'gray', title: 'GConsoleCommandGlobalSystem · GLogGlobalSystem', lines: ['Core — 명령 실행 · 최근 로그'] },
    { id: 'R6', x: 520, y: 550, w: 440, h: 52, color: 'gray', title: 'GMemoryGlobalSystem', lines: ['Core — 메모리 풀 통계'] }
  ],
  edges: [
    { points: [[220, 170], [220, 150], [460, 150], [460, 132]], end: 'triangle', label: '상속', lx: 340, ly: 150 },
    { from: 'W1', to: 'R1', fs: 'r', ts: 'l', label: '호출' },
    { from: 'W2', to: 'R2', fs: 'r', ts: 'l' },
    { from: 'W3', to: 'R3', fs: 'r', ts: 'l' },
    { from: 'W4', to: 'R4', fs: 'r', ts: 'l' },
    { from: 'W5', to: 'R5', fs: 'r', ts: 'l' },
    { from: 'W6', to: 'R6', fs: 'r', ts: 'l' }
  ]
};
