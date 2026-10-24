// 7. 게임 프로젝트: 만들기 · 빌드 · 쿠킹 · 패키지
// 가운데 줄 = 만들기 → 솔루션 → 빌드. 빌드 결과에서 에디터 · 게임만 · 쿠킹 세 갈래로 나뉜다. 상자 아래 번호 = 본문 표의 단계 번호.
module.exports = {
  file: 'JGEngine빌드와실행_01_게임프로젝트',
  alt: '게임 프로젝트: 만들기 · 빌드 · 쿠킹 · 패키지 — CreateGameProject.bat으로 프로젝트를 생성하고 GenerateProjectFiles.bat으로 MyGame.sln을 만든 뒤 DevelopEngine으로 빌드하면, JGLauncher.exe로 에디터를, -game으로 게임만 실행하고, JGConsole.exe -runCook으로 Package/ReleaseGame에 패키지를 생성한다',
  type: 'flow',
  w: 920,
  h: 590,
  nodes: [
    { id: 'A', x: 270, y: 20, w: 380, h: 56, color: 'blue', title: 'CreateGameProject.bat D:\\Games\\MyGame', lines: ['1 만들기 — 게임 프로젝트 생성'] },
    { id: 'B', x: 270, y: 106, w: 380, h: 56, color: 'blue', title: 'MyGame\\GenerateProjectFiles.bat', lines: ['2 솔루션 — 프로젝트 파일 생성'] },
    { id: 'C', x: 270, y: 192, w: 380, h: 56, color: 'blue', title: 'MyGame.sln', lines: ['엔진 프로젝트 + MyGame · MyGameEditor'] },
    { id: 'D', x: 270, y: 278, w: 380, h: 56, color: 'green', title: 'DevelopEngine 빌드', lines: ['3 빌드 → MyGame\\Bin\\DevelopEngine'] },
    { id: 'E', x: 30, y: 400, w: 260, h: 56, color: 'purple', title: 'JGLauncher.exe', lines: ['4 실행 — 에디터 창 "JGEditor - MyGame"'] },
    { id: 'F', x: 330, y: 400, w: 260, h: 56, color: 'purple', title: 'JGLauncher.exe -game', lines: ['게임만 실행'] },
    { id: 'G', x: 630, y: 400, w: 260, h: 70, color: 'orange', title: 'JGConsole.exe -runCook', lines: ['-project=D:\\Games\\MyGame', '5 쿠킹 · 패키지'] },
    { id: 'H', x: 630, y: 510, w: 260, h: 56, color: 'yellow', title: 'MyGame\\Package\\ReleaseGame\\', lines: ['MyGame.exe · Bin · Content'] }
  ],
  edges: [
    { from: 'A', to: 'B' },
    { from: 'B', to: 'C' },
    { from: 'C', to: 'D' },
    { from: 'D', to: 'E', fs: 'b', ts: 't', my: 367 },
    { from: 'D', to: 'F', fs: 'b', ts: 't' },
    { from: 'D', to: 'G', fs: 'b', ts: 't', my: 367 },
    { from: 'G', to: 'H', fs: 'b', ts: 't' }
  ]
};
