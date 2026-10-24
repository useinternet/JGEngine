// 2. 모듈 의존 그래프 — jgengine.lua 의 links(A → B = A가 B를 링크), 아래에서 위로(Core가 맨 아래)
// 한 모듈로 여러 화살표가 모이면 들어가는 자리를 나눴다. JGEditor → GameFrameWorks 는 R4 줄의 빈 통로(x=530)로 곧게 내린다.
// 색: Core 초록 · Runtime 파랑 · Editor 보라 · Programs 회색(범례)
const R5 = 30;
const R4 = 130;
const R3 = 230;
const R2 = 330;
const R1 = 430;
const R0 = 530;
const n = (id, x, y, w, color, title, line) => ({ id, x, y, w, h: 50, color, title, lines: [line] });

module.exports = {
  file: 'JGEngine구조도_01_모듈의존',
  alt: '모듈 의존 그래프 — Asset · Audio · 빌드 도구 · 실행기는 Core를, Graphics는 Asset을, AI · GUI · GameFrameWorks는 Graphics를, 개발 창 모듈은 GUI를, GameHost · JGConsole은 GameFrameWorks를, JGEditor는 GameFrameWorks와 Devkit을 링크하고 JGGameBootstrap은 의존이 없다',
  type: 'flow',
  w: 1000,
  h: 600,
  groups: [
    { x: 20, y: 20, w: 420, h: 90, label: '범례' }
  ],
  texts: [
    { x: 66, y: 41, anchor: 'start', size: 11.5, text: 'A → B : A가 B를 링크(의존)' }
  ],
  nodes: [
    // 범례 색
    { id: 'K1', x: 34, y: 54, w: 80, h: 32, color: 'green', title: 'Core' },
    { id: 'K2', x: 124, y: 54, w: 110, h: 32, color: 'blue', title: 'Runtime 모듈' },
    { id: 'K3', x: 244, y: 54, w: 80, h: 32, color: 'purple', title: 'Editor' },
    { id: 'K4', x: 334, y: 54, w: 92, h: 32, color: 'gray', title: 'Programs' },
    // R5
    n('JGEDITOR', 455, R5, 150, 'purple', 'JGEditor', '에디터 호스트'),
    // R4
    n('DEVCONSOLE', 20, R4, 140, 'purple', 'DevConsole', '콘솔 창'),
    n('DEVSTAT', 180, R4, 140, 'purple', 'DevStatistics', '메모리 통계 창'),
    n('DEVKIT', 340, R4, 140, 'blue', 'Devkit', '그래픽스 테스트'),
    n('GAMEHOST', 580, R4, 140, 'blue', 'GameHost', '게임만 실행'),
    n('JGCONSOLE', 740, R4, 140, 'gray', 'JGConsole', '명령 실행기 · 쿠커'),
    // R3
    n('GUI', 180, R3, 140, 'blue', 'GUI', '에디터 UI(ImGui)'),
    n('AI', 345, R3, 140, 'blue', 'AI', '빈 모듈'),
    n('GFW', 510, R3, 220, 'blue', 'GameFrameWorks', '규칙 · 월드 · 게임 UI · 네트워크'),
    // R2
    n('GRAPHICS', 260, R2, 140, 'blue', 'Graphics', 'DirectX 12 렌더러'),
    n('BOOT', 820, R2, 140, 'gray', 'JGGameBootstrap', '패키지 exe · 의존 없음'),
    // R1
    n('AUDIO', 20, R1, 140, 'blue', 'Audio', '사운드 재생'),
    n('ASSET', 180, R1, 140, 'blue', 'Asset', '에셋 · 경로 토큰'),
    n('BUILDTOOL', 340, R1, 140, 'gray', 'JGBuildTool', '솔루션 생성 도구'),
    n('HEADERTOOL', 500, R1, 140, 'gray', 'JGHeaderTool', '리플렉션 코드 생성'),
    n('LAUNCHER', 660, R1, 140, 'gray', 'JGLauncher', '개발 실행기'),
    n('GAMELAUNCH', 820, R1, 140, 'gray', 'JGGameLaunch', '패키지 진입 DLL'),
    // R0
    { id: 'CORE', x: 20, y: R0, w: 960, h: 50, color: 'green', title: 'Core', lines: ['정적 라이브러리 — JGGameBootstrap을 뺀 모든 모듈이 링크'] }
  ],
  edges: [
    // → Core
    { from: 'AUDIO', to: 'CORE', points: [[90, 480], [90, 530]] },
    { from: 'ASSET', to: 'CORE', points: [[250, 480], [250, 530]] },
    { from: 'BUILDTOOL', to: 'CORE', points: [[410, 480], [410, 530]] },
    { from: 'HEADERTOOL', to: 'CORE', points: [[570, 480], [570, 530]] },
    { from: 'LAUNCHER', to: 'CORE', points: [[730, 480], [730, 530]] },
    { from: 'GAMELAUNCH', to: 'CORE', points: [[890, 480], [890, 530]] },
    // → Asset
    { from: 'GRAPHICS', to: 'ASSET', points: [[290, 380], [290, 430]] },
    // → Graphics
    { from: 'GUI', to: 'GRAPHICS', points: [[290, 280], [290, 330]] },
    { from: 'AI', to: 'GRAPHICS', points: [[372, 280], [372, 330]] },
    { from: 'GFW', to: 'GRAPHICS', points: [[620, 280], [620, 355], [400, 355]] },
    // → GUI
    { from: 'DEVCONSOLE', to: 'GUI', points: [[90, 180], [90, 205], [210, 205], [210, 230]] },
    { from: 'DEVSTAT', to: 'GUI', points: [[250, 180], [250, 230]] },
    { from: 'DEVKIT', to: 'GUI', points: [[410, 180], [410, 205], [290, 205], [290, 230]] },
    // → GameFrameWorks
    { from: 'GAMEHOST', to: 'GFW', points: [[650, 180], [650, 230]] },
    { from: 'JGCONSOLE', to: 'GFW', points: [[810, 180], [810, 205], [700, 205], [700, 230]] },
    { from: 'JGEDITOR', to: 'GFW', points: [[530, 80], [530, 230]] },
    // → Devkit
    { from: 'JGEDITOR', to: 'DEVKIT', points: [[468, 80], [468, 130]] }
  ]
};
