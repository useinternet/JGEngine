// 1. 계층 — ThirdParty → Runtime 모듈 → Editor · Programs / 게임 프로젝트
// 위에서 아래로 계층을 쌓았다. 묶음 사이 화살표는 Mermaid 의 TP → RT, RT → TOOL, RT → GAME, TOOL → GAME 그대로.
// 모듈 이름을 한 상자에 나열하지 않고 모듈마다 상자 하나(이름 위 · 한글 설명 아래), Editor 와 Programs 는 안쪽 묶음으로 나눴다.
const ROW1 = 196;
const ROW2 = 260;
const rt = (id, x, w, y, title, line, color) => ({ id, x, y, w, h: 50, color: color || 'blue', title, lines: [line] });

module.exports = {
  file: 'JGEngine구조도_00_계층',
  alt: '계층 — ThirdParty 라이브러리 위에 Runtime 모듈(Core 정적 라이브러리 · Asset · Graphics · Audio · GameFrameWorks · GameHost 등)이 있고, Runtime 모듈 위에 Editor · Programs와 엔진 밖 게임 프로젝트가 올라가며, 게임 프로젝트는 Editor · Programs도 사용한다',
  type: 'flow',
  w: 1000,
  h: 710,
  groups: [
    { x: 20, y: 20, w: 960, h: 100, label: 'ThirdParty' },
    { x: 20, y: 160, w: 960, h: 166, label: 'Runtime 모듈' },
    { x: 20, y: 366, w: 620, h: 324, label: 'Editor · Programs' },
    { x: 36, y: 398, w: 588, h: 100, label: 'Editor — 에디터 창' },
    { x: 36, y: 512, w: 588, h: 162, label: 'Programs — 빌드 도구 · 실행기 · 패키지 진입점' },
    { x: 680, y: 366, w: 300, h: 324, label: '게임 프로젝트 — 엔진 밖 (예: ProjectAH)' }
  ],
  nodes: [
    // ThirdParty
    { id: 'T1', x: 40, y: 56, w: 260, h: 50, color: 'yellow', title: 'spdlog · rapidjson · zlib · stb', lines: ['로그 · JSON · 압축 · 이미지 등'] },
    { id: 'T2', x: 320, y: 56, w: 180, h: 50, color: 'yellow', title: 'assimp', lines: ['FBX 임포트'] },
    { id: 'T3', x: 520, y: 56, w: 180, h: 50, color: 'yellow', title: 'miniaudio', lines: ['오디오 재생'] },
    { id: 'T4', x: 720, y: 56, w: 240, h: 50, color: 'yellow', title: 'Dear ImGui · ImPlot', lines: ['GUI 모듈 폴더 안에 내장'] },
    // Runtime 모듈
    rt('CORE', 40, 174, ROW1, 'Core', '정적 라이브러리', 'green'),
    rt('ASSET', 226, 174, ROW1, 'Asset', '에셋 · 경로 토큰'),
    rt('GFX', 412, 174, ROW1, 'Graphics', 'DirectX 12 렌더러'),
    rt('AUDIO', 598, 174, ROW1, 'Audio', '사운드 재생'),
    rt('AI', 784, 174, ROW1, 'AI', '빈 모듈'),
    rt('GFW', 40, 360, ROW2, 'GameFrameWorks', '규칙 · 월드 · 게임 UI · 네트워크'),
    rt('GUI', 412, 174, ROW2, 'GUI', '개발용 · 에디터 UI(ImGui)'),
    rt('DEVKIT', 598, 174, ROW2, 'Devkit', '개발용 · 그래픽스 테스트'),
    rt('HOST', 784, 174, ROW2, 'GameHost', '게임만 실행'),
    // Editor
    { id: 'ED1', x: 52, y: 434, w: 180, h: 50, color: 'purple', title: 'JGEditor', lines: ['에디터 호스트'] },
    { id: 'ED2', x: 244, y: 434, w: 180, h: 50, color: 'purple', title: 'DevConsole', lines: ['콘솔 창'] },
    { id: 'ED3', x: 436, y: 434, w: 180, h: 50, color: 'purple', title: 'DevStatistics', lines: ['메모리 통계 창'] },
    // Programs
    { id: 'P1', x: 52, y: 548, w: 180, h: 50, color: 'gray', title: 'JGBuildTool', lines: ['솔루션 생성 도구'] },
    { id: 'P2', x: 244, y: 548, w: 180, h: 50, color: 'gray', title: 'JGHeaderTool', lines: ['리플렉션 코드 생성'] },
    { id: 'P3', x: 436, y: 548, w: 180, h: 50, color: 'gray', title: 'JGLauncher', lines: ['개발 실행기'] },
    { id: 'P4', x: 52, y: 610, w: 180, h: 50, color: 'gray', title: 'JGConsole', lines: ['명령 실행기 · 쿠커'] },
    { id: 'P5', x: 244, y: 610, w: 180, h: 50, color: 'gray', title: 'JGGameLaunch', lines: ['패키지 진입 DLL'] },
    { id: 'P6', x: 436, y: 610, w: 180, h: 50, color: 'gray', title: 'JGGameBootstrap', lines: ['패키지 exe'] },
    // 게임 프로젝트
    { id: 'GM', x: 700, y: 425, w: 260, h: 50, color: 'orange', title: 'Source/Name', lines: ['게임 모듈'] },
    { id: 'GE', x: 700, y: 515, w: 260, h: 50, color: 'orange', title: 'Source/NameEditor', lines: ['게임 에디터 모듈'] },
    { id: 'GC', x: 700, y: 605, w: 260, h: 50, color: 'orange', title: 'Content', lines: ['게임 콘텐츠'] }
  ],
  edges: [
    { from: 'ThirdParty', to: 'Runtime', points: [[500, 120], [500, 160]] },
    { from: 'Runtime', to: 'Editor · Programs', points: [[330, 326], [330, 366]] },
    { from: 'Runtime', to: '게임 프로젝트', points: [[830, 326], [830, 366]] },
    { from: 'Editor · Programs', to: '게임 프로젝트', points: [[640, 540], [680, 540]] }
  ]
};
