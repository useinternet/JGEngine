// 0-1. gfw.selftest와 별칭 — 위 = 명령을 넣는 곳, 가운데 = 등록 파일 한 곳(명령 · 별칭 · 그룹 표), 아래 = 시험 파일의 진입 함수와 결과
// 2026-10-24 자체 시험 모음(H-084 · H-094 "이대로 + 옛 이름 6개 별칭")
// 코드: SelfTest/GameFrameWorkSelfTest.h(HSelfTestResult · 진입 함수 선언) · .cpp(GROUPS · gfw.selftest · 별칭 6개), SelfTest/*SelfTest.cpp
module.exports = {
  file: 'GameFrameWork_SelfTest_00_자체시험구조',
  alt: 'gfw.selftest와 별칭 — JGConsole이나 에디터 DevConsole에서 넣은 명령을 GConsoleCommandGlobalSystem이 실행하면, 등록 파일 GameFrameWorkSelfTest.cpp의 gfw.selftest가 그룹 표에서 고른 그룹(없으면 8개 모두)을 차례로 돌리고, 옛 명령 6개는 같은 진입 함수를 부르는 별칭이며, 진입 함수는 통과 · 실패 · 건너뜀 수(HSelfTestResult)를 돌려주고 gfw.selftest는 그룹마다 한 줄과 요약 한 줄을 남긴 뒤 실패가 있으면 false를 반환한다',
  type: 'flow',
  w: 1000,
  h: 600,
  groups: [
    { x: 20, y: 190, w: 960, h: 230, label: 'SelfTest/GameFrameWorkSelfTest.cpp' }
  ],
  nodes: [
    { id: 'A1', x: 40, y: 20, w: 240, h: 56, color: 'gray', title: 'JGConsole', lines: ['헤드리스 · 종료 코드로 판정'] },
    { id: 'A2', x: 40, y: 100, w: 240, h: 56, color: 'gray', title: 'JGEditor DevConsole', lines: ['창이 있는 실행'] },
    { id: 'CC', x: 360, y: 60, w: 300, h: 56, color: 'blue', title: 'GConsoleCommandGlobalSystem', lines: ['Core 콘솔 명령 시스템'] },
    { id: 'G1', x: 40, y: 236, w: 300, h: 70, color: 'purple', title: 'gfw.selftest', lines: ['[group ...] [net=<sub>] [-list]', '그룹마다 한 줄 + 요약 한 줄'] },
    { id: 'G2', x: 380, y: 236, w: 260, h: 96, color: 'purple', title: '별칭 6개 — 옛 이름', lines: ['gmtest · gameui.selftest', 'datatable.selftest · cookcheck.selftest', 'net.test · audio.selftest'] },
    { id: 'G3', x: 680, y: 236, w: 280, h: 96, color: 'yellow', title: 'GROUPS — 그룹 표', lines: ['gamemaster · world · viewport', 'gameui · datatable · cookcheck', 'net · audio'] },
    { id: 'RUN', x: 40, y: 470, w: 600, h: 98, color: 'green', title: '시험 파일의 진입 함수 — SelfTest/', lines: ['PGameMasterSelfTest::Run · PWorldSelfTest::Run · PGameViewportSelfTest::Run', 'RunGameUISelfTest · RunDataTableSelfTest · RunCookCheckSelfTest', 'PGameplayNetSelfTest::Run(sub) · RunAudioSelfTest'] },
    { id: 'RES', x: 680, y: 470, w: 280, h: 98, color: 'blue', title: 'HSelfTestResult', lines: ['Passed · Failed · Skipped', '실패가 있으면 명령이 false', 'JGConsole 종료 1'] }
  ],
  edges: [
    { from: 'A1', to: 'CC', fs: 'r', ts: 'l', to_off: -12 },
    { from: 'A2', to: 'CC', fs: 'r', ts: 'l', to_off: 12 },
    { points: [[430, 116], [430, 222], [190, 222], [190, 236]], label: '실행', lx: 430, ly: 150 },
    { from: 'CC', to: 'G2', fs: 'b', ts: 't', fo: 0, to_off: 0 },
    { from: 'G1', to: 'G3', fs: 'b', ts: 'b', to_off: -60, via: [[190, 370], [760, 370]], label: '고른 그룹 · 없으면 모두', lx: 500, ly: 370 },
    { from: 'G3', to: 'RUN', fs: 'b', ts: 't', fo: 60, via: [[880, 440], [500, 440]], label: '그룹마다 호출', lx: 690, ly: 440 },
    { from: 'G2', to: 'RUN', fs: 'b', ts: 't', fo: -60, to_off: -10 },
    { from: 'RUN', to: 'RES', fs: 'r', ts: 'l' }
  ]
};
