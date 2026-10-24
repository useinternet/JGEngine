// 7-1. 게임 프로젝트 생성과 솔루션 — CreateGameProject.bat → PProjectCreator(템플릿) → 프로젝트 폴더 → GenerateProjectFiles.bat → 게임 솔루션 → 실행 · 쿠킹
// 코드: Build/BatchFiles/CreateGameProject.bat · GenerateGameProjectFiles.bat, JGBuildTool/Class/ProjectCreator.cpp, Build/Templates/GameProject/
module.exports = {
  file: 'Programs_06_게임프로젝트',
  alt: '게임 프로젝트 생성과 솔루션 — CreateGameProject.bat이 JGBuildTool -newproject를 실행하면 PProjectCreator가 이름을 검사하고 Build/Templates/GameProject를 복사 · 치환해 .jgproject와 게임 · 에디터 모듈 소스를 만들고, 프로젝트의 GenerateProjectFiles.bat이 엔진의 GenerateGameProjectFiles.bat으로 JGHeaderTool · JGBuildTool을 -project 모드로 실행해 엔진 프로젝트와 게임 모듈을 함께 담은 <Name>.sln을 생성하며, 빌드한 Bin에서 에디터 · 게임만 실행 · 쿠킹을 한다',
  type: 'flow',
  w: 980,
  h: 470,
  groups: [
    { x: 20, y: 20, w: 940, h: 130, label: '만들기' },
    { x: 20, y: 170, w: 940, h: 130, label: '솔루션 생성 · 빌드' },
    { x: 20, y: 320, w: 940, h: 130, label: '실행 — <Project>/Bin/<구성>/' }
  ],
  nodes: [
    { id: 'CGP', x: 40, y: 64, w: 210, h: 56, color: 'white', title: 'CreateGameProject.bat', lines: ['<폴더> [이름]'] },
    { id: 'PC', x: 290, y: 56, w: 290, h: 72, color: 'purple', title: 'PProjectCreator', lines: ['JGBuildTool -newproject', '이름 검사 · 템플릿 복사 · 토큰 치환'] },
    { id: 'PRJ', x: 620, y: 48, w: 320, h: 90, color: 'yellow', align: 'start', title: '<Project>/', lines: ['<Name>.jgproject — 엔진 위치 · 모듈 · 패키지', 'Source/<Name> · Source/<Name>Editor', 'GenerateProjectFiles.bat'] },
    { id: 'GPF', x: 40, y: 214, w: 210, h: 56, color: 'white', title: 'GenerateProjectFiles.bat', lines: ['프로젝트 루트'] },
    { id: 'GGP', x: 290, y: 206, w: 290, h: 72, color: 'white', title: 'GenerateGameProjectFiles.bat', lines: ['엔진 Build/BatchFiles', 'JGHeaderTool · JGBuildTool -project'] },
    { id: 'SLN', x: 620, y: 206, w: 320, h: 72, color: 'blue', title: '<Name>.sln · <Name>.lua', lines: ['엔진 프로젝트 + 게임 · 에디터 모듈', '빌드 출력은 모두 프로젝트 Bin'] },
    { id: 'R1', x: 40, y: 364, w: 280, h: 56, color: 'green', title: 'JGLauncher.exe', lines: ['에디터 — 게임 · 에디터 모듈 연결'] },
    { id: 'R2', x: 350, y: 364, w: 280, h: 56, color: 'green', title: 'JGLauncher.exe -game', lines: ['GameHost — 게임 모듈만'] },
    { id: 'R3', x: 660, y: 364, w: 280, h: 56, color: 'orange', title: 'JGConsole.exe -runCook', lines: ['Package/<구성>/'] }
  ],
  edges: [
    { from: 'CGP', to: 'PC', fs: 'r', ts: 'l' },
    { from: 'PC', to: 'PRJ', fs: 'r', ts: 'l' },
    { points: [[780, 138], [780, 160], [145, 160], [145, 214]] },
    { from: 'GPF', to: 'GGP', fs: 'r', ts: 'l' },
    { from: 'GGP', to: 'SLN', fs: 'r', ts: 'l' },
    { points: [[780, 278], [780, 310], [300, 310], [300, 364]], label: '빌드', lx: 780, ly: 294 },
    { points: [[780, 310], [490, 310], [490, 364]] },
    { points: [[780, 310], [800, 310], [800, 364]] }
  ]
};
