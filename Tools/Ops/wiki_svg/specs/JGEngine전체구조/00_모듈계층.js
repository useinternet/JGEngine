// 모듈 계층 — 각 *.module.json 의 ModuleDependencies(A → B = A가 B에 의존), 아래에서 위로(Core가 맨 아래). 바로 아래 층 의존만 그린다.
// 색: Core 초록 · 런타임 모듈 파랑 · 에디터 보라 · 프로그램 회색(범례)
const R3 = 140;
const R2 = 250;
const R1 = 360;
const R0 = 470;
const n = (id, x, y, w, color, title, line) => ({ id, x, y, w, h: 50, color, title, lines: [line] });

module.exports = {
  file: 'JGEngine전체구조_00_모듈계층',
  alt: '모듈 계층 — Core 위에 Graphics, 그 위에 GUI와 GameFrameWork가 있고, JGEditor는 GUI와 GameFrameWork를, GameHost와 JGConsole은 GameFrameWork를 사용하며, 실행기와 빌드 도구는 Core에만 의존하고 JGGameBootstrap은 의존이 없다',
  type: 'flow',
  w: 1210,
  h: 540,
  groups: [
    { x: 20, y: 20, w: 520, h: 90, label: '범례' }
  ],
  texts: [
    { x: 66, y: 41, anchor: 'start', size: 11.5, text: 'A → B : A가 B에 의존(바로 아래 층만 그림)' }
  ],
  nodes: [
    { id: 'K1', x: 34, y: 54, w: 80, h: 32, color: 'green', title: 'Core' },
    { id: 'K2', x: 124, y: 54, w: 130, h: 32, color: 'blue', title: '런타임 모듈' },
    { id: 'K3', x: 264, y: 54, w: 100, h: 32, color: 'purple', title: '에디터' },
    { id: 'K4', x: 374, y: 54, w: 100, h: 32, color: 'gray', title: '프로그램' },
    // R3
    n('JGEDITOR', 40, R3, 170, 'purple', 'JGEditor', '에디터 시작 모듈'),
    n('GAMEHOST', 260, R3, 170, 'blue', 'GameHost', '게임 시작 모듈'),
    n('JGCONSOLE', 480, R3, 170, 'gray', 'JGConsole', '명령 실행기 · 쿠커'),
    n('BOOT', 1040, R3, 150, 'gray', 'JGGameBootstrap', '패키지 exe'),
    // R2
    n('GUI', 40, R2, 170, 'blue', 'GUI', '에디터 UI(ImGui)'),
    n('GFW', 260, R2, 390, 'blue', 'GameFrameWork', '월드 · 규칙 · 게임 UI · 네트워크 · 에셋 · 오디오'),
    // R1
    n('GRAPHICS', 40, R1, 610, 'blue', 'Graphics', 'DirectX 12 렌더러 · GPU 자원'),
    n('LAUNCHER', 680, R1, 120, 'gray', 'JGLauncher', '개발 실행기'),
    n('HEADERTOOL', 810, R1, 120, 'gray', 'JGHeaderTool', '리플렉션 코드'),
    n('BUILDTOOL', 940, R1, 120, 'gray', 'JGBuildTool', '프로젝트 파일'),
    n('GAMELAUNCH', 1070, R1, 120, 'gray', 'JGGameLaunch', '패키지 진입'),
    // R0
    { id: 'CORE', x: 40, y: R0, w: 1150, h: 50, color: 'green', title: 'Core', lines: ['정적 라이브러리 — JGGameBootstrap을 뺀 모든 모듈과 프로그램이 링크'] }
  ],
  edges: [
    { from: 'JGEDITOR', to: 'GUI', points: [[125, 190], [125, 250]] },
    { from: 'JGEDITOR', to: 'GFW', points: [[190, 190], [190, 220], [300, 220], [300, 250]] },
    { from: 'GAMEHOST', to: 'GFW', points: [[380, 190], [380, 250]] },
    { from: 'JGCONSOLE', to: 'GFW', points: [[565, 190], [565, 250]] },
    { from: 'GUI', to: 'GRAPHICS', points: [[125, 300], [125, 360]] },
    { from: 'GFW', to: 'GRAPHICS', points: [[455, 300], [455, 360]] },
    { from: 'GRAPHICS', to: 'CORE', points: [[345, 410], [345, 470]] },
    { from: 'LAUNCHER', to: 'CORE', points: [[740, 410], [740, 470]] },
    { from: 'HEADERTOOL', to: 'CORE', points: [[870, 410], [870, 470]] },
    { from: 'BUILDTOOL', to: 'CORE', points: [[1000, 410], [1000, 470]] },
    { from: 'GAMELAUNCH', to: 'CORE', points: [[1130, 410], [1130, 470]] }
  ]
};
