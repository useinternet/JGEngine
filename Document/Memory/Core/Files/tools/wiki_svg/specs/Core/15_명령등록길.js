// 7-1. 콘솔 명령 등록 흐름
module.exports = {
  file: 'Core_15_명령등록길',
  alt: '명령 등록 과정 — 모듈 .cpp의 static HAutoConsoleCommand는 DLL 로드 시 자기 바이너리의 목록에 연결되고, ConnectModule 후 RegisterAll로 레지스트리에 등록되며, 모듈 종료 시 UnregisterAll로 해당 DLL의 명령만 제거되고, 실행 파일의 명령과 기본 help는 레지스트리 생성 시 등록된다',
  type: 'flow',
  w: 850,
  h: 520,
  nodes: [
    { id: 'A', x: 30, y: 20, w: 280, h: 70, color: 'yellow', title: '모듈 .cpp 파일', lines: ['static HAutoConsoleCommand', 'X(이름, 사용법, 설명, 함수)'] },
    { id: 'B', x: 30, y: 120, w: 280, h: 60, color: 'purple', title: 'DLL 로드 시 (정적 초기화)', lines: ['해당 바이너리의 목록에만 연결'] },
    { id: 'C', x: 30, y: 210, w: 280, h: 60, color: 'green', title: 'ConnectModule', lines: ['StartupModule 완료 후'] },
    { id: 'D', x: 30, y: 300, w: 280, h: 60, color: 'green', title: 'RegisterAll', lines: ['해당 DLL 목록의 명령을 레지스트리에 등록'] },
    { id: 'E', x: 360, y: 210, w: 220, h: 60, color: 'green', title: '모듈 종료', lines: ['ShutdownModule 직전'] },
    { id: 'F', x: 360, y: 300, w: 220, h: 60, color: 'green', title: 'UnregisterAll', lines: ['해당 DLL의 명령만 해제'] },
    { id: 'X', x: 620, y: 300, w: 200, h: 60, color: 'gray', title: '실행 파일의 명령 · 기본 help' },
    { id: 'R', x: 250, y: 430, w: 320, h: 60, color: 'blue', title: 'GConsoleCommandGlobalSystem', lines: ['콘솔 명령 레지스트리'] }
  ],
  edges: [
    { from: 'A', to: 'B' },
    { from: 'B', to: 'C' },
    { from: 'C', to: 'D' },
    { from: 'D', to: 'R', fs: 'b', ts: 't', to_off: -100 },
    { from: 'E', to: 'F' },
    { from: 'F', to: 'R', fs: 'b', ts: 't', to_off: 60 },
    { from: 'X', to: 'R', fs: 'b', ts: 'r', label: '레지스트리 생성 시', lw: 120 }
  ]
};
