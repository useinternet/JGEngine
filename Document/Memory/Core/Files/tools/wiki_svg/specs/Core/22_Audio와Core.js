// 11. Audio 모듈과 Core
module.exports = {
  file: 'Core_22_Audio와Core',
  alt: 'Audio 모듈과 Core — 사운드를 사용하는 게임 모듈이 1 모듈 연결, 2 Play 호출 순으로 Audio를 사용하고, GAudioGlobalSystem은 Core 관리자에 등록되어 창 이벤트 WndProc의 WM_ACTIVATEAPP와 로그 · 콘솔 명령을 사용하며, HAudio는 HFileHelper로 파일을 읽어 miniaudio 0.11.25에 전달한다',
  type: 'flow',
  w: 960,
  h: 460,
  groups: [
    { x: 280, y: 80, w: 240, h: 330, label: 'Audio 모듈 (DLL)' },
    { x: 690, y: 20, w: 240, h: 330, label: 'Core' }
  ],
  nodes: [
    { id: 'GAME', x: 25, y: 235, w: 190, h: 50, color: 'orange', title: '사운드를 사용하는 게임 모듈' },
    { id: 'SYS', x: 300, y: 130, w: 200, h: 70, color: 'purple', title: 'GAudioGlobalSystem', lines: ['재생 끝난 사운드 정리', '창 비활성 처리'] },
    { id: 'API', x: 300, y: 320, w: 200, h: 70, color: 'purple', title: 'HAudio', lines: ['Play · PlayMusic · Stop', '버스 볼륨'] },
    { id: 'CS', x: 710, y: 70, w: 200, h: 50, color: 'blue', title: 'GCoreSystem', lines: ['관리자 · 전역 시스템 목록'] },
    { id: 'WC', x: 710, y: 140, w: 200, h: 50, color: 'green', title: 'WndProc', lines: ['창 이벤트'] },
    { id: 'LC', x: 710, y: 210, w: 200, h: 50, color: 'green', title: '로그 · 콘솔 명령' },
    { id: 'FH', x: 710, y: 280, w: 200, h: 50, color: 'yellow', title: 'HFileHelper', lines: ['파일 · 경로'] },
    { id: 'MA', x: 710, y: 380, w: 200, h: 50, color: 'gray', title: 'miniaudio 0.11.25' }
  ],
  edges: [
    { from: 'GAME', to: 'SYS', fs: 'r', ts: 'l', mx: 240, label: '1 모듈 연결', lx: 240, ly: 212, lpos: 'l' },
    { from: 'GAME', to: 'API', fs: 'r', ts: 'l', mx: 240, label: '2 Play 호출', lx: 240, ly: 308, lpos: 'l' },
    { from: 'SYS', to: 'CS', fs: 'r', ts: 'l', mx: 560, label: '등록', lx: 625, ly: 95 },
    { from: 'SYS', to: 'WC', fs: 'r', ts: 'l', label: 'WM_ACTIVATEAPP', lx: 625, ly: 165 },
    { from: 'SYS', to: 'LC', fs: 'r', ts: 'l', mx: 560 },
    { from: 'API', to: 'FH', fs: 'r', ts: 'l', mx: 560, label: '파일 읽기', lx: 625, ly: 305 },
    { from: 'API', to: 'MA', fs: 'r', ts: 'l', mx: 560 }
  ]
};
