// 시작 순서 (StartupModule)
// 설정 읽기에서 에디터 빌드 / 패키지 두 갈래로 나뉜 뒤 명령줄 인자 적용에서 합쳐지고, 그 아래는 한 줄로 내려간다.
module.exports = {
  file: 'GameHost_00_시작순서',
  alt: '시작 순서(StartupModule) — loadSettings가 에디터 빌드면 .jgproject의 GameModules · Package.Window를, 패키지면 Bin/JGPackage.json을 읽고 명령줄 인자를 우선 적용한 뒤, createWindow → 엔진 모듈 연결 → 게임 모듈 연결 → VSync 설정 · PGameViewport 생성 → OnResize · OnInput 구독과 renderFrame 예약 순으로 진행한다',
  type: 'flow',
  w: 700,
  h: 850,
  nodes: [
    { id: 'A', x: 230, y: 20, w: 240, h: 54, color: 'blue', title: 'loadSettings', lines: ['설정 로드'] },
    { id: 'A1', x: 240, y: 100, w: 220, h: 80, color: 'white', shape: 'diamond', title: '에디터 빌드인가' },
    { id: 'A2', x: 30, y: 220, w: 270, h: 60, color: 'yellow', title: '.jgproject', lines: ['GameModules + Package.Window'] },
    { id: 'A3', x: 400, y: 220, w: 270, h: 60, color: 'yellow', title: 'Bin/JGPackage.json', lines: ['패키지 서술자'] },
    { id: 'B', x: 130, y: 320, w: 440, h: 60, color: 'blue', title: '명령줄 인자 우선 적용', lines: ['-windowed · -fullscreen · -res=WxH · -novsync'] },
    { id: 'C', x: 150, y: 410, w: 400, h: 72, color: 'green', title: 'createWindow', lines: ['기본 1920x1080 클라이언트', '작업 영역보다 크면 비율 유지 축소'] },
    { id: 'D', x: 150, y: 512, w: 400, h: 56, color: 'purple', title: '엔진 모듈 연결', lines: ['Graphics → Asset → GameFrameWorks'] },
    { id: 'E', x: 150, y: 598, w: 400, h: 56, color: 'purple', title: '게임 모듈 연결', lines: ['에디터 모듈은 연결하지 않음'] },
    { id: 'F', x: 150, y: 684, w: 400, h: 56, color: 'green', title: 'VSync 설정 · PGameViewport 생성', lines: ['뷰포트 크기 = 창 크기'] },
    { id: 'G', x: 150, y: 770, w: 400, h: 56, color: 'orange', title: 'OnResize · OnInput 구독', lines: ['End 단계에 renderFrame 예약'] }
  ],
  edges: [
    { from: 'A', to: 'A1' },
    { from: 'A1', to: 'A2', fs: 'l', ts: 't', label: '예 — JGLauncher -game', lpos: 'l', lx: 165, ly: 185 },
    { from: 'A1', to: 'A3', fs: 'r', ts: 't', label: '아니오 — 패키지', lx: 535, ly: 185 },
    { from: 'A2', to: 'B', fs: 'b', ts: 't', to_off: -185 },
    { from: 'A3', to: 'B', fs: 'b', ts: 't', to_off: 185 },
    { from: 'B', to: 'C' },
    { from: 'C', to: 'D' },
    { from: 'D', to: 'E' },
    { from: 'E', to: 'F' },
    { from: 'F', to: 'G' }
  ]
};
