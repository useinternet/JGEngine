// 4-1. JGConsole 실행 과정 — 생성 → GameFrameWork 연결 → 명령 앞 옵션(-graphics · -game) → 명령 실행 → (-graphics) 메인 루프 → 종료
// 코드: Source/Programs/JGConsole/Main.cpp(main · runCommandLine · consumeConsoleOptions · connectGraphics · connectGameModules)
module.exports = {
  file: 'Programs_03_JGConsole실행',
  alt: 'JGConsole 실행 과정 — GCoreSystem을 실행 모듈 없이 생성하고 GameFrameWork를 연결해 엔진 명령을 등록한 뒤, 명령 앞 옵션 -graphics는 작은 창과 Graphics를, -game은 .jgproject의 게임 모듈을 연결하고, 명령을 한 번 실행한 다음 -graphics이면 RequestExit나 창 닫힘까지 메인 루프를 돌고 Destroy로 연결 역순 해제 후 종료 코드를 반환한다',
  type: 'flow',
  w: 860,
  h: 560,
  nodes: [
    { id: 'S1', x: 250, y: 20, w: 320, h: 56, color: 'green', title: 'GCoreSystem::Create()', lines: ['실행 모듈 없이 생성'] },
    { id: 'S2', x: 250, y: 106, w: 320, h: 56, color: 'purple', title: 'ConnectModule("GameFrameWork")', lines: ['엔진 모듈 연결 · 명령 등록'] },
    { id: 'S3', x: 250, y: 192, w: 320, h: 56, color: 'purple', title: 'consumeConsoleOptions', lines: ['명령 앞의 -graphics · -game'] },
    { id: 'S4', x: 250, y: 292, w: 320, h: 56, color: 'purple', title: 'Execute(명령 토큰)', lines: ['명령 한 번 실행 · -runCook = cook.run'] },
    { id: 'S5', x: 250, y: 378, w: 320, h: 56, color: 'purple', title: 'while (GCoreSystem::Update())', lines: ['-graphics만 — RequestExit · 창 닫힘까지'] },
    { id: 'S6', x: 250, y: 470, w: 320, h: 56, color: 'green', title: 'GCoreSystem::Destroy()', lines: ['연결 역순 해제 → 종료 코드 0 · 1'] },
    { id: 'G', x: 620, y: 150, w: 220, h: 72, color: 'blue', title: '-graphics', lines: ['작은 창 → MainWindow', 'ConnectModule("Graphics")'] },
    { id: 'GM', x: 620, y: 262, w: 220, h: 56, color: 'yellow', title: '-game', lines: ['.jgproject GameModules 연결'] },
    { id: 'KEY', x: 20, y: 192, w: 190, h: 90, color: 'white', shape: 'note', title: '명령이 없으면', lines: ['help 출력', '모르는 명령이면 종료 코드 1'] }
  ],
  edges: [
    { from: 'S1', to: 'S2', fs: 'b', ts: 't' },
    { from: 'S2', to: 'S3', fs: 'b', ts: 't' },
    { from: 'S3', to: 'S4', fs: 'b', ts: 't' },
    { from: 'S4', to: 'S5', fs: 'b', ts: 't' },
    { from: 'S5', to: 'S6', fs: 'b', ts: 't' },
    { from: 'S3', to: 'G', fs: 'r', ts: 'l', fo: -12 },
    { from: 'S3', to: 'GM', fs: 'r', ts: 'l', fo: 12 },
    { from: 'S3', to: 'KEY', fs: 'l', ts: 'r', dash: true }
  ]
};
