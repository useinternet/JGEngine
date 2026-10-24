// 시작 흐름 — JGLauncher(에디터 / -game)와 패키지 exe 가 GCoreSystem::Create 에 실행 모듈(LaunchModule)을 넘기고,
// 실행 모듈이 엔진 · 게임 모듈을 연결한다. 두 실행 모듈은 GameFrameWork 의 PGameViewport 로 게임 화면을 그린다.
module.exports = {
  file: 'JGEngine전체구조_01_시작흐름',
  alt: '시작 흐름 — JGLauncher는 JGEditor를, JGLauncher -game과 패키지 실행 파일(JGGameBootstrap → JGGameLaunch)은 GameHost를 실행 모듈로 GCoreSystem::Create에 넘기고, 두 실행 모듈은 GameFrameWork의 PGameViewport로 월드와 게임 UI를 그린다',
  type: 'flow',
  w: 980,
  h: 610,
  nodes: [
    { id: 'L1', x: 60, y: 60, w: 220, h: 52, color: 'gray', title: 'JGLauncher.exe', lines: ['개발 실행기'] },
    { id: 'R1', x: 420, y: 60, w: 220, h: 52, color: 'gray', title: 'JGLauncher.exe -game', lines: ['개발 중 게임만 실행'] },
    { id: 'P1', x: 720, y: 60, w: 240, h: 52, color: 'gray', title: '<Name>.exe', lines: ['패키지 실행 파일(JGGameBootstrap)'] },
    { id: 'P2', x: 720, y: 160, w: 240, h: 52, color: 'gray', title: 'JGGameLaunch.dll', lines: ['JGPackage.json의 LaunchModule'] },
    { id: 'CORE', x: 60, y: 270, w: 900, h: 52, color: 'green', title: 'GCoreSystem::Create', lines: ['전역 시스템 등록 → 실행 모듈(LaunchModule) 연결 → 프레임 루프'] },
    { id: 'ED', x: 60, y: 380, w: 380, h: 70, color: 'purple', title: 'JGEditor', lines: ['Graphics · GUI · GameFrameWork 연결', '프로젝트의 게임 · 에디터 모듈 연결'] },
    { id: 'GH', x: 560, y: 380, w: 400, h: 70, color: 'blue', title: 'GameHost', lines: ['Graphics · GameFrameWork 연결', '게임 모듈 연결 · 창 전체에 게임 화면'] },
    { id: 'VP', x: 300, y: 520, w: 400, h: 52, color: 'blue', title: 'PGameViewport', lines: ['GameFrameWork 게임 뷰포트 — 월드 + 게임 UI'] }
  ],
  edges: [
    { from: 'L1', to: 'CORE', points: [[170, 112], [170, 270]], label: 'LaunchModule = JGEditor' },
    { from: 'R1', to: 'CORE', points: [[530, 112], [530, 270]], label: 'LaunchModule = GameHost' },
    { from: 'P1', to: 'P2', points: [[840, 112], [840, 160]] },
    { from: 'P2', to: 'CORE', points: [[840, 212], [840, 270]] },
    { from: 'CORE', to: 'ED', points: [[250, 322], [250, 380]] },
    { from: 'CORE', to: 'GH', points: [[760, 322], [760, 380]] },
    { from: 'ED', to: 'VP', points: [[400, 450], [400, 520]], label: '씬 뷰포트 창에 표시' },
    { from: 'GH', to: 'VP', points: [[600, 450], [600, 520]], label: '스왑체인에 제출' }
  ]
};
