// 5. 엔진 ↔ 게임 프로젝트 — 위 줄 엔진, 가운데 줄 게임 프로젝트, 아래 줄 실행(왼쪽) · 패키지(오른쪽)
// 엔진 Content → JGLauncher 는 왼쪽 바깥 통로(x=22), JGConsole -runCook → 패키지는 오른쪽 바깥 통로(x=982)로 돌려 게임 프로젝트 줄을 가로지르지 않게 했다.
module.exports = {
  file: 'JGEngine구조도_05_엔진과게임프로젝트',
  alt: '엔진 ↔ 게임 프로젝트 — 엔진의 배치 파일이 템플릿으로 게임 프로젝트의 .jgproject · 소스 · 솔루션을 생성하고, 솔루션 빌드로 JGLauncher가 에디터 또는 게임만 실행하며(엔진 Content 다음 게임 Content 로드), JGConsole -runCook이 .jgproject를 읽어 패키지의 exe · Bin · Content를 생성한다',
  type: 'flow',
  w: 1000,
  h: 516,
  groups: [
    { x: 40, y: 20, w: 922, h: 118, label: '엔진 C:/Develop/JGEngine' },
    { x: 40, y: 176, w: 922, h: 118, label: '게임 프로젝트 D:/Games/MyGame' },
    { x: 40, y: 332, w: 430, h: 102, label: '실행' },
    { x: 500, y: 332, w: 462, h: 164, label: 'Package/ReleaseGame' }
  ],
  nodes: [
    // 엔진
    { id: 'EC', x: 56, y: 64, w: 150, h: 50, color: 'blue', title: 'Content', lines: ['/JGEngine/ · Shaders'] },
    { id: 'TPL', x: 236, y: 64, w: 220, h: 50, color: 'blue', title: 'Build/Templates/GameProject', lines: ['게임 프로젝트 템플릿'] },
    { id: 'BT', x: 486, y: 56, w: 250, h: 66, color: 'blue', title: 'CreateGameProject.bat · GenerateGameProjectFiles.bat', lines: ['프로젝트 생성 · 솔루션 생성'] },
    { id: 'CK', x: 770, y: 64, w: 180, h: 50, color: 'gray', title: 'JGConsole -runCook', lines: ['쿠킹 · 패키지'] },
    // 게임 프로젝트
    { id: 'PC', x: 56, y: 212, w: 170, h: 66, color: 'orange', title: 'Content', lines: ['/JGGame/'] },
    { id: 'SL', x: 256, y: 212, w: 200, h: 66, color: 'orange', title: 'MyGame.sln', lines: ['빌드 → Bin/DevelopEngine'] },
    { id: 'SRC', x: 486, y: 212, w: 250, h: 66, color: 'orange', title: 'Source/MyGame · Source/MyGameEditor', lines: ['게임 모듈 · 에디터 모듈'] },
    { id: 'JP', x: 770, y: 212, w: 180, h: 66, color: 'orange', title: 'MyGame.jgproject', lines: ['EngineRoot · GameModules', 'EditorModules · Package'] },
    // 실행
    { id: 'R1', x: 56, y: 368, w: 190, h: 50, color: 'green', title: 'JGLauncher.exe', lines: ['JGEditor + 게임 · 에디터 모듈'] },
    { id: 'R2', x: 266, y: 368, w: 190, h: 50, color: 'green', title: 'JGLauncher.exe -game', lines: ['GameHost + 게임 모듈'] },
    // 패키지
    { id: 'P1', x: 516, y: 400, w: 136, h: 80, color: 'purple', title: 'MyGame.exe', lines: ['JGGameBootstrap'] },
    { id: 'P2', x: 663, y: 400, w: 136, h: 80, color: 'purple', title: 'Bin', lines: ['런타임 DLL · JGGameLaunch · JGPackage.json'] },
    { id: 'P3', x: 810, y: 400, w: 136, h: 80, color: 'purple', title: 'Content', lines: ['JGEngine · JGGame'] }
  ],
  edges: [
    { from: 'TPL', to: 'BT', fs: 'r', ts: 'l' },
    { from: 'BT', to: 'SL', points: [[551, 122], [551, 157], [356, 157], [356, 212]] },
    { from: 'BT', to: 'SRC', points: [[611, 122], [611, 212]] },
    { from: 'BT', to: 'JP', points: [[671, 122], [671, 157], [830, 157], [830, 212]] },
    { from: 'JP', to: 'CK', points: [[900, 212], [900, 114]], dash: true, label: '-project=' },
    { from: 'SL', to: 'R1', points: [[296, 278], [296, 313], [200, 313], [200, 368]] },
    { from: 'SL', to: 'R2', points: [[386, 278], [386, 368]] },
    { from: 'EC', to: 'R1', points: [[56, 89], [22, 89], [22, 393], [56, 393]], dash: true, label: '시작 때 먼저 로드', lx: 22, ly: 157 },
    { from: 'PC', to: 'R1', points: [[120, 278], [120, 368]], dash: true, label: '그다음 로드', lpos: 'l' },
    { from: 'CK', to: 'P1', points: [[950, 89], [982, 89], [982, 376], [584, 376], [584, 400]] },
    { from: 'CK', to: 'P2', points: [[950, 89], [982, 89], [982, 376], [731, 376], [731, 400]] },
    { from: 'CK', to: 'P3', points: [[950, 89], [982, 89], [982, 376], [878, 376], [878, 400]] }
  ]
};
