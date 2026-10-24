// 8-1. 메인 트리 도우미 — 세션 · 대시보드가 MainTree.cmd로 빌드 · 실행을 요청하면 트리가 빌 때까지 기다렸다가 잠금을 잡고 실행한다
// 코드: Build/BatchFiles/MainTree.cmd · MainTree.ps1 · RunEditor.cmd
module.exports = {
  file: 'Programs_07_메인트리도우미',
  alt: '메인 트리 도우미 — 세션은 MainTree.cmd -name=<세션>으로 status · build · exec · hold · release를, 대시보드는 RunEditor.cmd로 editor를 요청하고, MainTree.ps1은 잠금 · 이 트리의 MSBuild와 컴파일러 · 최근 빌드 출력 · Bin 프로세스로 트리가 바쁜지 판정해 비면 Temp/MainTree/lock을 잡고 MSBuild나 프로그램을 실행하며, 바쁘면 -wait 동안 기다리고 시간이 지나면 종료 코드 90을 반환한다',
  type: 'flow',
  w: 980,
  h: 390,
  nodes: [
    { id: 'S', x: 20, y: 60, w: 190, h: 72, color: 'white', title: '세션', lines: ['MainTree.cmd', '-name=<세션> <동사>'] },
    { id: 'RE', x: 20, y: 200, w: 190, h: 72, color: 'white', title: 'RunEditor.cmd', lines: ['대시보드 에디터 단추', '→ editor'] },
    { id: 'MT', x: 260, y: 112, w: 280, h: 90, color: 'purple', title: 'MainTree.ps1', lines: ['status · build · exec', 'hold · release · editor', '기록 Temp/MainTree/history.log'] },
    { id: 'CHK', x: 590, y: 102, w: 150, h: 110, color: 'yellow', shape: 'diamond', title: '트리가 비었나' },
    { id: 'RUN', x: 790, y: 60, w: 170, h: 90, color: 'green', title: '잠금 잡고 실행', lines: ['Temp/MainTree/lock', 'MSBuild · 프로그램', '끝나면 놓음'] },
    { id: 'WAIT', x: 790, y: 200, w: 170, h: 72, color: 'white', title: '-wait 동안 대기', lines: ['넘기면 종료 코드 90'] },
    { id: 'BUSY', x: 260, y: 250, w: 480, h: 110, color: 'white', shape: 'note', align: 'start', title: '바쁜 트리', lines: ['다른 세션의 잠금(build · exec · hold)', '이 트리의 MSBuild · cl · link · premake5', '최근 빌드 출력(Bin · Temp/Obj)', 'build · hold만 — 이 트리 Bin의 에디터 · 런처 · 콘솔'] }
  ],
  edges: [
    { from: 'S', to: 'MT', fs: 'r', ts: 'l', to_off: -14 },
    { from: 'RE', to: 'MT', fs: 'r', ts: 'l', to_off: 14 },
    { from: 'MT', to: 'CHK', fs: 'r', ts: 'l' },
    { from: 'CHK', to: 'RUN', fs: 't', ts: 'l', label: '예' },
    { from: 'CHK', to: 'WAIT', fs: 'b', ts: 'l', label: '아니오' },
    { from: 'WAIT', to: 'CHK', fs: 't', ts: 'r', dash: true }
  ]
};
