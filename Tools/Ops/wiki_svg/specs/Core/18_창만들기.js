// 9-1. 창 생성 주체
module.exports = {
  file: 'Core_18_창만들기',
  alt: '창 생성 — 실행 모듈(JGEditor 또는 GameHost)의 StartupModule이 HPlatform::CreateJWindow로 Windows 창(PWindowsJWindow)을 생성하고, HCoreSystemGlobalValues의 MainWindow와 프레임 단계 UpdateWindow의 메시지 펌프에 등록한다',
  type: 'flow',
  w: 560,
  h: 400,
  nodes: [
    { id: 'LM', x: 170, y: 20, w: 220, h: 60, color: 'purple', title: '실행 모듈의 StartupModule', lines: ['JGEditor 또는 GameHost'] },
    { id: 'PF', x: 170, y: 110, w: 220, h: 44, color: 'yellow', title: 'HPlatform::CreateJWindow' },
    { id: 'WW', x: 145, y: 184, w: 270, h: 76, color: 'green', title: 'PWindowsJWindow', lines: ['Windows 창', '표시 방식: 창 모드 · 테두리 없는 전체 화면'] },
    { id: 'MW', x: 30, y: 310, w: 230, h: 60, color: 'blue', title: 'HCoreSystemGlobalValues', lines: ['MainWindow에 등록'] },
    { id: 'PUMP', x: 300, y: 310, w: 230, h: 60, color: 'blue', title: '프레임 단계 UpdateWindow에 메시지 펌프 등록' }
  ],
  edges: [
    { from: 'LM', to: 'PF' },
    { from: 'PF', to: 'WW' },
    { from: 'WW', to: 'MW', fs: 'b', ts: 't' },
    { from: 'WW', to: 'PUMP', fs: 'b', ts: 't' }
  ]
};
