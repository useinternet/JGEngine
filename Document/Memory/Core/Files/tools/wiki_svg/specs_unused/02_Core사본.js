// 1-2. 바이너리별 Core 사본
// 왼쪽 = 모듈 DLL마다 포함된 Core 사본, 오른쪽 = 실행 파일의 실제 관리자. Link_Module이 DLL 쪽 사본을 실행 파일의 관리자에 연결한다.
module.exports = {
  file: 'Core_02_Core사본',
  alt: '바이너리별 Core 사본 — Graphics.dll · GameFrameWorks.dll · 게임 모듈 DLL에 포함된 Core 사본이 Link_Module을 통해 JGLauncher.exe의 실제 GCoreSystem에 연결됨',
  type: 'flow',
  w: 720,
  h: 460,
  groups: [
    { x: 30, y: 30, w: 220, h: 120, label: 'Graphics.dll', color: 'purple' },
    { x: 30, y: 170, w: 220, h: 120, label: 'GameFrameWorks.dll', color: 'purple' },
    { x: 30, y: 310, w: 220, h: 120, label: '게임 모듈 DLL', color: 'orange' },
    { x: 450, y: 160, w: 240, h: 140, label: 'JGLauncher.exe', color: 'gray' }
  ],
  nodes: [
    { id: 'C1', x: 60, y: 80, w: 160, h: 50, color: 'green', title: 'Core 사본' },
    { id: 'C2', x: 60, y: 220, w: 160, h: 50, color: 'green', title: 'Core 사본' },
    { id: 'C3', x: 60, y: 360, w: 160, h: 50, color: 'green', title: 'Core 사본' },
    { id: 'C0', x: 480, y: 210, w: 180, h: 70, color: 'blue', title: 'Core 사본', lines: ['실제 GCoreSystem 인스턴스'] }
  ],
  edges: [
    { from: 'C1', to: 'C0', fs: 'r', ts: 'l', to_off: -20, via: [[400, 105], [400, 225]], label: 'Link_Module — 관리자 주소 공유', lw: 100, lx: 325, ly: 105 },
    { from: 'C2', to: 'C0', fs: 'r', ts: 'l', label: 'Link_Module', lx: 325, ly: 245 },
    { from: 'C3', to: 'C0', fs: 'r', ts: 'l', to_off: 20, via: [[400, 385], [400, 265]], label: 'Link_Module', lx: 325, ly: 385 }
  ]
};
