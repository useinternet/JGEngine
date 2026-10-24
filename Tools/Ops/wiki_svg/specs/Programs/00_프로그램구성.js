// 0. Programs — 엔진 프로그램 여섯 개를 역할(빌드 도구 · 실행 진입점 · 패키지 실행)로 나누고, 각 진입점이 연결하는 실행 모듈을 잇는다
// 코드: Source/Programs/*/Main.cpp · JGGameLaunch.cpp, Build/BatchFiles/PreBuild.bat
// 색: 보라 = 엔진 프로그램, 흰색 = 배치, 파랑 = 생성물, 초록 = 실행 모듈
module.exports = {
  file: 'Programs_00_프로그램구성',
  alt: 'Programs (엔진 프로그램) — PreBuild.bat이 실행하는 빌드 도구 JGHeaderTool · JGBuildTool은 Temp/CodeGen과 JGEngine.sln을 생성하고, 실행 진입점 JGConsole은 GameFrameWork를, JGLauncher는 JGEditor 또는 -game이면 GameHost를 실행 모듈로 연결하며, 패키지의 <Name>.exe(JGGameBootstrap)는 JGGameLaunch.dll을 통해 GameHost를 실행한다',
  type: 'flow',
  w: 900,
  h: 540,
  groups: [
    { x: 20, y: 20, w: 560, h: 170, label: '빌드 도구' },
    { x: 620, y: 20, w: 260, h: 170, label: '생성물' },
    { x: 20, y: 210, w: 560, h: 180, label: '실행 진입점' },
    { x: 20, y: 410, w: 560, h: 110, label: '패키지 실행' },
    { x: 620, y: 210, w: 260, h: 310, label: '실행 모듈' }
  ],
  nodes: [
    { id: 'PB', x: 40, y: 85, w: 170, h: 56, color: 'white', title: 'PreBuild.bat', lines: ['엔진 프로젝트 파일 생성'] },
    { id: 'HT', x: 260, y: 52, w: 300, h: 56, color: 'purple', title: 'JGHeaderTool', lines: ['리플렉션 표시를 읽어 코드 생성'] },
    { id: 'BT', x: 260, y: 122, w: 300, h: 56, color: 'purple', title: 'JGBuildTool', lines: ['module.json으로 솔루션 생성'] },
    { id: 'CG', x: 640, y: 52, w: 220, h: 56, color: 'blue', title: 'Temp/CodeGen/', lines: ['생성 코드 · 모듈 목록'] },
    { id: 'SLN', x: 640, y: 122, w: 220, h: 56, color: 'blue', title: 'JGEngine.sln', lines: ['Temp/ProjectFiles/*.vcxproj'] },
    { id: 'CO', x: 40, y: 252, w: 260, h: 56, color: 'purple', title: 'JGConsole', lines: ['명령 하나 실행 · 쿠킹'] },
    { id: 'LA', x: 40, y: 322, w: 260, h: 56, color: 'purple', title: 'JGLauncher', lines: ['개발 실행 — 에디터 · 게임만'] },
    { id: 'BS', x: 40, y: 452, w: 220, h: 56, color: 'purple', title: '<Name>.exe', lines: ['JGGameBootstrap 사본'] },
    { id: 'GL', x: 320, y: 452, w: 230, h: 56, color: 'purple', title: 'JGGameLaunch.dll', lines: ['패키지 실행 진입점'] },
    { id: 'GFW', x: 640, y: 252, w: 220, h: 56, color: 'green', title: 'GameFrameWork', lines: ['엔진 모듈 · 명령 등록'] },
    { id: 'ED', x: 640, y: 322, w: 220, h: 56, color: 'green', title: 'JGEditor', lines: ['에디터 실행 모듈'] },
    { id: 'GH', x: 640, y: 452, w: 220, h: 56, color: 'green', title: 'GameHost', lines: ['게임 실행 모듈'] }
  ],
  edges: [
    { from: 'PB', to: 'HT', fs: 'r', ts: 'l', fo: -10 },
    { from: 'PB', to: 'BT', fs: 'r', ts: 'l', fo: 10 },
    { from: 'HT', to: 'CG', fs: 'r', ts: 'l' },
    { from: 'BT', to: 'SLN', fs: 'r', ts: 'l', label: 'premake' },
    { from: 'CO', to: 'GFW', fs: 'r', ts: 'l', label: '시작할 때 연결' },
    { from: 'LA', to: 'ED', fs: 'r', ts: 'l', fo: -10, to_off: -10, label: 'LaunchModule' },
    { points: [[300, 360], [600, 360], [600, 470], [640, 470]], label: '-game', lx: 600, ly: 415 },
    { from: 'BS', to: 'GL', fs: 'r', ts: 'l', label: 'DLL 호출' },
    { from: 'GL', to: 'GH', fs: 'r', ts: 'l', fo: 10, to_off: 10 }
  ]
};
